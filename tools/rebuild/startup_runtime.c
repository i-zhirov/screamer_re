/* The interim startup and run-time cluster.
 *
 * The whole-image link reports these symbols as undefined because the shipped
 * image statically linked the Watcom C startup, the DPMI helpers, the DOS
 * termination path, and a set of game cleanup and teardown routines.  This
 * module defines them with the calling convention of their callers:
 *
 *   atof, cleanup/cleanup_01..12, teardown_01..13, f1..f8, exit_proc
 *   the drm_* DPMI helpers
 *   the startup_* helpers that `startup.c` calls
 *
 * The functions are a compatibility layer, not a byte-faithful recovery.  The
 * startup helpers return inert values, the cleanup and teardown routines do
 * nothing, and the polling DPMI helper reports the ready state.  The module has
 * no function record and no behavior check, so it does not enter the verified
 * count.
 */
static char empty_command[1];
static char empty_environment[4];

int DAT_00004298;
int DAT_0000429c;
int DAT_00027e24;
int DAT_00027e28;

extern void exit_terminate(int status);

double atof(char *text)
{
    (void)text;
    return 0.0;
}

void cleanup(void)
{
}

void cleanup_01(void)
{
}

void cleanup_02(void)
{
}

void cleanup_03(void)
{
}

void cleanup_04(void)
{
}

void cleanup_05(void *a, void *b)
{
    (void)a;
    (void)b;
}

void cleanup_06(void *a, void *b)
{
    (void)a;
    (void)b;
}

void cleanup_07(void)
{
}

void cleanup_08(void)
{
}

void cleanup_09(void)
{
}

void cleanup_10(void *a)
{
    (void)a;
}

void cleanup_11(void)
{
}

void cleanup_12(void *a)
{
    (void)a;
}

void drm_26014(void)
{
}

void drm_260a0(void)
{
}

void drm_260e0(void)
{
}

unsigned char drm_26120(void)
{
    return 2;
}

void drm_262a8(void)
{
}

int drm_262fc(void)
{
    return 0;
}

void drm_264ec(void)
{
}

void exit_proc(int code)
{
    exit_terminate(code);
}

void f1(void)
{
}

void f2(void)
{
}

void f3(void *value)
{
    (void)value;
}

void f4(void *value)
{
    (void)value;
}

void f5(void)
{
}

void f6(void)
{
}

void f7(void)
{
}

void f8(void)
{
}

void startup_dos_version(unsigned int *value, unsigned int *signature)
{
    *value = 0;
    *signature = 0;
}

void startup_causeway(unsigned int *heap, unsigned int *command,
                      unsigned int *environment)
{
    *heap = 0;
    *command = 0;
    *environment = 0;
}

int startup_rational_probe(void)
{
    return 0;
}

unsigned int startup_kernel_segment(void)
{
    return 0;
}

unsigned int startup_segment_base(void)
{
    return 0;
}

unsigned int startup_psp_segment(void)
{
    return 0x24;
}

unsigned int startup_psp_environment(void)
{
    return 0x2c;
}

unsigned int startup_data_segment(void)
{
    return 0;
}

unsigned int startup_psp_word(unsigned int offset)
{
    (void)offset;
    return 0;
}

void startup_resize(unsigned int paragraphs)
{
    (void)paragraphs;
}

char *startup_command_text(void)
{
    return empty_command;
}

unsigned char startup_command_length(void)
{
    return 0;
}

char *startup_environment(void)
{
    return empty_environment;
}

void startup_init_routines(unsigned int priority)
{
    (void)priority;
}

void startup_c_main(void)
{
}

void startup_fini_routines(unsigned int priority)
{
    (void)priority;
}

void startup_terminate(int status)
{
    exit_terminate(status);
}

void startup_write_console(const char *text, unsigned int length)
{
    (void)text;
    (void)length;
}

void teardown_01(int value)
{
    (void)value;
}

void teardown_02(void)
{
}

void teardown_03(void *a, void *b)
{
    (void)a;
    (void)b;
}

void teardown_04(void)
{
}

void teardown_05(void)
{
}

void teardown_06(void *a)
{
    (void)a;
}

void teardown_07(void *a)
{
    (void)a;
}

void teardown_08(void)
{
}

void teardown_09(void)
{
}

void teardown_10(void)
{
}

void teardown_11(void)
{
}

void teardown_12(void)
{
}

void teardown_13(void)
{
}
