/* The rebuilt `options_screen` module.
 *
 * The original sits at linear 0x21338 in STARTH.EXE and holds 464 instructions
 * and 1715 bytes up to the `ret` at 0x219ea.  The record keeps the 1716-byte
 * extent and sets no function size.
 *
 * The only direct caller is `input_map` at 0x211e0.  It calls this function
 * when the input flag `DAT_0000001b` is set and the mode `DAT_001d260` is not
 * 0x10.  The data references name this function as the options screen: the
 * three labels at 0x1f74 are `CD`, `FX`, and `MU`, and the three pointers at
 * 0x1f80 select the audio volume slots 0x1d2bc, 0x1d2c0, and 0x1d2c4.  The
 * function draws the screen, fades the audio volumes, waits for the input, and
 * exits on the escape key `DAT_00000003`.
 *
 * The state table at 0x1f20 holds seven records of three dwords: the title x,
 * the static y, and the moving y.  The states 0 through 3 select the menu
 * rows, and the states 4 through 6 select the three audio volume rows.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * compiler converts the float values to integers through the Watcom helper at
 * 0x4d9f4, which truncates toward zero.  The local assembly stub makes
 * `__CHP` truncate in the same way.  The harness sets the four float constants
 * from their bit patterns, because the minimal image does not load the
 * constant segment.  The three labels are extern, so the harness can check the
 * selected label.
 */
extern void rect_fill(int x, int y, int width, int height);
extern void menu_state_reset(int state, int value);
extern void sound_marker(void);
extern void music_play(void *song);
extern void view_move(int a, int b, int c);
extern void default_data_copy(void);
extern void sprite_blit(void *source, void *dest);
extern void input_clear(int key);
extern void menu_list_begin(void);
extern int sprintf(char *buffer, const char *format, ...);
extern void text_draw(int x, int y, char *text, int font, int color);
extern void sound_driver_set(int value);
extern void menu_poll(void);
extern void sound_slots_update(void);
extern void volume_bar_draw(int value, char *label);
extern void video_fill(void);
extern void input_apply(int value, int flag);
extern void input_apply_release(void);

extern unsigned char DAT_00000003;
extern unsigned char DAT_0000001e;
extern unsigned char DAT_0000004d;
extern unsigned char DAT_0000004f;
extern unsigned char DAT_000007fd;
extern unsigned int DAT_000007f9;
extern int DAT_0016684;
extern int DAT_0016688;
extern int DAT_001668e;
extern int DAT_0016692;
extern int DAT_0016696;
extern int DAT_001669a;
extern void *DAT_0018f14;
extern void *DAT_0018fd4;
extern void *DAT_0018fd8;
extern void *DAT_0018fdc;
extern int DAT_001cb18;
extern int DAT_001cb1c;
extern int DAT_001cb20;
extern int DAT_001d260;
extern int DAT_001d2bc;
extern int DAT_001d2c0;
extern int DAT_001d2c4;
extern int DAT_001d2cc;
extern void *DAT_0021528;
extern char options_label_cd[];
extern char options_label_fx[];
extern char options_label_mu[];
extern float options_fade_rate;
extern float options_rise_rate;
extern float options_min_delta;
extern double options_time_scale;

static int options_pos[7][3] = {
    {0x3b, 0x07, 0x0b},
    {0x53, 0x08, 0x0c},
    {0x71, 0x09, 0x0d},
    {0x89, 0x0a, 0x0e},
    {0xa7, 0x39, 0x3b},
    {0xa7, 0x3a, 0x3c},
    {0xd9, 0x3d, 0x3e}
};

static char options_format[] = "\003%d";

static int *options_volume_ptr(int index)
{
    if (index == 0) {
        return &DAT_001d2c0;
    }
    if (index == 1) {
        return &DAT_001d2bc;
    }
    return &DAT_001d2c4;
}

static char *options_label_ptr(int index)
{
    if (index == 0) {
        return options_label_cd;
    }
    if (index == 1) {
        return options_label_fx;
    }
    return options_label_mu;
}

void options_screen(void)
{
    char buffer[32];
    int music_started;
    int sound_started;
    int volume_index;
    int state;
    int sub_state;
    int exit_flag;
    int selection;
    int animation;
    int count;
    int y;
    int y_offset;
    int index;
    int list_exit;
    int volume0;
    int volume1;
    int volume2;
    int *volume;
    float delta;
    float timer;
    float scroll;

    rect_fill(0, 0, 0x280, 0x190);
    rect_fill(0, 0, 0x280, 0x190);
    music_started = 0;
    sound_started = 0;
    volume_index = 0;
    scroll = 0.0f;
    state = (int)scroll;
    sub_state = state;
    exit_flag = state;
    selection = DAT_0016696;
    if (selection == 0 || selection > DAT_0016692) {
        selection = DAT_001668e;
    }
    DAT_000007f9 = 0;
    delta = 0.0f;
    volume0 = DAT_001d2bc;
    volume1 = DAT_001d2c0;
    volume2 = DAT_001d2c4;
    timer = 0.0f;
    list_exit = 0;

    for (;;) {
        if (exit_flag != 0) {
            break;
        }
        if ((DAT_001d260 & 0x10) != 0) {
            timer = timer + delta;
            if (timer >= 1.0f) {
                menu_state_reset(3, 0);
                timer = 0.0f;
            }
        }
        volume0 = (int)((float)volume0 - delta * options_fade_rate);
        if (volume0 <= 0) {
            volume0 = 0;
        }
        volume1 = (int)((float)volume1 - delta * options_fade_rate);
        if (volume1 <= 0) {
            volume1 = 0;
            if (sound_started == 0) {
                sound_marker();
                sound_started = 1;
            }
        }
        volume2 = (int)((float)volume2 - delta * options_fade_rate);
        if (volume2 <= 0) {
            volume2 = 0;
            if (music_started == 0) {
                music_play((void *)0x62b0);
                music_started = 1;
            }
        }
        view_move(volume1, volume0, volume2);
        default_data_copy();
        sprite_blit(DAT_0018f14, DAT_0021528);
        if (DAT_0016688 != 0) {
            DAT_001668e = 0;
            DAT_0016692 = 0;
            count = 0;
        }
        animation = 0;
        if (DAT_0000004d != 0 && state != 0 && sub_state < 2) {
            state = state - 1;
            scroll = 0.0f;
            sub_state = 0;
            input_clear(0x4b);
        }
        if (DAT_0000004f != 0 && state < 6 && sub_state < 2) {
            state = state + 1;
            scroll = 0.0f;
            sub_state = 0;
            input_clear(0x4d);
        }
        if (DAT_00000003 != 0) {
            input_clear(1);
            break;
        }
        if (DAT_0000001e != 0 && sub_state < 2) {
            input_clear(0x1c);
            scroll = 0.01f;
            if (state == 3 && selection < DAT_0016692) {
                list_exit = 0;
                selection = selection + 1;
            }
            if (state == 2 && selection > DAT_001668e) {
                list_exit = 0;
                selection = selection - 1;
            }
            if (state == 0 && DAT_0016688 == 0 && count != 0) {
                break;
            }
            if (state == 1) {
                sub_state = 1;
            }
            if (state == 4) {
                animation = 1;
                volume = options_volume_ptr(volume_index);
                *volume = (int)((float)*volume - delta * options_rise_rate);
                if (*volume < 0) {
                    *volume = 0;
                }
            }
            if (state == 5) {
                animation = 1;
                volume = options_volume_ptr(volume_index);
                *volume = (int)((float)*volume + delta * options_rise_rate);
                if (*volume > 0x7fff) {
                    *volume = 0x7fff;
                }
            }
            if (state == 6) {
                animation = 1;
                volume_index = volume_index + 1;
                if (volume_index == 3) {
                    volume_index = 0;
                }
            }
        }
        y = 0;
        y_offset = 0;
        count = DAT_0016692 - DAT_001668e;
        if (selection > DAT_0016692) {
            selection = DAT_0016692;
        }
        if (selection < DAT_001668e) {
            selection = DAT_001668e;
        }
        if (DAT_0016684 != 0 && (sub_state == 0 || sub_state == 1)) {
            if (count != 0) {
                index = 0;
                menu_list_begin();
                while (count + 1 > index) {
                    if (index == 10 && y == 0) {
                        y = 0x0e;
                        y_offset = 0;
                    }
                    sprintf(buffer, options_format, index + DAT_001668e);
                    if (index + DAT_001668e == selection
                        && (DAT_000007fd & 0x20) != 0) {
                        index = index;
                    } else {
                        text_draw(y_offset + 0x80, y + 0x9c, buffer, 5, 9);
                    }
                    y_offset = y_offset + 0x1a;
                    index = index + 1;
                }
            } else {
                if (sub_state == 0) {
                    if (DAT_001669a != 0) {
                        sound_driver_set(0);
                    }
                    menu_poll();
                    if (state <= 3) {
                        text_draw(0x80, 0x8c, DAT_0018fd4, 5, 3);
                    }
                }
            }
        }
        if (DAT_0016684 != 0) {
            if (sub_state == 1) {
                menu_poll();
                if (DAT_001669a != 0) {
                    sound_driver_set(0);
                    menu_poll();
                }
                if (DAT_0016688 == 0) {
                    text_draw(0x80, 0x8c, DAT_0018fd8, 5, 3);
                    goto draw_state;
                }
                list_exit = 0;
                sub_state = sub_state + 1;
                goto draw_state;
            }
            if (sub_state == 2) {
                menu_poll();
                if (state <= 3 && DAT_0016688 != 0) {
                    text_draw(0x80, 0x8c, DAT_0018fdc, 5, 3);
                    goto draw_state;
                }
                if (DAT_0016688 == 0 && DAT_001669a == 0) {
                    sound_driver_set(1);
                }
                sub_state = 0;
                goto draw_state;
            }
        }

draw_state:
        DAT_001cb1c = 0xc2;
        DAT_001cb18 = options_pos[state][0] * 2;
        if (animation != 0 || (*(unsigned int *)&scroll & 0x7fffffff) != 0) {
            DAT_001cb20 = options_pos[state][2];
        } else {
            DAT_001cb20 = options_pos[state][1];
        }
        if ((*(unsigned int *)&scroll & 0x7fffffff) != 0) {
            scroll = scroll + delta;
            if (scroll > 1.0f) {
                scroll = 0.0f;
            }
        }
        sound_slots_update();
        if (state > 3) {
            volume_bar_draw(*options_volume_ptr(volume_index),
                options_label_ptr(volume_index));
        }
        video_fill();
        delta = (float)((double)DAT_000007f9 * options_time_scale);
        if (delta < options_min_delta) {
            delta = 0.01f;
        }
        DAT_000007f9 = 0;
    }

    if (DAT_001d2cc != 0) {
        if (list_exit == 0) {
            input_apply(selection, 1);
        } else {
            input_apply_release();
        }
    }
    view_move(DAT_001d2c0, DAT_001d2bc, DAT_001d2c4);
    default_data_copy();
}
