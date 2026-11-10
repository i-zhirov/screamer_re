/* The test harness for the rebuilt `player_cockpit_camera` module.
 *
 * The harness holds the car object, the view object, the roll scale, and the
 * height bias.  The list mirror stub counts its calls and saves the table, the
 * name, and the two slot pointers.
 *
 * The harness runs four paths:
 *
 *     0  positive values: the three angles, the three positions, the two saved
 *        copies, and the height bias
 *     1  negative values: the sums stay negative and the height bias is negative
 *     2  another roll scale and a zero bias: the roll follows the new scale
 *     3  the two list slots sit at +0x460 and +0x468 with the table and name
 *     D  the exit marker
 *
 * The float comparisons use a small tolerance, because the intermediate
 * product need not land on the same bit pattern as the stated decimal value.
 */
#pragma off (check_stack)

void player_cockpit_camera(char *car, char *view);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char car[0x500];
static char view[0x40];
static int mirror_calls;
static void *mirror_table;
static char *mirror_name;
static int *mirror_slot;
static int *mirror_slot_next;

float camera_roll_scale;
int camera_height_bias;

void object_list_mirror(void *table, char *name, int *slot)
{
    if (mirror_calls == 0) {
        mirror_table = table;
        mirror_name = name;
        mirror_slot = slot;
    } else if (mirror_calls == 1) {
        mirror_slot_next = slot;
    }
    mirror_calls = mirror_calls + 1;
}

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void put_float(char *base, int offset, float value)
{
    *(float *)(base + offset) = value;
}

static void put_int(char *base, int offset, int value)
{
    *(int *)(base + offset) = value;
}

static int almost(float value, float expected, float tolerance)
{
    float difference;

    difference = value - expected;
    if (difference < 0.0f) {
        difference = -difference;
    }
    return difference < tolerance;
}

static int float_at(char *base, int offset, float expected)
{
    return almost(*(float *)(base + offset), expected, 0.0001f);
}

static int int_at(char *base, int offset, int expected)
{
    return *(int *)(base + offset) == expected;
}

int main(void)
{
    int index;
    int ok;

    camera_roll_scale = 3.0f;
    camera_height_bias = 3;

    put_float(car, 0x4c8, 0.5f);
    put_float(car, 0x4e0, 0.25f);
    put_float(car, 0x4cc, 1.0f);
    put_float(car, 0x128, 0.5f);
    put_float(car, 0x4e8, 0.3f);
    put_float(car, 0x4d0, 0.1f);
    put_int(car, 0x4b0, 100);
    put_int(car, 0x4bc, 20);
    put_int(car, 0x04, 10);
    put_int(car, 0x4b4, 5);
    put_int(car, 0x4c0, 2);
    put_int(car, 0x4b8, 50);
    for (index = 0; index < 0x40; index = index + 1) {
        view[index] = 0x7f;
    }

    player_cockpit_camera(car, view);

    ok = float_at(view, 0x30, 0.75f)
        && float_at(view, 0x34, 1.5f)
        && float_at(view, 0x38, 1.0f)
        && int_at(view, 0x00, 120)
        && int_at(view, 0x0c, 120)
        && int_at(view, 0x04, 20)
        && int_at(view, 0x08, 50)
        && int_at(view, 0x14, 50);
    put(ok ? '0' : 'x');

    for (index = 0; index < 0x500; index = index + 1) {
        car[index] = 0;
    }
    for (index = 0; index < 0x40; index = index + 1) {
        view[index] = 0;
    }
    camera_roll_scale = 3.0f;
    camera_height_bias = -500;

    put_float(car, 0x4c8, -2.5f);
    put_float(car, 0x4e0, -1.5f);
    put_float(car, 0x4cc, -1.0f);
    put_float(car, 0x128, -2.0f);
    put_float(car, 0x4e8, -1.5f);
    put_float(car, 0x4d0, 0.5f);
    put_int(car, 0x4b0, -100);
    put_int(car, 0x4bc, -20);
    put_int(car, 0x04, 100);
    put_int(car, 0x4b4, 250);
    put_int(car, 0x4c0, 50);
    put_int(car, 0x4b8, -64);

    player_cockpit_camera(car, view);

    ok = float_at(view, 0x30, -4.0f)
        && float_at(view, 0x34, -3.0f)
        && float_at(view, 0x38, -4.0f)
        && int_at(view, 0x00, -120)
        && int_at(view, 0x0c, -120)
        && int_at(view, 0x04, -100)
        && int_at(view, 0x08, -64)
        && int_at(view, 0x14, -64);
    put(ok ? '1' : 'x');

    for (index = 0; index < 0x500; index = index + 1) {
        car[index] = 0;
    }
    for (index = 0; index < 0x40; index = index + 1) {
        view[index] = 0;
    }
    camera_roll_scale = 2.0f;
    camera_height_bias = 0;

    put_float(car, 0x4e8, 1.0f);

    player_cockpit_camera(car, view);

    ok = float_at(view, 0x30, 0.0f)
        && float_at(view, 0x34, 0.0f)
        && float_at(view, 0x38, 2.0f)
        && int_at(view, 0x00, 0)
        && int_at(view, 0x0c, 0)
        && int_at(view, 0x04, 0)
        && int_at(view, 0x08, 0)
        && int_at(view, 0x14, 0);
    put(ok ? '2' : 'x');

    ok = mirror_calls == 6
        && mirror_table == (void *)0x6530
        && mirror_name == (char *)0x6350
        && mirror_slot == (int *)(car + 0x460)
        && mirror_slot_next == (int *)(car + 0x468);
    put(ok ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
