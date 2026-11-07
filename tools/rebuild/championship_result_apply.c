/* The rebuilt function at linear 0x29bac in STARTH.EXE.
 *
 * The original body holds 90 instructions and 348 bytes up to the `ret` at
 * 0x29d07.  The next function starts at 0x29d08, so the store record keeps the
 * 348-byte extent and sets no function size.  The identity check uses 90
 * instructions.
 *
 * The only caller is the call at 0x29027 in `net_race_screen` at 0x28ba4.  The
 * call sits in the mode 2 overlay that runs after a race: it shows the loading
 * screen at 0x2a364, runs 0x29d08, saves the record through `race_record_save`
 * at 0x2e82c, calls this function, and then enters state 5 through
 * `race_state_begin` at 0x294c8.  State 5 is the championship screen at 0x2cf68.
 *
 * The function applies the finished-race result to the championship standings.
 * The flag `DAT_00118e1c` bit 0x20 aborts the function.  The flag
 * `DAT_00118e1d` bit 0x10 sets the result `DAT_00120804` to 1, otherwise to 0.
 *
 * The selected record `DAT_001203dc` picks one 0x2c-byte record from the table
 * at object-3 offset 0x22bc.  The record field `+0x24` is the championship
 * group.  The function stores the car number `DAT_0011d270`, the race time
 * `DAT_0011d268`, and the points of the car `DAT_001021f0[DAT_0011d270]` into
 * the standings row of the group at `DAT_00120400`, `DAT_001203e8`, and
 * `DAT_00120418`.  The original copies the time bits with a dword move, so the
 * module declares the time and the time array as `int`, and it uses no
 * floating-point instruction.
 *
 * The function then sums the points of the groups from 0 up to the group, adds
 * 9 points for every higher group up to the sixth, advances the selected
 * record, and sets the result to 2 when the sum is below the threshold at the
 * value field `+0x04` of the new record.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 * The original uses no floating-point instruction, so the module needs no FPU
 * stub.
 */
extern unsigned char DAT_00118e1c;
extern unsigned char DAT_00118e1d;
extern int DAT_00120804;
extern int DAT_001203dc;
extern int DAT_001203e0;
extern int DAT_0011d270;
extern int DAT_0011d268;
extern int DAT_001021f0[];
extern int DAT_001203e8[];
extern int DAT_00120400[];
extern int DAT_00120418[];

struct champ_record {
    int type;
    int value;
    int title_index;
    int group;
    int field_10;
    int field_14;
    char name[12];
    int field_24;
    int font;
};

extern struct champ_record DAT_001022bc[];

void championship_result_apply(void)
{
    int group;
    int next;
    int index;
    int sum;

    if ((DAT_00118e1c & 0x20) != 0) {
        return;
    }
    DAT_00120804 = (DAT_00118e1d & 0x10) != 0 ? 1 : 0;

    group = DAT_001022bc[DAT_001203dc].field_24;
    DAT_00120400[group] = DAT_0011d270;
    DAT_001203e8[group] = DAT_0011d268;
    DAT_00120418[group] = DAT_001021f0[DAT_0011d270];

    sum = 0;
    for (index = 0; index <= group; index = index + 1) {
        sum = sum + DAT_00120418[index];
    }
    for (index = group + 1; index < 6; index = index + 1) {
        sum = sum + 9;
    }

    next = DAT_001203dc + 1;
    DAT_001203e0 = next;
    DAT_001203dc = next;
    if (sum < DAT_001022bc[next].value) {
        DAT_00120804 = 2;
    }
}
