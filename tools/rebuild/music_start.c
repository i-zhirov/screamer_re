/* The rebuilt `music_start` module.
 *
 * The original sits at linear 0x2ffc4 in STARTH.EXE and holds 41 instructions
 * and 140 bytes: the frame, the parameter slot, the status load at `+0x10`, the
 * six-way jump table, one message per state, the status test, the exit call, and
 * the epilogue.
 *
 * The function reads the status word at `+0x10` of the music bank struct.  A
 * status from 1 to 6 prints the matching message.  A non-zero status calls
 * `exit(-1)`.  A zero status returns at once.
 *
 *     status  message
 *     1       Cannot initialize midi system.
 *     2       Cannot initialize sound driver.
 *     3       Invalid midi banks.
 *     4       Cannot initialize song.
 *     5       Song still playing
 *     6       Song not playing
 *
 * The original dispatches through a compiler jump table.  The pinned OpenWatcom
 * V2 compiler does not place that table in the code object, so the rebuilt `jmp`
 * reads the entry at `CS:0` and the run faults.  The source therefore uses an
 * if-else chain, as the earlier `race_state_update` module does.  The chain keeps
 * one print call per state and no jump table, so the behavior check can run.
 */
extern int printf(const char *format, ...);
extern void exit(int status);

void music_start(char *volatile record)
{
    if (*(int *)(record + 0x10) == 1) {
        printf("Cannot initialize midi system.");
    } else if (*(int *)(record + 0x10) == 2) {
        printf("Cannot initialize sound driver.");
    } else if (*(int *)(record + 0x10) == 3) {
        printf("Invalid midi banks.");
    } else if (*(int *)(record + 0x10) == 4) {
        printf("Cannot initialize song.");
    } else if (*(int *)(record + 0x10) == 5) {
        printf("Song still playing");
    } else if (*(int *)(record + 0x10) == 6) {
        printf("Song not playing");
    }
    if (*(int *)(record + 0x10) != 0) {
        exit(-1);
    }
}
