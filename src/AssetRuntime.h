#pragma once

#include <string>

namespace asset_runtime {

std::string findAsset(const std::string& assetsDir, const std::string& fileName, bool warnIfMissing = true);

} // namespace asset_runtime
