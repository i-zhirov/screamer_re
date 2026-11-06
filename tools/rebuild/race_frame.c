/* The rebuilt `race_frame` module.
 *
 * The original sits at linear 0x2807c in STARTH.EXE and holds 48 instructions
 * and 189 bytes: the three register saves, the frame, the 0x3c-byte local
 * area, the `memcpy` call at 0x8c150 with the count 0x3e800, the two
 * video-swap calls at 0x1075f and 0x107a1 behind the flags 0x10 and 0x20 of
 * `DAT_0011d274`, the overlay dispatch at 0x28160, the heading update at
 * 0x29610, the state update at 0x29564, the camera call at 0x8ac78 with the
 * object 0x109a0, the sound call at 0x81a70 with the block 0x6350, the overlay
 * switch through the compiler table at `cs:[eax*4 + 0x18068]`, the fade call
 * at 0x29818 with the float `DAT_00120378`, the float compare against the
 * constant 0xee0, and the epilogue.
 *
 * The switch reads the race state `DAT_001203b8`.  The state 0 calls
 * `race_panel_0` at 0x2e0b4, the state 1 calls nothing, the state 2 calls
 * `race_panel_2` at 0x2e21c, the state 3 calls `race_panel_3` at 0x2e58c,
 * and the state 4 calls `race_panel_4` at 0x2e724.  A value above 4 skips the
 * switch.
 *
 * The constant at 0xee0 sits in a low segment that the shared harness image
 * does not load.  The rebuild therefore reads the threshold through the global
 * `frame_limit`, and the harness sets the exact bit pattern.
 */
extern void *DAT_0011fad0;
extern void *DAT_00121528;
extern unsigned char DAT_0011d274;
extern float DAT_0011d24c;
extern unsigned int DAT_0011fac4;
extern unsigned int DAT_0011d234;
extern float DAT_00120378;
extern unsigned int DAT_001203b8;
extern float frame_limit;

extern void *memcpy(void *destination, const void *source, unsigned int count);
extern void video_block_swap_a(void);
extern void video_block_swap_b(void);
extern void overlay_dispatch(void);
extern void race_heading_update(void);
extern void race_state_update(void);
extern void camera_update(void *object);
extern void sound_update(void *device);
extern void race_panel_0(void);
extern void race_panel_2(void);
extern void race_panel_3(void);
extern void race_panel_4(void);
extern void fade_update(float delta);

void race_frame(void)
{
    memcpy(DAT_00121528, DAT_0011fad0, 0x3e800);
    if ((DAT_0011d274 & 0x10) != 0) {
        video_block_swap_a();
    }
    if ((DAT_0011d274 & 0x20) != 0) {
        video_block_swap_b();
    }
    overlay_dispatch();
    race_heading_update();
    race_state_update();
    camera_update((void *)0x109a0);
    sound_update((void *)0x6350);
    switch (DAT_001203b8) {
    case 0:
        race_panel_0();
        break;
    case 2:
        race_panel_2();
        break;
    case 3:
        race_panel_3();
        break;
    case 4:
        race_panel_4();
        break;
    }
    fade_update(DAT_00120378);
    if (DAT_0011d24c > frame_limit && DAT_0011fac4 == 0) {
        DAT_0011d234 = 1;
    }
}
