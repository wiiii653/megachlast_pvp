#pragma once

#include <chrono>
#include <cstdint>
#include <random>

struct RNG {
    std::mt19937 rng;

    RNG()
        : rng(static_cast<uint32_t>(
              std::chrono::high_resolution_clock::now().time_since_epoch().count()))
    {}

    explicit RNG(uint32_t seed)
        : rng(seed)
    {}

    int irand(int a, int b)
    {
        std::uniform_int_distribution<int> d(a, b);
        return d(rng);
    }

    float frand(float a, float b)
    {
        std::uniform_real_distribution<float> d(a, b);
        return d(rng);
    }
};
