#pragma once

#include <gnu-efi/efi.h>

#include "paging.hpp"
#include <bootproto/kernel.h>

namespace memory {

class efi_mem_map {
public:
    UINTN length;
    UINTN total;
    UINTN size;
    UINTN key;
    UINT32 version;
    EFI_MEMORY_DESCRIPTOR *entries;

    efi_mem_map() : length(0), total(0), size(0), key(0), version(0), entries(nullptr) {}

    // Update the map from UEFI
    void update(EFI_BOOT_SERVICES &bs);

    inline size_t num_entries() const { return length / size; }

    util::counted<bootproto::mem_entry> build_kernel_map(efi_mem_map &map);

    util::counted<bootproto::frame_block> build_frame_blocks(const util::counted<bootproto::mem_entry> &kmap);

    inline util::offset_iterator<EFI_MEMORY_DESCRIPTOR> begin() { return util::offset_iterator<EFI_MEMORY_DESCRIPTOR>(entries, size); }
    inline util::offset_iterator<EFI_MEMORY_DESCRIPTOR> end() { return util::offset_pointer(entries, length); }

};

inline constexpr size_t bytes_to_pages(size_t bytes) {
    return ((bytes - 1) / page_size) + 1;
}

// Add the kernel's memory map as a module tot eh kernel args.
// util::vector<bootproto::mem_entry> build_kernel_map(efi_mem_map &map);

} // namespace memory
