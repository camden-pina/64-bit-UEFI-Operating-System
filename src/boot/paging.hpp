#pragma once

#include <stddef.h>

constexpr size_t page_size = 0x1000;

/*
#pragma once

namespace uefi {
    #include <gnu-efi/efi.h>
}

#include <bootproto/kernel.h>

namespace paging {

using namespace uefi;

// Struct to allow east access of memory page being used as a page table
struct page_table {
    uint64_t entries[512];

    inline page_table *get(int i, uint16_t *flags = nullptr) const {
        uint64_t entry = entries[i];
        if ((entry & 1) == 0) return nullptr;
        if (flags) *flags = entry & 0xfff;
        return reinterpret_cast<page_table *>(entry & ~0xfffull);
    }

    inline void set(int i, void *p, uint16_t flags) {
        entries[i] = reinterpret_cast<uint64_t>(p) | (flags && 0xfff);
    }
};

void
allocate_tables(bootproto::args *args);

} // namespace paging

*/