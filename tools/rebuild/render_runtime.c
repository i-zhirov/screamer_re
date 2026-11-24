/* The interim render and video cluster.
 *
 * The whole-image link reports these symbols as undefined because the shipped
 * image holds the drawing, palette, sprite, PCX, HUD, camera, panel, and view
 * layer.  This module defines the 33 entry points with the calling convention
 * of their callers.
 *
 * The functions are a compatibility layer, not a byte-faithful recovery.  The
 * draw, sprite, palette, and view entry points are inert and return 0.  The
 * module has no function record and no behavior check, so it does not enter
 * the verified count.
 */
void camera_transform(void *camera)
{
    (void)camera;
}

void cockpit_resource_load(char *name, void *out)
{
    (void)name;
    (void)out;
}

void draw(int x, int y, char *text, int font, int color)
{
    (void)x;
    (void)y;
    (void)text;
    (void)font;
    (void)color;
}

void font_table_init(void)
{
}

void gauge_line_draw(int x1, int y1, int x2, int y2)
{
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
}

void hud_overlay_0(void)
{
}

void hud_overlay_1(void)
{
}

void hud_overlay_2(void)
{
}

int lzw_expand(void *input, void *output, int input_size, int count,
               int handle)
{
    (void)input;
    (void)output;
    (void)input_size;
    (void)count;
    (void)handle;
    return 0;
}

void material_name_set(char *name)
{
    (void)name;
}

void message_format(char *format, char *buffer)
{
    (void)format;
    (void)buffer;
}

void message_show(char *buffer)
{
    (void)buffer;
}

void palette_set(char *palette)
{
    (void)palette;
}

void panel_reset(void)
{
}

void panel_update(void)
{
}

int pcx_decode(char *path, char *name, int *out, char *palette, int mode)
{
    (void)path;
    (void)name;
    (void)out;
    (void)palette;
    (void)mode;
    return 0;
}

void radar_line_draw(int x1, int y1, int x2, int y2)
{
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
}

void rect_fill(int x, int y, int width, int height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void render_frame_select(int x, int y, int z)
{
    (void)x;
    (void)y;
    (void)z;
}

void render_prepare(void)
{
}

int report_message(char *message)
{
    (void)message;
    return 0;
}

void scene_setup(int value)
{
    (void)value;
}

void screen_mode_set(unsigned int width, unsigned int height,
                     unsigned int value)
{
    (void)width;
    (void)height;
    (void)value;
}

void sprite_blit(void *source, void *dest)
{
    (void)source;
    (void)dest;
}

void sprite_flush(void)
{
}

void video_bank_set(int bank)
{
    (void)bank;
}

void video_rect_copy(int x, int y, int width, int height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void view_move_0(int a)
{
    (void)a;
}

void view_move_1(int a, int b)
{
    (void)a;
    (void)b;
}

void view_move_2(int a, int b)
{
    (void)a;
    (void)b;
}

void view_render(void *view)
{
    (void)view;
}

void view_transition(int value)
{
    (void)value;
}

void world_render(int x, int y, int angle)
{
    (void)x;
    (void)y;
    (void)angle;
}
