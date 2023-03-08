#pragma once

#include <gnu-efi/efi.h>

namespace hw {

EFI_STATUS find_acpi_table(EFI_SYSTEM_TABLE *st, OUT void *acpi_tables);

} // namespace hw
