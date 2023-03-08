#pragma once

#include <bootproto/init.h>

#include <gnu-efi/efi.h>
#include <stddef.h>

namespace memory {

class efi_mem_map;

class allocator {
public:
    allocator(EFI_BOOT_SERVICES &bs);

    static void init(bootproto::modules_page *&modules, EFI_BOOT_SERVICES *bs);

    void *allocate(size_t size, bool zero = false);
    void *allocate_pages(size_t count, bool zero);

    template <typename M>
    M *allocate_module(size_t extra = 0) {
        return static_cast<M *>(allocate_module_untyped(sizeof(M) + extra));
    }

    void free(void *p);

    void memset(void *start, size_t size, uint8_t val);
    void copy(void *to, void *from, size_t size);

private:
    void add_modules();
    bootproto::module *allocate_module_untyped(size_t size);

    EFI_BOOT_SERVICES &m_bs;
    bootproto::modules_page *m_modules;
    bootproto::module *m_next_mod;

};

} // namespace memory

extern memory::allocator *g_alloc;

void *operator new (size_t size, void *p);
void *operator new(size_t size);
void *operator new [] (size_t size);
void operator delete (void *p) noexcept;
void operator delete [] (void *p) noexcept;
