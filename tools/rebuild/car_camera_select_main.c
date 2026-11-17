#pragma off (check_stack)

void car_camera_select(char *view);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define STRIDE 0x590
#define CARS 3
#define VIEW_SIZE 0x98

unsigned char DAT_00110ab8[STRIDE * CARS];
int DAT_0010f8ec;
int DAT_0010634c;
int DAT_0010a18;
int DAT_0010a1c;

static char text[8];
static unsigned char view[0x100];
static int camera_calls;
static int camera_last;

void car_camera(char *car)
{
    camera_calls = camera_calls + 1;
    camera_last = (int)(car - (char *)DAT_00110ab8) / STRIDE;
}

void *memcpy(void *target, const void *source, unsigned int count)
{
    unsigned char *out = (unsigned char *)target;
    const unsigned char *in = (const unsigned char *)source;
    unsigned int index;

    for (index = 0; index < count; index++) {
        out[index] = in[index];
    }
    return target;
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

static void set_int(int car, int offset, int value)
{
    *(int *)(DAT_00110ab8 + car * STRIDE + offset) = value;
}

static void reset(void)
{
    int index;
    int offset;

    DAT_0010f8ec = CARS;
    DAT_0010634c = 0;
    DAT_0010a18 = 0;
    DAT_0010a1c = 0;
    for (index = 0; index < STRIDE * CARS; index++) {
        DAT_00110ab8[index] = 0;
    }
    for (index = 0; index < CARS; index++) {
        for (offset = 0; offset < VIEW_SIZE; offset++) {
            DAT_00110ab8[index * STRIDE + 0x4f8 + offset] =
                (unsigned char)(0x40 + index);
        }
    }
    for (index = 0; index < 0x100; index++) {
        view[index] = 0xaa;
    }
    camera_calls = 0;
    camera_last = -1;
}

static int copied_from(int car)
{
    const unsigned char *source =
        DAT_00110ab8 + car * STRIDE + 0x4f8;
    int offset;

    for (offset = 0; offset < VIEW_SIZE; offset++) {
        if (view[offset] != source[offset]) {
            return 0;
        }
    }
    if (view[VIEW_SIZE] != 0xaa) {
        return 0;
    }
    return 1;
}

int main(void)
{
    reset();
    DAT_0010a18 = 0;
    DAT_0010a1c = 1;
    set_int(0, 0x564, 1);
    car_camera_select((char *)view);
    put(camera_calls == 3
            && camera_last == 2
            && DAT_0010634c == 3
            && DAT_0010a18 == 1
            && copied_from(1)
        ? '0' : 'x');

    reset();
    DAT_0010a18 = 0;
    DAT_0010a1c = 1;
    set_int(0, 0x564, 0);
    car_camera_select((char *)view);
    put(camera_calls == 3
            && camera_last == 2
            && DAT_0010a18 == 0
            && copied_from(0)
        ? '1' : 'x');

    reset();
    DAT_0010a18 = 1;
    DAT_0010a1c = 1;
    set_int(1, 0x564, 1);
    car_camera_select((char *)view);
    put(camera_calls == 3
            && DAT_0010a18 == 1
            && copied_from(1)
        ? '2' : 'x');

    reset();
    DAT_0010f8ec = 0;
    DAT_0010a18 = 2;
    DAT_0010a1c = 0;
    set_int(2, 0x564, 0);
    car_camera_select((char *)view);
    put(camera_calls == 0
            && DAT_0010634c == 0
            && DAT_0010a18 == 2
            && copied_from(2)
        ? '3' : 'x');

    reset();
    DAT_0010a18 = 0;
    DAT_0010a1c = 2;
    set_int(0, 0x564, 1);
    car_camera_select((char *)view);
    put(camera_calls == 3
            && DAT_0010a18 == 2
            && copied_from(2)
            && view[VIEW_SIZE] == 0xaa
        ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
