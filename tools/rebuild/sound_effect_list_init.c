/* The rebuilt `sound_effect_list_init` module.
 *
 * The original sits at linear 0x24600 in STARTH.EXE and holds 284 bytes from
 * 0x24600 to the `ret` at 0x2471a.  The first basic block ends at the `ret` at
 * 0x24664, so `le.function_body` reports 35 instructions and the plan counts
 * them.  The out-of-line continuation at 0x24665 runs on the common epilogue
 * at 0x24710.  The next function starts at 0x2471c, so the store record keeps
 * the 284-byte extent and sets no separate function size.  The identity window
 * uses the first 35 instructions.
 *
 * The function loads the player sound effect lists into the 0x24-byte sample
 * records at DAT_0015224.  It returns at once when the state at DAT_0001d2c8
 * is zero.  It probes the memory through `memory_available` at 0x2f8ca:
 *
 *     1  when the memory is below 3000000 and the mode at DAT_0001d260 is 4,
 *        the function clears the state and returns
 *     2  when the memory is below 400000, the function clears the state and
 *        returns
 *
 * The function saves the record index at DAT_00018d84 to DAT_0001906c, loads
 * the list SOUND_FX member SPFXBASE of the type GEN into the record at the
 * index, and stores the row count at DAT_00019068.  When the memory is above
 * 1900000, it loads the member SPFXFULL after the first rows, sets the full
 * flag DAT_00019064 to 1, and adds the second row count.  Otherwise it clears
 * the flag.  It then adds the total row count to the index DAT_00018d84.
 *
 * scene_setup at 0x15984 calls the function at 0x15a5f after it loads the
 * SFXSBASE and level lists, and sound_variant_select at 0x24a84 reads the
 * full flag and the row count.  The list text sits at object-3 offsets 0xe78
 * through 0xea0.  The original has the EBP frame, so the module builds with
 * `-of+`.  The function holds no floating-point instruction, so the module
 * needs no FPU stub, and the module uses no assembly.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness build defines `SOUND_EFFECT_LIST_INIT_HARNESS`, so the function
 * builds the constant text in local buffers.  The default build keeps the
 * literals of the original for the later period build.
 */
extern int DAT_00018d84;
extern int DAT_00019064;
extern int DAT_00019068;
extern int DAT_0001906c;
extern int DAT_0001d260;
extern int DAT_0001d2c8;
extern void *DAT_000062ac;
extern char DAT_0015224[];

extern unsigned int memory_available(void *pool);
extern int list_file_load(char *type, char *name, char *list, void *table,
                          int stride, int count);

#ifdef SOUND_EFFECT_LIST_INIT_HARNESS
static void fill(char *text, char a, char b, char c, char d,
                 char e, char f, char g, char h)
{
    text[0] = a;
    text[1] = b;
    text[2] = c;
    text[3] = d;
    text[4] = e;
    text[5] = f;
    text[6] = g;
    text[7] = h;
    text[8] = 0;
}
#endif

void sound_effect_list_init(void)
{
#ifdef SOUND_EFFECT_LIST_INIT_HARNESS
    char type_text[0x10];
    char base_text[0x10];
    char full_text[0x10];
    char list_text[0x10];
#else
    char *type_text = "GEN";
    char *base_text = "SPFXBASE";
    char *full_text = "SPFXFULL";
    char *list_text = "SOUND_FX";
#endif
    int start;
    int count;

#ifdef SOUND_EFFECT_LIST_INIT_HARNESS
    fill(type_text, 'G', 'E', 'N', 0, 0, 0, 0, 0);
    fill(base_text, 'S', 'P', 'F', 'X', 'B', 'A', 'S', 'E');
    fill(full_text, 'S', 'P', 'F', 'X', 'F', 'U', 'L', 'L');
    fill(list_text, 'S', 'O', 'U', 'N', 'D', '_', 'F', 'X');
#endif

    if (DAT_0001d2c8 == 0) {
        return;
    }
    if ((int)memory_available(DAT_000062ac) < 3000000
        && DAT_0001d260 == 4) {
        DAT_0001d2c8 = 0;
        return;
    }
    if ((int)memory_available(DAT_000062ac) < 400000) {
        DAT_0001d2c8 = 0;
        return;
    }
    start = DAT_00018d84;
    DAT_0001906c = start;
    count = list_file_load(type_text, base_text, list_text,
                           DAT_0015224 + start * 0x24, 0x24, 0x24);
    if ((int)memory_available(DAT_000062ac) > 1900000) {
        DAT_00019064 = 1;
        count = count + list_file_load(
            type_text, full_text, list_text,
            DAT_0015224 + (DAT_00018d84 + count) * 0x24, 0x24, 0x24);
    } else {
        DAT_00019064 = 0;
    }
    DAT_00019068 = count;
    DAT_00018d84 = DAT_00018d84 + count;
}
