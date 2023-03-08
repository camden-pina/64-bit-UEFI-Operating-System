#include "loader.hpp"
#include <bootproto/kernel.h>
#include <bootproto/init.h>
#include "logger.hpp"
#include "fs.hpp"
#include "memory_map.hpp"
#include "allocator.hpp"
#include <elf/file.h>
#include <elf/headers.hpp>

namespace loader {

util::buffer
load_file(fs::file &disk, const descriptor &desc) {
    logger::log(L"Loading file '%s'", desc.path);

    fs::file file = disk.open(desc.path);
    util::buffer b = file.load();

    // logger::log(L"\tLoaded at: 0x%lx, %l bytes", b.data, b.size);

    return b;
}

static void
create_module(util::buffer data, const descriptor &desc, bool loaded) {
    size_t path_len = logger::wstrlen(desc.path);
    bootproto::module_program *mod = g_alloc->allocate_module<bootproto::module_program>(path_len);
    mod->mod_type = bootproto::module_type::program;
    mod->base_address = reinterpret_cast<uintptr_t>(data.pointer);
    mod->size = data.count;

    if (loaded)
        mod->mod_flags = static_cast<bootproto::module_flags>(
            static_cast<uint8_t>(mod->mod_flags) |
            static_cast<uint8_t>(bootproto::module_flags::no_load));

    // TODO: support non-ascii path characters and do real utf-16 to utf-8 conversion
    for (size_t i = 0; i < path_len; i++) {
        char c = desc.path[i];
        mod->filename[i] = c == '\\' ? '/' : c;
    }

    mod->filename[path_len] = 0;
}

bootproto::program *
load_program(fs::file &disk, const descriptor &desc, bool add_module) {
    logger::log(L"Loading program '%s'", desc.desc);

    util::buffer data = load_file(disk, desc);

    if (add_module)
        create_module(data, desc, true);

    elf::file program(data.pointer, data.count);
    if (!program.valid()) {
        auto *header = program.header();
        logger::log(L"    program size: %li", data.count);
        logger::log(L"       word size: %li", header->word_size);
        logger::log(L"      endianness: %li", header->endianness);
        logger::log(L"ELF ident version %li", header->ident_version);
        logger::log(L"          OS ABI: %li", header->os_abi);
        logger::log(L"       file type: %li", header->file_type);
        logger::log(L"    machine type: %li",header->machine_type);
        logger::log(L"     ELF version: %li", header->version);

        logger::fail(L"ELF file not valid");
    }

    size_t num_sections = 0;
    for (auto &seg : program.programs()) {
        if (seg.type == elf::segment_type::load)
            ++num_sections;
    }

    bootproto::program_section *sections = new bootproto::program_section[num_sections];

    size_t next_section = 0;
    for (auto &seg : program.programs()) {
        if (seg.type != elf::segment_type::load)
            continue;

        bootproto::program_section &section = sections[next_section++];

        size_t page_count = memory::bytes_to_pages(seg.mem_size);

        if (seg.mem_size > seg.file_size) {
            void *pages = g_alloc->allocate_pages(page_count, true);
            void *source = util::offset_pointer(data.pointer, seg.offset);

            g_alloc->copy(pages, source, seg.file_size);
            section.phys_addr = reinterpret_cast<uintptr_t>(pages);
        } else {
            section.phys_addr = program.base() + seg.offset;
        }

        section.virt_addr = seg.vaddr;
        section.size = seg.mem_size;
        section.type = static_cast<bootproto::section_flags>(seg.flags);
    }

    bootproto::program *prog = new bootproto::program;
    prog->sections = { .pointer = sections, .count = num_sections };
    prog->phys_base = program.base();
    prog->entrypoint = program.entrypoint();
    return prog;
}

void
load_module(fs::file &disk, const descriptor &desc) {
    logger::log(L"Loading module", desc.desc);

    util::buffer data = load_file(disk, desc);
    create_module(data, desc, false);
}

void
verify_kernel_header(bootproto::program &program) {
    logger::log(L"Verifying kernel header");

    const bootproto::header *header =
        reinterpret_cast<const bootproto::header *>(program.sections[0].phys_addr);

    if (header->magic != bootproto::header_magic)
        logger::fail(L"Bad kernel magic number");
    if (header->length < sizeof(bootproto::header))
        logger::fail(L"Bad kernel header length");
    if (header->version < bootproto::min_header_version)
        logger::fail(L"Kernel header version not supported");

    logger::log(L"\tLoaded kernel version: %li.%li.%li",
        header->version_major, header->version_minor, header->version_patch,
        header->version_gitsha);
}

} // namespace loader
