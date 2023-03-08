#pragma once

#include <stddef.h>
#include <stdarg.h>

#include <gnu-efi/efi.h>

namespace logger {

enum class type {
    success = 0,
    warn = 1,
    fail = 2,
    info = 3,
    active = 4,
};

namespace {
    EFI_SYSTEM_TABLE *m_st = nullptr;
} // namespace [private]
    
void init(EFI_SYSTEM_TABLE *st);

void log(const wchar_t *fmt, va_list ap);
size_t log(enum type type, const wchar_t *fmt, va_list ap);

void log(const wchar_t *fmt, ...);
size_t log(enum type type, const wchar_t *fmt, ...);

void fail(const wchar_t *fmt, ...);

void log_lvl(enum type type);

void clear_screen();
void set_cursor_pos(UINTN x, UINTN y);
void new_line();

extern EFI_STATUS _status;
extern bool _new_line;

void try_or_fail(EFI_STATUS s, const wchar_t *fmt, ...);

void try_or_warn(EFI_STATUS s, const wchar_t *fmt);

size_t wstrlen(const wchar_t *s);

} // namespace logger
