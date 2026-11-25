/* The parser and conversion core of the rebuilt `formatter` module.
 *
 * The original driver sits at 0xd68c0 and is transliterated in
 * `formatter.asm`.  It calls the field parser at 0xd6b41, then the conversion
 * dispatcher at 0xd6e59, pads the field, and emits every character through
 * the caller sink.  This file rebuilds the parser and the conversion core
 * behind the register interfaces that the driver uses:
 *
 *   format_parse     eax = format cursor, edx = argument cell, ebx = spec
 *   format_convert   eax = scratch, ebx = spec, ecx = empty, edx = arguments
 *
 * The driver's `spec` record holds the fields at the same offsets:
 *
 *   +0x00 destination handle      +0x04 width, then leading fill
 *   +0x08 precision, then body    +0x0c zero pad
 *   +0x10 output count            +0x14 flags
 *   +0x15 conversion              +0x16 fill
 *   +0x17 prefix                  +0x20 empty string
 *
 * The parser and the dispatcher read the argument list as a pointer to the
 * current argument cell and advance it by four bytes per argument.  The
 * conversions are the game's subset: `s`, `c`, `d`, `i`, `u`, `x`, `X`, `o`,
 * `p`, `%`, and the literal default.  The floating-point conversions are not
 * rebuilt; the game uses none.
 */
struct format_spec {
    void *out;
    int leading;
    int body;
    int zero_pad;
    int count;
    unsigned char flags;
    unsigned char conv;
    unsigned char fill;
    char prefix[3];
    char reserved[6];
    char empty[1];
};

char *format_parse(char *cursor, void *args, struct format_spec *spec);
char *format_convert_c(char *scratch, struct format_spec *spec, char *empty,
    void *args);

static int fetch_int(void *args)
{
    char **cell = (char **)args;
    int value = *(int *)(*cell);

    *cell = *cell + 4;
    return value;
}

static unsigned int fetch_uint(void *args)
{
    char **cell = (char **)args;
    unsigned int value = *(unsigned int *)(*cell);

    *cell = *cell + 4;
    return value;
}

static char *fetch_pointer(void *args)
{
    char **cell = (char **)args;
    char *value = *(char **)(*cell);

    *cell = *cell + 4;
    return value;
}

static int bounded_length(const char *text, int limit)
{
    int length = 0;

    while (text[length] != 0 && (limit < 0 || length < limit)) {
        length = length + 1;
    }
    return length;
}

static int digits(unsigned int value, int base, int upper, char *buffer)
{
    const char *table = upper
        ? "0123456789ABCDEF"
        : "0123456789abcdef";
    char reverse[33];
    int length = 0;
    int index;

    do {
        reverse[length] = table[value % (unsigned int)base];
        length = length + 1;
        value = value / (unsigned int)base;
    } while (value != 0);
    for (index = 0; index < length; index = index + 1) {
        buffer[index] = reverse[length - 1 - index];
    }
    return length;
}

char *format_parse(char *cursor, void *args, struct format_spec *spec)
{
    spec->prefix[0] = 0;
    spec->fill = ' ';
    spec->flags = 0;
    spec->leading = 0;
    spec->body = -1;

    for (;;) {
        if (*cursor == '-') {
            spec->flags = spec->flags | 0x08;
            cursor = cursor + 1;
        } else if (*cursor == '#') {
            spec->flags = spec->flags | 0x01;
            cursor = cursor + 1;
        } else if (*cursor == '+') {
            spec->flags = (spec->flags | 0x04) & 0xfd;
            cursor = cursor + 1;
        } else if (*cursor == ' ') {
            if ((spec->flags & 0x04) == 0) {
                spec->flags = spec->flags | 0x02;
            }
            cursor = cursor + 1;
        } else if (*cursor == '0') {
            spec->fill = '0';
            cursor = cursor + 1;
        } else {
            break;
        }
    }
    if (*cursor == '*') {
        int width = fetch_int(args);

        if (width < 0) {
            spec->flags = spec->flags | 0x08;
            width = -width;
        }
        spec->leading = width;
        cursor = cursor + 1;
    } else {
        while (*cursor >= '0' && *cursor <= '9') {
            spec->leading = spec->leading * 10 + (*cursor - '0');
            cursor = cursor + 1;
        }
    }
    if (*cursor == '.') {
        cursor = cursor + 1;
        spec->body = 0;
        if (*cursor == '*') {
            int precision = fetch_int(args);

            if (precision < 0) {
                precision = -1;
            }
            spec->body = precision;
            cursor = cursor + 1;
        } else {
            while (*cursor >= '0' && *cursor <= '9') {
                spec->body = spec->body * 10 + (*cursor - '0');
                cursor = cursor + 1;
            }
        }
    }
    if (*cursor == 'h') {
        spec->flags = spec->flags | 0x10;
        cursor = cursor + 1;
    } else if (*cursor == 'l' || *cursor == 'w') {
        spec->flags = spec->flags | 0x20;
        cursor = cursor + 1;
    } else if (*cursor == 'N' || *cursor == 'L') {
        spec->flags = spec->flags | 0x40;
        cursor = cursor + 1;
    } else if (*cursor == 'F') {
        spec->flags = spec->flags | 0x80;
        cursor = cursor + 1;
    }
    return cursor;
}

static void set_prefix(struct format_spec *spec, const char *prefix, int length)
{
    spec->prefix[0] = prefix[0];
    spec->prefix[1] = length > 1 ? prefix[1] : 0;
    spec->prefix[2] = 0;
}

char *format_convert_c(char *scratch, struct format_spec *spec, char *empty,
    void *args)
{
    int precision = spec->body;
    int width = spec->leading;
    unsigned char flags = spec->flags;
    unsigned char conv = spec->conv;
    char prefix[3];
    int prefix_length = 0;
    int body = 0;
    int zero_pad = 0;
    int leading;
    char *text = scratch;

    prefix[0] = 0;
    prefix[1] = 0;

    if (conv == 's' || conv == 'S') {
        char *value = fetch_pointer(args);

        if (value == 0) {
            value = empty;
        }
        body = bounded_length(value, precision);
        text = value;
    } else if (conv == 'c') {
        scratch[0] = (char)fetch_int(args);
        body = 1;
        text = scratch;
    } else if (conv == 'd' || conv == 'i') {
        int value = fetch_int(args);
        unsigned int magnitude;

        if ((flags & 0x10) != 0) {
            value = (short)value;
        }
        if (value < 0) {
            prefix[0] = '-';
            prefix_length = 1;
            magnitude = (unsigned int)(-(long)value);
        } else {
            if ((flags & 0x04) != 0) {
                prefix[0] = '+';
                prefix_length = 1;
            } else if ((flags & 0x02) != 0) {
                prefix[0] = ' ';
                prefix_length = 1;
            }
            magnitude = (unsigned int)value;
        }
        body = digits(magnitude, 10, 0, scratch);
        text = scratch;
        if (precision >= 0) {
            spec->fill = ' ';
            if (precision > body) {
                zero_pad = precision - body;
            }
        }
    } else if (conv == 'u' || conv == 'x' || conv == 'X' || conv == 'o') {
        unsigned int value = fetch_uint(args);
        int base = 10;
        int upper = 0;

        if ((flags & 0x10) != 0) {
            value = (unsigned short)value;
        }
        if (conv == 'x' || conv == 'X') {
            base = 16;
            upper = conv == 'X';
        } else if (conv == 'o') {
            base = 8;
        }
        if (conv == 'o' && (flags & 0x01) != 0) {
            scratch[0] = '0';
            body = 1 + digits(value, 8, 0, scratch + 1);
        } else {
            body = digits(value, base, upper, scratch);
        }
        text = scratch;
        if (base == 16 && (flags & 0x01) != 0 && value != 0) {
            prefix[0] = '0';
            prefix[1] = upper ? 'X' : 'x';
            prefix_length = 2;
        }
        if (precision >= 0) {
            spec->fill = ' ';
            if (precision > body) {
                zero_pad = precision - body;
            }
        }
    } else if (conv == 'p' || conv == 'P') {
        unsigned int value = fetch_uint(args);
        int index;
        int upper = conv == 'P';

        if (width == 0) {
            width = 8;
        }
        for (index = 0; index < 8; index = index + 1) {
            unsigned int nibble = (value >> ((7 - index) * 4)) & 0xf;
            scratch[index] = upper
                ? "0123456789ABCDEF"[nibble]
                : "0123456789abcdef"[nibble];
        }
        body = 8;
        text = scratch;
    } else {
        scratch[0] = conv;
        body = 1;
        text = scratch;
        width = 0;
    }

    set_prefix(spec, prefix, prefix_length);
    spec->body = body;
    spec->zero_pad = zero_pad;
    leading = width - (prefix_length + zero_pad + body);
    if (leading < 0) {
        leading = 0;
    }
    spec->leading = leading;
    return text;
}

void formatter_string_sink(void *out, char ch)
{
    struct format_spec *spec = (struct format_spec *)out;
    char *cursor = (char *)spec->out;

    *cursor = ch;
    spec->out = cursor + 1;
    spec->count = spec->count + 1;
}
