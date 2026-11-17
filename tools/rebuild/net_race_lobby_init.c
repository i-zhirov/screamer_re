/* The rebuilt `net_race_lobby_init` module.
 *
 * The original sits at linear 0x2ca40 in STARTH.EXE and holds 60 instructions
 * and 223 bytes up to the `ret` at 0x2cb1e.  The next function starts at
 * 0x2cb20, so the store record keeps the 224-byte extent and sets no separate
 * function size.  The identity check uses 60 instructions.
 *
 * The function is the state 8 init of the state init dispatcher at 0x2be94.
 * The jump table at object offset 0x1be70 maps the state 8 to the call at
 * 0x2bed3.  The matching state 8 update at 0x29605 calls `net_race_lobby` at
 * 0x2d7c4, and `race_state_update` at 0x29564 is a `race_frame` callee at
 * 0x2807c, so the record takes the section `race_frame callees`.
 *
 * The function sets `DAT_00120378` at 1.5, `DAT_00120380` at 8.0, and
 * `DAT_0011fac8` at 1, and runs the init at 0x2dff4.  It builds the member
 * `netH.pcx` from the format at object-3 offset 0x11bc and loads it from
 * `gfx\graf.dat` at object-3 offset 0x11c6 into `DAT_0011fad0` with the
 * palette `DAT_0010cf5c` through `pcx_load` at 0x1b5a4.  A non-zero result
 * runs the cleanup at 0x15150.
 *
 * The function sets `DAT_00118e08` at 0.08, `DAT_000007f9` at 8, and
 * `DAT_00120998` at 1, presents the buffer with the scale 0 and the offset 0
 * through `video_present` at 0x27ec8, copies 0x3e800 bytes to `DAT_00121528`
 * through `memcpy` at 0x8c150, fills the screen through `video_fill` at
 * 0x2aa98, and presents again.  It then clears the lobby flag `DAT_00120994`,
 * clears the eight 0x38-byte player records at `DAT_0012099c` through `memset`
 * at 0xd1ec0, and waits through the timing loop at 0x1ec48.
 *
 * The state 8 update, the net race lobby member, and the player record clear
 * support the name `net_race_lobby_init`.  The original has no symbol, so the
 * record keeps the name with the confidence inferred.
 *
 * The function uses no floating-point instruction.  The original has the EBP
 * frame, so the module builds with `-of+`.
 */
extern float DAT_00120378;
extern float DAT_00120380;
extern int DAT_0011fac8;
extern void *DAT_0011fad0;
extern float DAT_00118e08;
extern int DAT_000007f9;
extern int DAT_00120998;
extern void *DAT_00121528;
extern int DAT_00120994;
extern char DAT_0012099c[];
extern unsigned char DAT_0010cf5c[];
extern char net_lobby_format[];
extern char net_lobby_archive[];

extern int sprintf(char *buffer, const char *format, ...);
extern int pcx_load(char *file, char *member, void *pixels, char *palette);
extern void sub_15150(void);
extern void video_present(float scale, int offset);
extern void *memcpy(void *target, const void *source, unsigned int count);
extern void video_fill(void);
extern void *memset(void *target, int value, unsigned int count);
extern void sub_2dff4(void);
extern void sub_1ec48(void);

void net_race_lobby_init(void)
{
    char member[0xc0];

    DAT_00120378 = 1.5f;
    DAT_00120380 = 8.0f;
    DAT_0011fac8 = 1;
    sub_2dff4();
    sprintf(member, net_lobby_format, 'H');
    if (pcx_load(net_lobby_archive, member, DAT_0011fad0,
            (char *)DAT_0010cf5c) != 0) {
        sub_15150();
    }
    DAT_00118e08 = 0.08f;
    DAT_000007f9 = 8;
    DAT_00120998 = 1;
    video_present(0.0f, 0);
    memcpy(DAT_00121528, DAT_0011fad0, 0x3e800);
    video_fill();
    video_present(0.0f, 0);
    DAT_00120994 = 0;
    memset(DAT_0012099c, 0, 0x1c0);
    sub_1ec48();
}
