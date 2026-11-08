/* The test harness for the rebuilt `car_select_update` module.
 *
 * The harness holds the 14 car display records at `DAT_0011d550`, the object
 * pointer table at `DAT_0010653c`, the display objects, and the five float
 * globals.  It runs five paths:
 *
 *     0  the full path: the step pi/2 advances every angle, and the object
 *        angles take 256, 512, and 512 from the record angles pi/4, 0, and pi/2
 *     1  the wrap: the angle 6.0 plus pi/2 reaches 2 * pi, so the function
 *        clears the record angle to 0 before the object write
 *     2  the permuted table: the record i selects the object 13 - i, so the
 *        object writes follow the record index
 *     3  the negative floats: the conversions truncate toward zero, so -1.0
 *        gives 1723 and -0.5 gives 1886 after the low 11-bit mask
 *     4  the boundary: the angle 2 * pi with the step 0 reaches the limit, so
 *        the comparison resets the record angle to 0
 *     D  the exit marker
 *
 * The float checks compare the bit patterns exactly.
 */
#pragma off (check_stack)

void car_select_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct car_record {
    int object_index;
    int field_04;
    int angle_x;
    int angle_y;
    int angle_z;
    char rest[0x30];
};

struct display_object {
    int field_00;
    int field_04;
    int x;
    int y;
    int z;
    int angle_x;
    int angle_y;
    int angle_z;
};

float DAT_0018e08;
float DAT_001011d3;
float DAT_001011d7;
float DAT_001011db;
float DAT_001011df;

struct car_record DAT_0011d550[15];
struct display_object *DAT_0010653c[16][7];

static struct display_object objects[15];
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

static unsigned int bits(float value)
{
    union {
        float f;
        unsigned int u;
    } number;

    number.f = value;
    return number.u;
}

static void set_global_bits(float *p, unsigned int pattern)
{
    union {
        float f;
        unsigned int u;
    } number;

    number.u = pattern;
    *p = number.f;
}

static void set_angle(int index, int offset, unsigned int pattern)
{
    union {
        float f;
        unsigned int u;
    } number;

    number.u = pattern;
    *(float *)((char *)&DAT_0011d550[index] + offset) = number.f;
}

static unsigned int angle_bits(int index, int offset)
{
    return bits(*(float *)((char *)&DAT_0011d550[index] + offset));
}

static void reset(void)
{
    int i;
    int j;

    for (i = 0; i < 15; ++i) {
        DAT_0011d550[i].object_index = i;
        DAT_0011d550[i].field_04 = 0x11111111;
        DAT_0011d550[i].angle_x = 0x11111111;
        DAT_0011d550[i].angle_y = 0x11111111;
        DAT_0011d550[i].angle_z = 0x11111111;
        for (j = 0; j < 0x30; ++j) {
            DAT_0011d550[i].rest[j] = 0x11;
        }
        objects[i].field_00 = 0x22222222;
        objects[i].field_04 = 0x22222222;
        objects[i].x = 0x22222222;
        objects[i].y = 0x22222222;
        objects[i].z = 0x22222222;
        objects[i].angle_x = 0x22222222;
        objects[i].angle_y = 0x22222222;
        objects[i].angle_z = 0x22222222;
        for (j = 0; j < 7; ++j) {
            DAT_0010653c[i][j] = 0;
        }
        DAT_0010653c[i][0] = &objects[i];
    }
    set_global_bits(&DAT_0018e08, 0x3f800000);
    set_global_bits(&DAT_001011d3, 0x3fc90fdb);
    set_global_bits(&DAT_001011d7, 0x40c90fdb);
    set_global_bits(&DAT_001011db, 0x44800000);
    set_global_bits(&DAT_001011df, 0x40490fdb);
}

static int record_untouched(int index)
{
    int j;

    if (DAT_0011d550[index].field_04 != 0x11111111) {
        return 0;
    }
    for (j = 0; j < 0x30; ++j) {
        if (DAT_0011d550[index].rest[j] != 0x11) {
            return 0;
        }
    }
    return 1;
}

static int object_untouched(int index)
{
    if (objects[index].field_00 != 0x22222222) {
        return 0;
    }
    if (objects[index].field_04 != 0x22222222) {
        return 0;
    }
    if (objects[index].x != 0x22222222 || objects[index].y != 0x22222222) {
        return 0;
    }
    if (objects[index].z != 0x22222222) {
        return 0;
    }
    return 1;
}

static int full_ok(void)
{
    int i;

    for (i = 0; i < 14; ++i) {
        if (angle_bits(i, 0x08) != 0x3f490fdb) {
            return 0;
        }
        if (angle_bits(i, 0x0c) != 0x3fc90fdb) {
            return 0;
        }
        if (angle_bits(i, 0x10) != 0x3fc90fdb) {
            return 0;
        }
        if (!record_untouched(i)) {
            return 0;
        }
        if (objects[i].angle_x != 256) {
            return 0;
        }
        if (objects[i].angle_y != 512) {
            return 0;
        }
        if (objects[i].angle_z != 512) {
            return 0;
        }
        if (!object_untouched(i)) {
            return 0;
        }
    }
    if (DAT_0011d550[14].angle_x != 0x11111111) {
        return 0;
    }
    if (objects[14].angle_y != 0x22222222) {
        return 0;
    }
    return 1;
}

static int wrap_ok(void)
{
    int i;

    for (i = 0; i < 14; ++i) {
        if (DAT_0011d550[i].angle_y != 0) {
            return 0;
        }
        if (objects[i].angle_x != 256) {
            return 0;
        }
        if (objects[i].angle_y != 0) {
            return 0;
        }
        if (objects[i].angle_z != 0) {
            return 0;
        }
    }
    return 1;
}

static int permute_ok(void)
{
    int i;

    for (i = 0; i < 14; ++i) {
        if (DAT_0011d550[i].object_index != 13 - i) {
            return 0;
        }
        if (angle_bits(i, 0x0c) != 0x3fc90fdb) {
            return 0;
        }
        if (objects[13 - i].angle_x != 256) {
            return 0;
        }
        if (objects[13 - i].angle_y != 512) {
            return 0;
        }
        if (objects[13 - i].angle_z != 256) {
            return 0;
        }
    }
    return 1;
}

static int negative_ok(void)
{
    int i;

    for (i = 0; i < 14; ++i) {
        if (DAT_0011d550[i].angle_y != 0) {
            return 0;
        }
        if (objects[i].angle_x != 1723) {
            return 0;
        }
        if (objects[i].angle_y != 0) {
            return 0;
        }
        if (objects[i].angle_z != 1886) {
            return 0;
        }
    }
    return 1;
}

static int boundary_ok(void)
{
    int i;

    for (i = 0; i < 14; ++i) {
        if (DAT_0011d550[i].angle_y != 0) {
            return 0;
        }
        if (objects[i].angle_x != 0) {
            return 0;
        }
        if (objects[i].angle_y != 0) {
            return 0;
        }
        if (objects[i].angle_z != 0) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    int i;

    reset();
    for (i = 0; i < 14; ++i) {
        set_angle(i, 0x08, 0x3f490fdb);
        set_angle(i, 0x0c, 0x00000000);
        set_angle(i, 0x10, 0x3fc90fdb);
    }
    car_select_update();
    put(full_ok() ? '0' : 'x');

    reset();
    for (i = 0; i < 14; ++i) {
        set_angle(i, 0x08, 0x3f490fdb);
        set_angle(i, 0x0c, 0x40c00000);
        set_angle(i, 0x10, 0x00000000);
    }
    car_select_update();
    put(wrap_ok() ? '1' : 'x');

    reset();
    for (i = 0; i < 14; ++i) {
        DAT_0011d550[i].object_index = 13 - i;
        set_angle(i, 0x08, 0x3f490fdb);
        set_angle(i, 0x0c, 0x00000000);
        set_angle(i, 0x10, 0x3f490fdb);
    }
    car_select_update();
    put(permute_ok() ? '2' : 'x');

    reset();
    set_global_bits(&DAT_0018e08, 0x00000000);
    for (i = 0; i < 14; ++i) {
        set_angle(i, 0x08, 0xbf800000);
        set_angle(i, 0x0c, 0x00000000);
        set_angle(i, 0x10, 0xbf000000);
    }
    car_select_update();
    put(negative_ok() ? '3' : 'x');

    reset();
    set_global_bits(&DAT_0018e08, 0x00000000);
    for (i = 0; i < 14; ++i) {
        set_angle(i, 0x08, 0x00000000);
        set_angle(i, 0x0c, 0x40c90fdb);
        set_angle(i, 0x10, 0x00000000);
    }
    car_select_update();
    put(boundary_ok() ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
