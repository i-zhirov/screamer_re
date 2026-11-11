/* The rebuilt `sprite_entry_draw` module.
 *
 * The original sits at linear 0x25b10 in STARTH.EXE.  The plan row records a
 * 70-byte body of 22 instructions up to the first `ret` at 0x25b55.  The
 * function continues through five early exits and ends at the `ret` at
 * 0x25ccb, so the body holds 126 instructions and 444 bytes.  The next record
 * starts at 0x25d28, so the store record keeps the 536-byte extent and sets no
 * separate function size.  The identity check uses 22 instructions.
 *
 * The only direct caller is the sprite-list flush at 0x25af0.  It walks the
 * 0x28-byte entries of the list at `DAT_0015cd0` and calls this function for
 * every entry with bit 0 of the first dword set.  An entry holds the active
 * flag at `+0x00`, the x at `+0x04`, the y at `+0x08`, the sprite record index
 * at `+0x0c`, and a reserved cell at `+0x10`.  The sprite record at
 * `DAT_0015ccc + index * 0x14` holds the x offset, the y offset, the width, the
 * height, and the data offset in that order.
 *
 * The function clips the sprite rectangle against the rectangle
 * `DAT_0015ce4` through `DAT_0015cf0` and returns at once when the sprite
 * leaves the clip rectangle.  It reads the one-byte-per-pixel data at
 * `DAT_0015ce0` from the data offset of the record, adds the two clip skips,
 * and copies the visible pixels into the framebuffer at `DAT_0021528`.  A zero
 * source byte leaves the destination byte unchanged.  The function also stores
 * the absolute destination pointer at `+0x14`, the framebuffer offset at
 * `+0x18`, the visible width at `+0x1c`, the visible height at `+0x20`, and the
 * negated visible width at `+0x24` of the entry.
 *
 * The scratch cells 0x15cf4 through 0x15d18 hold the clip skips, the edges, the
 * rectangle, and the row stride.  Only this function reads and writes them, so
 * the rebuilt function keeps them in locals.
 *
 * The original uses `pushal` and `popal` and has no EBP frame, so the module
 * builds without `-of+`.  The function holds no floating-point instruction,
 * calls no function, and needs no FPU stub and no assembly.
 */
struct sprite_record {
    int x_offset;
    int y_offset;
    int width;
    int height;
    int data_offset;
};

struct active_entry {
    int active;
    int x;
    int y;
    int sprite;
    int reserved;
    int dest;
    int dest_offset;
    int clip_width;
    int clip_height;
    int clip_pitch;
};

extern struct sprite_record *DAT_0015ccc;
extern unsigned char *DAT_0015ce0;
extern int DAT_0015ce4;
extern int DAT_0015ce8;
extern int DAT_0015cec;
extern int DAT_0015cf0;
extern unsigned char *DAT_0021528;

void sprite_entry_draw(struct active_entry *entry)
{
    struct sprite_record *sprite;
    unsigned char *source;
    unsigned char *dest;
    int x1;
    int y1;
    int left;
    int right;
    int top;
    int bottom;
    int amount;
    int width;
    int height;
    int source_skip;
    int source_down;
    int source_remainder;
    int row_stride;
    int visible_width;
    int rows;
    int columns;
    int offset;

    sprite = DAT_0015ccc + entry->sprite;
    source_skip = 0;
    source_remainder = 0;
    x1 = entry->x + sprite->x_offset;
    if (x1 < DAT_0015ce4) {
        amount = DAT_0015ce4 - x1;
        width = sprite->width;
        if (width - amount <= 0) {
            return;
        }
        source_skip = amount;
        right = (width - amount) + DAT_0015ce4;
        source_remainder = amount;
        left = DAT_0015ce4;
    } else {
        left = x1;
        width = sprite->width;
        right = x1 + width;
        if (right > DAT_0015ce8) {
            if (x1 >= DAT_0015ce8) {
                return;
            }
            source_remainder = right - DAT_0015ce8;
            right = DAT_0015ce8;
        }
    }

    source_down = 0;
    y1 = entry->y + sprite->y_offset;
    if (y1 < DAT_0015cec) {
        amount = DAT_0015cec - y1;
        source_down = amount;
        height = sprite->height;
        if (height - amount <= 0) {
            return;
        }
        top = DAT_0015cec;
        bottom = (height - amount) + DAT_0015cec;
    } else {
        top = y1;
        height = sprite->height;
        bottom = y1 + height;
        if (bottom > DAT_0015cf0) {
            if (y1 >= DAT_0015cf0) {
                return;
            }
            bottom = DAT_0015cf0;
        }
    }

    offset = top * 0x280 + left;
    dest = DAT_0021528 + offset;
    source = DAT_0015ce0 + sprite->data_offset
        + sprite->width * source_down + source_skip;
    visible_width = right - left;
    rows = bottom - top;

    entry->dest = (int)dest;
    entry->dest_offset = offset;
    entry->clip_width = visible_width;
    entry->clip_height = rows;
    entry->clip_pitch = source_remainder - sprite->width;
    row_stride = source_remainder - sprite->width + 0x280;

    do {
        columns = visible_width;
        do {
            unsigned char pixel = *source++;

            if (pixel != 0) {
                *dest = pixel;
            }
            dest++;
        } while (--columns > 0);
        source += source_remainder;
        dest += row_stride;
    } while (--rows > 0);
}
