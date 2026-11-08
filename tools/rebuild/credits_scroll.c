/* The rebuilt `credits_scroll` module.
 *
 * The original sits at linear 0x27c34 in STARTH.EXE and holds 133 instructions
 * and 466 bytes up to the `ret` at 0x27e05.  The next function starts at
 * 0x27e08, so the store record keeps the 468-byte extent and sets no function
 * size.  The identity check uses 133 instructions.
 *
 * The only direct caller at 0x27a45 is `hud_update` at 0x27a34.  The
 * `object_update_all` frame step calls `hud_update` at the end of the object
 * pass, so the record takes the section `object_update_all callees`.
 *
 * The pointer table `DAT_0011cef8` holds the pages of the credits text that
 * `member_list_load` reads from the `attr` member of `data\local.dat`.  A line
 * that starts with `#` ends the current page.  The index `DAT_0011ce3c` holds
 * the first line of the page.  The function walks up to 0x14 lines from that
 * index and stops at the `#` line.
 *
 * The position cell `DAT_0011ce58[i]` holds the line offset.  The integer
 * filter at 0x22bb4 moves the cell toward `-(DAT_0011ce40 * 0x190)` at the
 * coefficient `DAT_0010cf08[0x5a]`, unless the cell already sits at or below
 * -0x168, which sets the timer `DAT_0011ce48` to 4.0.  The function measures
 * each line
 * through `text_measure` at 0x254a8, centers it on the 0x140 screen width, and
 * draws it through `text_draw` at 0x25020 at the doubled position and the row
 * `i * 0x28` with the font 0 and the color 2.
 *
 * After the page, the timer grows by the frame delta `DAT_00118e04`.  A timer
 * below 4.0 returns.  Otherwise the timer clears and the toggle
 * `DAT_0011ce40` flips.  The flip from 1 to 0 advances `DAT_0011ce3c` past the
 * `#` line, increments the page state `DAT_0011ce54`, and resets the 0x14
 * positions to 0x15e.  When the new first line is the -1 sentinel, the function
 * sets the session state `DAT_0011ce44` to 3, clears the page state and the
 * index, and resets the 0x14 positions to 0x15e and the 0x14 cells at
 * `DAT_0011cea8` to 0x64.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int DAT_0011ce3c;
extern int DAT_0011ce40;
extern int DAT_0011ce44;
extern float DAT_0011ce48;
extern int DAT_0011ce54;
extern int DAT_0011ce58[];
extern int DAT_0011cea8[];
extern char *DAT_0011cef8[];
extern float DAT_00118e04;
extern void filter_update_int(int *value, float target, int index);
extern void text_measure(int *width, int *height, char *text, int font);
extern void text_draw(int x, int y, char *text, int font, int color);

void credits_scroll(void)
{
    int line;
    int index;
    int target;
    int width;
    int height;
    int row;
    int center;

    line = 0;
    index = DAT_0011ce3c;
    do {
        if (DAT_0011ce58[line] > -360) {
            target = -(DAT_0011ce40 * 400);
            filter_update_int(&DAT_0011ce58[line], (float)target, 0x5a);
        } else {
            DAT_0011ce48 = 4.0f;
        }
        text_measure(&width, &height, DAT_0011cef8[index], 0);
        row = line * 0x14;
        center = (0x140 - width / 2) >> 1;
        text_draw(2 * (DAT_0011ce58[line] + center), row * 2,
                  DAT_0011cef8[index], 0, 2);
        index = index + 1;
        line = line + 1;
    } while (DAT_0011cef8[index][0] != '#');

    DAT_0011ce48 = DAT_0011ce48 + DAT_00118e04;
    if (DAT_0011ce48 < 4.0f) {
        return;
    }
    DAT_0011ce48 = 0.0f;
    if (DAT_0011ce40 != 0) {
        DAT_0011ce40 = 0;
        DAT_0011ce3c = index + 1;
        DAT_0011ce54 = DAT_0011ce54 + 1;
        for (line = 0; line < 0x14; line = line + 1) {
            DAT_0011ce58[line] = 0x15e;
        }
    } else {
        DAT_0011ce40 = 1;
    }
    if (DAT_0011cef8[DAT_0011ce3c] == (char *)-1) {
        DAT_0011ce44 = 3;
        DAT_0011ce54 = 0;
        DAT_0011ce3c = 0;
        for (line = 0; line < 0x14; line = line + 1) {
            DAT_0011ce58[line] = 0x15e;
            DAT_0011cea8[line] = 0x64;
        }
    }
}
