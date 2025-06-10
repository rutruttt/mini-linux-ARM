#pragma once

struct FileDescriptorInfo {
    FileEntry* entry;       //pointer to the actual filesystem entry (nullptr if not a disk file, e.g., for UART)
    int currentOffset;      //current read/write position for this open instance
    int flags;              //stores open mode flags (e.g., O_RDONLY, O_WRONLY, O_RDWR)
};
