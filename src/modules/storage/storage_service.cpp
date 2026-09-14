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

bool storage_wait_ready(unsigned long timeout_ms) {
    unsigned long start = millis();
    while (millis() - start < timeout_ms) {
        if (SD.cardSize() > 0 && SD.exists("/")) {
            delay(50);  // Small delay to ensure filesystem is fully ready
            return true;
        }
        delay(10);
    }
    return false;
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

bool storage_ensure_dir(const char* path) {
    // Recursively create directories if they don't exist
    std::string pathStr(path);
    size_t pos = 1;  // Skip leading /
    
    while ((pos = pathStr.find('/', pos)) != std::string::npos) {
        std::string dir = pathStr.substr(0, pos);
        if (!SD.exists(dir.c_str())) {
            if (!SD.mkdir(dir.c_str())) {
                Serial.printf("[STORAGE] Failed to create directory: %s\n", dir.c_str());
                return false;
            }
        }
        pos++;
    }
    return true;
}

std::string storage_escape_lua_string(const std::string& input) {
    std::string output;
    for (char c : input) {
        switch (c) {
            case '\"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default: output += c; break;
        }
    }
    return output;
}

bool storage_write(const char* path, const char* data) {
    // Ensure parent directory exists
    std::string pathStr(path);
    size_t lastSlash = pathStr.find_last_of('/');
    if (lastSlash != std::string::npos) {
        std::string dir = pathStr.substr(0, lastSlash);
        if (!storage_ensure_dir(dir.c_str())) {
            Serial.printf("[STORAGE] Failed to ensure directory for: %s\n", path);
            return false;
        }
    }

    File f = SD.open(path, FILE_WRITE);
    if (!f) {
        Serial.printf("[STORAGE] Failed to open file for writing: %s\n", path);
        return false;
    }

    size_t written = f.print(data);
    f.close();
    
    if (written != strlen(data)) {
        Serial.printf("[STORAGE] Write incomplete for %s (wrote %zu/%zu bytes)\n", path, written, strlen(data));
        return false;
    }
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