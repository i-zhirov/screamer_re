/* The test harness for the rebuilt `startup` module.
 *
 * The harness holds the DOS and DPMI service stubs, the extender record, the
 * command line, the environment, and the two module buffers.  Each path sets
 * the stubs and calls the module:
 *
 *     0  the Phar Lap path: the "DX" version word, the PSP high address, the
 *        rounded break, the resize, the 0x2c environment segment, the command
 *        copy, the `no87=` scan, the program name, the full BSS clear, the
 *        initializer, the main invoker, and the exit chain
 *     1  the Rational DOS/4G path with a zero data base: the kernel segment,
 *        the 0x2c0 environment segment, and the first 0x1000 BSS bytes only
 *     2  the Rational DOS/4G path with a non-zero data base: the non-zero
 *        subtype
 *     3  the CauseWay path: the heap break, the command offset, the shifted
 *        environment offset, and the data segment as the PSP
 *     4  the unknown extender path: the type 0, the PSP 0x24, and the 0x2c
 *        environment segment
 *     5  the exit-with-message path: the message, the newline, the finalizer,
 *        and the code
 *     D  the exit marker
 *
 * The harness builds every text at run time, because DOS/4G 2.01a under
 * DOSBox-X leaves the initialized data pages zero.  The markers go through the
 * data buffer `text`, because the `print` helper of this harness writes garbage
 * for a string literal (the same fault shows in the output of the driver at
 * work/rebuild/out.txt).
 */
#pragma off (check_stack)

void startup(void);
void startup_exit_with_msg(const char *message, int status);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

extern unsigned char startup_osmajor;
extern unsigned char startup_osminor;
extern unsigned int startup_stack_top;
extern unsigned int startup_curbrk;
extern unsigned int startup_psp;
extern unsigned int startup_d16_info;
extern unsigned char startup_extender;
extern unsigned char startup_extender_subtype;
extern unsigned short startup_saved_ds;
extern unsigned int startup_env_offset;
extern unsigned short startup_env_segment;
extern unsigned short startup_no87;
extern unsigned int startup_stack_low;
extern unsigned int startup_dynend;
extern unsigned int startup_command_line;
extern unsigned int startup_program_name;
extern unsigned char startup_test_bss[0x1020];
extern char startup_command_buffer[0x100];

static unsigned int version_value;
static unsigned int version_signature;
static int rational_result;
static unsigned int kernel_value;
static unsigned int base_value;
static unsigned int psp_segment_value;
static unsigned int psp_environment_value;
static unsigned int data_segment_value;
static unsigned int psp_word_value;
static unsigned int cause_heap_value;
static unsigned int cause_command_value;
static unsigned int cause_environment_value;
static unsigned char command_length_value;
static int resize_calls;
static unsigned int resize_paragraphs;
static int init_calls;
static unsigned int init_priority;
static int main_calls;
static int fini_calls;
static unsigned int fini_priority;
static int terminate_calls;
static int terminate_status;
static char console_buffer[0x20];
static unsigned int console_length;
static char environment_buffer[0x40];
static char command_text[0x20];
static char command_expected[0x20];
static char program_text[0x10];
static char message_text[0x10];
static char console_expected[0x10];

static char text[8];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static int same(const char *first, const char *second)
{
    while (*first != 0) {
        if (*first != *second) {
            return 0;
        }
        first = first + 1;
        second = second + 1;
    }
    return *second == 0;
}

static void copy(char *destination, const char *source)
{
    int index;

    index = 0;
    while (source[index] != 0) {
        destination[index] = source[index];
        index = index + 1;
    }
    destination[index] = 0;
}

static void fill(char *target, char a, char b, char c, char d,
                 char e, char f, char g, char h)
{
    target[0] = a;
    target[1] = b;
    target[2] = c;
    target[3] = d;
    target[4] = e;
    target[5] = f;
    target[6] = g;
    target[7] = h;
    target[8] = 0;
}

static void build_texts(void)
{
    fill(command_text, ' ', ' ', 'A', 'L', 'P', 'H', 'A', ' ');
    fill(command_text + 8, 'B', 'E', 'T', 'A', 0, 0, 0, 0);
    fill(command_expected, 'A', 'L', 'P', 'H', 'A', ' ', 'B', 'E');
    fill(command_expected + 8, 'T', 'A', 0, 0, 0, 0, 0, 0);
    fill(program_text, 'P', 'R', 'O', 'G', '.', 'E', 'X', 'E');
    fill(message_text, 'B', 'O', 'O', 'M', 0, 0, 0, 0);
    fill(console_expected, 'B', 'O', 'O', 'M', 13, 10, 0, 0);
}

static void build_environment(int no87)
{
    int index;
    char *text_pointer;

    index = 0;
    text_pointer = program_text;
    environment_buffer[index] = 'P';
    index = index + 1;
    environment_buffer[index] = 'A';
    index = index + 1;
    environment_buffer[index] = 'T';
    index = index + 1;
    environment_buffer[index] = 'H';
    index = index + 1;
    environment_buffer[index] = '=';
    index = index + 1;
    environment_buffer[index] = 'C';
    index = index + 1;
    environment_buffer[index] = ':';
    index = index + 1;
    environment_buffer[index] = '\\';
    index = index + 1;
    environment_buffer[index] = 0;
    index = index + 1;
    if (no87 != 0) {
        environment_buffer[index] = 'N';
        index = index + 1;
        environment_buffer[index] = 'O';
        index = index + 1;
        environment_buffer[index] = '8';
        index = index + 1;
        environment_buffer[index] = '7';
        index = index + 1;
        environment_buffer[index] = '=';
        index = index + 1;
        environment_buffer[index] = '1';
        index = index + 1;
        environment_buffer[index] = 0;
        index = index + 1;
    }
    environment_buffer[index] = 0;
    index = index + 1;
    environment_buffer[index] = 0;
    index = index + 1;
    environment_buffer[index] = 0;
    index = index + 1;
    while (text_pointer[0] != 0) {
        environment_buffer[index] = text_pointer[0];
        index = index + 1;
        text_pointer = text_pointer + 1;
    }
    environment_buffer[index] = 0;
}

static void build_command(void)
{
    int index;

    index = 0;
    while (index < 0x100) {
        startup_command_buffer[index] = 0;
        index = index + 1;
    }
    copy(startup_command_buffer, command_text);
    copy(startup_command_buffer + 0x40, command_text);
}

static void reset(void)
{
    int index;

    version_value = 0;
    version_signature = 0x50484152;
    rational_result = 0;
    kernel_value = 0;
    base_value = 0;
    psp_segment_value = 0;
    psp_environment_value = 0;
    data_segment_value = 0x150;
    psp_word_value = 0;
    cause_heap_value = 0;
    cause_command_value = 0;
    cause_environment_value = 0;
    command_length_value = 12;
    resize_calls = 0;
    resize_paragraphs = 0;
    init_calls = 0;
    init_priority = 0;
    main_calls = 0;
    fini_calls = 0;
    fini_priority = 0;
    terminate_calls = 0;
    terminate_status = 0;
    console_length = 0;
    index = 0;
    while (index < 0x20) {
        console_buffer[index] = 0;
        index = index + 1;
    }
    index = 0;
    while (index < 0x1020) {
        startup_test_bss[index] = 0x5a;
        index = index + 1;
    }
    build_texts();
    build_command();
    build_environment(0);
}

void startup_dos_version(unsigned int *value, unsigned int *signature)
{
    *value = version_value;
    *signature = version_signature;
}

void startup_causeway(unsigned int *heap, unsigned int *command,
                      unsigned int *environment)
{
    *heap = cause_heap_value;
    *command = cause_command_value;
    *environment = cause_environment_value;
}

int startup_rational_probe(void)
{
    return rational_result;
}

unsigned int startup_kernel_segment(void)
{
    return kernel_value;
}

unsigned int startup_segment_base(void)
{
    return base_value;
}

unsigned int startup_psp_segment(void)
{
    return psp_segment_value;
}

unsigned int startup_psp_environment(void)
{
    return psp_environment_value;
}

unsigned int startup_data_segment(void)
{
    return data_segment_value;
}

unsigned int startup_psp_word(unsigned int offset)
{
    if (offset == 0x5c) {
        return psp_word_value;
    }
    return 0;
}

void startup_resize(unsigned int paragraphs)
{
    resize_calls = resize_calls + 1;
    resize_paragraphs = paragraphs;
}

char *startup_command_text(void)
{
    return startup_command_buffer + cause_command_value;
}

unsigned char startup_command_length(void)
{
    return command_length_value;
}

char *startup_environment(void)
{
    return environment_buffer;
}

void startup_init_routines(unsigned int priority)
{
    init_calls = init_calls + 1;
    init_priority = priority;
}

void startup_c_main(void)
{
    main_calls = main_calls + 1;
}

void startup_fini_routines(unsigned int priority)
{
    fini_calls = fini_calls + 1;
    fini_priority = priority;
}

void startup_terminate(int status)
{
    terminate_calls = terminate_calls + 1;
    terminate_status = status;
}

void startup_write_console(const char *message, unsigned int length)
{
    unsigned int index;

    index = 0;
    while (index < length && console_length < 0x1f) {
        console_buffer[console_length] = message[index];
        console_length = console_length + 1;
        index = index + 1;
    }
    console_buffer[console_length] = 0;
}

static int bss_all_zero(unsigned int count)
{
    unsigned int index;

    index = 0;
    while (index < count) {
        if (startup_test_bss[index] != 0) {
            return 0;
        }
        index = index + 1;
    }
    return 1;
}

static int bss_tail_untouched(unsigned int start, unsigned int count)
{
    unsigned int index;

    index = start;
    while (index < count) {
        if (startup_test_bss[index] != 0x5a) {
            return 0;
        }
        index = index + 1;
    }
    return 1;
}

static int common_path_ok(void)
{
    return same((char *)startup_command_line, command_expected)
        && same((char *)startup_program_name, program_text)
        && startup_stack_low == startup_program_name + 9
        && startup_saved_ds == 0x150
        && init_calls == 1 && init_priority == 0xff
        && main_calls == 1
        && fini_calls == 1 && fini_priority == 0xff
        && terminate_calls == 1 && terminate_status == 0;
}

int main(void)
{
    reset();
    version_value = 0x44580503;
    version_signature = 0x50484136;
    psp_word_value = 0x12345;
    build_environment(1);
    startup();
    put(startup_osmajor == 3 && startup_osminor == 5
        && startup_extender == 6 && startup_extender_subtype == 0
        && startup_psp == 0x24 && startup_curbrk == 0x13000
        && resize_calls == 1 && resize_paragraphs == 0x13
        && startup_env_offset == 0 && startup_env_segment == 0x2c
        && startup_no87 == 1 && bss_all_zero(0x1020)
        && common_path_ok() ? '0' : 'x');

    reset();
    version_value = 0x00000503;
    rational_result = 1;
    kernel_value = 0x1234;
    psp_segment_value = 0x2222;
    psp_environment_value = 0x2c0;
    startup();
    put(startup_d16_info == 0x1234
        && startup_extender == 1 && startup_extender_subtype == 0
        && startup_psp == 0x2222
        && startup_env_offset == 0 && startup_env_segment == 0x2c0
        && startup_no87 == 0 && bss_all_zero(0x1000)
        && bss_tail_untouched(0x1000, 0x1020)
        && common_path_ok() ? '1' : 'x');

    reset();
    version_value = 0x00000503;
    rational_result = 1;
    kernel_value = 0x1234;
    base_value = 0x1000;
    psp_segment_value = 0x2222;
    psp_environment_value = 0x2c0;
    startup();
    put(startup_extender == 1 && startup_extender_subtype == 1
        && startup_psp == 0x2222 && startup_env_segment == 0x2c0
        && bss_all_zero(0x1000)
        && bss_tail_untouched(0x1000, 0x1020)
        && common_path_ok() ? '2' : 'x');

    reset();
    version_value = 0x42430503;
    cause_heap_value = 0x1000;
    cause_command_value = 0x20;
    cause_environment_value = 0x2c;
    build_environment(1);
    startup();
    put(startup_extender == 9 && startup_extender_subtype == 0
        && startup_psp == 0x150
        && startup_stack_top - startup_curbrk == 0x1000
        && resize_calls == 1
        && resize_paragraphs == (startup_curbrk >> 12)
        && startup_env_offset == 0x2c0 && startup_env_segment == 0x150
        && startup_no87 == 1 && bss_all_zero(0x1020)
        && common_path_ok() ? '3' : 'x');

    reset();
    version_value = 0x00000503;
    rational_result = 0;
    startup();
    put(startup_extender == 0 && startup_extender_subtype == 0
        && startup_psp == 0x24
        && startup_env_offset == 0 && startup_env_segment == 0x2c
        && startup_no87 == 0 && bss_all_zero(0x1020)
        && common_path_ok() ? '4' : 'x');

    reset();
    startup_exit_with_msg(message_text, 3);
    put(same(console_buffer, console_expected)
        && fini_calls == 1 && fini_priority == 0xff
        && terminate_calls == 1 && terminate_status == 3
        && main_calls == 0 ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
