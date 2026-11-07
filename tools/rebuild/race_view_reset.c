/* The rebuilt `race_view_reset` module.
 *
 * The original sits at linear 0x13580 in STARTH.EXE and holds 91 instructions
 * and 330 bytes up to the `ret` at 0x136c9.  The next function starts at
 * 0x136cc, so the store record keeps the 332-byte extent and sets no separate
 * function size.  The identity check uses 91 instructions.
 *
 * The only direct caller at 0x12946 is race_session_start at 0x128e0, which
 * runs this function before the track load at 0x13540.  The caller takes the
 * section The timing and filter layer, so this module takes the section
 * race_session_start callees.
 *
 * The function clears the view state of every 0x590-byte car record at
 * DAT_00010ab8.  It clears the four view flags at +0x54c, +0x550, +0x56c, and
 * +0x574 and sets the view mode at +0x580 to 8.  The four flags and the mode
 * sit in the view block at +0x4f8, which car_camera and player_view_setup use.
 * The function then clears the chosen flag at +0x10 of the seven 0x14-byte
 * view slots at DAT_00101da4.
 *
 * The function also clears the shared view cells DAT_0010a9c, DAT_0010a94,
 * DAT_0010a74, DAT_0010a78, and DAT_0010aa8, and the shared loop counter
 * DAT_0000634c.
 *
 * The caller, the per-car view fields, the view slots, and the shared cells
 * support the name race_view_reset.  The original has no symbol, so the
 * record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses no floating-point instruction and calls nothing, so it needs
 * no FPU stub.
 */
struct view_slot {
    int kind;
    int mode;
    int distance;
    int video;
    int chosen;
};

extern int DAT_0000f8ec;
extern int DAT_0000634c;
extern unsigned char DAT_00010ab8[];
extern int DAT_0010a74;
extern int DAT_0010a78;
extern int DAT_0010a94;
extern int DAT_0010a9c;
extern int DAT_0010aa8;
extern struct view_slot DAT_00101da4[7];

void race_view_reset(void)
{
    struct view_slot *slot;

    DAT_0000634c = 0;
    DAT_0010a9c = 0;
    for (; DAT_0000634c < DAT_0000f8ec; DAT_0000634c++) {
        *(int *)(DAT_00010ab8 + DAT_0000634c * 0x590 + 0x574) = 0;
    }
    DAT_0000634c = 0;
    DAT_0010a94 = 0;
    for (; DAT_0000634c < DAT_0000f8ec; DAT_0000634c++) {
        *(int *)(DAT_00010ab8 + DAT_0000634c * 0x590 + 0x56c) = 0;
    }
    DAT_0000634c = 0;
    DAT_0010a74 = 0;
    for (; DAT_0000634c < DAT_0000f8ec; DAT_0000634c++) {
        *(int *)(DAT_00010ab8 + DAT_0000634c * 0x590 + 0x54c) = 0;
    }
    DAT_0000634c = 0;
    DAT_0010a78 = 0;
    for (; DAT_0000634c < DAT_0000f8ec; DAT_0000634c++) {
        *(int *)(DAT_00010ab8 + DAT_0000634c * 0x590 + 0x550) = 0;
    }
    DAT_0000634c = 0;
    DAT_0010aa8 = 0;
    for (; DAT_0000634c < DAT_0000f8ec; DAT_0000634c++) {
        *(int *)(DAT_00010ab8 + DAT_0000634c * 0x590 + 0x580) = 8;
    }
    for (slot = DAT_00101da4; slot < DAT_00101da4 + 7; slot++) {
        slot->chosen = 0;
    }
}
