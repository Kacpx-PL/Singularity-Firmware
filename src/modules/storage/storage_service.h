#pragma once
#include <cstddef>
#include <vector>
#include <string>

#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

struct DirEntry {
    std::string name;
    bool is_dir;
};

bool storage_init();
bool storage_is_ready();
bool storage_exists(const char* path);
bool storage_read(const char* path, char* buf, size_t maxlen);
bool storage_write(const char* path, const char* data);
size_t storage_get_file_size(const char* path);
std::vector<std::string> storage_list_files(const char* dir, const char* extension);
std::vector<std::string> storage_list_dirs(const char* dir);
std::vector<DirEntry> storage_list_dir_entries(const char* dir);