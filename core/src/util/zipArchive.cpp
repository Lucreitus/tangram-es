#include "zipArchive.h"

namespace Tangram {

ZipArchive::ZipArchive() {
    mz_zip_zero_struct(&archive);
}

ZipArchive::~ZipArchive() {
    // Close the archive.
    // mz_zip_reader_end(&archive);
}

// Load a zip archive from its compressed data in memory. This creates a list of entries for
// the archive, but does not decompress any data. If the archive is successfully loaded this
// returns true, otherwise returns false.
bool ZipArchive::loadFromMemory(std::vector<char> compressedArchiveData) {
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
bool ZipArchive::decompressFile(const std::string& path, std::function<char*(size_t)> allocator) {
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

}
