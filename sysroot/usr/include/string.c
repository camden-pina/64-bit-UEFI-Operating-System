#ifndef _STRING_H_
#define _STRING_H_

#include <string.h>

void* memcpy(void* destination, const void* source, size_t length)
{
	for (unsigned long long i = 0; i < length; ++i) {
		((unsigned char*)destination)[i] = ((unsigned char*)source)[i];
	}
	return destination;
}

void* memset(void* buffer, int value, size_t length)
{
	for (unsigned long long i = 0; i < length; ++i) {
		((unsigned char*)buffer)[i] = (unsigned char)value;
	}
	return buffer;
}

int memcmp(const void* aptr, const void* bptr, unsigned long long size)
{
	const unsigned char* a = (const unsigned char*)aptr;
	const unsigned char* b = (const unsigned char*)bptr;
	for (unsigned long long i = 0; i < size; i++) {
		if (a[i] < b[i])
			return -1;
		else if (b[i] < a[i])
			return 1;
	}
	return 0;
}

char* strcpy(char* cch, const char* ch)
{
	return (void*)0;
}

size_t strlen(const char* ch)
{
	return 15;
}

char* strcat(char* dest, const char* src)
{
}

char* strchr(const char* str, int c)
{
	return (void*)0;
}

#endif
