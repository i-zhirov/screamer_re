/* The rebuilt `music_play` module.
 *
 * The original sits at linear 0x2ff14 in STARTH.EXE and holds 39 instructions
 * and 111 bytes: the three register saves, the frame, the request slot, the
 * two state guards, the ready check at `0xd4239`, the start call at `0xd38e6`,
 * the selection call at `0x2febc`, the state stores, and the epilogue.
 *
 * The function processes a music request record.  A negative word at `+0x04`
 * sets the state 6.  A zero ready result sets the state 5.  Otherwise the
 * function starts the track, selects it when the word at `+0x0c` is 1, and sets
 * the state 0.  The function returns the state it wrote.
 */
extern int music_ready(int track);
extern void music_start(int track);
extern void music_select(char *record, int value);

int music_play(char *record)
{
    int track;

    if (*(int *)(record + 4) < 0) {
        *(int *)(record + 0x10) = 6;
        return 6;
    }
    track = *(int *)record;
    if (music_ready(track) == 0) {
        *(int *)(record + 0x10) = 5;
        return 5;
    }
    music_start(track);
    if (*(int *)(record + 0xc) == 1) {
        music_select(record, 1);
    }
    *(int *)(record + 0x10) = 0;
    return 0;
}
