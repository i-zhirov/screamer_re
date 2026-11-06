/* The rebuilt `view_move` module.
 *
 * The original sits at linear 0x1f94c in STARTH.EXE and holds 22 instructions
 * and 65 bytes: the frame, the three argument slots, the three calls, the
 * shifted view position, and the epilogue.
 *
 * The function moves the view: it calls the view helper with the second
 * argument, calls the position setter with the constant `0x62b0` and the
 * shifted third argument, and calls the position setter with the doubled and
 * shifted first argument.
 */
extern void view_move_0(int a);
extern void view_move_1(int a, int b);
extern void view_move_2(int a, int b);

void view_move(int a, int b, int c)
{
    int v;

    view_move_0(b);
    view_move_1(0x62b0, c >> 8);
    v = (a + a) >> 8;
    view_move_2(v, v);
}
