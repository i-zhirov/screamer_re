/* The rebuilt `race_session_start` module.
 *
 * The original sits at linear 0x128e0 in STARTH.EXE.  The function body up to
 * the first return holds 42 instructions and 136 bytes: the six register saves,
 * the frame, the parameter slot, the flag store, the race data load at
 * `0x131a8`, the first error return, the seven constant stores, the object reset
 * at `0x13580`, the track load at `0x13540`, and the second error return.  The
 * true extent continues through `0x12c09` and holds the mode switch, the default
 * path, the mode-specific start, and the frame setup.
 *
 * The function sets the flags `_DAT_00118e1c = 0x102`, `DAT_0011521c = 0x1234`,
 * and `_DAT_0011cb14 = 1`, clears five state globals, and dispatches on the game
 * mode `DAT_0011d260`:
 *
 *     0x001, 0x002             0x12c0c
 *     0x004                    0x12e18
 *     0x010                    0x12d0c
 *     0x020                    0x13120
 *     0x040                    0x130a0
 *     0x100                    0x12f50
 *     other                    the default path
 *
 * The default path runs `view_move(0, 0, 0)` at `0x1f94c` and
 * `0x21da8(parameter ^ 1)`.  The other call targets:
 *
 *     0x131a8  race_data_load      0x13580  race_objects_reset
 *     0x13540  race_track_load     0x1f94c  view_move
 *     0x21da8  mode_flag_set       0x4cf3c  message_format
 *     0x258d0  message_show        0x8c160  memset
 *     0x2aa98  video_fill          0x2471c  panel_reset
 *     0x1f718  table_lookup        0x264ec  input_refresh
 *     0x263cc  input_apply         0x2febc  music_select
 *     0x11568  object_list_link    0x19ab0  player_update
 *     0x1eae0  mode10_step_a       0x1ec48  mode10_step_b
 */
extern int _DAT_00118e1c;
extern int _DAT_00118e18;
extern int DAT_0011cb24;
extern int DAT_00113cc8;
extern int _DAT_00118e38;
extern int DAT_00119060;
extern int DAT_0011521c;
extern int _DAT_0011cb14;
extern int DAT_0011d260;
extern char DAT_0011d294[];
extern int DAT_0011d2a0;
extern int DAT_0011d2a8;
extern int DAT_0011d2cc;
extern int DAT_000203bc;
extern int DAT_0000cf40;
extern int DAT_0000cf54;
extern char DAT_0000cf5c[];
extern int DAT_0000ff88;
extern int DAT_0000ffa4;
extern int DAT_00001df0;
extern int DAT_00001d7c[];
extern int DAT_00001db4[];
extern int DAT_00001f00[];
extern int DAT_000100e8;
extern int DAT_000007f9;
extern int DAT_00110a9c;
extern void *DAT_00021528;
extern int _DAT_00118e04;
extern int _DAT_00118e10;
extern int _DAT_00118e24;
extern int _DAT_00118e28;

extern int race_data_load(int value);
extern void race_objects_reset(void);
extern int race_track_load(int value);
extern int game_mode_1_start(int value);
extern int game_mode_4_start(int value);
extern int game_mode_10_start(int value);
extern int game_mode_20_start(int value);
extern int game_mode_40_start(int value);
extern int game_mode_100_start(int value);
extern void view_move(int x, int y, int z);
extern void mode_flag_set(int value);
extern void message_format(char *format, char *buffer);
extern void message_show(char *buffer);
extern void *memset(void *target, unsigned int count, int value);
extern void video_fill(void);
extern void panel_reset(void);
extern int table_lookup(int index);
extern void input_refresh(void);
extern int input_apply(unsigned int state, void *device);
extern void music_select(char *record, int value);
extern void object_list_link(char *first, char *second);
extern void player_update(void);
extern void mode10_step_a(int value);
extern void mode10_step_b(void);

int race_session_start(int value)
{
    int result;
    int index;
    int saved;

    _DAT_00118e1c = 0x102;
    result = race_data_load(value);
    if (result != 0) {
        return result;
    }
    _DAT_00118e18 = 0;
    DAT_0011cb24 = 0;
    DAT_00113cc8 = 0;
    _DAT_00118e38 = 0;
    DAT_00119060 = 0;
    DAT_0011521c = 0x1234;
    _DAT_0011cb14 = 1;
    race_objects_reset();
    result = race_track_load(value);
    if (result != 0) {
        return result;
    }
    switch (DAT_0011d260) {
    case 1:
    case 2:
        result = game_mode_1_start(value);
        break;
    case 4:
        result = game_mode_4_start(value);
        break;
    case 0x10:
        result = game_mode_10_start(value);
        break;
    case 0x20:
        result = game_mode_20_start(value);
        break;
    case 0x40:
        result = game_mode_40_start(value);
        break;
    case 0x100:
        result = game_mode_100_start(value);
        break;
    default:
        result = 0;
        break;
    }
    if (result != 0) {
        return result;
    }
    view_move(0, 0, 0);
    mode_flag_set(value ^ 1);
    if (value != 0) {
        message_format(DAT_0011d294, DAT_0000cf5c);
        message_show(DAT_0000cf5c);
    }
    memset(DAT_00021528, 0x3e800, 0);
    video_fill();
    if (DAT_0011d260 == 0x100) {
        index = 0;
    } else {
        index = DAT_000203bc + 2;
        panel_reset();
        if (DAT_0011d2a0 == 0xc) {
            DAT_000100e8 = table_lookup(
                DAT_00001d7c[DAT_0000ff88 + 2]);
        } else {
            DAT_000100e8 = table_lookup(
                DAT_00001d7c[DAT_0000ff88]);
        }
        DAT_0000ffa4 = DAT_0000cf40;
        if (DAT_0000cf54 != 0) {
            DAT_00001df0 = 1;
        } else {
            DAT_00001db4[DAT_0011d2a8 * 5] = 1;
        }
    }
    input_refresh();
    if (DAT_0011d2cc != 0) {
        input_apply(DAT_00001f00[index] + 1, (void *)1);
        music_select((char *)0x62b0, 1);
    }
    object_list_link((char *)0x6530, (char *)0x6350);
    saved = DAT_00110a9c;
    DAT_00110a9c = 1;
    player_update();
    DAT_00110a9c = saved;
    if (DAT_0011d260 == 0x10) {
        mode10_step_a(0x50);
        mode10_step_b();
        mode10_step_a(0x50);
        mode10_step_b();
    }
    _DAT_00118e04 = 0;
    _DAT_00118e10 = 0;
    _DAT_00118e24 = 0;
    DAT_000007f9 = 0;
    _DAT_00118e28 = 0;
    return 0;
}
