/* The test harness for the rebuilt `sprite_entry_draw` module.
 *
 * The harness holds the sprite record table `DAT_0015ccc`, the pixel data
 * `DAT_0015ce0`, the framebuffer `DAT_0021528`, the clip rectangle
 * `DAT_0015ce4` through `DAT_0015cf0`, an eight-row framebuffer, and a pixel
 * buffer.  Every case refills the framebuffer with a fixed pattern, copies the
 * pattern to a reference buffer, runs the rebuilt function, and applies the
 * observed clip-and-copy rule to the reference buffer with a plain nested
 * loop.  The case passes when the whole framebuffer, sentinels included,
 * matches the reference.
 *
 * The sprite records:
 *
 *     0  the 4 by 3 sprite at the data offset 0 with no offset
 *     1  the 4 by 3 sprite at the data offset 0x20 with the offset -1, -2
 *     2  the 8 by 1 sprite at the data offset 0x40 with no offset
 *
 * The pixel data holds zero bytes, so the transparency rule is covered.
 *
 * The paths:
 *
 *     0  the sprite (2, 1): no clip and the entry outputs
 *     1  the sprite (-2, 0): the left clip of two pixels
 *     2  the sprite (0x27e, 0): the right clip of two pixels
 *     3  the sprite (2, -1): the top clip of one row
 *     4  the sprite (2, 4): the bottom clip of one row
 *     5  the sprite (0x400, 0): the whole sprite outside the clip rectangle
 *     6  the sprite (0, 0) with the offset -1, -2: the left and top clip
 *     7  the sprite (0x27e, 4): the right and bottom clip
 *     D  the exit marker
 */
#pragma off (check_stack)

#define STRIDE 0x280
#define ROWS 8
#define BUFFER_SIZE (STRIDE * ROWS + 16)

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

void sprite_entry_draw(struct active_entry *entry);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct sprite_record *DAT_0015ccc;
unsigned char *DAT_0015ce0;
int DAT_0015ce4;
int DAT_0015ce8;
int DAT_0015cec;
int DAT_0015cf0;
unsigned char *DAT_0021528;

static struct sprite_record sprites[3];
static unsigned char pixels[0x100];
static unsigned char framebuffer[BUFFER_SIZE];
static unsigned char reference[BUFFER_SIZE];
static struct active_entry last_entry;
static char text[8];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void fill_pattern(void)
{
    int index;

    for (index = 0; index < BUFFER_SIZE; index++) {
        framebuffer[index] = (unsigned char)(index * 7 + 3);
    }
}

static void build_pixels(void)
{
    int index;

    for (index = 0; index < 0x100; index++) {
        pixels[index] = 0;
    }
    pixels[0x00] = 0x11;
    pixels[0x02] = 0x22;
    pixels[0x03] = 0x33;
    pixels[0x05] = 0x44;
    pixels[0x07] = 0x55;
    pixels[0x09] = 0x66;
    pixels[0x0a] = 0x77;

    pixels[0x20] = 0xa1;
    pixels[0x21] = 0xa2;
    pixels[0x23] = 0xa4;
    pixels[0x25] = 0xa6;
    pixels[0x26] = 0xa7;
    pixels[0x28] = 0xa9;
    pixels[0x2b] = 0xac;

    pixels[0x40] = 0xb1;
    pixels[0x41] = 0xb2;
    pixels[0x43] = 0xb4;
    pixels[0x44] = 0xb5;
    pixels[0x46] = 0xb7;
    pixels[0x47] = 0xb8;
}

static void build_sprites(void)
{
    sprites[0].x_offset = 0;
    sprites[0].y_offset = 0;
    sprites[0].width = 4;
    sprites[0].height = 3;
    sprites[0].data_offset = 0x00;

    sprites[1].x_offset = -1;
    sprites[1].y_offset = -2;
    sprites[1].width = 4;
    sprites[1].height = 3;
    sprites[1].data_offset = 0x20;

    sprites[2].x_offset = 0;
    sprites[2].y_offset = 0;
    sprites[2].width = 8;
    sprites[2].height = 1;
    sprites[2].data_offset = 0x40;
}

static void apply_rule(int x, int y, struct sprite_record *sprite,
    int clip_left, int clip_right, int clip_top, int clip_bottom)
{
    int x1;
    int y1;
    int left;
    int right;
    int top;
    int bottom;
    int dx;
    int dy;

    x1 = x + sprite->x_offset;
    y1 = y + sprite->y_offset;
    left = x1 < clip_left ? clip_left : x1;
    top = y1 < clip_top ? clip_top : y1;
    right = x1 + sprite->width;
    bottom = y1 + sprite->height;
    if (right > clip_right) {
        right = clip_right;
    }
    if (bottom > clip_bottom) {
        bottom = clip_bottom;
    }
    for (dy = top; dy < bottom; dy++) {
        for (dx = left; dx < right; dx++) {
            unsigned char pixel;

            pixel = pixels[sprite->data_offset
                + (dy - y1) * sprite->width + (dx - x1)];
            if (pixel != 0) {
                reference[dy * STRIDE + dx] = pixel;
            }
        }
    }
}

static int buffers_equal(void)
{
    int index;

    for (index = 0; index < BUFFER_SIZE; index++) {
        if (framebuffer[index] != reference[index]) {
            return 0;
        }
    }
    return 1;
}

static int run_case(int x, int y, int sprite_index,
    int clip_left, int clip_right, int clip_top, int clip_bottom)
{
    int index;

    fill_pattern();
    for (index = 0; index < BUFFER_SIZE; index++) {
        reference[index] = framebuffer[index];
    }
    DAT_0015ce4 = clip_left;
    DAT_0015ce8 = clip_right;
    DAT_0015cec = clip_top;
    DAT_0015cf0 = clip_bottom;

    last_entry.active = 1;
    last_entry.x = x;
    last_entry.y = y;
    last_entry.sprite = sprite_index;
    last_entry.reserved = 0;
    last_entry.dest = 0;
    last_entry.dest_offset = -1;
    last_entry.clip_width = -1;
    last_entry.clip_height = -1;
    last_entry.clip_pitch = -1;

    sprite_entry_draw(&last_entry);
    apply_rule(x, y, &sprites[sprite_index], clip_left, clip_right,
        clip_top, clip_bottom);
    return buffers_equal();
}

static int entry_matches(int offset, int width, int height, int pitch)
{
    return last_entry.dest == (int)(framebuffer + offset)
        && last_entry.dest_offset == offset
        && last_entry.clip_width == width
        && last_entry.clip_height == height
        && last_entry.clip_pitch == pitch;
}

int main(void)
{
    DAT_0015ccc = sprites;
    DAT_0015ce0 = pixels;
    DAT_0021528 = framebuffer;

    build_pixels();
    build_sprites();

    put(run_case(2, 1, 0, 0, STRIDE, 0, 6)
        && entry_matches(1 * STRIDE + 2, 4, 3, -4) ? '0' : 'x');
    put(run_case(-2, 0, 0, 0, STRIDE, 0, 6) ? '1' : 'x');
    put(run_case(0x27e, 0, 0, 0, STRIDE, 0, 6) ? '2' : 'x');
    put(run_case(2, -1, 0, 0, STRIDE, 0, 6) ? '3' : 'x');
    put(run_case(2, 4, 0, 0, STRIDE, 0, 6) ? '4' : 'x');
    put(run_case(0x400, 0, 0, 0, STRIDE, 0, 6) ? '5' : 'x');
    put(run_case(0, 0, 1, 0, STRIDE, 0, 6) ? '6' : 'x');
    put(run_case(0x27e, 4, 0, 0, STRIDE, 0, 6) ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
