#include "AssetRuntime.h"

#include <cstdio>
#include <vector>

namespace asset_runtime {

std::string findAsset(const std::string& assetsDir, const std::string& fileName, bool warnIfMissing)
{
    std::vector<std::string> candidates = {
        assetsDir + "/" + fileName,
        std::string("assets/") + fileName,
    };
    for(const auto& path : candidates){
        FILE* f = std::fopen(path.c_str(), "rb");
        if(f){
            std::fclose(f);
            return path;
        }
    }
    if(warnIfMissing)
        std::fprintf(stderr, "Warning: asset '%s' not found\n", fileName.c_str());
    return {};
}

} // namespace asset_runtime
