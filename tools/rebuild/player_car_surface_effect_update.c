/* The rebuilt `player_car_surface_effect_update` module.
 *
 * The original sits at linear 0x1814c in STARTH.EXE and holds 56 instructions
 * and 208 bytes up to the `ret` at 0x1821b.  The next function starts at
 * 0x1821c, so the store record keeps the 208-byte extent and sets no separate
 * function size.  The identity check uses 56 instructions.
 *
 * The only direct caller at 0x15bc8 is the player car step group at 0x15bac.
 * The group runs on the player object and calls the suspension update at
 * 0x168b4 and the skid update at 0x17d14 before this function, and the reset
 * update at 0x18528, the spin update at 0x188b8, the wheel effect update at
 * 0x19980, and the car sound update at 0x242bc after it.  No named function
 * sits between the group and the target, so the module takes the section
 * The vehicle state chain.
 *
 * The state at `+0x148` gates the surface selection.  When the state is zero,
 * the track node value at `+0x66c` selects the effect control word at `+0x7b0`
 * and the effect scale at object-1 offset 0xb26d8: 0 gives 0 and 0.002, 1 gives
 * 0x8000 and 0.03, and 2 gives 0xff00 and 0.03.  The cornering update at
 * 0x16ca0 reads the control word at `+0x7b0` and sets the effect state to 2
 * through 0x19478 only when the word is zero.
 *
 * When the node value at `+0x66c` differs from the previous value at `+0x58c`,
 * the function starts a sound through 0x1f764 with the handle at `+0x7cc` and
 * the sample index from the table at object-3 offset 0x1d7c.  The car index at
 * object-3 offset 0xff88 selects the table entry, and the race mode 0xc at
 * object-3 offset 0x1d2a0 selects the second pair.  The table holds the four
 * entries 3, 7, 10, and 11, so the race mode 0xc selects 10 or 11 instead of 3
 * or 7.
 *
 * The caller position, the track node value, the effect control word, the
 * effect scale, and the surface sound support the name
 * `player_car_surface_effect_update`.  The original has no symbol, so the
 * record keeps the name with the confidence inferred.
 *
 * The float scale lands as the literal bit pattern, so the module needs no FPU
 * instruction and no constant segment.  The original has the EBP frame, so the
 * module builds with `-of+`.  The module uses no assembly.
 */
extern float DAT_000c26d8;
extern int DAT_0011d2a0;
extern int DAT_0000ff88;
extern int DAT_00001d7c[];
extern void sound_sample_start(int handle, int sample);

void player_car_surface_effect_update(void *object)
{
    char *base;
    int node_value;
    int index;

    base = (char *)object;

    if (*(int *)(base + 0x148) == 0) {
        node_value = *(int *)(base + 0x66c);
        switch (node_value) {
        case 0:
            *(int *)(base + 0x7b0) = 0;
            *(int *)&DAT_000c26d8 = 0x3b03126f;
            break;
        case 1:
            *(int *)(base + 0x7b0) = 0x8000;
            *(int *)&DAT_000c26d8 = 0x3cf5c28f;
            break;
        case 2:
            *(int *)(base + 0x7b0) = 0xff00;
            *(int *)&DAT_000c26d8 = 0x3cf5c28f;
            break;
        default:
            break;
        }
    }

    if (*(int *)(base + 0x66c) != *(int *)(base + 0x58c)) {
        index = (DAT_0011d2a0 == 0xc) ? 1 : 0;
        sound_sample_start(
            *(int *)(base + 0x7cc),
            DAT_00001d7c[DAT_0000ff88 + index * 2]);
    }
}
