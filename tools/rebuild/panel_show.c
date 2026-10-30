/* The rebuilt `panel_show` module.
 *
 * The original sits at linear 0x2dfac in STARTH.EXE and holds 22 instructions
 * and 71 bytes: the frame, the three argument slots, the five state stores,
 * the panel update call at `0x25af0`, and the epilogue.
 *
 * The function stores the doubled position pair and the size, sets the visible
 * flag and clears the count, and updates the panel.
 */
extern unsigned int DAT_0011cb14;
extern unsigned int DAT_0011cb18;
extern unsigned int DAT_0011cb1c;
extern unsigned int DAT_0011cb20;
extern unsigned int DAT_0011cb24;
extern void panel_update(void);

void panel_show(int x, int y, int size)
{
    DAT_0011cb18 = x + x;
    DAT_0011cb1c = y + y;
    DAT_0011cb14 = 1;
    DAT_0011cb24 = 0;
    DAT_0011cb20 = size;
    panel_update();
}
