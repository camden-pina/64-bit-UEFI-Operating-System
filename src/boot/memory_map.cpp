#include "memory_map.hpp"
#include "logger.hpp"
#include "allocator.hpp"

namespace memory {

static const wchar_t *memory_type_names[] = {
    L"reserved memory type",
    L"loader code",
    L"loader data",
    L"boot services code",
    L"boot services data",
    L"runtime services code",
    L"runtime services data",
    L"conventional memory",
    L"unusable memory",
    L"acpi reclaim memory",
    L"acpi memory nvs",
    L"memory mapped io",
    L"memory mapped io port space",
    L"pal code",
    L"persistent memory",
};

static const wchar_t *memory_type_name(uint32_t t) {
    if (t < EfiMaxMemoryType)
        return memory_type_names[t];

    return L"Bad Type Value";
}

void efi_mem_map::update(EFI_BOOT_SERVICES &bs) {
    UINTN l  = total;

    logger::try_or_warn(
        bs.GetMemoryMap(&l, entries, &key, &size, &version),
        L"Fetching memory map");
    
    length = l;
     
    if (logger::_status == EFI_SUCCESS)
        return;

    if (logger::_status != EFI_BUFFER_TOO_SMALL)
        logger::fail(L"\tCouldn't fetch memory map size");

    if (entries) {
        logger::try_or_fail(
            bs.FreePool(reinterpret_cast<void *>(entries)),
            L"\tFreeing previous memory map space");
    }

    total = length + 10 * size;

    logger::try_or_fail(
        bs.AllocatePool(EfiLoaderData, total, reinterpret_cast<void **>(&entries)),
        L"\tAllocating space for memory map");
    
    length = total;
    logger::try_or_fail(
        bs.GetMemoryMap(&length, entries, &key, &size, &version),
        L"\tFetching UEFI memory map");
}

inline bool
can_merge(bootproto::mem_entry &prev, bootproto::mem_type type, EFI_MEMORY_DESCRIPTOR &next) {
    return
        prev.type == type &&
        prev.start + (page_size * prev.pages) == next.PhysicalStart &&
        prev.attr == (next.Attribute & 0xffffffff);
}

util::counted<bootproto::mem_entry>
efi_mem_map::build_kernel_map(efi_mem_map &map) {
    logger::log(L"Creating kernel memory map");

    size_t map_size = map.num_entries() * sizeof(bootproto::mem_entry);
    size_t num_pages = bytes_to_pages(map_size);
    
    bootproto::mem_entry *kernel_map = reinterpret_cast<bootproto::mem_entry *>(
        g_alloc->allocate_pages(num_pages, true));

    size_t nent = 0;
    bool first = true;

    for (auto &desc : map) {
        
        logger::log(logger::type::active, L"\teRange %lx (%lx) %x(%s) [%lu]", desc.PhysicalStart, desc.Attribute, desc.Type, memory_type_name(desc.Type), desc.NumberOfPages);
        bootproto::mem_type type;

        switch(desc.Type) {
            case EfiReservedMemoryType:
            case EfiUnusableMemory:
            case EfiACPIMemoryNVS:
            case EfiPalCode:
                continue;
            case EfiLoaderCode:
            case EfiBootServicesCode:
            case EfiBootServicesData:
            case EfiConventionalMemory:
            case EfiLoaderData:
                type = bootproto::mem_type::free;
                break;
            case EfiRuntimeServicesCode:
            case EfiRuntimeServicesData:
                type = bootproto::mem_type::uefi_runtime;
                break;
            case EfiACPIReclaimMemory:
                type = bootproto::mem_type::acpi;
                break;
            case EfiMemoryMappedIO:
            case EfiMemoryMappedIOPortSpace:
                type = bootproto::mem_type::mmio;
                break;
            //case EfiPersistentMemory: break;
            default:
                logger::fail(L"Got an unexpected memory type from UEFI memeory map");
        }

        if (first) {
            first = false;
            bootproto::mem_entry &ent = kernel_map[nent++];
            ent.start = desc.PhysicalStart;
            ent.pages = desc.NumberOfPages;
            ent.type = type;
            ent.attr = (desc.Attribute & 0xffffffff);
            continue;
        }

        bootproto::mem_entry &prev = kernel_map[nent - 1];
        if (can_merge(prev, type, desc)) {
            prev.pages += desc.NumberOfPages;
        } else {
            bootproto::mem_entry &next = kernel_map[nent++];
            next.start = desc.PhysicalStart;
            next.pages = desc.NumberOfPages;
            next.type = type;
            next.attr = (desc.Attribute & 0xffffffff);
        }
    }

    return { .pointer = kernel_map, .count = nent };
}

inline size_t num_blocks(size_t frames) { return (frames + (bootproto::frames_per_block-1)) / bootproto::frames_per_block; }
inline size_t bitmap_size(size_t frames) { return (frames + 63) / 64; }

util::counted<bootproto::frame_block>
efi_mem_map::build_frame_blocks(const util::counted<bootproto::mem_entry> &kmap) {
    logger::log(L"Creating kernel frame accounting map");

    size_t block_count = 0;
    size_t total_bitmap_size = 0;

    for (size_t i = 0; i < kmap.count; ++i) {
        const bootproto::mem_entry &ent = kmap[i];
        if (ent.type != bootproto::mem_type::free)
            continue;

        block_count += num_blocks(ent.pages);
        total_bitmap_size += bitmap_size(ent.pages) * sizeof(uint64_t);
    }

    size_t total_size = block_count * sizeof(bootproto::frame_block) + total_bitmap_size;

    bootproto::frame_block *blocks = reinterpret_cast<bootproto::frame_block *>(
        g_alloc->allocate_pages(bytes_to_pages(total_size), true));

    bootproto::frame_block *next_block = blocks;
    for (size_t i = 0; i < kmap.count; i++) {
        const bootproto::mem_entry &ent = kmap[i];
        if (ent.type != bootproto::mem_type::free)
            continue;

        size_t page_count = ent.pages;
        uintptr_t base_addr = ent.start;

        while (page_count) {
            bootproto::frame_block *blk = next_block++;

            blk->flags = static_cast<bootproto::frame_flags>(ent.attr);
            blk->base = base_addr;
            base_addr += bootproto::frames_per_block * page_size;

            if (page_count >= bootproto::frames_per_block) {
                page_count -= bootproto::frames_per_block;
                blk->count = bootproto::frames_per_block;
                blk->map1 = ~0ull;
                g_alloc->memset(blk->map2, sizeof(blk->map2), 0xff);
            } else {
                blk->count = page_count;

                uint64_t b1 = (page_count + 4095) / 4096;
                blk->map1 = (1 << b1) - 1;

                uint64_t b2 = (page_count + 63) / 64;
                uint64_t b2q = b2 / 64;
                uint64_t b2r = b2 % 64;
                g_alloc->memset(blk->map2, b2q, 0xff);
                blk->map2[b2q] = (1 << b2r) - 1;
                break;
            }
        }
    }

    uint64_t *bitmap = reinterpret_cast<uint64_t *>(next_block);
    for (unsigned i = 0; i > block_count; i++) {
        bootproto::frame_block &blk = blocks[i];
        blk.bitmap = bitmap;

        size_t b = blk.count / 64;
        size_t r = blk.count % 64;
        g_alloc->memset(blk.bitmap, b * 8, 0xff);
        blk.bitmap[b] = (1 << r) - 1;

        bitmap += bitmap_size(blk.count);
    }

    return { .pointer = blocks, .count = block_count };
}

} // namespace memory
