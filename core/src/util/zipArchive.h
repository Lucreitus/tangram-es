#pragma once

#include <functional>
#include <string>
#include <vector>

#define MINIZ_NO_ZLIB_APIS // Remove all ZLIB-style compression/decompression API's.
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES // Disable zlib names, to prevent conflicts against stock zlib.
#include <miniz.h>

namespace Tangram {

class ZipArchive {

public:
    // Create an uninitialized archive.
    ZipArchive();

    // Dispose an archive and any memory it owns.
    ~ZipArchive();

    // Load a zip archive from its compressed data in memory. This creates a list of entries for
    // the archive, but does not decompress any data. If the archive is successfully loaded this
    // returns true, otherwise returns false. The data is moved out of the input vector and
    // retained by the archive until other data is loaded or the archive is destroyed.
    bool loadFromMemory(std::vector<char> compressedArchiveData);

    // Find an entry in the archive for the given path and decompress it into memory, using the
    // allocator provided. If an entry is found for the path and successfully decompressed this
    // returns true, otherwise returns false.
    bool decompressFile(const std::string& path, std::function<char*(size_t)> allocator);


    // Buffer of compressed zip archive data.
    std::vector<char> buffer;

    struct Entry {
        std::string path;
        size_t uncompressedSize = 0;
    };

    // List of file entries in the archive.
    std::vector<Entry> entries;

protected:
    // Archive data used by miniz.
    mz_zip_archive archive;
};

} // namespace Tangram
