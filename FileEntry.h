#pragma once

#include "Constants.h"
#include <cstdint>

enum FileType { REGULAR = '-', DIRECTORY = 'd' };

//file table entry
struct FileEntry {
    bool status;
    uint8_t* const fileStart;
    FileEntry* parentEntry;
    char name[MAX_NAME];    //can create the file with this name only if parent doesnt have anyy other child with this name. no matter the type
    FileType type;
    int fileSize;
    //actual implementations in main.cpp lol
    FileEntry();
    void initStorage(uint8_t* actualStart);
    void SetNewFile(const char* name, FileType type);
};