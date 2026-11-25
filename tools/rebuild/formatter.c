/* The rebuilt `formatter` module.
 *
 * The original sits at linear 0xd68c0 in STARTH.EXE and holds 223
 * instructions and 641 bytes.  It is the Watcom format engine that the
 * rebuilt `sprintf` at 0x4d32c and the rebuilt `__fprtf` at 0xd429f call.  The
 * original driver reads the format, calls the field parser at 0xd6b41 and the
 * conversion dispatcher at 0xd6e59, pads each field, and emits every character
 * through the sink function pointer that the caller passes in `ecx`.
 *
 * The rebuilt function keeps the same contract: the destination handle is the
 * first argument, the format the second, the argument list the third, and a
 * sink the fourth.  The sink writes one character.  The string sink writes
 * through a `char **` cursor, so `sprintf` can pass the address of its local
 * cursor; the stream sink writes through the DOS handle in the `FILE` record.
 *
 * The rebuild covers the formats that the game uses: literal text, `%%`, `%s`,
 * `%c`, `%d`, `%i`, `%u`, `%x`, `%X`, `%o`, and `%p`, with the `-`, `0`, `+`,
 * space, and `#` flags and a numeric width and precision.  The format parser
 * also accepts `*` width and precision and the `h`, `l`, `w`, `L`, `N`, and
 * `F` length modifiers.  The floating-point conversions `e`, `E`, `f`, `F`,
 * `g`, and `G` are not rebuilt: the game uses no floating-point format, so the
 * module echoes the conversion character for them, as it does for an unknown
 * conversion.
 *
 * The original parses the argument list as a pointer to the current argument
 * cell and advances it by four bytes per argument.  The rebuild does the same,
 * because the rebuilt `sprintf` passes the address of its local `va_list`.
 *
 * The module is a functional rebuild, not a byte-faithful one.  The V2 C
 * compiler cannot reproduce the original Watcom 10.0a instruction sequence, so
 * the identity check stays open and the store records `rebuilt`.
 */
typedef void (*format_sink)(void *out, char ch);

void formatter_string_sink(void *out, char ch)
{
    char **cursor = (char **)out;

    *(*cursor)++ = ch;
}

static char *fetch_pointer(void *args)
{
    char **cell = (char **)args;
    char *value = *(char **)(*cell);

    *cell = *cell + 4;
    return value;
}

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

int formatter(void *out, const char *format, void *args, format_sink sink)
{
    const char *cursor = format;
    int count = 0;

    while (*cursor != 0) {
        if (*cursor != '%') {
            sink(out, *cursor);
            cursor = cursor + 1;
            count = count + 1;
            continue;
        }
        cursor = cursor + 1;
        {
            int left = 0;
            int zero_fill = 0;
            int plus = 0;
            int space = 0;
            int alternate = 0;
            int width = 0;
            int precision = -1;
            int short_flag = 0;
            int long_flag = 0;
            char conversion;
            char body[40];
            char prefix[2];
            int body_length = 0;
            int prefix_length = 0;
            int zero_pad = 0;
            int width_pad;
            const char *text = body;
            int index;

            for (;;) {
                if (*cursor == '-') {
                    left = 1;
                    cursor = cursor + 1;
                } else if (*cursor == '0') {
                    zero_fill = 1;
                    cursor = cursor + 1;
                } else if (*cursor == '+') {
                    plus = 1;
                    space = 0;
                    cursor = cursor + 1;
                } else if (*cursor == ' ') {
                    if (!plus) {
                        space = 1;
                    }
                    cursor = cursor + 1;
                } else if (*cursor == '#') {
                    alternate = 1;
                    cursor = cursor + 1;
                } else {
                    break;
                }
            }
            if (*cursor == '*') {
                width = fetch_int(args);
                if (width < 0) {
                    left = 1;
                    width = -width;
                }
                cursor = cursor + 1;
            } else {
                while (*cursor >= '0' && *cursor <= '9') {
                    width = width * 10 + (*cursor - '0');
                    cursor = cursor + 1;
                }
            }
            if (*cursor == '.') {
                cursor = cursor + 1;
                precision = 0;
                if (*cursor == '*') {
                    precision = fetch_int(args);
                    if (precision < 0) {
                        precision = -1;
                    }
                    cursor = cursor + 1;
                } else {
                    while (*cursor >= '0' && *cursor <= '9') {
                        precision = precision * 10 + (*cursor - '0');
                        cursor = cursor + 1;
                    }
                }
            }
            if (*cursor == 'h') {
                short_flag = 1;
                cursor = cursor + 1;
            } else if (*cursor == 'l' || *cursor == 'w') {
                long_flag = 1;
                cursor = cursor + 1;
            } else if (*cursor == 'L' || *cursor == 'N' || *cursor == 'F') {
                cursor = cursor + 1;
            }
            conversion = *cursor;
            if (conversion != 0) {
                cursor = cursor + 1;
            }

            prefix[0] = 0;
            prefix[1] = 0;

            if (conversion == 's' || conversion == 'S') {
                char *value = fetch_pointer(args);
                if (value == 0) {
                    value = "";
                }
                body_length = bounded_length(value, precision);
                text = value;
            } else if (conversion == 'c') {
                body[0] = (char)fetch_int(args);
                body_length = 1;
                text = body;
            } else if (conversion == 'd' || conversion == 'i') {
                int value = fetch_int(args);
                unsigned int magnitude;

                if (short_flag) {
                    value = (short)value;
                }
                if (value < 0) {
                    prefix[0] = '-';
                    prefix_length = 1;
                    magnitude = (unsigned int)(-(long)value);
                } else {
                    if (plus) {
                        prefix[0] = '+';
                        prefix_length = 1;
                    } else if (space) {
                        prefix[0] = ' ';
                        prefix_length = 1;
                    }
                    magnitude = (unsigned int)value;
                }
                body_length = digits(magnitude, 10, 0, body);
                text = body;
            } else if (conversion == 'u' || conversion == 'x'
                       || conversion == 'X' || conversion == 'o') {
                unsigned int value = fetch_uint(args);
                int base = 10;
                int upper = 0;

                if (short_flag) {
                    value = (unsigned short)value;
                }
                if (conversion == 'x' || conversion == 'X') {
                    base = 16;
                    upper = conversion == 'X';
                } else if (conversion == 'o') {
                    base = 8;
                }
                body_length = digits(value, base, upper, body);
                text = body;
                if (alternate) {
                    if (base == 16 && value != 0) {
                        prefix[0] = '0';
                        prefix[1] = upper ? 'X' : 'x';
                        prefix_length = 2;
                    } else if (base == 8 && body[0] != '0') {
                        zero_pad = 1;
                    }
                }
            } else if (conversion == 'p' || conversion == 'P') {
                unsigned int value = fetch_uint(args);

                if (width == 0) {
                    width = 8;
                }
                body_length = digits(value, 16, conversion == 'P', body);
                text = body;
                if (body_length < 8) {
                    zero_pad = 8 - body_length;
                }
            } else {
                body[0] = conversion;
                body_length = 1;
                text = body;
            }
            (void)long_flag;

            if ((conversion == 'd' || conversion == 'i'
                 || conversion == 'u' || conversion == 'x'
                 || conversion == 'X' || conversion == 'o')
                && precision >= 0 && precision > body_length) {
                zero_pad = precision - body_length;
            }
            width_pad = width - (prefix_length + zero_pad + body_length);
            if (width_pad < 0) {
                width_pad = 0;
            }
            if (!left && !zero_fill) {
                for (index = 0; index < width_pad; index = index + 1) {
                    sink(out, ' ');
                    count = count + 1;
                }
            }
            for (index = 0; index < prefix_length; index = index + 1) {
                sink(out, prefix[index]);
                count = count + 1;
            }
            if (!left && zero_fill) {
                for (index = 0; index < width_pad; index = index + 1) {
                    sink(out, '0');
                    count = count + 1;
                }
            }
            for (index = 0; index < zero_pad; index = index + 1) {
                sink(out, '0');
                count = count + 1;
            }
            for (index = 0; index < body_length; index = index + 1) {
                sink(out, text[index]);
                count = count + 1;
            }
            if (left) {
                for (index = 0; index < width_pad; index = index + 1) {
                    sink(out, ' ');
                    count = count + 1;
                }
            }
        }
    }
    return count;
}
