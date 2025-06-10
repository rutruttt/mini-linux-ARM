#include "FileEntry.h"
#include "uart.h"
#include "FileDescriptorInfo.h"

#include <cstdio>
#include <cstring>
#include <cerrno> // Include cerrno for errno

//flags to use: (check website "opengroup" for more info about <fcntl.h>,<sys/stat.h>)
#include <fcntl.h>          //the open function ARGUMENTS (O_... flags)
#include <sys/stat.h>       //sigh. for sbrk imlementation (struct stat - info existing file: file type, FILE MODE (S_... flags) )

// Global variables from main.cpp
extern FileEntry fileTable[];
extern FileDescriptorInfo openFiles[];  //if i added an "is_fd_active" array i would need to make sure that 0,1,2 are always active, i dont wanna. im fine with the nullptr check.

// --- Simple Heap Management Pointer ---
static uint8_t* __current_heap_break = nullptr;

bool is_fd_active(int fd){
    if (fd<0 || fd>=MAX_FD)
        return false;
    if (fd < 3)
        return true;
    return openFiles[fd].entry != nullptr;
}

// --- Newlib Syscall Implementations ---
extern "C" int _write(int fd, const char *buf, int len) {
    extern unsigned int FILE_MAXSIZE;
    if (buf == nullptr || len < 0) {
        errno = EINVAL; // Invalid arguments
        return -1;
    }
    if (!is_fd_active(fd) || fd == 0) { // fd=0 is the input file which IS invalid for write
        errno = EBADF; // Bad file descriptor or operation not supported
        return -1;
    }

    FileEntry* entry = openFiles[fd].entry;

    if ((fd == 1 || fd == 2) && entry == nullptr) {
        // stdout or stderr not redirected to a fileDescriptor, use UART
        for(int i = 0; i < len; i++)
            write_char_uart(buf[i]);
        return len;
    }

    if (openFiles[fd].flags & O_RDONLY) { // Check if the file was opened with O_RDONLY. If so, cannot write.
        errno = EBADF; // Operation not permitted on this file descriptor mode
        return -1;
    }

    // Determine the actual write position and how many bytes can be written
    uint8_t* write_start_pos = entry->fileStart + openFiles[fd].currentOffset;
    int bytes_written = 0;

    // Calculate available space in the file's allocated storage
    // You have FILE_MAXSIZE defined which is the maximum allocated size for each file
    // The actual usable space for the file is from fileStart up to fileStart + FILE_MAXSIZE
    unsigned int available_storage_space = FILE_MAXSIZE - openFiles[fd].currentOffset;


    if (openFiles[fd].flags & O_APPEND) {
        // For O_APPEND, always write at the end of the current file size
        // This implicitly means currentOffset should be set to fileSize before writing.
        openFiles[fd].currentOffset = entry->fileSize;
        write_start_pos = entry->fileStart + openFiles[fd].currentOffset;
        available_storage_space = FILE_MAXSIZE - openFiles[fd].currentOffset;
    }


    // The number of bytes we can *actually* write is limited by 'len' and 'available_storage_space'.
    int bytes_to_copy = (len < available_storage_space) ? len : available_storage_space;

    // Check if we can write anything at all
    if (bytes_to_copy <= 0) {
        // If no space or len is 0, return 0 bytes written.
        // This is important to avoid writing beyond allocated file storage.
        return 0;
    }

    for (int i = 0; i < bytes_to_copy; i++) {
        write_start_pos[i] = buf[i];
    }
    bytes_written = bytes_to_copy;

    // Update current offset
    openFiles[fd].currentOffset += bytes_written;

    // Update file size if current write extended beyond the previous file size
    if (openFiles[fd].currentOffset > entry->fileSize) {
        entry->fileSize = openFiles[fd].currentOffset;
    }

    return bytes_written;
}

// Reads data from a file descriptor. `scanf` eventually calls this.
extern "C" int _read(int fd, char *buf, int len) {
    if (buf == nullptr || len < 0) {
        errno = EINVAL; // Invalid arguments
        return -1;
    }
    // Cannot read from stdout or stderr (fd 1 or 2) in standard POSIX.
    if (!is_fd_active(fd) || fd == 1 || fd == 2) {
        errno = EBADF; // Bad file descriptor or invalid for read
        return -1;
    }

    FileEntry* entry = openFiles[fd].entry;

    if (fd==0 && entry==nullptr){
        // stdout or stderr not redirected to a fileDescriptor, use UART
        for(int i = 0; i < len; i++)
            buf[i]=read_char_uart();
        return len;
    }

    if (openFiles[fd].flags & O_WRONLY) {
        errno = EBADF; // File opened write-only
        return -1;
    }
    int bytes_remaining_in_file = entry->fileSize - openFiles[fd].currentOffset;
    int bytes_to_read = (len < bytes_remaining_in_file) ? len : bytes_remaining_in_file;

    uint8_t* read_pos = entry->fileStart+openFiles[fd].currentOffset;
    for (int i = 0; i < bytes_to_read; i++)
        buf[i] = read_pos[i];

    openFiles[fd].currentOffset += bytes_to_read;
    return bytes_to_read;
}

FileEntry* find_file(const char*); //from main
FileEntry* create_file(const char* name, FileType type);
extern "C" int _open(const char *name, int flags, ...) {
    if (name == nullptr) {
        errno = EFAULT;
        return -1;
    }

    // Find a free file descriptor slot (starting from 3)
    int fd = -1;
    for (int i = 3; i < MAX_FD; ++i) {
        if (openFiles[i].entry == nullptr) {
            fd = i;
            break;
        }
    }

    if (fd == -1) {
        errno = EMFILE; // Too many open files
        return -1;
    }

    FileEntry* entry = find_file(name);

    // Handle O_CREAT flag
    if (entry == nullptr) {
        if (!(flags & O_CREAT)) {
            errno = ENOENT; // File does not exist and O_CREAT not specified
            return -1;
        }

        entry=create_file(name,REGULAR);
        if (entry == nullptr) {
            errno = ENOSPC; // No space left on device (no free fileTable entry)
            return -1;
        }
    }

    // Handle existing file
    // Cannot open directory for writing or truncating
    if (entry->type == DIRECTORY) {
        if (!(flags & O_RDONLY) || flags & O_TRUNC) {
            errno = EISDIR; // trying to open directory for writing/truncate a directory
            return -1;
        }
    }

    if (flags & O_TRUNC) 
        entry->fileSize = 0;  // truncate if it's a regular file

    // Populate the FdInfo table for the opened file descriptor
    openFiles[fd].entry = entry;    //its NOT nullptr due to prior logic
    openFiles[fd].flags = flags;
    if (flags & O_APPEND)
        openFiles[fd].currentOffset = entry->fileSize;
    else
        openFiles[fd].currentOffset = 0;

    return fd; // Return the new file descriptor
}

extern "C" int _close(int fd) {
    // Disallow closing standard streams (0, 1, 2) directly in this model, as they are managed conceptually by the OS and are always "open".
    // If they were redirected, the redirection mechanism would handle detaching.
    if (!is_fd_active(fd) || fd < 3) {
        errno = EBADF;
        return -1;
    }

    openFiles[fd].entry = nullptr;
    return 0; // Success
}

//FROM NOW ON: printf/fptintf recursively reference to them, but not depend on their behavior
//namely FUNCTION FROM NOW ON ARE NOT NEEDED FOR MY SYSTEM AT ALL (in a sense that a trivial implementation is enough)
//i just give some a full implementation to like practice??

// _sbrk(int incr)
// For memory allocation (heap). `malloc` uses this.
extern "C" void* _sbrk(int incr) {
    // Initialize heap break on first call
    if (__current_heap_break == nullptr) {
        // A common bare-metal strategy: heap starts right after static data/file storage.
        // You MUST ensure this address is correct for your linker script.
        __current_heap_break = FILESTORAGE_TOP;
    }

    uint8_t* prev_heap_break = __current_heap_break;
    uint8_t* new_heap_break = __current_heap_break + incr;

    // Basic bounds check: prevent heap from growing into invalid regions (e.g., stack).
    // You would define RAM_END_ADDR or STACK_BOTTOM based on your memory map.
    // Example: const uint8_t* RAM_END_ADDR = (uint8_t*)0x20020000; // Define this yourself in Constants.h
    // if (new_heap_break < FILESTORAGE_TOP || new_heap_break >= RAM_END_ADDR) {
    //     errno = ENOMEM; // Out of memory
    //     return (void*)-1; // Indicate error
    // }

    __current_heap_break = new_heap_break;
    return (void*)prev_heap_break; // Return the start of the newly allocated memory
}
// _fstat implementation (needed by printf)
extern "C" int _fstat(int file, struct stat *st) {
    if (!is_fd_active(file)) {
        errno = EBADF;
        return -1;
    }

    memset(st, 0, sizeof(struct stat));

    if (file < 3) {
        // stdin/stdout/stderr - character devices
        st->st_mode = S_IFCHR;
        return 0;
    }

    FileEntry* entry = openFiles[file].entry;
    if (entry->type == DIRECTORY) {
        st->st_mode = S_IFDIR;
    } else {
        st->st_mode = S_IFREG;
        st->st_size = entry->fileSize;
    }
    return 0;
}

// _isatty implementation (needed by printf)
extern "C" int _isatty(int file) {
    if (!is_fd_active(file))
        return 0; // Return 0 (false) if not a valid FD or not a TTY
    return (file < 3); // Only stdin/stdout/stderr are terminals
}

// _lseek implementation
extern "C" int _lseek(int file, int ptr, int dir) {
    if (!is_fd_active(file) || file < 3) {
        errno = EBADF;
        return -1;
    }

    FileEntry* entry = openFiles[file].entry;
    switch(dir) {
        case SEEK_SET:
            openFiles[file].currentOffset = ptr;
        break;
        case SEEK_CUR:
            openFiles[file].currentOffset += ptr;
        break;
        case SEEK_END:
            openFiles[file].currentOffset = entry->fileSize + ptr;
        break;
        default:
            errno = EINVAL; // Invalid argument for 'dir'
            return -1;
    }
    // TODO: Add bounds check for openFiles[file].currentOffset to ensure it doesn't go negative or beyond file size/max size.
    // If it goes out of bounds, set errno to EINVAL and return -1.

    return openFiles[file].currentOffset;
}

// _getpid()
// Often stubbed out or implemented minimally for embedded contexts.
extern "C" int _getpid() {
    // In a single-process bare-metal system, a dummy PID is common.
    return 1; // Return a dummy Process ID (e.g., 1 for the main/only program)
}

// _kill(int pid, int sig)
// Often stubbed out or implemented minimally for embedded contexts.
extern "C" int _kill(int pid, int sig) {
    errno = ENOSYS; // Operation not supported on this system
    return -1; // Indicate error: process signaling/killing is not supported
}

// _exit(int status)
// For terminating the program.
extern "C" void _exit(int status) {
    (void)status; // Parameter is typically exit status, but ignored in bare-metal.
    // In a bare-metal context, this usually halts the system or resets it.
    // A simple infinite loop prevents the program from returning unexpectedly.
    while(1);
}