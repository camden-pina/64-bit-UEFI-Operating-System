#include "video.hpp"
#include "logger.hpp"
#include <bootproto/init.h> 
#include "allocator.hpp"

namespace video {

void init(EFI_BOOT_SERVICES *bs) { m_bs = bs; }

EFI_GRAPHICS_OUTPUT_PROTOCOL *
get_gop() {
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = nullptr;
    EFI_GUID guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

    EFI_STATUS status = m_bs->LocateProtocol(&guid, nullptr, (void **)&gop);

    logger::try_or_fail(
        status,
        L"Locating Graphics Output Protocol.");

    return gop;
}

video::screen *
pick_mode() {
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = get_gop();

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = gop->Mode->Info;

    uint32_t best = gop->Mode->Mode;
    uint32_t res = info->HorizontalResolution * info->VerticalResolution;

    int pixmode = static_cast<int>(info->PixelFormat);

    const uint32_t modes = gop->Mode->MaxMode;
    for (uint32_t  i = 0; i < modes; i++) {
        UINTN size = 0;
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *new_info = nullptr;

        logger::try_or_fail(
            gop->QueryMode(gop, i, &size, &new_info),
            L"Locating suitable graphics output mode.");

        logger::log(L"\tDetected framebuffer: %dx%d[%d] @0x%x\r\n",
        new_info->HorizontalResolution, new_info->VerticalResolution,
        new_info->PixelsPerScanLine, gop->Mode->FrameBufferBase);


        const uint32_t new_res = new_info->HorizontalResolution * new_info->VerticalResolution;
        int new_pixmode = static_cast<int>(new_info->PixelFormat);

        if (new_pixmode <= pixmode && new_res >= res) {
            best = i;
            res = new_res;
            pixmode = new_pixmode;
        }
    }

    screen *s = new screen;
    s->mode = {
        .vertical = gop->Mode->Info->VerticalResolution,
        .horizontal = gop->Mode->Info->HorizontalResolution,
        .scanline = gop->Mode->Info->PixelsPerScanLine,
        .layout = bootproto::fb_layout::unknown,
    };

    s->framebuffer = {
        .pointer = reinterpret_cast<void *>(gop->Mode->FrameBufferBase),
        .count = gop->Mode->FrameBufferSize,
    };

    const wchar_t *type = nullptr;
    switch(info->PixelFormat) {
        case PixelRedGreenBlueReserved8BitPerColor:
            s->mode.layout = bootproto::fb_layout::rgb8;
            type = L"rgb8";
            break;
        case PixelBlueGreenRedReserved8BitPerColor:
            s->mode.layout = bootproto::fb_layout::bgr8;
            type = L"bgr8";
            break;
        default:
            type = L"unknown";
    }

    logger::log(L"\tDetected framebuffer: %dx%d[%d] color-format %s @0x%x\r\n",
        info->HorizontalResolution, info->VerticalResolution,
        info->PixelsPerScanLine, type, gop->Mode->FrameBufferBase);

    logger::try_or_fail(
        gop->SetMode(gop, best),
        L"Setting graphics output mode");

    logger::clear_screen();
    logger::set_cursor_pos(0, 0);

    return s;
}

void
make_module(screen *s) {
    bootproto::module_framebuffer *modfb =
        g_alloc->allocate_module<bootproto::module_framebuffer>();

    modfb->mod_type = bootproto::module_type::framebuffer;
    modfb->type = bootproto::fb_type::uefi;

    modfb->framebuffer = s->framebuffer;
    modfb->mode = s->mode;
}

} // namespace video