// ================================================
//   MEGACHLAST PvP — pure oldskool demoscene duel
//   Coded with love and fixed-point by Claude
//   Greets to Future Crew / Farbrausch / Haujobb
//   "this is going to look *sick* on a CRT"
// ================================================

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/Audio.hpp>

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <chrono>
#include <cstdio>
#include <fstream>
#include "ProceduralSynth.h"
#include <string>
#include "AppRuntime.h"
#include "ArenaLayout.h"
#include "GameConstants.h"
#include "GameTypes.h"
#include "Particles.h"
#include "Random.h"
#include "SettingsIO.h"
#include "FxGovernor.h"
#include "AudioDuck.h"
#include "AssetRuntime.h"
#include "AudioRuntime.h"
#include "BoardRuntime.h"
#include "BotController.h"
#include "BuildInfo.h"
#include "FxRuntime.h"
#include "FrameRuntime.h"
#include "GameUpdateRuntime.h"
#include "GraphicsRuntime.h"
#include "HudRuntime.h"
#include "InputRuntime.h"
#include "ControlsInput.h"
#include "ProjectileRuntime.h"
#include "RenderRuntime.h"
#include "RoundRuntime.h"
#include "SimulationRuntime.h"
#include "CliOptions.h"
#include "EffectsRuntime.h"
#include "PerfLog.h"

static Config cfg;
static app_runtime::RuntimeOptions g_runtime{};

// Optional runtime bot tuning overrides (set via assets/settings.cfg)
// Negative value = not set (use built-in defaults)
static app_runtime::BotRuntime g_bot_runtime{};
static app_runtime::AudioSettings g_audio{};
static GraphicsSettings g_graphics{};
static ControllerSettings g_controllers{};

static ProceduralSynth g_synth;

// ── Board seed & layout ───────────────────────────────────────────────────────
static board_runtime::State g_board_state{};

static app_runtime::FxRuntimeState g_fx_runtime{};

static float g_shake_timer = 0.f;
static float g_shake_duration = 0.f;
static float g_shake_intensity = 0.f;

static void fire(std::array<Bullet,MAX_BULLETS>& bullets, const Player& p, int owner){
    game_update_runtime::fireFromPlayer(bullets, p, owner, cfg.bullet_speed, cfg.bullet_ttl);
}

static float currentMusicDuckGain()
{
    return fx_runtime::currentMusicDuckGain(g_audio.music_duck);
}

static void triggerMusicDuck(float depth, float duration)
{
    fx_runtime::triggerMusicDuck(g_audio.music_duck, depth, duration);
}

static void triggerScreenShake(float duration, float intensity)
{
    fx_runtime::triggerScreenShake(duration,
                                   intensity,
                                   g_shake_timer,
                                   g_shake_duration,
                                   g_shake_intensity);
}

static void updateFxGovernor(float dt, GameState state)
{
    fx_runtime::updateGovernor(dt,
                               state,
                               g_fx_runtime.fx_state,
                               g_fx_runtime.particle_spawn_budget,
                               g_fx_runtime.particle_spawns_this_frame,
                               g_fx_runtime.fx_level);
}

static void beginParticleSpawnFrame()
{
    fx_runtime::beginParticleSpawnFrame(g_runtime.perf_level,
                                        g_fx_runtime.fx_level,
                                        g_fx_runtime.particle_spawn_budget,
                                        g_fx_runtime.particle_spawns_this_frame);
}

static bool runSmokeTest(const std::string& assetsDir)
{
    constexpr std::array<const char*, 7> requiredAssets = {
        "sansation.ttf", "press_start_2p.ttf", "pl1blu.png", "pl2red.png",
        "menu.mp3", "ingame.mp3", "get_ready.mp3"
    };
    for(const char* asset : requiredAssets){
        if(asset_runtime::findAsset(assetsDir, asset).empty()){
            std::fprintf(stderr, "Smoke test failed: missing asset %s in %s\n", asset, assetsDir.c_str());
            return false;
        }
    }

    board_runtime::State board{};
    board.world_seed = 0x4D454741u;
    Player p1{}, p2{};
    std::array<Bullet, MAX_BULLETS> bullets{};
    std::array<Mirror, MIRROR_PAIRS * 2> mirrors{};
    std::array<PowerUp, MAX_POWERUPS> powerups{};
    std::array<Bomb, MAX_BOMBS> bombs{};
    std::array<BarrierBrick, BARRIER_BRICKS * 2> barriers{};
    std::array<SpecialStar, MAX_SPECIAL_STARS> specialStars{};
    bot_controller::RuntimeState botRuntime{};
    RNG rng(0x534D4F4Bu);
    board_runtime::resetRound(board, p1, p2, bullets, mirrors, powerups, bombs,
                              barriers, specialStars, botRuntime, rng);
    if(board.round_number != 1 || board.layout_name[0] == '\0' ||
       p1.energy != 100.f || p2.energy != 100.f){
        std::fprintf(stderr, "Smoke test failed: invalid initial game state\n");
        return false;
    }

    std::fprintf(stderr, "Smoke test passed: assets and initial game state verified\n");
    return true;
}

// =============================================================================
// MAIN
// =============================================================================
int main(int argc, char** argv){
    // ── CLI ──────────────────────────────────────────────────────────────────
    cli_options::Parsed cli{};
    auto cliResult = cli_options::parse(argc, argv, cli);
    if(cliResult == cli_options::ParseResult::HELP){
        cli_options::printUsage(argv[0]);
        return 0;
    }
    if(cliResult == cli_options::ParseResult::ERROR){
        cli_options::printUsage(argv[0]);
        return 2;
    }

    // ── Settings file ────────────────────────────────────────────────────────
    g_runtime.assets_dir = cli.assetsDir;
    // Start from the built-in control bindings; the settings file may override.
    g_controllers.profiles = controls::defaultProfiles();
    std::string cfgpath = app_runtime::settingsPath(g_runtime);
    auto save_settings_file = [&](const std::string& path){
        settings_io::saveSettingsFile(path, cfg, g_runtime.bot_enabled, g_runtime.bot_difficulty,
                                      g_audio.music_volume, g_audio.sfx_volume, g_audio.muted,
                                      g_graphics,
                                      g_controllers,
                                      g_bot_runtime.overrides);
    };
    auto load_settings_file = [&](const std::string& path) -> bool {
        return settings_io::loadSettingsFile(path, cfg, g_runtime.bot_enabled, g_runtime.bot_difficulty,
                                             g_audio.music_volume, g_audio.sfx_volume, g_audio.muted,
                                             g_graphics,
                                             g_controllers,
                                             g_bot_runtime.overrides);
    };
    if(!load_settings_file(cfgpath))
        std::fprintf(stderr, "No settings file at %s — using defaults\n", cfgpath.c_str());
    setCanvasAspect(g_graphics.screen_aspect);

    // CLI takes precedence over settings file
    app_runtime::applyCliOptions(cli, g_runtime);

    if(cli.smokeTest) return runSmokeTest(g_runtime.assets_dir) ? 0 : 1;

    graphics_runtime::applyGlProfile(cli.glProfile, g_runtime.verbose);
    if(cli.glInfo) graphics_runtime::printStartupDiagnostics(stderr);

    // ── Window ────────────────────────────────────────────────────────────────
    sf::ContextSettings windowSettings = graphics_runtime::makeWindowContextSettings();
    sf::RenderWindow win(
        sf::VideoMode(sf::Vector2u{
            static_cast<unsigned int>(W * g_graphics.window_scale),
            static_cast<unsigned int>(H * g_graphics.window_scale)}),
        "Megachlast PvP",
        sf::Style::Default,
        sf::State::Windowed,
        windowSettings);
    if(cli.glInfo) graphics_runtime::printWindowDiagnostics(stderr, win);
    win.setFramerateLimit(60);
    bool isFullscreen = false;
    auto applyWindowedGraphicsSettings = [&]{
        if(isFullscreen) return;
        win.create(sf::VideoMode(sf::Vector2u{
                       static_cast<unsigned int>(W * g_graphics.window_scale),
                       static_cast<unsigned int>(H * g_graphics.window_scale)}),
                   "Megachlast PvP",
                   sf::Style::Default,
                   sf::State::Windowed,
                   windowSettings);
        win.setMouseCursorVisible(true);
        win.setFramerateLimit(60);
    };

    // Offscreen texture for CRT post-process (scanlines, shake)
    sf::RenderTexture rt;
    if(!rt.resize(sf::Vector2u{(unsigned)W, (unsigned)H})){
        std::fprintf(stderr, "Failed to create %dx%d render texture\n", W, H);
        return 1;
    }
    rt.setSmooth(false);

    // ── Game objects ──────────────────────────────────────────────────────────
    RNG R;
    // Seed the board-generation system from the hardware clock once per session.
    // Every round will then derive a unique deterministic seed from this root.
    g_board_state.world_seed = static_cast<uint32_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    g_board_state.round_number = 0;
    Player p1{}, p2{};
    std::array<Bullet,   MAX_BULLETS>    bullets{};
    std::array<Mirror,   MIRROR_PAIRS*2> mirrors{};
    std::array<Particle, MAX_PARTICLES>  particles{};
    std::array<Star,     NUM_STARS>      stars{};
    std::array<SpectStar, NUM_SPECT>     spectStars{};
    std::array<PowerUp,  MAX_POWERUPS>   powerups{};
    std::array<Bomb,     MAX_BOMBS>      bombs{};
    std::array<BarrierBrick, BARRIER_BRICKS*2> barriers{};
    std::array<SpecialStar, MAX_SPECIAL_STARS> specialStars{};
    float powerupSpawnTimer = POWERUP_SPAWN_INTERVAL * POWERUP_FIRST_SPAWN_FACTOR; // first spawn sooner
    render_runtime::initStars(stars, R);
    render_runtime::initSpectStars(spectStars);
    board_runtime::resetRound(g_board_state,
                              p1,
                              p2,
                              bullets,
                              mirrors,
                              powerups,
                              bombs,
                              barriers,
                              specialStars,
                              g_bot_runtime.state,
                              R);

    // ── State ─────────────────────────────────────────────────────────────────
    GameState state      = GameState::MENU;
    int       winner     = 0;
    MatchState match{};
    MatchSetup matchSetup{};
    bool      showBlink  = true;
    float     blinkTimer = 0.f;
    float     menuAnim   = 0.f;
    int       menu_sel   = 0;
    int  cd1=0, cd2=0;
    float countdownTimer = 0.f;
    float fightFlashTimer = 0.f;  // "FIGHT!" overlay after countdown
    float knockoutTimer = 0.f;
    int knockoutScorer = 0;
    bool knockoutEndsMatch = false;
    float donate_msg_timer = 0.f; // short feedback when link copied
    int donate_sel = 0;
    std::array<FragFloat, MAX_FRAG_FLOATS> fragFloats{};

    int settings_sel = 0;

    int controls_profile = 0;
    int controls_sel = 0;
    int controls_scroll = 0;
    bool controls_capturing = false;
    int controls_capture_action = 0;

    game_update_runtime::InputState input{};
    effects_runtime::Context effectsContext{};
    effectsContext.particleSpawnBudget = &g_fx_runtime.particle_spawn_budget;
    effectsContext.particleSpawnsThisFrame = &g_fx_runtime.particle_spawns_this_frame;

    // ── Assets ────────────────────────────────────────────────────────────────
    sf::Font font;
    sf::Font retroTitleFont;
    bool haveFont = false;
    bool haveRetroTitleFont = false;
    {
        std::string fp = asset_runtime::findAsset(g_runtime.assets_dir, "sansation.ttf");
        if(!fp.empty()) haveFont = font.openFromFile(fp);
        if(haveFont) font.setSmooth(false);
        if(!haveFont) fprintf(stderr, "Font not loaded\n");

        std::string retroFp = asset_runtime::findAsset(g_runtime.assets_dir, "press_start_2p.ttf");
        if(!retroFp.empty()) haveRetroTitleFont = retroTitleFont.openFromFile(retroFp);
        if(haveRetroTitleFont) retroTitleFont.setSmooth(false);
        if(!haveRetroTitleFont) fprintf(stderr, "Retro title font not loaded; using default font\n");
    }

    // ── Ship sprites ──────────────────────────────────────────────────────────
    sf::Texture tex_p1, tex_p2;
    bool haveTex1 = false, haveTex2 = false;
    {
        std::string p1path = asset_runtime::findAsset(g_runtime.assets_dir, "pl1blu.png");
        std::string p2path = asset_runtime::findAsset(g_runtime.assets_dir, "pl2red.png");
        if(!p1path.empty()) haveTex1 = tex_p1.loadFromFile(p1path);
        if(!p2path.empty()) haveTex2 = tex_p2.loadFromFile(p2path);
        if(!haveTex1) fprintf(stderr, "Warning: pl1blu.png not loaded\n");
        if(!haveTex2) fprintf(stderr, "Warning: pl2red.png not loaded\n");
    }

    app_runtime::MusicRuntime musicRuntime{};
    {
        auto tracks = audio_runtime::loadMusicTracks(g_runtime.assets_dir,
                                                     g_runtime.no_music,
                                                     musicRuntime.menu,
                                                     musicRuntime.ingame,
                                                     musicRuntime.get_ready);
        musicRuntime.have_menu = tracks.haveMenu;
        musicRuntime.have_ingame = tracks.haveIngame;
        musicRuntime.have_get_ready = tracks.haveGetReady;
    }

    g_synth.init();
    auto apply_audio_settings = [&](sf::Music& m){
        audio_runtime::applyAudioSettings(m, g_audio.muted, g_audio.music_volume, currentMusicDuckGain());
    };
    auto apply_all_music_settings = [&]{
        if(musicRuntime.have_menu)      apply_audio_settings(musicRuntime.menu);
        if(musicRuntime.have_ingame)    apply_audio_settings(musicRuntime.ingame);
        if(musicRuntime.have_get_ready) apply_audio_settings(musicRuntime.get_ready);
    };
    auto apply_active_music_settings = [&]{
        if(musicRuntime.have_menu && musicRuntime.menu.getStatus() == sf::Music::Status::Playing)
            apply_audio_settings(musicRuntime.menu);
        if(musicRuntime.have_ingame && musicRuntime.ingame.getStatus() == sf::Music::Status::Playing)
            apply_audio_settings(musicRuntime.ingame);
        if(musicRuntime.have_get_ready && musicRuntime.get_ready.getStatus() == sf::Music::Status::Playing)
            apply_audio_settings(musicRuntime.get_ready);
    };
    apply_all_music_settings();

    auto playMenuMusic = [&]{
        audio_runtime::playMenuMusic(musicRuntime.have_menu,
                                     musicRuntime.menu,
                                     musicRuntime.have_ingame,
                                     musicRuntime.ingame,
                                     musicRuntime.have_get_ready,
                                     musicRuntime.get_ready,
                                     apply_audio_settings);
    };
    auto playIngameMusic = [&]{
        audio_runtime::playIngameMusic(musicRuntime.have_menu,
                                       musicRuntime.menu,
                                       musicRuntime.have_ingame,
                                       musicRuntime.ingame,
                                       musicRuntime.have_get_ready,
                                       musicRuntime.get_ready,
                                       apply_audio_settings);
    };
    auto playGetReady = [&](bool loop){
        audio_runtime::playGetReady(loop,
                                    musicRuntime.have_menu,
                                    musicRuntime.menu,
                                    musicRuntime.have_ingame,
                                    musicRuntime.ingame,
                                    musicRuntime.have_get_ready,
                                    musicRuntime.get_ready,
                                    apply_audio_settings);
    };

    auto applyRoundModifier = [](Player& player, RoundModifier modifier){
        constexpr float duration = 12.f;
        switch(modifier){
            case RoundModifier::SHIELD: player.shieldTimer = duration; break;
            case RoundModifier::RAPID: player.rapidTimer = duration; break;
            case RoundModifier::SPREAD: player.spreadTimer = duration; break;
            case RoundModifier::OVERDRIVE: player.overdriveTimer = duration; break;
            case RoundModifier::COUNT: break;
        }
    };

    auto resetBoardRound = [&]{
        g_board_state.forced_layout_pick = arenaPresetLayoutPick(matchSetup.arena);
        board_runtime::resetRound(g_board_state,
                                  p1,
                                  p2,
                                  bullets,
                                  mirrors,
                                  powerups,
                                  bombs,
                                  barriers,
                                  specialStars,
                                  g_bot_runtime.state,
                                  R);
        applyRoundModifier(p1, matchSetup.p1_modifier);
        applyRoundModifier(p2, matchSetup.p2_modifier);
    };

    auto applyFragTransition = [&](Player& victim, int scorer){
        round_runtime::applyFragTransition(p1,
                                           p2,
                                           victim,
                                           scorer,
                                           cfg,
                                           match,
                                           winner,
                                           state,
                                           cd1,
                                           cd2,
                                           powerupSpawnTimer,
                                           countdownTimer,
                                           resetBoardRound,
                                           playGetReady);
        knockoutTimer = 1.15f;
        knockoutScorer = scorer;
        knockoutEndsMatch = (state == GameState::GAME_OVER);
    };

    auto startCountdownFromMenu = [&]{
        round_runtime::startCountdownFromMenu(p1,
                                              p2,
                                              match,
                                              cd1,
                                              cd2,
                                              powerupSpawnTimer,
                                              countdownTimer,
                                              state,
                                              resetBoardRound,
                                              playGetReady);
    };
    auto openMatchSetup = [&]{
        matchSetup = {};
        state = GameState::MATCH_SETUP;
    };
    auto openSettingsMenu = [&]{
        round_runtime::openSettingsMenu(state, settings_sel);
    };
    auto openDonateScreen = [&]{
        donate_sel = 0;
        round_runtime::openDonateScreen(state, donate_msg_timer);
    };
    auto pauseGameplay = [&]{
        round_runtime::pauseGameplay(state, musicRuntime.have_ingame, musicRuntime.ingame);
    };
    auto resetPlayingRound = [&]{
        round_runtime::resetPlayingRound(p1,
                                         p2,
                                         match,
                                         cd1,
                                         cd2,
                                         powerupSpawnTimer,
                                         resetBoardRound);
    };
    auto resumeGameplay = [&]{
        round_runtime::resumeGameplay(state, musicRuntime.have_ingame, musicRuntime.ingame, apply_audio_settings);
    };
    auto rematchCountdownRound = [&]{
        round_runtime::rematchCountdownRound(p1,
                                             p2,
                                             match,
                                             cd1,
                                             cd2,
                                             powerupSpawnTimer,
                                             countdownTimer,
                                             state,
                                             resetBoardRound,
                                             playGetReady);
    };

    input_runtime::FrameContext inputContext{};
    inputContext.state = &state;
    inputContext.menuSel = &menu_sel;
    inputContext.donateSel = &donate_sel;
    inputContext.settingsSel = &settings_sel;
    inputContext.cfg = &cfg;
    inputContext.botEnabled = &g_runtime.bot_enabled;
    inputContext.botDifficulty = &g_runtime.bot_difficulty;
    inputContext.musicVolume = &g_audio.music_volume;
    inputContext.sfxVolume = &g_audio.sfx_volume;
    inputContext.graphicsSettings = &g_graphics;
    inputContext.controllers = &g_controllers;
    inputContext.matchSetup = &matchSetup;
    inputContext.donateMsgTimer = &donate_msg_timer;
    inputContext.botDebug = &g_bot_runtime.state.debug;
    inputContext.perfLevel = &g_runtime.perf_level;
    inputContext.muted = &g_audio.muted;
    inputContext.isFullscreen = &isFullscreen;
    inputContext.input = &input;
    inputContext.controlsProfile = &controls_profile;
    inputContext.controlsSel = &controls_sel;
    inputContext.controlsScroll = &controls_scroll;
    inputContext.controlsCapturing = &controls_capturing;
    inputContext.controlsCaptureAction = &controls_capture_action;
    inputContext.assetsDir = &g_runtime.assets_dir;
    inputContext.cfgPath = &cfgpath;
    inputContext.playMenuMusic = playMenuMusic;
    inputContext.applyAllMusicSettings = apply_all_music_settings;
    inputContext.applyGraphicsSettings = applyWindowedGraphicsSettings;
    inputContext.saveSettings = save_settings_file;
    inputContext.loadSettings = load_settings_file;
    inputContext.startCountdownRound = openMatchSetup;
    inputContext.startConfiguredMatch = startCountdownFromMenu;
    inputContext.openSettings = openSettingsMenu;
    inputContext.openDonate = openDonateScreen;
    inputContext.pauseGameplay = pauseGameplay;
    inputContext.resetPlayingRound = resetPlayingRound;
    inputContext.resumeGameplay = resumeGameplay;
    inputContext.rematchCountdownRound = rematchCountdownRound;

    // ── Perf logging ──────────────────────────────────────────────────────────
    sf::Clock clock;
    float fps_acc = 0.f; int fps_frames = 0; float fps_display = 0.f;
    float total_run_time = 0.f, perf_log_acc = 0.f;
    float last_music_duck_gain = currentMusicDuckGain();
    float duck_apply_acc = 0.f;
    std::ofstream perf_ofs;
    perf_log::openCsv(perf_ofs, g_runtime.perf_log);

    // =========================================================================
    // MAIN LOOP
    // =========================================================================
    while(win.isOpen()){
        float dt = clock.restart().asSeconds();
        if(dt > 0.05f) dt = 0.05f;
        if(knockoutTimer > 0.f) knockoutTimer = std::max(0.f, knockoutTimer - dt);
        updateFxGovernor(dt, state);

        audio_duck::update(g_audio.music_duck, dt);
        frame_runtime::updateMusicDuck(dt,
                                       duck_apply_acc,
                                       last_music_duck_gain,
                                       currentMusicDuckGain,
                                       apply_active_music_settings);

        beginParticleSpawnFrame();
        effectsContext.perfLevel = g_runtime.perf_level;
        effectsContext.fxLevel = g_fx_runtime.fx_level;
        total_run_time += dt;

        perf_log::updateFps(dt, g_runtime.verbose, g_runtime.perf_level, fps_acc, fps_frames, fps_display);

        if(perf_ofs && g_runtime.perf_log.interval > 0){
            int ap = 0;
            for(const auto& p : particles) if(p.alive) ap++;
            int ab = 0;
            for(const auto& b : bullets) if(b.alive) ab++;
            perf_log::maybeWriteCsv(perf_ofs,
                                    g_runtime.perf_log,
                                    dt,
                                    perf_log_acc,
                                    g_runtime.perf_level,
                                    g_fx_runtime.fx_level,
                                    static_cast<int>(std::lround(fps_display)),
                                    ap,
                                    ab,
                                    g_fx_runtime.particle_spawn_budget,
                                    g_fx_runtime.particle_spawns_this_frame);
        }

        if(perf_log::durationExpired(total_run_time, g_runtime.perf_log)){
            win.close(); break;
        }

        frame_runtime::updateMenuBlinkAndPhase(dt,
                                               menuAnim,
                                               blinkTimer,
                                               showBlink,
                                               p1,
                                               p2,
                                               mirrors,
                                               barriers);

        input_runtime::updateInputForFrame(win, inputContext);

        frame_runtime::UpdateContext frameCtx{};
        frameCtx.state = &state;
        frameCtx.countdownTimer = &countdownTimer;
        frameCtx.fightFlashTimer = &fightFlashTimer;
        frameCtx.dt = dt;
        frameCtx.haveMusic = musicRuntime.have_menu;
        frameCtx.music = &musicRuntime.menu;
        frameCtx.haveIngameMusic = musicRuntime.have_ingame;
        frameCtx.ingameMusic = &musicRuntime.ingame;
        frameCtx.haveGetReady = musicRuntime.have_get_ready;
        frameCtx.getReady = &musicRuntime.get_ready;
        frameCtx.shakeTimer = &g_shake_timer;
        frameCtx.shakeDuration = &g_shake_duration;
        frameCtx.shakeIntensity = &g_shake_intensity;
        frameCtx.p1 = &p1;
        frameCtx.p2 = &p2;
        frameCtx.playMenuMusic = playMenuMusic;
        frameCtx.playIngameMusic = playIngameMusic;
        frame_runtime::updateStateAndTimers(frameCtx);

        float starBright = (state == GameState::MENU) ? 1.f : 0.4f;
        render_runtime::updateStars(stars, dt, R);
        render_runtime::updateSpectStars(spectStars, dt, R, static_cast<int>(state));

        projectile_runtime::Hooks projectileHooks = simulation_runtime::buildProjectileHooks(
            [&](auto& parts, RNG& rng, float x, float y, int hue){
                effects_runtime::spawnExplosion(effectsContext, parts, rng, x, y, hue);
            },
            [&](auto& parts, RNG& rng, float x, float y, int victimId){
                effects_runtime::spawnShipDisintegration(effectsContext, parts, rng, x, y, victimId);
            },
            [&](auto& parts, RNG& rng, float x, float y){
                effects_runtime::spawnSpark(effectsContext, parts, rng, x, y);
            },
            [&](auto& parts, RNG& rng, float x, float y, int owner){
                effects_runtime::spawnHitSpark(effectsContext, parts, rng, x, y, owner);
            },
            effects_runtime::spawnFragFloat,
            triggerMusicDuck,
            triggerScreenShake,
            applyFragTransition);

        simulation_runtime::FrameContext simCtx{};
        simCtx.state = state;
        simCtx.input = &input;
        simCtx.p1 = &p1;
        simCtx.p2 = &p2;
        simCtx.bullets = &bullets;
        simCtx.powerups = &powerups;
        simCtx.bombs = &bombs;
        simCtx.mirrors = &mirrors;
        simCtx.barriers = &barriers;
        simCtx.specialStars = &specialStars;
        simCtx.particles = &particles;
        simCtx.fragFloats = &fragFloats;
        simCtx.rng = &R;
        simCtx.cfg = &cfg;
        simCtx.botEnabled = g_runtime.bot_enabled;
        simCtx.botDifficulty = g_runtime.bot_difficulty;
        simCtx.botOverrides = &g_bot_runtime.overrides;
        simCtx.botRuntime = &g_bot_runtime.state;
        simCtx.synth = &g_synth;
        simCtx.muted = g_audio.muted;
        simCtx.sfxVolume = g_audio.sfx_volume;
        simCtx.cd1 = &cd1;
        simCtx.cd2 = &cd2;
        simCtx.dt = dt;
        simCtx.powerupSpawnTimer = &powerupSpawnTimer;
        simCtx.fireFn = fire;
        simCtx.spawnThrusterFn = [&](auto& parts, RNG& rng, float x, float y, int id){
            effects_runtime::spawnThruster(effectsContext, parts, rng, x, y, id);
        };
        simCtx.projectileHooks = &projectileHooks;
        simulation_runtime::simulatePlayingFrame(simCtx);

        simulation_runtime::ParticleFrameContext particleCtx{};
        particleCtx.bullets = &bullets;
        particleCtx.particles = &particles;
        particleCtx.fragFloats = &fragFloats;
        particleCtx.rng = &R;
        particleCtx.dt = dt;
        particleCtx.spawnTrailFn = [&](auto& parts, RNG& rng, float x, float y, int owner){
            effects_runtime::spawnTrail(effectsContext, parts, rng, x, y, owner);
        };
        simulation_runtime::updateParticlesAndTrails(particleCtx);

        // =====================================================================
        // RENDER — all to offscreen texture, then blit with optional shake
        // =====================================================================
        rt.clear(sf::Color(0,0,0,255));

        // Animated plasma background
        render_runtime::drawPlasmaBg(rt, menuAnim, state == GameState::MENU);

        render_runtime::drawStars(rt, stars, starBright);

        render_runtime::IngameElementsContext renderCtx{};
        renderCtx.state = state;
        renderCtx.menuAnim = menuAnim;
        renderCtx.fxLevel = g_fx_runtime.fx_level;
        renderCtx.mirrors = &mirrors;
        renderCtx.p1 = &p1;
        renderCtx.p2 = &p2;
        renderCtx.bullets = &bullets;
        renderCtx.tex1 = haveTex1 ? &tex_p1 : nullptr;
        renderCtx.tex2 = haveTex2 ? &tex_p2 : nullptr;
        renderCtx.drawPlasmaDivider = [&]{ render_runtime::drawPlasmaDivider(rt, menuAnim); };
        renderCtx.drawPowerups = [&]{ for(const auto& u : powerups) render_runtime::drawPowerUp(rt, u, menuAnim); };
        renderCtx.drawBombs = [&]{ for(const auto& bomb : bombs) render_runtime::drawBomb(rt, bomb, menuAnim, g_fx_runtime.fx_level); };
        renderCtx.drawSpecialStars = [&]{ render_runtime::drawSpecialStars(rt, specialStars, menuAnim); };
        renderCtx.drawBarriers = [&]{ render_runtime::drawAllBarriers(rt, barriers, menuAnim); };
        renderCtx.drawParticles = [&]{ render_runtime::drawParticles(rt, particles); };
        renderCtx.drawFragFloats = [&]{ if(haveFont) hud_runtime::drawFragFloats(rt, font, fragFloats); };
        renderCtx.drawPlayerRows = [&]{ if(haveFont) hud_runtime::drawPlayerRows(rt, font, p1, p2); };
        render_runtime::drawIngameElements(rt, renderCtx);

        bool postfxDisabled = render_runtime::isPostfxDisabled(g_runtime.no_postfx,
                                                               g_graphics,
                                                               state,
                                                               g_fx_runtime.fx_level);

        render_runtime::SceneOverlayContext sceneOverlayCtx{};
        sceneOverlayCtx.state = state;
        sceneOverlayCtx.postfxDisabled = postfxDisabled;
        sceneOverlayCtx.vignetteEnabled = g_graphics.vignette_enabled;
        sceneOverlayCtx.drawMenuSpectStars = [&]{ render_runtime::drawSpectStars(rt, spectStars); };
        render_runtime::drawSceneOverlays(rt, sceneOverlayCtx);

        render_runtime::PostfxContext postfxCtx{};
        postfxCtx.postfxDisabled = postfxDisabled;
        postfxCtx.perfLevel = g_runtime.perf_level;
        postfxCtx.state = state;
        postfxCtx.fxLevel = g_fx_runtime.fx_level;
        postfxCtx.menuAnim = menuAnim;
        postfxCtx.scanlinesEnabled = g_graphics.scanlines_enabled;
        postfxCtx.copperBarsEnabled = g_graphics.copper_bars_enabled;
        postfxCtx.drawScanlines = [&]{ render_runtime::drawScanlines(rt); };
        render_runtime::drawPostfxOverlays(rt, postfxCtx);

        if(haveFont){
            hud_runtime::TextOverlayContext hudCtx{};
            hudCtx.appVersion = BUILD_VERSION_LABEL;
            hudCtx.state = state;
            hudCtx.dt = dt;
            hudCtx.menuAnim = menuAnim;
            hudCtx.menuSel = menu_sel;
            hudCtx.donateSel = donate_sel;
            hudCtx.showBlink = showBlink;
            hudCtx.donateMsgTimer = &donate_msg_timer;
            hudCtx.perfLevel = g_runtime.perf_level;
            hudCtx.fxLevel = g_fx_runtime.fx_level;
            hudCtx.fpsDisplay = fps_display;
            hudCtx.botEnabled = g_runtime.bot_enabled;
            hudCtx.botDifficulty = g_runtime.bot_difficulty;
            hudCtx.winner = winner;
            hudCtx.p1Score = p1.score;
            hudCtx.p2Score = p2.score;
            hudCtx.p1RoundWins = match.p1_round_wins;
            hudCtx.p2RoundWins = match.p2_round_wins;
            hudCtx.knockoutTimer = knockoutTimer;
            hudCtx.knockoutScorer = knockoutScorer;
            hudCtx.knockoutEndsMatch = knockoutEndsMatch;
            hudCtx.matchSetup = &matchSetup;
            hudCtx.countdownTimer = countdownTimer;
            hudCtx.layoutColor = board_runtime::layoutAccentColor(g_board_state);
            hudCtx.layoutName = g_board_state.layout_name;
            hudCtx.boardSeed = g_board_state.board_seed;
            hudCtx.fightFlashTimer = fightFlashTimer;
            hudCtx.settingsSel = settings_sel;
            hudCtx.controlsProfile = controls_profile;
            hudCtx.controlsSel = controls_sel;
            hudCtx.controlsScroll = controls_scroll;
            hudCtx.controlsCapturing = controls_capturing;
            hudCtx.controlsCaptureAction = controls_capture_action;
            hudCtx.cfg = &cfg;
            hudCtx.musicVolume = g_audio.music_volume;
            hudCtx.sfxVolume = g_audio.sfx_volume;
            hudCtx.graphicsSettings = &g_graphics;
            hudCtx.controllers = &g_controllers;
            hudCtx.drawMenuTitle = [&]{
                render_runtime::drawMenuTitle(rt,
                                              haveRetroTitleFont ? retroTitleFont : font,
                                              menuAnim,
                                              g_fx_runtime.fx_level);
            };
            hud_runtime::drawTextOverlays(rt, font, hudCtx);
        }

        rt.display();

        render_runtime::CompositeContext compositeCtx{};
        compositeCtx.texture = &rt.getTexture();
        compositeCtx.postfxDisabled = postfxDisabled;
        compositeCtx.vignetteEnabled = g_graphics.vignette_enabled;
        compositeCtx.chromaticEnabled = g_graphics.chromatic_enabled && state == GameState::PLAYING;
        compositeCtx.shakeTimer = g_shake_timer;
        compositeCtx.shakeDuration = g_shake_duration;
        compositeCtx.shakeIntensity = g_shake_intensity;
        compositeCtx.randomRange = [&](float lo, float hi){ return R.frand(lo, hi); };
        render_runtime::compositeToWindow(win, compositeCtx);

        win.display();
    }

    return 0;
}
