#include "storage_service.h"
#include <SD.h>
#include <SPI.h>

bool storage_init() {
    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    return SD.begin(SD_SPI_CS_PIN, SPI, 25000000);
}

bool storage_is_ready() {
    return SD.cardSize() > 0;
}

bool storage_exists(const char* path) {
    return SD.exists(path);
}

bool storage_read(const char* path, char* buf, size_t maxlen) {
    File f = SD.open(path, FILE_READ);
    if (!f) return false;

    size_t bytesRead = f.readBytes(buf, maxlen - 1);
    buf[bytesRead] = '\0';

    f.close();
    return true;
}

bool storage_write(const char* path, const char* data) {
    File f = SD.open(path, FILE_WRITE);
    if (!f) return false;

    f.print(data);
    f.close();
    return true;
}

std::vector<std::string> storage_list_files(const char* dir, const char* extension) {
    std::vector<std::string> results;

    File root = SD.open(dir);
    if (!root || !root.isDirectory()) {
        return results;
    }

    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            std::string name = file.name();
            size_t extLen = strlen(extension);
            if (name.size() > extLen &&
                name.compare(name.size() - extLen, extLen, extension) == 0) {
                results.push_back(name);
            }
        }
        file = root.openNextFile();
    }

    return results;
}

std::vector<std::string> storage_list_dirs(const char* dir) {
    std::vector<std::string> results;

    File root = SD.open(dir);
    if (!root || !root.isDirectory()) {
        return results;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            results.push_back(file.name());
        }
        file = root.openNextFile();
    }

    return results;
}

std::vector<DirEntry> storage_list_dir_entries(const char* dir) {
    std::vector<DirEntry> results;

    File root = SD.open(dir);
    if (!root || !root.isDirectory()) return results;

    File file = root.openNextFile();
    while (file) {
        results.push_back({file.name(), file.isDirectory()});
        file = root.openNextFile();
    }

    return results;
}

size_t storage_get_file_size(const char* path) {
    File f = SD.open(path, FILE_READ);
    if (!f) return 0;
    size_t size = f.size();
    f.close();
    return size;
}