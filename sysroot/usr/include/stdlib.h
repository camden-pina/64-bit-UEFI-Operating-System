#ifndef _STDLIB_H
#define _STDLIB_H 1

#ifdef __cplusplus
extern "C"
{
#endif

typedef size_t intptr_t;

void abort(void);
int atexit(void(*)(void));
int atoi(const char*);
void free(void*);
char* getenv(const char*);
void* malloc(size_t);

void *calloc(size_t num, size_t size);

void exit(int status);

#ifdef __cplusplus
}
#endif

#endif
