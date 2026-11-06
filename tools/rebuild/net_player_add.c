/* The rebuilt `net_player_add` module.
 *
 * The original sits at linear 0x2dddc in STARTH.EXE and holds 111 instructions
 * and 394 bytes up to the `ret` at `0x2df65`: the six register saves, the
 * frame, the two local slots, the slot call at `0xc6fbc`, the record index
 * stores, the inline name copy, the four field stores, the two pointer walks,
 * the default copy at `0xc7100`, the register call at `0xc7478`, and the
 * epilogue.  The merged store record keeps the 396-byte extent.
 *
 * The function takes one argument in `eax` and returns the record index.  The
 * slot call returns the index into the 0x38-byte record array at object-3
 * offset 0x2099c.  The record keeps the argument at `+0x04`, the index at
 * `+0x08`, the name at `+0x0c`, the two cells at `+0x10` and `+0x14`, the two
 * values at `+0x18` and `+0x1c`, the two cells at `+0x20` and `+0x24`, and the
 * 0x10-byte block at `+0x28`.
 *
 * The name copy uses the `-oi` option, so `strcpy` expands into the same
 * two-byte copy loop as the original.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
#include <string.h>

struct net_player {
    int field_00;
    int field_04;
    int field_08;
    char name[4];
    int field_10;
    int field_14;
    int field_18;
    int field_1c;
    int field_20;
    int field_24;
    int field_28;
    int field_2c;
    int field_30;
    int field_34;
};

extern int DAT_001203c4;
extern int DAT_001203c8;
extern char DAT_001203cc[];
extern int *DAT_00102dec;
extern int DAT_001021e4[];
extern int DAT_0011d274;
extern int DAT_0011d230;
extern struct net_player DAT_0012099c[];
extern int net_player_slot(void);
extern int __cdecl net_player_default(void *block);
extern int __cdecl net_player_register(void *record, int size, char *name,
                                        int value, int count);

int net_player_add(int id)
{
    int index;

    index = net_player_slot();
    DAT_0012099c[index].field_04 = id;
    DAT_0012099c[index].field_08 = index;
    strcpy(DAT_0012099c[index].name, DAT_001203cc);
    DAT_0012099c[index].field_10 = DAT_001203c4;
    DAT_0012099c[index].field_14 = DAT_001203c8;
    DAT_0012099c[index].field_1c = *(int *)((char *)DAT_00102dec + 0x18);
    DAT_0012099c[index].field_18 =
        DAT_001021e4[*(int *)((char *)DAT_00102dec + 0x28)];
    DAT_0012099c[index].field_20 = DAT_0011d274;
    DAT_0012099c[index].field_24 = DAT_0011d230;
    net_player_default(&DAT_0012099c[index].field_28);
    net_player_register(&DAT_0012099c[index], 0xc,
                        DAT_0012099c[index].name, 0x74, 9);
    return index;
}
