#pragma once
#include <cstdint>

extern "C" {
    extern uint8_t FILESTORAGE_BASE[];
    extern uint8_t FILESTORAGE_TOP[];
}

#define NUM_FILES 1024
#define MAX_NAME 32
#define MAX_PATH 256
#define MAX_LINE 4096
#define MAX_ARGS 100
#define MAX_FD 16