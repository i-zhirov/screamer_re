/* The test harness for the rebuilt `camera_reference_update` module.
 *
 * The harness holds the control byte, the player world position and heading,
 * the view saved copies and yaw, and the three camera cells.  Every camera cell
 * starts at a distinct sentinel value on each path, so a missing store or an
 * extra store fails the path.  The x and z values and the heading all differ,
 * so a swapped store fails.
 *
 * The paths:
 *
 *     0  control 0x01: the player pose, because bit 0 is set and bit 0x20 is
 *        clear
 *     1  control 0x00: the view pose, because bit 0 is clear
 *     2  control 0x21: the view pose, because bit 0x20 is set
 *     3  control 0x20: the view pose
 *     4  control 0xff: the view pose
 *     5  control 0x05: the player pose, because the other bits do not matter
 *     6  control 0xfe: the view pose, because bit 0 is clear
 *     D  the exit marker
 *
 * The harness sets every float through an int bit pattern, because the minimal
 * image does not load the constant segment.
 */
#pragma off (check_stack)

void camera_reference_update(void *view);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char DAT_00118e1d;
int DAT_0013ccc;
int DAT_0013cd0;
float DAT_0013cd4;
int DAT_0011fdcc;
int DAT_0011fdd4;
float DAT_0011fde8;

static char text[8];
static char view[0x38];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void check(char marker, int condition)
{
    put(condition ? marker : 'x');
}

static void set_player(int x, int z, int yaw)
{
    DAT_0011fdcc = x;
    DAT_0011fdd4 = z;
    *(int *)&DAT_0011fde8 = yaw;
}

static void set_view(int x, int z, int yaw)
{
    *(int *)(view + 0x0c) = x;
    *(int *)(view + 0x14) = z;
    *(int *)(view + 0x34) = yaw;
}

static void reset_cells(void)
{
    DAT_0013ccc = 0x55555555;
    DAT_0013cd0 = 0x66666666;
    *(int *)&DAT_0013cd4 = 0x77777777;
}

static int player_published(int x, int z, int yaw)
{
    return DAT_0013ccc == x
        && DAT_0013cd0 == z
        && *(int *)&DAT_0013cd4 == yaw;
}

static int view_published(int x, int z, int yaw)
{
    return DAT_0013ccc == x
        && DAT_0013cd0 == z
        && *(int *)&DAT_0013cd4 == yaw;
}

int main(void)
{
    set_player(0x11111111, 0x22222222, 0x3fa00000);
    set_view(0x33333333, 0x44444444, 0x3f800000);

    DAT_00118e1d = 0x01;
    reset_cells();
    camera_reference_update(view);
    check('0', player_published(0x11111111, 0x22222222, 0x3fa00000));

    DAT_00118e1d = 0x00;
    reset_cells();
    camera_reference_update(view);
    check('1', view_published(0x33333333, 0x44444444, 0x3f800000));

    DAT_00118e1d = 0x21;
    reset_cells();
    camera_reference_update(view);
    check('2', view_published(0x33333333, 0x44444444, 0x3f800000));

    DAT_00118e1d = 0x20;
    reset_cells();
    camera_reference_update(view);
    check('3', view_published(0x33333333, 0x44444444, 0x3f800000));

    DAT_00118e1d = 0xff;
    reset_cells();
    camera_reference_update(view);
    check('4', view_published(0x33333333, 0x44444444, 0x3f800000));

    DAT_00118e1d = 0x05;
    reset_cells();
    camera_reference_update(view);
    check('5', player_published(0x11111111, 0x22222222, 0x3fa00000));

    DAT_00118e1d = 0xfe;
    reset_cells();
    camera_reference_update(view);
    check('6', view_published(0x33333333, 0x44444444, 0x3f800000));

    put('D');
    quit();
    return 0;
}
