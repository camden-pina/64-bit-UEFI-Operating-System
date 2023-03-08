#include "hardware.hpp"
#include "logger.hpp"

namespace hw {

static bool guids_match(EFI_GUID guid1, EFI_GUID guid2) {
    bool good_chunk = (guid1.Data1 == guid2.Data1 && guid1.Data2 == guid2.Data2 &&
        guid1.Data3 == guid2.Data3);

    if (!good_chunk)
        return false;
    for (int i = 0; i < 8; i++) {
        if (guid1.Data4[i] != guid2.Data4[i]) return 0;
    }

    return true;
}

EFI_STATUS
find_acpi_table(EFI_SYSTEM_TABLE *st, OUT void *acpi_tables) {
    uintptr_t acpi1_table = 0;

    for (size_t i = 0;  i < st->NumberOfTableEntries; i++) {
        EFI_CONFIGURATION_TABLE *table = &st->ConfigurationTable[i];

        if (guids_match(table->VendorGuid, ACPI_20_TABLE_GUID)) {
            acpi_tables = table->VendorTable;
            return EFI_SUCCESS;
        }
    }

    acpi_tables = nullptr;

    return EFI_NOT_FOUND;
}

} // namespace hw
