#include "bootconfig.hpp"
#include "logger.hpp"

constexpr uint32_t modernosboot = 0xDEADBEEF;

void hex_dump(uint64_t addr, uint64_t offset, uint64_t len);

static const wchar_t *
read_string(util::buffer &data) {
    uint16_t size = *util::read<uint16_t>(data);
    const wchar_t *string = reinterpret_cast<const wchar_t *>(data.pointer);
    data += size;
    return string;
}

static void
read_descriptor(descriptor &e, util::buffer &data) {
    e.flags = static_cast<bootproto::desc_flags>(*util::read<uint16_t>(data));
    e.path  = read_string(data);
    hex_dump((uint64_t)data.pointer, 0, 32);
    e.desc = read_string(data);
}

void hex_dump(uint64_t addr, uint64_t offset, uint64_t len)
{
    logger::_new_line = false;
	uint64_t i = 0;
    uint64_t j = 0;

    unsigned char *buf = (unsigned char *)addr;
	for (i; i < len; i+=16)
	{
        logger::log(L"%08x: ", i);

        for (int j = 0; j < 16; j++) {
            logger::log(L"%02x ", buf[i + j]);
            if (j == 7)
                logger::log(L" ");
        }

        // print text area
        logger::log(L"|");

        for (j = 0; j < 16; j++) {
            if (buf[i + j] >= ' ' && buf[i + j] <= '~')
                logger::log(L"%c", buf[i + j]);
            else
                logger::log(L".");
        }
        logger::log(L"|");
        logger::new_line();
    }

/*
	// Handle first occurence
		if (i == 0)
		{
			logger::log(L"%08x : ", i);
		}
		if ((i % 2) == 0 && i != 0)
		{
			logger::log(L" ");
			if ((i % 16) == 0 && i != 0)
			{
				logger::log(L"| ");
				for (int j = 0; j < 16; j++)
				{
					logger::log(L"%c", *((wchar_t *)addr + offset  + (i-16) + j));
				}
				logger::new_line();
				logger::log(L"%02x : ", (uint64_t)((i/10) * 16));
			}
		}
		logger::log(L"[%02x]", *((char *)addr + offset + i));
	}
	logger::log(L" | ");
	for (int j = 0; j < 16; j++)
	{
		logger::log(L"%c", *((wchar_t *)addr + offset  + (i-16) + j));
	}
    logger::new_line();
    */
    logger::_new_line = true;
}

bootconfig::bootconfig(util::buffer data, EFI_BOOT_SERVICES *bs) {
    logger::log(L"Loading boot config");

    hex_dump((uint64_t)data.pointer, 0, 512);
    uint32_t hdr = *util::read<uint32_t>(data);
    hex_dump((uint64_t)&hdr, 0, 24);
    logger::log(L"[%lx]", hdr);
    logger::log(L"[%x]", hdr);
    logger::log(L"[%x]", modernosboot);
    if (hdr != modernosboot)
        logger::fail(L"Bad header in jsix_boot.dat");

    const uint8_t version = *util::read<uint8_t>(data);
    if (version != 0)
        logger::fail(L"Bad version in jsix_boot.dat");

    data += 1; // reserved byte
    uint16_t num_programs = *util::read<uint16_t>(data);
    uint16_t num_data = *util::read<uint16_t>(data);

    m_flags = *util::read<uint16_t>(data);

    hex_dump((uint64_t)data.pointer, 0, 32);

    read_descriptor(m_kernel, data);
    read_descriptor(m_init, data);

    m_programs.count = num_programs;
    m_programs.pointer = new descriptor[num_programs];

    for (unsigned i = 0; i < num_programs; i++)
        read_descriptor(m_programs[i], data);

    m_data.count = num_programs;
    m_data.pointer = new descriptor[num_data];

    for (unsigned i = 0; i < num_data; i++)
        read_descriptor(m_data[i], data);
}
