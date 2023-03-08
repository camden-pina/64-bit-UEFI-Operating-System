#include "allocator.hpp"
#include "no_construct.hpp"
#include "logger.hpp"
#include "paging.hpp"

memory::allocator *g_alloc = nullptr;

namespace memory {

void
allocator::init(bootproto::modules_page *&modules, EFI_BOOT_SERVICES *bs)
{
    new (g_alloc) allocator(*bs);
    modules = g_alloc->m_modules;
}

allocator::allocator(EFI_BOOT_SERVICES &bs)
    : m_bs(bs)
{
    add_modules();
}

void *
allocator::allocate_pages(size_t count, bool zero) {
    if (count & ~0xffffffffull) {
        logger::fail(L"Cannot allocate more than 16TiB in pages at once");
    }

    //if (~m_register || m_register->count = 0xff)
        //add_register();

    void *pages = nullptr;

    logger::try_or_fail(
        allocator::m_bs.AllocatePages(AllocateAnyPages, EfiLoaderData, count, (EFI_PHYSICAL_ADDRESS *)&pages),
        L"Allocating pages");

    //page_allocation &ent = m_register->entrues[m_register->count++];
    //ent.address = retinerpret_cast<uintptr_t>(pages);
    //ent.coutn = count;
    //ent.type = type;

    if (zero)
        allocator::m_bs.SetMem(pages, count * page_size, 0);

    return pages;
}

void *
allocator::allocate(size_t size, bool zero) {
    void *p = nullptr;

    logger::try_or_fail(
        m_bs.AllocatePool(EfiLoaderData, size, &p),
        L"Allocate memory pool");

    if (zero)
        m_bs.SetMem(p, size, 0);

    return p;
}

void
allocator::add_modules() {
    bootproto::modules_page *mods = reinterpret_cast<bootproto::modules_page *>(
        allocate_pages(1, true));

    if (m_modules)
        m_modules->next = reinterpret_cast<uintptr_t>(mods);

    mods->modules = reinterpret_cast<bootproto::module *>(mods + 1);
    m_modules = mods;
    m_next_mod = mods->modules;

    return;
}

bootproto::module *
allocator::allocate_module_untyped(size_t size) {
    size_t remaining =
        reinterpret_cast<uintptr_t>(m_modules) + page_size
        - reinterpret_cast<uintptr_t>(m_next_mod);

    if (size > remaining)
        add_modules();

    ++m_modules->count;
    bootproto::module *m = m_next_mod;
    m_next_mod = util::offset_pointer(m_next_mod, size);

    m->mod_length = size;

    return m;
}

void
allocator::free(void *p) {
    logger::try_or_fail(
        m_bs.FreePool(p),
        L"Free memory pool");
}

void
allocator::memset(void *start, size_t size, uint8_t val) {
    m_bs.SetMem(start, size, val);
}

void
allocator::copy(void *to, void *from, size_t size) {
    m_bs.CopyMem(to, from, size);
}

} // namespace memory

void *operator new (size_t size, void *p) { return p; }
void *operator new(size_t size) { return g_alloc->allocate(size); }
void *operator new [] (size_t size) { return g_alloc->allocate(size); }
void operator delete (void *p) noexcept { return g_alloc->free(p); }
void operator delete [] (void *p) noexcept { return g_alloc->free(p); }
