/* The rebuilt `stream_layer_init` module.
 *
 * The original sits at linear 0x4c810 in STARTH.EXE and holds 19 instructions
 * and 61 bytes: the `edx` save, the frame, the argument slot, the two state
 * stores, the six setup calls, and the epilogue.
 *
 * The function stores the stream pointer, clears the state word, and runs the
 * stream layer setup: the three stream calls, the TABS loaders, and the
 * run-time call.
 */
extern unsigned int DAT_0002153c;
extern void *DAT_00021574;
extern void stream_setup_0(void);
extern void stream_setup_1(void);
extern void stream_setup_2(void);
extern void persptab_load(void);
extern void vis_tab_load(void);
extern void stream_setup_5(void);

void stream_layer_init(void *stream)
{
    DAT_0002153c = 0;
    DAT_00021574 = stream;
    stream_setup_0();
    stream_setup_1();
    stream_setup_2();
    persptab_load();
    vis_tab_load();
    stream_setup_5();
}
