#pragma once

#include "fs.hpp"
#include "bootconfig.hpp"
#include <bootproto/kernel.h>
#include <util/count.hpp>

namespace loader {

// Load a file from disk into memory.
// @disk: Opened UEFII filesystem to load from.
// @desc: Program descriptor identifying the file.
util::buffer
load_file(fs::file &disk, const descriptor &desc);

// Parse and load an ELF file in memory into a loaded image.
// @disl: Opened UEFI filesystem to load from.
// @desc: Progam descriptor identifying the program.
// @add_module: Also create a module for this loaded program.
bootproto::program *
load_program(fs::file &disk, const descriptor &desc, bool add_module = false);

// Load a file from disk into memory, creating an init args module.
// @disk: Opened UEFI filesystem to load from.
// #desc: Program descriptor identifting the file.
void
load_module(fs::file &disk, const descriptor &desc);

void
verify_kernel_header(bootproto::program &program);

} // namespace loader
