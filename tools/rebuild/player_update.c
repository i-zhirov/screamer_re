/* The rebuilt `player_update` module.
 *
 * The original sits at linear 0x19ab0 in STARTH.EXE and holds 14 instructions
 * and 55 bytes: the frame, the five player structure addresses, the five step
 * calls at `0x19ae8`, `0x19e78`, `0x19f00`, `0x1a144`, and `0x212d0`, and the
 * epilogue.  The function runs the five steps on the player structure
 * `DAT_00110a20` in call order.
 */
extern unsigned char DAT_00110a20[];
extern void player_step_0(void *player);
extern void player_step_1(void *player);
extern void player_step_2(void *player);
extern void player_step_3(void *player);
extern void player_step_4(void *player);

void player_update(void)
{
    player_step_0(DAT_00110a20);
    player_step_1(DAT_00110a20);
    player_step_2(DAT_00110a20);
    player_step_3(DAT_00110a20);
    player_step_4(DAT_00110a20);
}
