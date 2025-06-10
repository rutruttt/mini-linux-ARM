#include <cstdio> // For fprintf, stderr
#include <cerrno>

bool IS_BASH_BUILTIN_ERROR = false; //in my system - only parsing (redirections) and "cd" are bash-builtins lol

// Function to print a user-friendly error message based on errno
void report_shell_error(const char* command, const char* operand, int err_code) {
    // Determine the prefix based on the context
    if (IS_BASH_BUILTIN_ERROR){
        fprintf(stderr, "-bash: ");     //example: -bash: <file>: No such file or directory
        IS_BASH_BUILTIN_ERROR = false;  //meaning the next command needs to EXPLICITLY SRT the flag if its a builtin error
    }
    if(command)
        fprintf(stderr, "%s: ", command);
    if(operand)
        fprintf(stderr, "%s: ", operand);

    //print the specific error message based on err_code
    switch (err_code) {
        case ENOENT:
            fprintf(stderr, "no such file or directory\n");
            break;
        case EISDIR:
            fprintf(stderr, "is a directory\n");
            break;
        case ENOTDIR:
            fprintf(stderr, "is not a directory\n");
        break;
        case EEXIST:
            fprintf(stderr, "file exists\n");
            break;
        case ENOSPC: //no space left on device (your "memory full" case)
            fprintf(stderr, "memory is full\n");
            break;
        case EBADF:
            fprintf(stderr, "bad file descriptor\n");
            break;
        case EMFILE:
            fprintf(stderr, "too many open files\n");
            break;
        case EFAULT:
            fprintf(stderr, "bad address\n");
        break;
        case E2BIG:
            fprintf(stderr, "too many arguments\n");
        break;
        // ... other errno values you might define
        default:
            fprintf(stderr, "unknown error (%d)\n", err_code);
            break;
    }
}