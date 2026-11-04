/* The rebuilt `net_race_lobby` module.
 *
 * The original sits at linear 0x2d7c4 in STARTH.EXE.  The first-return body
 * holds 198 instructions and 690 bytes up to the `ret` at 0x2da75; the body
 * continues through the case blocks to the `ret` at 0x2ddda, so the store
 * keeps the 1560-byte extent and sets no function size.
 *
 * The race state dispatcher `race_state_update` reaches this function from the
 * state entry that calls 0x2d7c4.  The function is the network race lobby.  It
 * reads the 0x38-byte player records at object-3 offset 0x2099c, shows the
 * local player and the remote players in two columns, and processes the
 * network events:
 *
 *     record +0x04   the event code
 *     record +0x08   the player index
 *     record +0x0c   the player name
 *     record +0x10   the icon cell
 *     record +0x14   the car index
 *     record +0x24   the player bit
 *     record +0x28   the 0x10-byte node id
 *
 * The event codes are 0 (join), 1 (leave), 2 (start), 3 (exit), 0x8844 (the
 * chosen player), and 0x1234 (quit).  The host picks the chosen player from the
 * frame counter `DAT_000007fd`, calls `net_player_add` with 0x8844, and stores
 * the choice in `DAT_00120b5c`.  A client waits for the 0x8844 event.
 *
 * A local player that is not the chosen player runs the loading transition: it
 * maps the palette at scale 0, loads `menu3h.pcx` from `gfx\graf.dat` into the
 * pixel buffer `DAT_0011fad0` with the palette at 0xcf5c, copies 0x3e800 bytes
 * to the video buffer `DAT_0021528`, sets the palette, and sets the active flag
 * `DAT_00120994`.  The active mode then shows the chosen player and waits for
 * the events.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
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
    char node[16];
};

extern struct net_player DAT_0012099c[];
extern int DAT_00120994;
extern int DAT_00120b60;
extern int DAT_00120b5c;
extern int DAT_001203c8;
extern int DAT_0011d224;
extern int DAT_000007fd;
extern char *DAT_0011fc34;
extern char *DAT_0011fc40;
extern char *DAT_0011fc44;
extern char *DAT_0011fc4c;
extern char *DAT_0011fc50;
extern void *DAT_0011fad0;
extern void *DAT_0021528;

extern void panel_draw_centered(char *text, int y, int font, int color);
extern void text_draw(int x, int y, char *text, int font, int color);
extern void panel_show(int x, int y, int data);
extern int net_receive(void *dest, int count, unsigned char **out);
extern void net_copy(unsigned char *source, unsigned char *dest, int count);
extern void net_queue_add(void *node);
extern void net_queue_sort(void);
extern int net_node_index(void *node);
extern void net_queue_remove(int index);
extern void net_poll(void);
extern int net_count(void);
extern int net_local_index(void);
extern int net_player_add(int id);
extern void race_state_begin(int state);
extern void video_present(int scale, int offset);
extern void video_fill(void);
extern int pcx_load(char *file, char *member, void *pixels, char *palette);
extern void cleanup(void);
extern void *memcpy(void *dest, const void *source, unsigned int count);
extern void palette_set(char *palette);
extern int table_lookup(int index);
extern void wait_for_input(void);
extern void menu_transition(void);
extern void net_lobby_exit(void);
extern int __cdecl sprintf(char *buffer, const char *format, ...);

static int node_equals(char *left, char *right)
{
    int index;

    for (index = 0; index < 16; ++index) {
        if (left[index] != right[index]) {
            return 0;
        }
    }
    return 1;
}

static void record_store(struct net_player *target, unsigned char *source)
{
    int index;
    unsigned char *dest;

    dest = (unsigned char *)target;
    for (index = 0; index < 0x38; ++index) {
        dest[index] = source[index];
    }
}

void net_race_lobby(void)
{
    struct net_player *players;
    unsigned char message[0x38];
    unsigned char *source;
    char buffer[0x30];
    int count;
    int local_index;
    int selected;
    int saved;
    int index;
    int step;
    int flag;
    int x;
    int y;
    int display;
    int event;

    players = DAT_0012099c;

    if (DAT_00120994 != 0) {
        goto active;
    }

    panel_draw_centered(DAT_0011fc40, 4, 2, 2);
    panel_draw_centered(DAT_0011fc4c, 0x14, 0, 2);
    count = net_count();
    local_index = net_local_index();

loop:
    if (net_receive(message, 0xc, &source) == 0) {
        goto no_message;
    }
    net_copy(source, message + 0xc, 0x2c);
    event = *(int *)(message + 4);
    if (event == 0) {
        goto event0;
    }
    if (event == 1) {
        goto event1;
    }
    if (event == 2) {
        goto event2;
    }
    if (event == 3) {
        goto event3;
    }
    if (event == 0x1234) {
        goto teardown;
    }
    net_poll();
    goto loop;

event0:
    net_queue_add(message + 0x28);
    net_queue_sort();
    index = net_node_index(message + 0x28);
    if (!node_equals(players[local_index].node, (char *)message + 0x28)) {
        record_store(&players[index], message);
    }
    net_poll();
    goto loop;

event1:
    index = net_node_index(message + 0x28);
    net_queue_remove(index);
    net_queue_sort();
    net_poll();
    goto loop;

event2:
    if (DAT_00120b60 == 0) {
        goto teardown;
    }
    if (local_index == 0) {
        selected = DAT_000007fd & 7;
        if (selected >= count) {
            selected = count - ((selected & 1) + 1);
        }
        saved = DAT_001203c8;
        DAT_001203c8 = selected;
        net_player_add(0x8844);
        DAT_001203c8 = saved;
        DAT_00120b5c = selected;
    } else {
        flag = 1;
        do {
            net_poll();
            if (flag == 0) {
                break;
            }
            if (net_receive(message, 0xc, &source) != 0) {
                net_copy(source, message + 0xc, 0x2c);
                if (*(int *)(message + 4) == 0x8844) {
                    flag = 0;
                    DAT_00120b5c = *(int *)(message + 0x14);
                }
            }
        } while (1);
    }
    if (local_index != DAT_00120b5c) {
        goto start_race;
    }
    race_state_begin(3);
    DAT_0011d224 = 0;
    net_poll();
    goto loop;

start_race:
    video_present(0, 0);
    video_fill();
    if (pcx_load((char *)0x12b1, (char *)0x12a6, DAT_0011fad0,
                 (char *)0xcf5c) != 0) {
        cleanup();
    }
    memcpy(DAT_0021528, DAT_0011fad0, 0x3e800);
    palette_set((char *)0xcf5c);
    video_fill();
    DAT_00120994 = 1;
    table_lookup(7);
    wait_for_input();
    net_poll();
    goto loop;

event3:
    if (count > 1) {
        goto loop;
    }
    net_lobby_exit();
    return;

no_message:
    count = net_count();
    local_index = net_local_index();
    DAT_00120b60 = 0;
    for (step = 0; step < count; ++step) {
        DAT_00120b60 |= players[step].field_24;
    }
    if (count > 1) {
        panel_draw_centered(DAT_0011fc50, 0xba, 0, 2);
    }
    y = 0x34;
    x = 0xc;
    sprintf(buffer, (char *)0x12be, players[local_index].name);
    text_draw(x * 2, y * 2, buffer, 0, 2);
    panel_show(x + 0x21, y - 0xf, players[local_index].field_10 + 0x10);
    x += 0x9c;
    display = 0;
    step = 0;
    while (step < count) {
        if (local_index == step) {
            ++step;
            continue;
        }
        sprintf(buffer, (char *)0x12c1, players[step].name);
        text_draw(x * 2, y * 2, buffer, 0, 2);
        panel_show(x + 0x21, y - 0xf, players[step].field_10 + 0x10);
        if (display & 1) {
            x += 0x9c;
        } else {
            y += 0x26;
            x -= 0x9c;
        }
        ++display;
        ++step;
    }
    return;

active:
    saved = DAT_001203c8;
    DAT_001203c8 = DAT_00120b5c;
    net_player_add(0x8844);
    DAT_001203c8 = saved;

active_wait:
    if (net_receive(message, 0xc, &source) == 0) {
        goto active_draw;
    }
    net_copy(source, message + 0xc, 0x2c);
    if (*(int *)(message + 8) != DAT_00120b5c) {
        net_poll();
        goto active_wait;
    }
    event = *(int *)(message + 4);
    if (event == 1) {
        goto teardown;
    }
    if (event == 3) {
        record_store(&players[DAT_00120b5c], message);
        net_poll();
        goto active_wait;
    }
    if (event == 4) {
        menu_transition();
        goto active_draw;
    }
    net_poll();
    goto active_wait;

active_draw:
    panel_draw_centered(DAT_0011fc34, 2, 3, 2);
    sprintf(buffer, (char *)0x12c4, players[DAT_00120b5c].name);
    panel_draw_centered(buffer, 0x50, 2, 2);
    panel_draw_centered(DAT_0011fc44, 0x6e, 2, 2);
    sprintf(buffer, (char *)0x12c7,
            (char *)0x1d908 + players[DAT_00120b5c].field_14 * 0x34 + 0x14);
    panel_draw_centered(buffer, 0x96, 3, 2);
    return;

teardown:
    net_lobby_exit();
    return;
}
