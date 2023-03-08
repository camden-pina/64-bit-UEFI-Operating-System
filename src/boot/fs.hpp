#pragma once

#include <gnu-efi/efi.h>
#include <util/count.hpp>

namespace fs {

class file {
public:
    file(file &&o);
    file(file &o);
    ~file();

    // open(const wchar_t *):
    // @path: Relative path to the target file from this one.
    //
    // Opens another file or directory, relative to  this one.
    // Ret:
    file open(const wchar_t *path);

    // load();
    //
    // Loads the contents of this file into memory.
    // Ret: Buffer describing the page aligned loaded memory.
    util::buffer load();

private:
    friend file get_boot_volume(EFI_HANDLE, EFI_BOOT_SERVICES *);

    file(EFI_FILE_PROTOCOL *f);
    EFI_FILE_PROTOCOL *m_file;
    EFI_BOOT_SERVICES *m_bs;
};

// get_boot_volume(EFI_HANDLE, EFI_BOOT_SERVICES):
// @img: 
// @bs: 
// Ret: A 'file' object representing the root directory of the volume.
file get_boot_volume(EFI_HANDLE img, EFI_BOOT_SERVICES *bs);

} // namespace fs
