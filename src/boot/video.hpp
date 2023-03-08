#pragma once

#include <bootproto/init.h>
#include <util/count.hpp>
#include <gnu-efi/efi.h>

namespace video {

namespace {
    EFI_BOOT_SERVICES *m_bs;
} // namespace [private]

struct screen {
    util::buffer framebuffer;
    bootproto::video_mode mode;
};

void
init(EFI_BOOT_SERVICES *bs);

video::screen *
pick_mode();

void
make_module(screen *s);

} // namespace video
