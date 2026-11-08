/* The rebuilt `text_draw` module.
 *
 * The original sits at linear 0x25020 in STARTH.EXE and holds 292 instructions
 * and 1044 bytes up to the `ret 4` at 0x25431.  The next record starts at
 * 0x254a8, so the store record keeps the 1160-byte extent and sets no separate
 * function size.  The identity check uses 292 instructions.
 *
 * The five arguments arrive in the Watcom register order: the screen x in eax,
 * the screen y in edx, the text in ebx, the font index in ecx, and the color on
 * the stack.  The color base is the color times sixteen, so each glyph pixel is
 * the four-bit glyph value or the color base.
 *
 * The font index selects a 12-byte descriptor from the table at DAT_00015438
 * with the height at +0x00, the first character at +0x04, and the character
 * count at +0x08.  It selects a 0x500-byte glyph row from the table at
 * DAT_00015434.  Each glyph entry holds the advance width in the first 32-bit
 * slot of 8 bytes and the bitmap offset in the second slot.  The bitmap base is
 * DAT_0001543c, and each row of a glyph holds `(width + 1) / 2` packed bytes.
 *
 * The clip rectangle is fixed at the right edge 0x280, the bottom edge
 * DAT_00015448, the top edge DAT_00015444, and the left edge DAT_0001544c.
 * The function returns 3 when the position is outside the clip rectangle or
 * above the glyph height, returns 2 when the font index is out of range, and
 * returns 0 after the text.
 *
 * The control bytes 1 and 2 clear and set the uniform mode, and the control
 * bytes 3 and 4 clear and set the widened mode.  The default uniform mode
 * carries the glyph in a cell as wide as the widest glyph plus two, and the
 * other mode uses the glyph width plus one, plus one more in the widened mode.
 *
 * The original keeps the draw state in the BSS cells at 0x154xx.  The C build
 * keeps the same state in locals.
 */

extern unsigned char *DAT_00021528;
extern unsigned char *DAT_00015434;
extern unsigned char *DAT_00015438;
extern unsigned char *DAT_0001543c;
extern int DAT_00015440;
extern int DAT_00015444;
extern int DAT_00015448;
extern int DAT_0001544c;

int text_draw(int x, int y, char *text, int font, int color)
{
    int color_base;
    int font_row;
    int rows_limit;
    int vertical_clip;
    int first;
    int count;
    int horizontal_clip;
    unsigned char *dest;
    int remaining;
    unsigned char *glyph_base;
    unsigned char *record;
    unsigned char *scan;
    int widest;
    int style1;
    int style2;
    char *text_ptr;
    int index;
    int character;
    int width;
    int width_plus1;
    int slack;
    int half;
    int clip;
    int clip_start;
    int skip;
    int adjusted;
    unsigned char *glyph_entry;
    unsigned char *source;
    unsigned char *row_start;
    unsigned char *row_dest;
    int column;
    int row_counter;
    unsigned int value;

    color_base = color << 4;

    if (y >= DAT_00015448) {
        return 3;
    }
    if (x >= 0x280) {
        return 3;
    }
    if ((unsigned int)font >= (unsigned int)DAT_00015440) {
        return 2;
    }

    font_row = font * 0x500;
    record = DAT_00015438 + font * 0xc;
    rows_limit = *(int *)record;
    if (DAT_00015448 - y < rows_limit) {
        rows_limit = DAT_00015448 - y;
    }

    vertical_clip = y - DAT_00015444;
    if (vertical_clip < 0) {
        vertical_clip = -vertical_clip;
        if (vertical_clip >= *(int *)record) {
            return 3;
        }
    } else {
        vertical_clip = 0;
    }

    first = *(int *)(record + 4);
    count = *(int *)(record + 8);

    horizontal_clip = x - DAT_0001544c;
    if (horizontal_clip < 0) {
        horizontal_clip = -horizontal_clip;
    } else {
        horizontal_clip = 0;
    }

    dest = DAT_00021528 + y * 0x280 + x;
    remaining = 0x280 - x;
    text_ptr = text;
    style1 = 1;
    style2 = 1;

    glyph_base = DAT_00015434 + font_row;
    widest = 1;
    if (count > 0) {
        scan = glyph_base;
        index = count;
        do {
            int candidate;

            candidate = *(int *)scan;
            if (widest <= candidate) {
                widest = candidate;
            }
            scan += 8;
            index--;
        } while (index != 0);
    }
    widest += 2;

    for (;;) {
        character = (unsigned char)*text_ptr;
        if (character == 0) {
            return 0;
        }

        if (character < first || character >= first + count) {
            if (character == 1) {
                style1 = 0;
            } else if (character == 2) {
                style1 = character;
            } else if (character == 3) {
                style2 = 0;
            } else if (character == 4) {
                style2 = character;
            }
        } else {
            glyph_entry = glyph_base + (character - first) * 8;
            width = *(int *)glyph_entry;
            clip_start = 0;
            clip = horizontal_clip;
            skip = 0;
            if (clip != 0) {
                if ((style1 & 0xf) != 0) {
                    adjusted = clip - width - 2;
                    if (adjusted >= 0) {
                        horizontal_clip = adjusted;
                        dest += width + 2;
                        remaining -= width + 2;
                        skip = 1;
                    } else {
                        clip_start = clip;
                        horizontal_clip = 0;
                    }
                } else {
                    adjusted = clip - widest;
                    if (adjusted >= 0) {
                        horizontal_clip = adjusted;
                        remaining -= widest;
                        dest += widest;
                        skip = 1;
                    } else {
                        clip_start = clip - ((widest - width - 1) >> 1);
                        horizontal_clip = 0;
                    }
                }
            }
            if (!skip) {
                width_plus1 = width + 1;
                slack = widest - width_plus1;
                half = slack >> 1;
                if ((style1 & 0xf) == 0) {
                    dest += half;
                }
                row_start = dest + vertical_clip * 0x280;
                source = DAT_0001543c + *(int *)(glyph_entry + 4);
                source += vertical_clip * (width_plus1 >> 1);
                row_counter = vertical_clip;
                do {
                    row_dest = row_start;
                    if ((style2 & 0xf) != 0) {
                        row_dest++;
                        remaining--;
                    }
                    column = 1;
                    for (;;) {
                        if (column < width_plus1) {
                            value = 0;
                            if (remaining > 0) {
                                value = *source;
                                if (column >= clip_start
                                    && (value & 0xf0) != 0) {
                                    *row_dest = (unsigned char)(
                                        ((value >> 4) | color_base) & 0xff);
                                }
                            }
                            column++;
                            remaining--;
                            if (remaining > 0 && column >= clip_start
                                && (value & 0x0f) != 0) {
                                *(row_dest + 1) = (unsigned char)(
                                    (value & 0x0f) | color_base);
                            }
                            column++;
                            row_dest += 2;
                            remaining--;
                            source++;
                        } else if (column > width_plus1) {
                            break;
                        } else {
                            column++;
                            row_dest++;
                            remaining--;
                        }
                    }
                    remaining += width;
                    remaining++;
                    if ((style2 & 0xf) != 0) {
                        remaining++;
                    }
                    row_start += 0x280;
                    row_counter++;
                } while (row_counter < rows_limit);

                remaining -= width;
                remaining--;
                if ((style2 & 0xf) != 0) {
                    remaining--;
                }
                if ((style1 & 0xf) != 0) {
                    dest += width_plus1;
                    if ((style2 & 0xf) != 0) {
                        dest++;
                    }
                } else {
                    remaining -= slack;
                    remaining++;
                    dest += (slack - half) + width_plus1;
                }
            }
        }
        text_ptr++;
    }
}
