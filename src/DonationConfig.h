#pragma once

#include <array>

namespace donations {

struct Entry {
    const char* label;
    const char* address;
};

inline constexpr std::array<Entry, 9> entries = {{
    {"BUY ME A COFFEE", "https://buymeacoffee.com/ojnen"},
    {"USDT / TRON (TRC-20)", "TEoNMfDBS86nAuFbBgpAxBVqKizzcpHF5t"},
    {"BTC / Bitcoin", "13A3DTPUUMq3hgBSJ5R5N4Sz9CVL2v4AY1"},
    {"ETH / Ethereum (ERC-20)", "0xac43ea88a19ee3d214b25cb2aeac5a156bbd7652"},
    {"LTC / Litecoin", "LMMgp5Esoiy8if7G4v51UgBu5BJY1axAEW"},
    {"XRP / XRP Ledger", "rQnFKC1eRLvviR8sqXG1GzvJZAg6nmr8co"},
    {"SOL / Solana", "HGkhk5mKH2LMAVRCfc49Nk535Py8iGKSMYvTg2Rv1dJb"},
    {"DOGE / Dogecoin", "D7Gq67sh2UdMzrbhdN5GkRHjk6fZG1WAXc"},
    {"TRX / TRON", "TKbrWfhD1S8WDyaQKo5CYy7rMgEN9RKgsn"}
}};

inline constexpr int entryCount = static_cast<int>(entries.size());

} // namespace donations
