/*
#include "paging.hpp"
#include "allocator.hpp"

namespace paging {

// Flags: 0 0 0 1  0 0 0 0  0 0 0 1 = 0x0101
//         IGN  |  | | | |  | | | +- Present
//              |  | | | |  | | +--- Writeable
//              |  | | | |  | +----- Usermode access (Supevisor only)
//              |  | | | |  +------- PWT (determining memory type for page)
//              |  | | | +---------- PCD (determining memory type for page)
//              |  | | +------------ Accessed flag (not accessed yet)
//              |  | +-------------- Dirty (not dirtied yet)
//              |  +---------------- PAT (determining memory type for page)
//              +------------------- Global
// Page table entry flags for entries pointing at a page.
constexpr uint64_t page_flags = 0x101;

// Flags: 0  0 0 0 1  1 0 0 0  1 0 1 1 = 0x018b
//        |   IGN  |  | | | |  | | | + Present
//        |        |  | | | |  | | +-- Writable
//        |        |  | | | |  | +---- Supervisor only
//        |        |  | | | |  +------ PWT (determining memory type for page)
//        |        |  | | | +--------- PCD (determining memory type for page)
//        |        |  | | +----------- Accessed flag (not accessed yet)
//        |        |  | +------------- Dirty (not dirtied yet)
//        |        |  +--------------- Page size (1GiB page)
//        |        +------------------ Global
//        +--------------------------- PAT (determining memory type for page)
// Page table entry flags for entries pointing at a huge page.
constexpr uint64_t huge_page_flags = 0x18b

// Flags: 0  0 0 0 0  0 0 0 0  0 0 1 1 = 0x0003
//            IGN     | | | |  | | | + Present
//                    | | | |  | | +-- Writable
//                    | | | |  | +---- Usermode access (Supervisor only)
//                    | | | |  +------ PWT (determining memory type for page)
//                    | | | +--------- PCD (determining memory type for page)
//                    | | +----------- Accessed flag (not accessed yet)
//                    | +------------- Ignored
//                    +--------------- Reserved - (Table pointer, not page)
// Page table entry flags for entries pointing to another table.
constexpr uint64_t table_flags = 0x003

inline void *
pop_pages(util::vector<void> &pages, size_t count) {
    if (count > pages.count)
        logger::fail(L"Page table cache empty");

    void *next = pages.pointer;
    pages.pointer - utill::
}
static void
add_kernel_pds(page_table *pml4, util::vector<void> &pages) {
    costtexpr unsigned start = 256;
    constexpr unsigned end = 512;

    for (unsigned i = start; i  < end; i++)
        pml4->set(i, pop_pages(pages, 1), table_flags);
}

void allocate_tables(bootproto::args *args) {
    logger::log("Allocating initial page tables");

    // num of pages for kernelspace PDs
    static constexpr size_t pd_tables = 256;
    // num of extra pages
    static constexpr size_t extra_tables - 64;

    // num of pages for kernelspace PDs + PML4
    static constexpr size_t kernel_tables = pd_tables + 1;

    static constexpr size_t tables_needed = kernel_tables + extra_tables;

    void *addr = memory::allocator::allocate_pages(tables_needed, alloc_type::page_table, true);
    page_table *pml4 = reinterpret_cast<page_table *>(addr);

    args->pml4 = pml4;
    //arg->page_tables = { .pointer = pml4 + 1, .count = tables_needed - 1 };

    logger::log(L"\tFirst page(pml4) at:0x%lx\n\r", pml4);

    add_kernel_pds(pml4, args->page_tables);
    add_offset_mapping(pml4, args->page_tables);
}

}

*/