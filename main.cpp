#include "FileEntry.h"
#include "StringUtilities.h"
#include "uart.h"
#include "Constants.h"
#include "FileDescriptorInfo.h"

#include <cerrno>
#include <cstdio> 

#define _LIBC           //declarations of syscalls are inside #ifdef _LIBC
#include <unistd.h>     //declarations of _read,_write,_close
#include <fcntl.h>      //declaration of _open (and also all the O_ flags lol)
//of course if that causes any issues, then just foward declare these functions (implemented in newlib-syscalls.cpp):
/*
extern "C" int _open(const char *name, int flags, ...);
extern "C" int _close(int fd);
extern "C" int _read(int fd, char *buf, int len);
extern "C" int _write(int fd, const char *buf, int len);
*/

extern bool IS_BASH_BUILTIN_ERROR;
void report_shell_error(const char* command, const char* operand, int err_code);

//global variables (will be placed in .bss because they arent initialized -- wait but what about fileTable)
char path_buffer[MAX_PATH];
char line_buffer[MAX_LINE];
char command_name[MAX_LINE];
char argv[MAX_ARGS][MAX_NAME];  //i wanted to do "char* argv[MAX_ARGS]" but then ill need dynamic allocation and i dont have a heap:((((
int argc;                       //always needs to be <MAX_ARGS
FileEntry fileTable[NUM_FILES]{}; //using the default constructor which is already purrfect :3

FileDescriptorInfo openFiles[MAX_FD];

FileEntry* currentDirEntry;

unsigned int FILE_MAXSIZE;
void initialize_filesystem() {
    path_buffer[0] = '\0';

    FILE_MAXSIZE = (FILESTORAGE_TOP - FILESTORAGE_BASE)/NUM_FILES;
    uint8_t* current_storage = FILESTORAGE_BASE;

    //initialize all file entries
    for (auto & entry : fileTable) {
        entry.initStorage(current_storage);
        current_storage += FILE_MAXSIZE;
    }

    currentDirEntry = fileTable;  //POINTER to fileTable[0]. its like writing "currentDirEntry = fileTable+0"
    //create root directory
    fileTable[0].SetNewFile("", DIRECTORY);

    for (FileDescriptorInfo & fdInfo : openFiles)
        fdInfo.entry=nullptr;
}

void add_to_path(const char* name) {
    char* p=path_buffer;
    while (*p != '\0') p++;
    if (p!=path_buffer)
        *p++ = '/';
    strcpy(p, name);
}
void remove_last_from_path() {
    char* p=path_buffer;
    while (*p != '\0') p++;
    while (p>path_buffer && *p != '/') p--;
    *p = '\0';
}

void add_to_parent(FileEntry* entry) {
    FileEntry** currentDirFile = (FileEntry**)(currentDirEntry->fileStart + currentDirEntry->fileSize);
    //the file itself is a list of POINTERS (type FileEntry*). so to go over the file we need FileEntry**
    *currentDirFile = entry;
    currentDirEntry->fileSize+=sizeof(FileEntry*);
}

void handle_backspace() {
    write_char_uart('\b');
    write_char_uart(' ');
    write_char_uart('\b');
}
void get_command() {
    char* ptrBuffer = line_buffer;    //array is a fucking const ptr, cant change it lol

    while (true) {
        char c = read_char_uart();

        if (c == '\r' || c == '\n') { // Enter (CR)
            write_char_uart('\n');
            break;
        }

        if ((c == '\b' || c == 127) && ptrBuffer > line_buffer)  //127=DEL is not represented by an escape sequence
        {
            handle_backspace();
            ptrBuffer--;
        }
        else if (ptrBuffer < line_buffer + MAX_LINE && c >= 32 && c < 127) {
            write_char_uart(c);
            *ptrBuffer++ = c;
        }
        //in the OTHER cases, dont do anything. because what was pressed is not a textual character so we dont need to write it.
    }
    *ptrBuffer = '\0';
}


FileEntry* find_file(const char* name) {
    int offset = 0;
    while (offset < currentDirEntry->fileSize) {
        FileEntry* childEntry = *(FileEntry**)(currentDirEntry->fileStart+offset);
        //whats the point?? the point is a regular files stores bytes=uint8_t, namely a pointer over it has type uint8_t*
        //but, specifically A DIRECTORY FILE stores FileEntry* one after another; sooo a pointer over such a file is FileEntry**
        if (childEntry->status && strcmp(name, childEntry->name) == 0)
            return childEntry;
        offset += sizeof(FileEntry*);
    }
    return nullptr;
}
FileEntry* create_file(const char* name, FileType type) {
    for (int i = 0; i < NUM_FILES; i++) {
        if (!fileTable[i].status) {
            fileTable[i].SetNewFile(name, type);
            add_to_parent(&fileTable[i]);
            return &fileTable[i];
        }
    }
    return nullptr;
}
bool set_redirection(int target_fd, const char *name, int new_flags)   //meant for target_fd = 0,1,2 ONLY
{
    int temp_fd = _open(name, new_flags);
    if (temp_fd == -1) {
        IS_BASH_BUILTIN_ERROR = true;
        report_shell_error(nullptr, name, errno); //sets the IS_BASH_BUILTIN_ERROR flag back to false
        return false;
    }

    openFiles[target_fd].entry = openFiles[temp_fd].entry;
    openFiles[target_fd].currentOffset = openFiles[temp_fd].currentOffset;
    openFiles[target_fd].flags = openFiles[temp_fd].flags;

    _close(temp_fd);
    return true;
}
bool parse_command() {
    argc = -1;
    const char* p = line_buffer;
    char name[MAX_NAME];

    while (*p == ' ') p++;

    while (*p && *p != '\n') {
        if (*p == '>' || (*p == '2' && *(p+1) == '>')) { //just something that starts with '2' MIGHT be an argument, the question is if its 2>
            int new_flags = O_WRONLY | O_CREAT;

            bool isErr = false;
            if (*p == '2') {
                isErr = true;
                p++;
            }
            p++; //consume the first '>'

            if (*p == '>') {
                new_flags |= O_APPEND;
                p++; //consume the second '>'
            }
            else
                new_flags |= O_TRUNC;

            while (*p == ' ') p++;

            strcpy_till_space(name, p);
            int target_fd = isErr ? 2 : 1;
            if (!set_redirection(target_fd, name, new_flags))
                return false; //ERROR HANDLED OUTSIDE (in main)

        }
        else if (*p == '<') {
            int new_flags = O_RDONLY;   //and def NOT O_CREAT

            p++;
            while (*p == ' ') p++;

            strcpy_till_space(name, p);
            if (!set_redirection(0, name, new_flags))
                return false; //ERROR HANDLED OUTSIDE (in main)
        }
        else {
            if (argc == -1) {
                strcpy_till_space(command_name, p);
                argc++;
            }
            else
                strcpy_till_space(argv[argc++], p);
        }

        while (*p && *p != ' ') p++;    //increment only the word youve read just now (via strcpy_till_space)
        while (*p == ' ') p++;          //increment till the exact next word
    }
    return true;
}

//Command implementations
void ls()   //i implement only ls that doesnt take any argument :3
{
    char temp_buffer[MAX_LINE];
    char* ptrBuffer = temp_buffer;

    int offset = 0;
    while (offset < currentDirEntry->fileSize) {
        FileEntry* childEntry = *(FileEntry**)(currentDirEntry->fileStart+offset);
        if (childEntry->status) {
            strcpy(ptrBuffer, childEntry->name);
            ptrBuffer += strlen(childEntry->name);
            *ptrBuffer++ = ' ';
        }
        offset += sizeof(FileEntry*);
    }
    *ptrBuffer++ = '\n';
    *ptrBuffer = '\0';
    printf(temp_buffer);
}

int cat_from_file(int fd, char* buf)   //we enter here only when we KNOW that (openFiles[fd].entry != nullptr)
{
    //if (S_ISDIR(st.st_mode)) { // S_ISDIR is a macro from sys/stat.h
    if (openFiles[fd].entry->type == DIRECTORY) {
        errno = EISDIR;
        return -1;
    }

    int bytes_read;
    while ((bytes_read = _read(fd, buf, MAX_LINE)) > 0) //cant read more than the fuckin buffer SIZE
        _write(1, buf, bytes_read); // Write to stdout (fd=1)
    //read returns -1 if had a problem in reading (bytes_read=-1), otherwise returns the number of bytes it reads.
    //namely if reading succeeds - because the loop is while bytes_read > 0, it will end exactly at bytes_read=0

    return bytes_read;
}
void cat() {
    char temp_buffer[MAX_LINE];

    if (argc > 0) { //case 1: cat with file arguments (e.g., cat file1 file2)
        for (int i = 0; i < argc; ++i) {
            int temp_fd = _open(argv[i], O_RDONLY);
            if (temp_fd == -1) {
                report_shell_error(command_name, argv[i], errno);
                continue;
            }
            if (cat_from_file(temp_fd, temp_buffer)==-1)
                report_shell_error(command_name, argv[i], errno);
            _close(temp_fd);
        }
    }
    else if (openFiles[0].entry) { //case 2: cat with input redirection (e.g., cat < file)
        if (cat_from_file(0, temp_buffer)==-1)
            report_shell_error(command_name, openFiles[0].entry->name, errno);
    }
    else { //case 3: Interactive cat (no arguments, no input redirection)
        while (true) {
            char* p=temp_buffer;

            while (true) {
                char c = read_char_uart();
                if (p == temp_buffer && c == 4) return; // CTRL+D

                if(c=='\r' || c=='\n' || c==4) {
                    if ((c=='\r' || c=='\n') && p < temp_buffer + MAX_LINE) {
                        write_char_uart('\n');
                        *p++ = '\n';
                    }
                    *p = '\0';
                    break;
                }
                if ((c == '\b' || c == 127) && p > temp_buffer) {
                    handle_backspace();
                    p--;
                }
                else if (p < temp_buffer + MAX_LINE && c >= 32 && c < 127) {
                    write_char_uart(c);
                    *p++ = c;
                }
            }
            //_write(1, temp_buffer, strlen(temp_buffer));
            //_write(1,...) TILL NULL TERMINATOR OF THE BUFFER (strlen) is just: fprintf(stdout,buffer) namely printf(buffer)
            printf(temp_buffer);
        }
    }
}

void rm() {
    if (argc == 0) {
        fprintf(stderr, "%s: missing operand\n", command_name); //directly print this specific error
        return;
    }
    for (int i = 0; i < argc; ++i) {
        FileEntry* entry = find_file(argv[i]);
        if (entry) //if file does exist then delete it, otherwise theres just no need
            if (entry->type==DIRECTORY){
                errno = EISDIR;
                report_shell_error(command_name, argv[i], errno);
            }
            else
                entry->status=false;
    }
}

void mkdir() {
    if (argc == 0) {
        fprintf(stderr, "%s: missing operand\n", command_name); //directly print this specific error
        return;
    }
    for (int i = 0; i < argc; ++i) {
        FileEntry* entry = find_file(argv[i]);
        if (entry){
            errno=EEXIST;
            report_shell_error(command_name, argv[i], errno);
        }
        else
            create_file(argv[i],DIRECTORY);
    }
}

void cd() {
    //IS_BASH_BUILTIN_ERROR = true;     
    //report_shell_error is the one that sets the flag back to flase... so if we set it to "true" here, it wont change back in cases without error

    if (argc == 0)     //lol this case SHOULDNT be missing operand it should go to the user directory (~) but i dont have users in my system lol :(((
    {
        //for built-ins, the -bash: prefix is added.
        fprintf(stderr, "-bash: %s: missing operand\n", command_name); //directly print this specific error
        return;
    }
    if (argc > 1) {
        errno = E2BIG;
        IS_BASH_BUILTIN_ERROR = true; 
        report_shell_error(command_name, nullptr, errno);
        return;
    }
    if (strcmp(argv[0],"..")==0) {
        currentDirEntry = currentDirEntry->parentEntry;
        remove_last_from_path();
        return;
    }
    FileEntry* entry = find_file(argv[0]);
    if (!entry){
        errno = ENOENT;
        IS_BASH_BUILTIN_ERROR = true; 
        report_shell_error(command_name, argv[0], errno); //changed entry->name to argv[0] to avoid nullptr dereference if entry is null
        return;
    }
    if (entry->type!=DIRECTORY) {
        errno = ENOTDIR;
        IS_BASH_BUILTIN_ERROR = true; 
        report_shell_error(command_name, argv[0], errno);
        return;
    }
    //if we got here that means we didnt report an
    currentDirEntry=entry;
    add_to_path(entry->name);
}

extern "C" void main() {
    init_uart();
    initialize_filesystem();
    setvbuf(stdout, nullptr, _IONBF, 0);

    while (true) {
        //RESET REDIRECTIONS for the new command
        
        openFiles[0].entry = nullptr;       //stdin
        //openFiles[0].flags = 0;           //reset flags
        //actually the flags/offset dont matter in the nullptr case :3 its always handled separately.
        openFiles[1].entry = nullptr; 
        openFiles[2].entry = nullptr; 

        printf("%c%s%c ", '/', path_buffer, '$');
        get_command();
        if (parse_command() && argc>-1) { //PARSES THE COMMAND AND SETS REDIRECTIONS FOR THE CURRENT COMMAND
            if (strcmp(command_name, "cat")==0)
                cat();
            else if (strcmp(command_name, "ls")==0)
                ls();
            else if (strcmp(command_name, "rm")==0)
                rm();
            else if (strcmp(command_name, "mkdir")==0)
                mkdir();
            else if (strcmp(command_name, "cd")==0)
                cd();
            else
                //use exit codes instead????????
                fprintf(stderr, "%s: command not found\n", command_name);
        }
    }
}