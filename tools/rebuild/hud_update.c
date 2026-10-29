/* The rebuilt `hud_update` module.
 *
 * The original sits at linear 0x27a34 in STARTH.EXE and holds 11 instructions
 * and 38 bytes: the frame `push ebp` and `mov ebp,esp`, the two guards on the
 * session state `DAT_0011ce44`, the three overlay calls, and the epilogue.
 *
 * The function runs the HUD overlays: when the session state is not 3, it runs
 * the first two overlays, and when bit 1 of the state is set, it runs the third
 * overlay.
 */
extern unsigned int DAT_0011ce44;
extern void hud_overlay_0(void);
extern void hud_overlay_1(void);
extern void hud_overlay_2(void);

void hud_update(void)
{
    if (DAT_0011ce44 != 3) {
        hud_overlay_0();
        hud_overlay_1();
    }
    if (DAT_0011ce44 & 2) {
        hud_overlay_2();
    }
}
