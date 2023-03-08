#include "logger.hpp"
#include "video.hpp"
#include "allocator.hpp"
#include "memory_map.hpp"
#include "hardware.hpp"
#include "bootconfig.hpp"
#include "fs.hpp"
#include "loader.hpp"

#include <bootproto/kernel.h>
#include <util/enum_bitfields.hpp>
#include <gnu-efi/efi.h>

/*
// start paging

uint64_t next_alloc_page = 0;

void *alloc_page() {
    void *page = (void *)next_alloc_page;
    next_alloc_page += 0x1000;
    return page;
}

struct uefi_mmap {
    uint64_t nbytes;
    uint8_t buffer[UEFI_MMAP_SIZE];
    uint64_t mapkey;
    uint64_t desc_size;
    uint32_t desc_version;
};

struct uefi_mmap uefi mmap;
void setup_paging(EFI_HANDLE img) {
    uefi_mmap.nbytes = UEFI_MMAP_SIZE;
    bs->GetMemoryMap, 5, &uefi_mmap.nbytes, &uefi_mmap.buffer, &uefi_mmap.map_key,
        &uefi_mmap.desc_size, &uefi_mmap.desc_version);

    uint64_t best_alloc_start = 0;
    uint64_t best_num_of_pages = 0;
    for (int i = 0; i < uefi_mmap.nbytes; i += uefi_mmap.desc_size) {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)&uefi_mmap.buffer[i];
        if (desc->Type !- EfiConventionalMemory) continue;
        if (desc0>NumberOfPages > best_number_of_pages) {
            best_num_of_pages = desc->NumberOfPages;
            best_alloc_start = desc->PhysicalStart;
        }
    }
    next_alloc_page = best_alloc_start;
}

// end paging

// start pml4

#pragma pack(1)
struct mapping_table {
    uint64_t entries[512];
};

#pragma pack()

__attribute__((aligned(0x1000)))
struct mapping_table pml4;

#define PAGE_BIT_P_PRESENT (1 << 0)
#define PAGE_BIT_RW_WRITBLE (1 << 1)
#define PAGE_BIT_US_USER (1 << 2)
#define PAGE_XD_NX (1 << 63)

// bit masl for page aligned 52-bit addresses
#define PAFE_ADDR_MASK 0x00ffffffffff00

#define PAGE_BIT_A_ACCESSES (1 << 5)
#define PAGE_BIT_D_DIRTY (1 << 6)

void identity_map_4kb(uint64_t logical) {
    int flags = PAGE_BIT_P_PRESENT | PAGE_BIT_RW_WRITBLE | PAGE_BIT_US_USER;

    // extract mapping table indicies from virtual address
    int pml4_idx = (logical >> 39) & 0x1dd;
    int pdp_idx = (logical >> 30) & 0x1ff;
    int pd_idx = (logical >> 21) & 0x1ff;
    int pt_idx = (logical >> 12) & 0x1ff;
    int p_idx = logical & 0x7ff;

    // did we define a PDPT for this PML4 index?
    if (!(pml4.entries[pml4_idx] & PAGE_BIT_P_PRESENT)) {
        // no, so lets allocate a new page for the PDPT
        uint64_t pdpt_alloc = (uint64_t)alloc_page();
        // zero it, this makes the PDPT an empty table with no PDTs present
        memzero((void *)pdpt_alloc, 0x1000);

        // update PML4 so it contains new PDPT
        pml4.entries[pml4_idx] = (pdpt_alloc & PAGE_ADDR_MASK) | flags;

        identity_map_4kb(pdpt_alloc);
    }
}

// end pml4
*/

enum pt_flag {
    present,
    readwrite,
    usersuper,
    writethrough,
    cache_disabled,
    accessed,
    larger_pages = 7,
    custom0 = 9,
    custom1,
    custim2,
    nx = 63,
};

struct page_dir_entry {
    uint64_t val;

    void set_flag(pt_flag flag, bool Enabled) {
        uint64_t bit_selector = (uint64_t) 1 << flag;
        val &= ~bit_selector;

        if (Enabled)
            val |= bit_selector;
    }

    bool get_flag(pt_flag flag) {
        uint64_t bit_selector = (uint64_t)1 << flag;
        return val & bit_selector > 0 ? true : false;
    }

    uint64_t get_addr() {
        return (val & 0x000ffffffffff000) >> 12;
    }

    void set_addr(uint64_t addr) {
        addr &= 0x000000ffffffffff;
        val &= 0xfff0000000000fff;
        val |= (addr << 12);
    }
};

class page_map_indexer {
public:
    uint64_t p_i;
    uint64_t pt_i;
    uint64_t pd_i;
    uint64_t pdp_i;

    page_map_indexer(uint64_t virt_addr) {
        virt_addr >>= 12;
        p_i = virt_addr & 0x1ff;
        virt_addr >>= 9;
        pt_i = virt_addr & 0x1ff;
        virt_addr >>= 9;
        pd_i = virt_addr & 0x1ff;
        virt_addr >>= 9;
        pdp_i = virt_addr & 0x1ff;
    }
};

struct page_table {
    page_dir_entry entries[512];
} __attribute__((aligned(0x1000)));

class page_table_manager {
public:
    page_table *pml4;

    page_table_manager(page_table *pml4_addr) {
        this->pml4 = pml4_addr;
    }

    void map_memory(void *virt_addr, void *phys_addr) {
        page_map_indexer indexer = page_map_indexer((uint64_t)virt_addr);
        page_dir_entry pde;

        pde = pml4->entries[indexer.pdp_i];
        page_table *pdp;
        if (!pde.get_flag(pt_flag::present)) {
            pdp = (page_table *)g_alloc->allocate_pages(1, true);
            pde.set_addr((uint64_t)pdp >> 12);
            pde.set_flag(pt_flag::present, true);
            pde.set_flag(pt_flag::readwrite, true);
            pml4->entries[indexer.pdp_i] = pde;
        } else {
            pdp = (page_table *)((uint64_t)pde.get_addr() << 12);
        }

        pde = pdp->entries[indexer.pd_i];
        page_table *pd;
        if (!pde.get_flag(pt_flag::present)) {
            pd = (page_table *)g_alloc->allocate_pages(1, true);
            pde.set_addr((uint64_t)pd >> 12);
            pde.set_flag(pt_flag::present, true);
            pde.set_flag(pt_flag::readwrite, true);
            pdp->entries[indexer.pd_i] = pde;
        } else {
            pd = (page_table *)((uint64_t)pde.get_addr() << 12);
        }

        pde = pd->entries[indexer.pt_i];
        page_table *pt;
        if (!pde.get_flag(pt_flag::present)) {
            pt = (page_table *)g_alloc->allocate_pages(1, true);
            pde.set_addr((uint64_t)pt >> 12);
            pde.set_flag(pt_flag::present, true);
            pde.set_flag(pt_flag::readwrite, true);
            pd->entries[indexer.pt_i] = pde;
        } else {
            pt = (page_table *)((uint64_t)pde.get_addr() << 12);
        }

        pde = pt->entries[indexer.p_i];
        pde.set_addr((uint64_t)phys_addr >> 12);
        pde.set_flag(pt_flag::present, true);
        pde.set_flag(pt_flag::readwrite, true);
        pt->entries[indexer.p_i] = pde;

    }
};

bootproto::args *
uefi_preboot(EFI_SYSTEM_TABLE *st) {

    logger::log(L"Performing UEFI pre-boot");

    //memory::init_pointer_fixup(st->BootServices, st->RuntimeServices);

    bootproto::args *args = new bootproto::args;

    args->runtime_services = st->RuntimeServices;

    logger::try_or_fail(
        hw::find_acpi_table(st, args->acpi_tables),
        L"Fetching ACPI 2.0 Tables");

    page_table *pml4 = (page_table *)g_alloc->allocate_pages(1, true);
    page_table_manager ptm(pml4);

    args->pml4 = pml4;

    //ptm.map_memory((void *t, *void *)t);


    //memory::mark_pointer_fixup(&args->runtime_services);

    //paging::allocate_tabless(args);

    return args;
}
memory::efi_mem_map
uefi_exit(bootproto::args *args, EFI_HANDLE img, EFI_BOOT_SERVICES *bs) {
    logger::log(L"Exiting UEFI");

    memory::efi_mem_map map;
    map.update(*bs);

    args->mem_map = map.build_kernel_map(map);
    args->frame_blocks = map.build_frame_blocks(args->mem_map);

    map.update(*bs);

    logger::try_or_fail(
        bs->ExitBootServices(img, map.key),
        L"Failed to exit boot services");


/*
    uefi::EFI_STATUS _ls = bs->ExitBootServices(img, map.key);
    if (_ls != EFI_SUCCESS) {
        logger::log(L"AMATURE CANT EVEN EXIT BOOT SERVICES RIGHT LOL");
    }
    */

    return map;
}

void
load_resources(bootproto::args *args, video::screen *screen, EFI_HANDLE img, EFI_BOOT_SERVICES *bs) {
    logger::log(L"Loading programs");

    fs::file disk = fs::get_boot_volume(img, bs);
    fs::file bc_data = disk.open(L"modernos_boot.dat");
    bootconfig bc { bc_data.load(), bs };

    args->kernel = loader::load_program(disk, bc.kernel(), true); // loader::load_program(disk, bc.kernel(), true);
    args->init = loader::load_program(disk, bc.init());
    args->flags = static_cast<bootproto::boot_flags>(bc.flags());

    if (screen) {
        video::make_module(screen);

        // Go through the screen-specific descriptors first to give them priority.
        for (const descriptor &desc : bc.programs()) {
            if (!util::bits::has(desc.flags, bootproto::desc_flags::graphical))
                continue;

            if (util::bits::has(desc.flags, bootproto::desc_flags::panic))
                args->panic = loader::load_program(disk, desc);
            else
                loader::load_module(disk, desc);
        }
    }

    // Load the non-graphical descriptors
    for (const descriptor &desc : bc.programs()) {
        if (util::bits::has(desc.flags, bootproto::desc_flags::graphical))
            continue;

        if (util::bits::has(desc.flags, bootproto::desc_flags::panic) && !args->panic)
            args->panic = loader::load_program(disk, desc);
        else
            loader::load_module(disk, desc);
    }

    // For now the only data we load is the symbol table
    for (const descriptor &desc : bc.data()) {
        if (!util::bits::has(desc.flags, bootproto::desc_flags::symbols))
            continue;

        util::buffer symbol_table = loader::load_file(disk, desc);
        args->symbol_table = symbol_table.pointer;
    }

    loader::verify_kernel_header(*args->kernel);
}

extern "C" EFI_STATUS
efi_main(EFI_HANDLE img, EFI_SYSTEM_TABLE *st) {
    EFI_BOOT_SERVICES *bs = st->BootServices;

    logger::init(st);

    // Log preamble
    logger::log(L"Eternal version 1.0");
    logger::log(L"Copyright (C) 2022 Camden Pina");

    logger::try_or_fail(
        st->BootServices->SetWatchdogTimer(0, 0, 0, nullptr),
        L"Disable watchdog timer.");

    bootproto::modules_page *modules = nullptr;
    memory::allocator::init(modules, bs);

    video::init(bs);
   
    // Query available video modes for the device.
    // If one is found, set the video mode and clear the screen.
    video::screen *screen = video::pick_mode();

    bootproto::args *args = uefi_preboot(st);
    load_resources(args, screen, img, bs);
    memory::efi_mem_map map = uefi_exit(args, img, bs);

    args->modules = reinterpret_cast<uintptr_t>(modules);

    //paging::map_program(args, *args->kernel);
    //paging::map_program(args, *args->panic);

    while (1);

    return EFI_SUCCESS;
}

// x86_64-w64-mingw32-g++ src/boot/main.cpp -nostdlib -nodefaultlibs -fno-builtin -ffreestanding -mno-red-zone -fshort-wchar -fno-omit-frame-pointer -Wall -Wextra -fno-exceptions -fno-rtti -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -I ext_lib/ -o BOOTX64.EFI
// x86_64-w64-mingw32-gcc src/boot/maisrc/boot/main.cn.c -nostdlsrc/boot/main.cib -nodefaultlibs -fno-builtin -ffreestanding -mno-red-zone -fshort-wchar -fno-omit-frame-pointer -Wall -Wextra -fno-exceptions -fno-rtti -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -I ext_lib/ -o BOOTX64.EFI