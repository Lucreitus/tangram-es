#pragma once

#include "util/url.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Tangram {

class Platform;
struct ZipArchive;

// Asset represents any file referenced by a scene.
class Asset {

public:
    // Create an asset representing a file at the given URL.
    Asset(Url url);

    // Get the URL of this asset.
    const Url& url() const { return m_url; }

    // Read the contents of the asset into a buffer of bytes.
    virtual std::vector<char> readBytes(const std::shared_ptr<Platform>& platform) const;

    // Get an asset representing a URL resolved against this asset.
    virtual std::shared_ptr<Asset> getRelativeAsset(Url url) const;

protected:
    // The URL from which this asset retrieves data. For a zipped file, this is the URL of the archive
    // that contains the file.
    Url m_url;
};

// ZippedAsset specializes Asset for files that are zip archives of scenes.
class ZippedAsset : public Asset {

public:
    // Creates an asset for a zip archive, representing a scene in zipped form.
    ZippedAsset(Url url, std::vector<char> zippedData);

    std::vector<char> readBytes(const std::shared_ptr<Platform>& platform) const override;

    std::shared_ptr<Asset> getRelativeAsset(Url url) const override;

private:

    ZippedAsset(ZippedAsset& base, std::string path);

    std::shared_ptr<ZipArchive> m_zipArchive;

    std::string m_pathInArchive;

    // Check if the filePath is the base scene yaml
    bool isBaseSceneYaml(const std::string& filePath) const;

};

} // namespace Tangram
