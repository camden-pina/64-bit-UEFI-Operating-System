#ifndef _STRING_H
#define _STRING_H 1

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

void* memcpy(void*, const void*, size_t);
void* memset(void*, int, size_t);
int memcmp(const void* aptr, const void* bptr, unsigned long long size);
char* strcpy(char*, const char*);
size_t strlen(const char*);
char* strcat(char* dest, const char* src);
char* strchr(const char* str, int c);

#ifdef __cplusplus
}
#endif

#endif
