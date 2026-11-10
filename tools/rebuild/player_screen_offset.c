/* The rebuilt `player_screen_offset` module.
 *
 * The original sits at linear 0x18ccc in STARTH.EXE and holds 69 instructions
 * and 226 bytes up to the `ret` at 0x18dad.  The next function starts at
 * 0x18db0, so the record keeps the 228-byte extent and sets no function size.
 *
 * The only direct caller at 0x15c7e is `player_car_link` at 0x15c1c.  The
 * caller passes the player object in `eax`, so the record takes the section
 * The vehicle state chain.
 *
 * The function forms the horizontal difference from the world reference
 * `DAT_00110a20` and `DAT_00110a28` to the object position at `+0x4b0` and
 * `+0x4b8`.  It takes the square root at 0x22bec of the summed squares.
 *
 * The two normalized ratios scale the screen limit 20.0 at object-3 offset
 * 0x854.  The function subtracts the scaled ratios from the object position
 * and stores the truncation at `+0x118` and `+0x120`.  It stores the height
 * `+0x4b4 - 20` at `+0x11c`.
 *
 * The original truncates through the Watcom helper at 0x4d9f4.  The local FPU
 * stub makes `__CHP` truncate toward zero.  The limit arrives through a
 * global, because the minimal image does not load the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int DAT_00110a20;
extern int DAT_00110a28;
extern float player_screen_offset_limit;

extern double sqrt(double value);

void player_screen_offset(char *record)
{
    int delta_x;
    int delta_z;
    int distance;
    float length;
    float ratio_x;
    float ratio_z;

    delta_x = DAT_00110a20 - *(int *)(record + 0x4b0);
    delta_z = DAT_00110a28 - *(int *)(record + 0x4b8);
    distance = delta_x * delta_x + delta_z * delta_z;
    length = (float)sqrt((double)distance);
    ratio_x = (float)(DAT_00110a20 - *(int *)(record + 0x4b0)) / length;
    ratio_z = (float)(DAT_00110a28 - *(int *)(record + 0x4b8)) / length;

    *(int *)(record + 0x118) = (int)((float)*(int *)(record + 0x4b0)
        - ratio_x * player_screen_offset_limit);
    *(int *)(record + 0x11c) = *(int *)(record + 0x4b4) - 20;
    *(int *)(record + 0x120) = (int)((float)*(int *)(record + 0x4b8)
        - ratio_z * player_screen_offset_limit);
}
