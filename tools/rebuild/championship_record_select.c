/* The rebuilt `championship_record_select` module.
 *
 * The original sits at linear 0x293a8 in STARTH.EXE and holds 78 instructions
 * and 288 bytes up to the `ret` at 0x294c7.  The next function starts at
 * 0x294c8, so the store record keeps the 288-byte extent and sets no separate
 * function size.  The identity check uses 78 instructions.
 *
 * The only direct caller at 0x292ae is the `net_race_screen` case for record
 * type 3 at 0x28ba4.  The type 3 record is the standings page that
 * `championship_screen` at 0x2cf68 draws.  The case calls this function and
 * then enters the state through `race_state_begin` at 0x294c8 with the value 5,
 * so the module takes the section `race_frame callees`.
 *
 * The selected record `DAT_001203dc` picks one 0x2c-byte record from the table
 * at object-3 offset 0x22bc.  The record field `+0x04` is the value, `+0x08`
 * is the title index, `+0x10` is the win record, `+0x14` is the lose record,
 * and `+0x24` is the standings row.
 *
 * The function reads the standings car value `DAT_00120400[row]` and the
 * result flag `DAT_00120804`.  A car value below 4 with a clear flag takes the
 * win path when the title index is below 3 or the car value is 1.
 *
 * On the win path a row other than 5 stores the old record in
 * `DAT_001203e0` and selects the win record.  Row 5 with the point sum
 * `DAT_001203e4` at or above the value selects the win record in both slots
 * and clears the six standings slots to -1.  Every other path selects the lose
 * record in `DAT_001203dc` and stores the record before the old one in
 * `DAT_001203e0`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * original uses no floating-point instruction, so the module needs no FPU
 * stub.
 */
extern int DAT_001203dc;
extern int DAT_001203e0;
extern int DAT_001203e4;
extern int DAT_00120400[6];
extern int DAT_00120804;

struct champ_record {
    int type;
    int value;
    int title_index;
    int group;
    int win_next;
    int lose_next;
    char name[12];
    int row;
    int font;
};

extern struct champ_record DAT_001022bc[];

void championship_record_select(void)
{
    struct champ_record *record;
    int current;
    int car;
    int next;
    int index;

    current = DAT_001203dc;
    record = &DAT_001022bc[current];
    car = DAT_00120400[record->row];
    if (car < 4 && DAT_00120804 == 0
            && (record->title_index < 3 || car == 1)) {
        if (record->row == 5) {
            if (DAT_001203e4 >= record->value) {
                next = record->win_next;
                DAT_001203dc = next;
                DAT_001203e0 = next;
                for (index = 0; index < 6; index = index + 1) {
                    DAT_00120400[index] = -1;
                }
                return;
            }
        } else {
            DAT_001203e0 = current;
            DAT_001203dc = record->win_next;
            return;
        }
    }
    DAT_001203dc = record->lose_next;
    DAT_001203e0 = current - 1;
}
