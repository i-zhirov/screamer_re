/* The rebuilt `startup` module.
 *
 * The original sits at linear 0x30170 in STARTH.EXE and holds 196 instructions
 * and 605 bytes.  It is the Watcom C/C++32 DOS-extender start routine from
 * `bld/clib/startup/a/cstrt386.asm` (OpenWatcom V2).  The body holds the
 * interrupt enable, the stack alignment, the stack top and break stores, the
 * DOS version call, the three extender paths (Phar Lap, CauseWay, Rational
 * DOS/4G), the command-line copy from the PSP, the environment scan for
 * `no87=`, the program-name copy, the BSS clear, the initializer call at
 * `0xd4374`, the main invoker call at `0xd4324`, and the exit chain at
 * `0x30389` and `0x303ac`.
 *
 * The original reads the PSP and the extender structures through segment
 * registers and runs the DOS and DPMI interrupts inline.  The C rebuild keeps
 * the decisions and the data effects, and moves each service behind a helper.
 * The helpers are the DOS version call, the Rational probe, the kernel segment,
 * the DPMI segment base, the PSP words, the command line, the environment, the
 * CauseWay record, the memory resize, the initializer, the main invoker, the
 * finalizer, the process terminator, and the console writer.  The test harness
 * supplies the helpers, so every extender path runs without a real extender.
 *
 * The original has no EBP frame, so the module builds without `-of+`.
 *
 * The harness build (`STARTUP_HARNESS`) puts the command line and the BSS in
 * named buffers, so the test can fill the BSS and read the copies.  The later
 * period build keeps the real `_edata`, `_end`, and stack addresses.
 *
 * Limitations: the rebuild does not set DS through the saved-DS slot, and it
 * does not run the `__GETDS` restore.  It also does not copy the program name
 * through the environment count word.  These differences keep the identity
 * check open, and the harness does not depend on them.
 */
extern void startup_dos_version(unsigned int *value, unsigned int *signature);
extern void startup_causeway(unsigned int *heap, unsigned int *command,
                             unsigned int *environment);
extern int startup_rational_probe(void);
extern unsigned int startup_kernel_segment(void);
extern unsigned int startup_segment_base(void);
extern unsigned int startup_psp_segment(void);
extern unsigned int startup_psp_environment(void);
extern unsigned int startup_data_segment(void);
extern unsigned int startup_psp_word(unsigned int offset);
extern void startup_resize(unsigned int paragraphs);
extern char *startup_command_text(void);
extern unsigned char startup_command_length(void);
extern char *startup_environment(void);
extern void startup_init_routines(unsigned int priority);
extern void startup_c_main(void);
extern void startup_fini_routines(unsigned int priority);
extern void startup_terminate(int status);
extern void startup_write_console(const char *text, unsigned int length);

unsigned char startup_osmajor;
unsigned char startup_osminor;
unsigned int startup_stack_top;
unsigned int startup_curbrk;
unsigned int startup_psp;
unsigned int startup_d16_info;
unsigned char startup_extender;
unsigned char startup_extender_subtype;
unsigned short startup_saved_ds;
unsigned int startup_env_offset;
unsigned short startup_env_segment;
unsigned short startup_no87;
unsigned int startup_stack_low;
unsigned int startup_dynend;
unsigned int startup_command_line;
unsigned int startup_program_name;

#ifdef STARTUP_HARNESS
unsigned char startup_test_bss[0x1020];
char startup_command_buffer[0x100];
#define STARTUP_BSS_ADDRESS startup_test_bss
#define STARTUP_BSS_SIZE ((unsigned int)sizeof(startup_test_bss))
#define STARTUP_COMMAND_DEST startup_command_buffer
#else
extern char _edata[];
extern char _end[];
#define STARTUP_BSS_ADDRESS ((unsigned char *)_edata)
#define STARTUP_BSS_SIZE ((unsigned int)(_end - _edata))
#define STARTUP_COMMAND_DEST ((char *)(((unsigned int)_end + 0xf) & ~0xf))
#endif

void startup_exit(int status)
{
    startup_fini_routines(0xff);
    startup_terminate(status);
}

void startup_exit_with_msg(const char *message, int status)
{
    unsigned int length;
    char newline[2];

    length = 0;
    while (message[length] != 0) {
        length = length + 1;
    }
    newline[0] = 13;
    newline[1] = 10;
    startup_write_console(message, length);
    startup_write_console(newline, 2);
    startup_exit(status);
}

void startup(void)
{
    unsigned int version;
    unsigned int signature;
    unsigned int heap;
    unsigned int cause_command;
    unsigned int cause_environment;
    unsigned int stack_top;
    unsigned int length;
    unsigned int bss_size;
    unsigned int index;
    char *command;
    char *command_base;
    char *environment;
    char *cursor;
    char *destination;
    char *program_destination;

    stack_top = (unsigned int)&version;
    stack_top = stack_top & 0xfffffffc;
    startup_stack_top = stack_top;
    startup_curbrk = stack_top;
    startup_psp = 0x24;

    command = startup_command_text();
    environment = startup_environment();

    signature = 0x50484152;
    startup_dos_version(&version, &signature);
    startup_osmajor = (unsigned char)(version & 0xff);
    startup_osminor = (unsigned char)((version >> 8) & 0xff);

    if ((version >> 16) == 0x4458) {
        startup_extender = (unsigned char)((signature & 0xff) - 0x30);
        startup_extender_subtype = 0;
        startup_psp = 0x24;
        startup_curbrk =
            (startup_psp_word(0x5c) + 0xfff) & 0xfffff000;
        startup_resize(startup_curbrk >> 12);
        startup_env_offset = 0;
        startup_env_segment = 0x2c;
    } else if ((version >> 16) == 0x4243) {
        heap = 0;
        cause_command = 0;
        cause_environment = 0;
        startup_causeway(&heap, &cause_command, &cause_environment);
        startup_extender = 9;
        startup_extender_subtype = 0;
        startup_curbrk = stack_top - heap;
        startup_resize(startup_curbrk >> 12);
        startup_psp = startup_data_segment();
        command = command + cause_command;
        startup_env_offset = cause_environment << 4;
        startup_env_segment = (unsigned short)startup_data_segment();
    } else if (startup_rational_probe() != 0) {
        version = startup_kernel_segment();
        if (version != 0) {
            startup_d16_info = version;
        }
        startup_extender = 1;
        startup_extender_subtype =
            (unsigned char)(startup_segment_base() != 0);
        startup_psp = startup_psp_segment();
        startup_env_offset = 0;
        startup_env_segment = (unsigned short)startup_psp_environment();
    } else {
        startup_extender = 0;
        startup_extender_subtype = 0;
        startup_psp = 0x24;
        startup_env_offset = 0;
        startup_env_segment = 0x2c;
    }

    startup_saved_ds = (unsigned short)startup_data_segment();

    length = startup_command_length();
    while (length != 0 && *command == ' ') {
        command = command + 1;
        length = length - 1;
    }
    command_base = STARTUP_COMMAND_DEST;
    destination = command_base;
    while (length != 0) {
        *destination = *command;
        destination = destination + 1;
        command = command + 1;
        length = length - 1;
    }
    *destination = 0;
    destination = destination + 1;
    *destination = 0;
    program_destination = destination;
    startup_program_name = (unsigned int)program_destination;

    startup_no87 = 0;
    cursor = environment;
    while (*cursor != 0) {
        unsigned int word;

        word = *((unsigned int *)cursor);
        word = word | 0x20202020;
        if (word == 0x37386f6e && cursor[4] == '=') {
            startup_no87 = 1;
        }
        while (*cursor != 0) {
            cursor = cursor + 1;
        }
        cursor = cursor + 1;
    }
    cursor = cursor + 3;
    while (*cursor != 0) {
        *program_destination = *cursor;
        program_destination = program_destination + 1;
        cursor = cursor + 1;
    }
    *program_destination = 0;
    program_destination = program_destination + 1;

    startup_command_line = (unsigned int)command_base;
    startup_stack_low = (unsigned int)program_destination;
    startup_dynend = (unsigned int)&bss_size;

    bss_size = STARTUP_BSS_SIZE;
    if (startup_extender == 1 && bss_size > 0x1000) {
        bss_size = 0x1000;
    }
    index = 0;
    while (index + 4 <= bss_size) {
        *((unsigned int *)(STARTUP_BSS_ADDRESS + index)) = 0;
        index = index + 4;
    }
    while (index < bss_size) {
        STARTUP_BSS_ADDRESS[index] = 0;
        index = index + 1;
    }

    startup_init_routines(0xff);
    startup_c_main();
    startup_exit(0);
}
