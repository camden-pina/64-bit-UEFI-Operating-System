#include "logger.hpp"

namespace logger {
    
EFI_STATUS _status = 0;
bool _indent = true;
bool _new_line = true;

static int vsprintf(wchar_t *str, const wchar_t *fmt, va_list ap);

static const wchar_t digits[] = { u'0', u'1', u'2', u'3', u'4', u'5', u'6', u'7', u'8', u'9', u'a', u'b', u'c', u'd', u'e', u'f' };

static const wchar_t *log_lvl_string[] = {
    L"  OK  ",
    L" WARN ",
    L" FAIL ",
    L" INFO ",
    L"ACTIVE",
};

static const wchar_t *status_codes[] = {
    L"EFI_SUCCESS",
    L"EFI_LOAD_ERROR",
    L"EFI_INVALID_PARAMETER",
    L"EFI_UNSUPPORTED",
    L"EFI_BAD_BUFFER_SIZE",
    L"EFI_BUFFER_TOO_SMALL",
    L"EFI_NOT_READY",
    L"EFI_DEVICE_ERROR",
    L"EFI_WRITE_PROTECTED",
    L"EFI_OUT_OF_RESOURCES",
    L"EFI_VOLUME_FULL",
    L"EFI_NO_MEDIA",
    L"EFI_MEDIA_CHANGED",
    L"EFI_NOT_FOUND",
    L"EFI_ACCESS_DENIED",
    L"EFI_NO_RESPONSE",
    L"EFI_NO_MAPPING",
    L"EFI_TIMEOUT",
    L"EFI_NOT_STARTED",
    L"EFI_ALREADY_STARTED",
    L"EFI_ABORTED",
    L"EFI_ICMP_ERROR",
    L"EFI_TFTP_ERROR",
    L"EFI_PROTOCOL_ERROR",
    L"EFI_INCOMPATIBLE_VERSION",
    L"EFI_SECURITY_VIOLATION",
    L"EFI_CRC_ERROR",
    L"EFI_END_OF_MEDIA",
    L"EFI_END_OF_FILE",
    L"EFI_INVALID_LANGUAGE",
    L"EFI_COMPROMISED_DATA",

    L"EFI_WARN_UNKOWN_GLYPH",
    L"EFI_WARN_UNKNOWN_GLYPH",
    L"EFI_WARN_DELETE_FAILURE",
    L"EFI_WARN_WRITE_FAILURE",
    L"EFI_WARN_BUFFER_TOO_SMALL",
};

static const int8_t log_lvl_color[] = {
    EFI_GREEN,
    EFI_YELLOW,
    EFI_RED,
    EFI_CYAN,
    EFI_MAGENTA,
};

void init(EFI_SYSTEM_TABLE *st) { m_st = st; }

void log(const wchar_t *fmt, va_list ap) {
    wchar_t _buffer[256];
    for (int i = 0; i < 255; i++) _buffer[i] = 0;
    size_t result = vsprintf(_buffer, fmt, ap);
    auto m_out = m_st->ConOut;
    m_out->OutputString(m_out, (CHAR16 *)_buffer);

    if (_new_line)
        logger::new_line();

    // return result;
}

size_t
log(enum type type, const wchar_t *fmt, va_list ap) {
    log_lvl(type);

    wchar_t _buffer[256];
    for (int i = 0; i < 255; i++) _buffer[i] = 0;
    size_t result = vsprintf(_buffer, fmt, ap);
    auto m_out = m_st->ConOut;
    m_out->OutputString(m_out, (CHAR16 *)_buffer);
    
    if (_new_line)
        logger::new_line();

    return result;
}

void log(const wchar_t *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    wchar_t _buffer[256];
    for (int i = 0; i < 255; i++) _buffer[i] = 0;
    size_t result = vsprintf(_buffer, fmt, args);
    auto m_out = m_st->ConOut;
    m_out->OutputString(m_out, (CHAR16 *)_buffer);

    if (_new_line)
        logger::new_line();

    va_end(args);

    // return result;
}

size_t
log(enum type type, const wchar_t *fmt, ...) {
    log_lvl(type);

    va_list args;
    va_start(args, fmt);

    wchar_t _buffer[256];
    for (int i = 0; i < 255; i++) _buffer[i] = 0;
    size_t result = vsprintf(_buffer, fmt, args);
    auto m_out = m_st->ConOut;
    m_out->OutputString(m_out, (CHAR16 *)_buffer);
    
    if (_new_line)
        logger::new_line();

    va_end(args);

    return result;
}

void
fail(const wchar_t *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    wchar_t _buffer[256];
    for (int i = 0; i < 255; i++) _buffer[i] = 0;
    size_t result = vsprintf(_buffer, fmt, args);
    auto m_out = m_st->ConOut;
    m_out->OutputString(m_out, (CHAR16 *)_buffer);

    va_end(args);

    while (1);
}

size_t
wstrlen(const wchar_t *s) {
    size_t count = 0;
    while (s && *s++) count++;
    return count;
}

// get_num_len(size_t):
// @n: integer to get the length of.
//
// Returns of length of @n
static int get_num_len(size_t n) {
    int l = 0;
    while (n /= 10) l++;

    return ++l;
}

static void llu_to_a(uint64_t n, wchar_t *buf, int &ptr) {
    int l = get_num_len(n);

    // Points to the location where the final 'digit' of @n should be inserted into the
    // @buf.
    wchar_t *p = buf + ptr + l - 1;

    // We insert the 'digits' in reverse as we decrement @buf.
    do {
        *p-- = digits[n % 10];
        n /= 10;
    } while (n != 0);

    // count the new offset with the inserted the digits.
    ptr += l;
}

static void lu_to_a(uint32_t n, wchar_t *buf, int &ptr) {
    int l = get_num_len(n);

    // Points to the location where the final 'digit' of @n should be inserted into the
    // @buf.
    wchar_t *p = buf + ptr + l - 1;

    // We insert the 'digits' in reverse as we decrement @buf.
    do {
        *p-- = digits[n % 10];
        n /= 10;
    } while (n != 0);

    // count the new offset with the inserted the digits.
    ptr += l;
}

static void lu_to_hex(uint32_t n, wchar_t *buf, int &ptr, int padding) {
    wchar_t *p = buf + ptr;

    bool begin = false;

    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (n >> (i * 4)) & 0xf;
        
        *p = digits[nibble];
        ptr++;
        if (*p != u'0')
            begin = true;
        
        if (*p == u'0' && begin == false && i >= padding) {
            p--;
            ptr--;
        }
        p++;
    }
}

static void llu_to_hex(uint64_t n, wchar_t *buf, int &ptr, int padding) {
    wchar_t *p = buf + ptr;

    bool begin = false;

    for (int i = 15; i >= 0; i--) {
        uint8_t nibble = (n >> (i * 4)) & 0xf;
        
        *p = digits[nibble];
        ptr++;
        if (*p != u'0')
            begin = true;
        
        if (*p == u'0' && begin == false && i >= padding) {
            p--;
            ptr--;
        }
        p++;
    }
}

/*
static void lu_to_hex(uint32_t n, wchar_t *buf, int &ptr, int padding) {
    wchar_t *p = buf + ptr;

    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (n >> (i * 4)) & 0xf;
        if (i <= padding)
        *p++ = digits[nibble];
    }
    ptr += 8;
}

static void llu_to_hex(uint64_t n, wchar_t *buf, int &ptr) {
    wchar_t *p = buf + ptr;

    for (int i = 15; i >= 0; i--) {
        uint8_t nibble = (n >> (i * 4)) & 0xf;
        *p++ = digits[nibble];
    }
    ptr += 16;
}
*/

int vsprintf(wchar_t *str, const wchar_t *fmt, va_list ap) {
    int i = 0; // index into @fmt.
    wchar_t *s;
    int ptr = 0; // index into @buf.
    int padding = 0;

    int tab_size = 2;

    while (fmt[i]) {
        if (fmt[i] == '\t') {
            for (int j = 0; j < tab_size; j++)
                str[ptr++] = ' ';
            i++;
        }

        if (fmt[i] != '%') {
            str[ptr++] = fmt[i];
            i++;
            continue;
        }

loop_hex:
        switch (fmt[++i]) {
            case '0':
                i++;
                if (fmt[i] == 'l')
                    padding = 16;
                else
                    padding = fmt[i] - '0';
                goto loop_hex;
                break;
            case 's':
                ptr += vsprintf(&str[ptr], va_arg(ap, wchar_t *), NULL);

                break;
            case 'c':
                str[ptr++] = (wchar_t)va_arg(ap, int);
                break;
            case 'i':
            case 'u':
                lu_to_a(va_arg(ap, uint32_t), str, ptr);
                break;
            case 'x':
                wchar_t int_buf[96];
                wchar_t pad_buf[96];

                lu_to_hex(va_arg(ap, uint32_t), str, ptr, padding);

                // for (int i = 0; i < padding; i++)

                break;
            case 'l':
                switch (fmt[++i]) {
                    case 'i':
                    case 'u':
                        llu_to_a(va_arg(ap, uint64_t), str, ptr);
                        break;
                    case 'x':
                        llu_to_hex(va_arg(ap, uint64_t), str, ptr, padding);
                    default:
                        break;
                }
            default:
                break;
        }
        i++;
    }
    str[ptr] = '\0';
    return ptr;
}

void log_lvl(enum type type) {
    auto out = m_st->ConOut;

    out->OutputString(out, (CHAR16 *)L"\r");
    if (_indent) {
        out->OutputString(out, (CHAR16 *)L"  ");
    }

    int8_t type_int = static_cast<int8_t>(type);

    UINTN color = log_lvl_color[type_int];

    out->OutputString(out, (CHAR16 *)L"[");
    out->SetAttribute(out, color);
    out->OutputString(out, (CHAR16 *)log_lvl_string[type_int]);
    out->SetAttribute(out, EFI_WHITE);
    out->OutputString(out, (CHAR16 *)L"] ");
}

void clear_screen() {
    auto out = m_st->ConOut;
    out->ClearScreen(out);
}

void set_cursor_pos(UINTN x, UINTN y) {
    auto out = m_st->ConOut;
    out->SetCursorPosition(out, x, y);
}

void new_line() {
    auto m_out = m_st->ConOut;
    m_out->OutputString(m_out, (CHAR16 *)L"\n\r");
}

void try_or_fail(EFI_STATUS s, const wchar_t *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    const wchar_t *p;
    if (fmt[0] == '\t') {
        _indent = true;
        p = &fmt[1];
    } else {
        _indent = false;
        p = fmt;
    }

    _new_line = false;

    logger::log(logger::type::active, p, ap);
    logger::_status = (s);

    if (s > 0 && s <= 4)
        logger::log(L": %s", status_codes[33 + s]);
    else if (s != 0)
        logger::log(L": %s", status_codes[s^0x8000000000000000]);

    if (EFI_ERROR(logger::_status))
        logger::log_lvl(logger::type::fail);
    else
        logger::log_lvl(logger::type::success);

    _new_line = true;
    logger::new_line();
    va_end(ap);
}

void try_or_warn(EFI_STATUS s, const wchar_t *fmt) {
        const wchar_t *p;
        if (fmt[0] == '\t') {
            _indent = true;
            p = &fmt[1];
        } else {
            _indent = false;
            p = fmt;
        }

        _new_line = false;

        logger::log(logger::type::active, p);
        logger::_status = (s);

        if (s > 0 && s <= 4)
            logger::log(L": %s", status_codes[33 + s]);
        else if (s != 0)
            logger::log(L": %s", status_codes[s^0x8000000000000000]);

        if (EFI_ERROR(logger::_status))
            logger::log_lvl(logger::type::warn);
        else
            logger::log_lvl(logger::type::success);

        _new_line = true;
        logger::new_line();
}

} // namespace logger
