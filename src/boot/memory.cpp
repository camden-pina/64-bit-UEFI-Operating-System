#include "memory.hpp"
#include "logger.hpp"

namespace memory {

size_t fixup_pointer_index = 0;
void **fixup_pointers[64];

static void
update_marked_addresses(EFI_EVENT, void *ctx) {
    EFI_RUNTIME_SERVICES *rs = reinterpret_cast<EFI_RUNTIME_SERVICES *>(ctx);

    for (size_t i = 0; i < fixup_pointer_index; i++) {
        if (fixup_pointers[i])
            rs->ConvertPointer(0, fixup_pointers[i]);
    }
}

void init_pointer_fixup(EFI_BOOT_SERVICES *bs, EFI_RUNTIME_SERVICES *rs) {
    EFI_EVENT event;
    bs->SetMem(&fixup_pointers, sizeof(fixup_pointers), 0);
    fixup_pointer_index = 0;

    logger::try_or_fail(
        bs->CreateEvent(
            EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
            TPL_CALLBACK,
            static_cast<EFI_EVENT_NOTIFY>(&update_marked_addresses),
            rs,
            &event),
        L"Creating memory virtualization event"
    );
}

void
mark_pointer_fixup(void **p) {
    fixup_pointer_index[fixup_pointer_index++] = p;
}

} // namespace
