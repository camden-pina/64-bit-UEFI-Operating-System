#include "fs.hpp"
#include "memory_map.hpp"
#include "logger.hpp"
#include "allocator.hpp"

namespace fs {

file::file(EFI_FILE_PROTOCOL *f)
    : m_file(f) { }

file::file(file &o)
    : m_file(o.m_file) {
    o.m_file = nullptr;
}

file::file(file &&o)
    : m_file(o.m_file) {
    o.m_file = nullptr;
}

file::~file() {
    if (m_file)
        m_file->Close(m_file);
}

file
file::open(const wchar_t *path) {
    EFI_FILE_PROTOCOL *fh = nullptr;

    logger::try_or_fail(
        m_file->Open(m_file, &fh, (CHAR16 *)path, EFI_FILE_MODE_READ, 0),
        L"Could not open relative path to file '%s'",
        path);

    return file(fh);
}

util::buffer
file::load() {
    uint8_t info_buf[sizeof(EFI_FILE_INFO) + 100];
    size_t size = sizeof(info_buf);
    EFI_GUID info_guid = EFI_FILE_INFO_ID;

    logger::try_or_fail(
        m_file->GetInfo(m_file, &info_guid, &size, &info_buf),
        L"Could not get file info");

    EFI_FILE_INFO *info = reinterpret_cast<EFI_FILE_INFO *>(&info_buf);

    size_t pages = memory::bytes_to_pages(info->Size);
    void *data = g_alloc->allocate_pages(pages, false);

    size = info->FileSize;
    logger::try_or_fail(
        m_file->Read(m_file, &size, data),
        L"Could not read from file");

    return { .pointer = data, .count = size };
}

file
get_boot_volume(EFI_HANDLE img, EFI_BOOT_SERVICES *bs) {
    logger::log(L"Looking up boot volume");

    EFI_GUID le_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *loaded_img = nullptr;

    logger::try_or_fail(
        bs->HandleProtocol(img, &le_guid, reinterpret_cast<void **>(&loaded_img)),
        L"Locating currently running UEFI loaded image");

    EFI_GUID sfs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*fs;
    logger::try_or_fail(
        bs->HandleProtocol(loaded_img->DeviceHandle, &sfs_guid, reinterpret_cast<void **>(&fs)),
        L"Locating filesystem protocol for boot volume");

    EFI_FILE_PROTOCOL *f;
    logger::try_or_fail(
        fs->OpenVolume(fs, &f),
        L"Opening the boot volume");

    return file(f);
}

} // namespace fs
