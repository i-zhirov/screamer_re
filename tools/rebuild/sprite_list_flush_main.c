/* The test harness for the rebuilt `sprite_list_flush` module.
 *
 * The harness holds the two list globals, four sprite entries, and the
 * `sprite_entry_draw` stub.  The stub records the call count and the entry
 * pointer of every call.  The entry has the active flag at `+0x00`, the x at
 * `+0x04`, the y at `+0x08`, the sprite record index at `+0x0c`, and the
 * reserved cell at `+0x10`, the same offsets as the original.
 *
 * The harness runs these paths:
 *
 *     0  one entry with the active flag 1: one call with the first entry
 *     1  three entries with the flags 1, 0, and 3: two calls with entry 0 and 2
 *     2  two entries without the active flag: no call
 *     3  the flags 0xfe and 0x100: no call, because bit 0 is clear
 *     4  one entry with the flags 0x101: one call, because bit 0 is set
 *     D  the exit marker
 *
 * Every path checks the call count and, on a call, the forwarded entry pointer.
 * The pointer check also confirms the 0x28-byte entry stride.
 */
#pragma off (check_stack)

struct sprite_entry {
    unsigned long flags;
    int x;
    int y;
    int sprite;
    int reserved;
    int pad[5];
};

struct sprite_entry *DAT_0015cd0;
int DAT_0015cd8;

void sprite_list_flush(void);
void sprite_entry_draw(struct sprite_entry *entry);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static struct sprite_entry list[4];
static int draw_calls;
static struct sprite_entry *draw_entry[4];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void sprite_entry_draw(struct sprite_entry *entry)
{
    if (draw_calls < 4) {
        draw_entry[draw_calls] = entry;
    }
    draw_calls = draw_calls + 1;
}

static void reset(int count)
{
    int index;

    for (index = 0; index < 4; index++) {
        list[index].flags = 0;
    }
    draw_calls = 0;
    draw_entry[0] = 0;
    draw_entry[1] = 0;
    draw_entry[2] = 0;
    draw_entry[3] = 0;
    DAT_0015cd0 = list;
    DAT_0015cd8 = count;
}

int main(void)
{
    reset(1);
    list[0].flags = 1;
    sprite_list_flush();
    put(draw_calls == 1 && draw_entry[0] == &list[0] ? '0' : 'x');

    reset(3);
    list[0].flags = 1;
    list[1].flags = 0;
    list[2].flags = 3;
    sprite_list_flush();
    put(draw_calls == 2 && draw_entry[0] == &list[0]
        && draw_entry[1] == &list[2] ? '1' : 'x');

    reset(2);
    sprite_list_flush();
    put(draw_calls == 0 ? '2' : 'x');

    reset(2);
    list[0].flags = 0xfe;
    list[1].flags = 0x100;
    sprite_list_flush();
    put(draw_calls == 0 ? '3' : 'x');

    reset(1);
    list[0].flags = 0x101;
    sprite_list_flush();
    put(draw_calls == 1 && draw_entry[0] == &list[0] ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
