/* The test harness for the rebuilt `formatter` module.
 *
 * The harness calls the formatter with a string sink and checks the output
 * bytes and the returned count.  Each case prints one marker, so a failure
 * names the case: `0` to `9`, then `a` to `o`, then `D` for the exit.
 *
 * The cases cover the formats that the game uses: `%s`, `%d`, `%02d`, `%03d`,
 * `%x`, `%X`, `%c`, `%.11s`, literal text, `%%`, width, left alignment, zero
 * fill, the `+` flag, a negative value, the `#` flag, a null string pointer,
 * unsigned decimal, octal, padded hex, left alignment with the zero flag, a
 * string precision, octal with the `#` flag, and a pointer.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#include <stdarg.h>

#pragma off (check_stack)

int formatter(void *out, const char *format, void *args,
    void (*sink)(void *, char));
void formatter_string_sink(void *out, char ch);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char buffer[64];
static char long_text[] = "abcdefghijklm";

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static int same(const char *actual, const char *expected, int count)
{
    int index = 0;

    while (expected[index] != 0) {
        if (index >= count || actual[index] != expected[index]) {
            return 0;
        }
        index = index + 1;
    }
    return index == count;
}

static int format_into(char *out, const char *fmt, ...)
{
    va_list args;
    int count;

    va_start(args, fmt);
    count = formatter(out, fmt, &args, formatter_string_sink);
    va_end(args);
    return count;
}

int main(void)
{
    int count;

    count = format_into(buffer, "%s", "abc");
    put(same(buffer, "abc", count) ? '0' : 'x');

    count = format_into(buffer, "%d", 123);
    put(same(buffer, "123", count) ? '1' : 'x');

    count = format_into(buffer, "%02d", 5);
    put(same(buffer, "05", count) ? '2' : 'x');

    count = format_into(buffer, "%03d", 42);
    put(same(buffer, "042", count) ? '3' : 'x');

    count = format_into(buffer, "%x", 0x1a2b);
    put(same(buffer, "1a2b", count) ? '4' : 'x');

    count = format_into(buffer, "%X", 0x1a2b);
    put(same(buffer, "1A2B", count) ? '5' : 'x');

    count = format_into(buffer, "%c", 'Z');
    put(same(buffer, "Z", count) ? '6' : 'x');

    count = format_into(buffer, "%.11s", long_text);
    put(same(buffer, "abcdefghijk", count) ? '7' : 'x');

    count = format_into(buffer, "a%db", 7);
    put(same(buffer, "a7b", count) ? '8' : 'x');

    count = format_into(buffer, "%%");
    put(same(buffer, "%", count) ? '9' : 'x');

    count = format_into(buffer, "%5d", 42);
    put(same(buffer, "   42", count) ? 'a' : 'x');

    count = format_into(buffer, "%-5d", 42);
    put(same(buffer, "42   ", count) ? 'b' : 'x');

    count = format_into(buffer, "%05d", 42);
    put(same(buffer, "00042", count) ? 'c' : 'x');

    count = format_into(buffer, "%+d", 42);
    put(same(buffer, "+42", count) ? 'd' : 'x');

    count = format_into(buffer, "%d", -7);
    put(same(buffer, "-7", count) ? 'e' : 'x');

    count = format_into(buffer, "%#x", 0x2a);
    put(same(buffer, "0x2a", count) ? 'f' : 'x');

    count = format_into(buffer, "%s", (char *)0);
    put(same(buffer, "", count) ? 'g' : 'x');

    count = format_into(buffer, "plain");
    put(same(buffer, "plain", count) ? 'h' : 'x');

    count = format_into(buffer, "%u", (unsigned int)4000000000ul);
    put(same(buffer, "4000000000", count) ? 'i' : 'x');

    count = format_into(buffer, "%o", 8);
    put(same(buffer, "10", count) ? 'j' : 'x');

    count = format_into(buffer, "%08x", 0x2a);
    put(same(buffer, "0000002a", count) ? 'k' : 'x');

    count = format_into(buffer, "%-08d", 42);
    put(same(buffer, "42      ", count) ? 'l' : 'x');

    count = format_into(buffer, "%.3s", "abcdef");
    put(same(buffer, "abc", count) ? 'm' : 'x');

    count = format_into(buffer, "%#o", 8);
    put(same(buffer, "010", count) ? 'n' : 'x');

    count = format_into(buffer, "%p", (void *)0x1234);
    put(same(buffer, "00001234", count) ? 'o' : 'x');

    put('D');
    quit();
    return 0;
}
