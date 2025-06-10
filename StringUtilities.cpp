
void strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

void strcpy_till_space(char* dest, const char* src) {
    while (*src && *src != ' ' && *src != '\n') {
        *dest++ = *src++;
    }
    *dest = 0;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *s1 - *s2;
}

int strlen(const char* s) {
    int len=0;
    while (*s++)
        len++;
    return len;
}