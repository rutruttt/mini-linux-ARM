#include "FileEntry.h"
#include "StringUtilities.h"

extern FileEntry* currentDirEntry;

FileEntry::FileEntry(): status(false), fileStart(nullptr) {}

void FileEntry::initStorage(uint8_t* actualStart) {
    const_cast<uint8_t*&>(fileStart) = actualStart;  //safe const modification
}

void FileEntry::SetNewFile(const char* name, mode_t type) {
    this->status = true;
    this->parentEntry = currentDirEntry;
    strcpy(this->name, name);
    this->type = type;
    this->fileSize = 0;
}