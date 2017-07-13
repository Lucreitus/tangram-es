#include "scene/asset.h"
#include "log.h"
#include "platform.h"

#define MINIZ_NO_ZLIB_APIS // Remove all ZLIB-style compression/decompression API's.
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES // Disable zlib names, to prevent conflicts against stock zlib.
#include <miniz.h>

namespace Tangram {

struct ZipArchive {

    struct Entry {
        std::string path;
        size_t uncompressedSize = 0;
    };

    ZipArchive() {
        mz_zip_zero_struct(&archive);
    }

    ~ZipArchive() {
        // Close the archive.
        mz_zip_reader_end(&archive);
    }

    // Load a zip archive from its compressed data in memory. This creates a list of entries for
    // the archive, but does not decompress any data. If the archive is successfully loaded this
    // returns true, otherwise returns false.
    bool loadFromMemory(std::vector<char> compressedArchiveData) {
        // Initialize the buffer and archive with the input data.
        buffer = std::move(compressedArchiveData);
        if (!mz_zip_reader_init_mem(&archive, buffer.data(), buffer.size(), 0)) {
            return false;
        }
        // Scan the archive entries into a list.
        auto numberOfFiles = mz_zip_reader_get_num_files(&archive);
        entries.reserve(numberOfFiles);
        for (size_t i = 0; i < numberOfFiles; i++) {
            Entry entry;
            mz_zip_archive_file_stat stats;
            if (mz_zip_reader_file_stat(&archive, i, &stats)) {
                entry.path = stats.m_filename;
                entry.uncompressedSize = stats.m_uncomp_size;
            }
            entries.push_back(entry);
        }
        return true;
    }

    // Find an entry in the archive for the given path and decompress it into memory, using the
    // allocator provided. If an entry is found for the path and successfully decompressed this
    // returns true, otherwise returns false.
    bool decompressFile(const std::string& path, std::function<char*(size_t)> allocator) {
        size_t index = 0;
        for (; index < entries.size(); index++) {
            auto& entry = entries[index];
            if (entry.path == path) {
                break;
            }
        }
        if (index >= entries.size()) {
            return false;
        }
        size_t outputSize = entries[index].uncompressedSize;
        char* outputBuffer = allocator(outputSize);
        if (!mz_zip_reader_extract_to_mem(&archive, index, outputBuffer, outputSize, 0)) {
            return false;
        }
        return true;
    }

    // Archive data used by miniz.
    mz_zip_archive archive;

    // Buffer of compressed zip archive data.
    std::vector<char> buffer;

    // List of file entries in the archive.
    std::vector<Entry> entries;
};

//
// Asset Class Implementation
//

Asset::Asset(Url url) : m_url(url) {}

std::vector<char> Asset::readBytes(const std::shared_ptr<Platform> &platform) const {
    return platform->bytesFromFile(m_url.string().c_str());
}

std::shared_ptr<Asset> Asset::getRelativeAsset(Url url) const {
    Url resolvedUrl = url.resolved(m_url);
    return std::make_shared<Asset>(resolvedUrl);
}

//
// ZippedAsset Class Implementation
//

ZippedAsset::ZippedAsset(Url url, std::vector<char> zippedData) : Asset(url) {
    // Initialize the zip archive from the zipped data.
    m_zipArchive = std::make_shared<ZipArchive>();
    m_zipArchive->loadFromMemory(zippedData);

    // Scan the archive for an entry to use as the root scene file. This asset represents that entry.
    for (const auto& entry : m_zipArchive->entries) {
        if (isBaseSceneYaml(entry.path)) {
            m_pathInArchive = entry.path;
        }
    }
}

ZippedAsset::ZippedAsset(ZippedAsset& base, std::string path) : Asset(base.url()) {
    m_zipArchive = base.m_zipArchive;
    m_pathInArchive = path;
}

bool ZippedAsset::isBaseSceneYaml(const std::string& filePath) const {
    auto extLoc = filePath.find(".yaml");
    if (extLoc == std::string::npos) { return false; }

    auto slashLoc = filePath.find("/");
    if (slashLoc != std::string::npos) { return false; }
    return true;
}

std::vector<char> ZippedAsset::readBytes(const std::shared_ptr<Platform>& platform) const {
    std::vector<char> fileData;

    if (!m_zipArchive) { return fileData; }

    auto allocator = [&](size_t size) {
        fileData.resize(size);
        return fileData.data();
    };

    if (!m_zipArchive->decompressFile(m_pathInArchive, allocator)) {
        LOGE("Cannot read zip entry \"%s\". Verify the path in the scene.", m_pathInArchive.c_str());
    }

    return fileData;
}

std::shared_ptr<Asset> ZippedAsset::getRelativeAsset(Url url) {
    if (url.isAbsolute()) {
        return std::make_shared<Asset>(url);
    }
    // If the URL is not absolute then treat it as a relative URL into the same archive this asset is in.
    return std::make_shared<ZippedAsset>(*this, url.path());
}

} // namespace Tangram
