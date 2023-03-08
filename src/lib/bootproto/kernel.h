#pragma once

//#include <bootproto/init.h>
#include <util/count.hpp>
#include <util/enum_bitfields.hpp>

namespace bootproto {

constexpr uint64_t header_magic = 0x4c454e52454b366aull; // 'j6KERNEL'
constexpr uint16_t header_version = 2;
constexpr uint16_t min_header_version = 2;

enum class section_flags : uint32_t {
    none    = 0,
    execute = 1,
    write   = 2,
    read    = 4,
};
is_bitfield(section_flags);

struct program_section {
    uintptr_t phys_addr;
    uintptr_t virt_addr;
    uint32_t size;
    section_flags type;
};

struct program {
    uintptr_t entrypoint;
    uintptr_t phys_base;
    util::counted<program_section> sections;
};

enum class mem_type : uint32_t {
    free,
    pending,
    acpi,
    uefi_runtime,
    mmio,
    persistent,
};

struct mem_entry {
    uintptr_t start;
    size_t pages;
    mem_type type;
    uint32_t attr;
};

enum class frame_flags : uint32_t {
    uncacheable      = 0x00000001,
    write_combining  = 0x00000002,
    write_through    = 0x00000004,
    write_back       = 0x00000008,
    uncache_exported = 0x00000010,

    write_protect    = 0x00001000,
    read_protect     = 0x00002000,
    exec_protect     = 0x00004000,
    non_volatile     = 0x00008000,

    read_only        = 0x00030000,
    earmarked        = 0x00040000,
    hw_crypto        = 0x00080000,
};

constexpr size_t frames_per_block = 64 * 64 * 64;

struct frame_block {
    uintptr_t base;
    uint32_t count;
    frame_flags flags;
    uint64_t  map1;
    uint64_t map2[64];
    uint64_t *bitmap;
};

enum class boot_flags : uint32_t {
    none  = 0x0000,
    debug = 0x0001,
    test  = 0x0002,
};

struct args {
    boot_flags flags;

    void *pml4;
    util::counted<void> page_tables;
    util::counted<mem_entry> mem_map;
    util::counted<frame_block> frame_blocks;

    program *kernel;
    program *init;
    program *panic;

    uintptr_t modules;
    void const *symbol_table;

    //struct screen screen;

    void *runtime_services;
    void *acpi_tables;

    // struct module_framebuffer screen;
};

struct header {
    uint64_t magic;

    uint16_t length;
    uint16_t version;

    uint16_t version_major;
    uint16_t version_minor;
    uint16_t version_patch;
    uint16_t reserved;

    uint32_t version_gitsha;

    uint64_t flags;
};

} // namespace bootproto
