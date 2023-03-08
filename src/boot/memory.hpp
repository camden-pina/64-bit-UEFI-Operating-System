#pragma once

namespace uefi {
    #include <gnu-efi/efi.h>
} // namespace uefi

namespace memory {

using namespace uefi;

void
init_pointer_fixup(EFI_BOOT_SERVICES *bs, EFI_RUNTIME_SERVICES *rs);

void
mark_pointer_fixup(void **p);

} // namespace memory
