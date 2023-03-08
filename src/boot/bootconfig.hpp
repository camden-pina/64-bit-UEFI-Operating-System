#pragma once

#include <bootproto/bootconfig.hpp>
#include <util/count.hpp>

#include <gnu-efi/efi.h>

struct descriptor {
    bootproto::desc_flags flags;
    wchar_t const *path;
    wchar_t const *desc;
};

class bootconfig {
public:

    using descriptors = util::counted<descriptor>;

    bootconfig(util::buffer data, EFI_BOOT_SERVICES *bs);

    inline uint16_t flags() const { return m_flags; }
    inline const descriptor &kernel() const { return m_kernel; }
    inline const descriptor &init() const { return m_init; }
    descriptors programs() { return m_programs; }
    descriptors data() { return m_data; }

private:
    uint16_t m_flags;
    descriptor m_kernel;
    descriptor m_init;
    descriptors m_programs;
    descriptors m_data;
}; // class bootconfig
