/* The test harness for the rebuilt `camera_update` module.
 *
 * The harness holds the camera record, the three globals of the module, the
 * `camera_transform` stub, and the `camera_project` stub.  The transform stub
 * records its argument and writes a position and a velocity into the record.
 * The projection stub records the pointer that it reads from the global
 * `camera_state`.  The harness runs three paths:
 *
 *     0  scale 0.5: the transform writes the position 11, 22, 33 and the
 *        velocity 7, 8, 9, and the module saves the new position, clears the
 *        velocity and the position, and scales the basis by 0.5
 *     1  scale -2.0: the transform writes the position -1, -2, -3, and the
 *        module saves the new position and scales the basis by -2.0
 *     2  scale 1.0: the transform writes the position 0, 0, 0, and the module
 *        keeps the signed basis values
 *     D  the exit marker
 *
 * The constants and the expected values are set from their exact bit patterns.
 * A float literal in this minimal image becomes a read of the constant segment,
 * which this harness does not load, so a literal comparison would compare
 * against zero.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

struct camera {
    int field_00;
    int field_04;
    int field_08;
    int field_0c;
    int field_10;
    int field_14;
    int basis_a[9];
    int position[3];
    int basis_b[9];
    int velocity[3];
};

void camera_update(struct camera *camera);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static struct camera camera;
struct camera *camera_state;
int saved_position[3];
float camera_scale;
float scaled_basis[9];

static int transform_calls;
static struct camera *transform_arg;
static int transform_state_ok;
static int project_calls;
static struct camera *project_arg;
static int transform_position[3];
static int transform_velocity[3];

void camera_transform(struct camera *camera)
{
    transform_calls = transform_calls + 1;
    transform_arg = camera;
    transform_state_ok = camera_state == camera;
    camera->position[0] = transform_position[0];
    camera->position[1] = transform_position[1];
    camera->position[2] = transform_position[2];
    camera->velocity[0] = transform_velocity[0];
    camera->velocity[1] = transform_velocity[1];
    camera->velocity[2] = transform_velocity[2];
}

void camera_project(void)
{
    project_calls = project_calls + 1;
    project_arg = camera_state;
}

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_bits_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *p = v.f;
}

static int same_float(float a, float b)
{
    union {
        float f;
        unsigned int u;
    } x;
    union {
        float f;
        unsigned int u;
    } y;

    x.f = a;
    y.f = b;
    return x.u == y.u;
}

static void set_position(int a, int b, int c)
{
    camera.position[0] = a;
    camera.position[1] = b;
    camera.position[2] = c;
}

static void set_velocity(int a, int b, int c)
{
    camera.velocity[0] = a;
    camera.velocity[1] = b;
    camera.velocity[2] = c;
}

static void set_transform(int px, int py, int pz, int vx, int vy, int vz)
{
    transform_position[0] = px;
    transform_position[1] = py;
    transform_position[2] = pz;
    transform_velocity[0] = vx;
    transform_velocity[1] = vy;
    transform_velocity[2] = vz;
}

static void set_basis(int first, int step)
{
    int index;

    for (index = 0; index < 9; index++) {
        camera.basis_b[index] = first + index * step;
    }
}

static void reset(unsigned int scale_bits)
{
    int *words;
    int index;

    words = (int *)&camera;
    for (index = 0; index < (int)(sizeof(struct camera) / sizeof(int)); index++) {
        words[index] = 0;
    }
    words = (int *)scaled_basis;
    for (index = 0; index < 9; index++) {
        words[index] = 0;
    }
    for (index = 0; index < 3; index++) {
        saved_position[index] = 0;
    }
    camera_state = 0;
    transform_calls = 0;
    transform_arg = 0;
    transform_state_ok = 0;
    project_calls = 0;
    project_arg = 0;
    for (index = 0; index < 3; index++) {
        transform_position[index] = 0;
        transform_velocity[index] = 0;
    }
    set_bits_float(&camera_scale, scale_bits);
}

static int cleared_ok(void)
{
    int index;

    for (index = 0; index < 3; index++) {
        if (camera.position[index] != 0 || camera.velocity[index] != 0) {
            return 0;
        }
    }
    return 1;
}

static int basis_ok(void)
{
    int index;
    float expected;

    for (index = 0; index < 9; index++) {
        expected = (float)camera.basis_b[index] * camera_scale;
        if (!same_float(scaled_basis[index], expected)) {
            return 0;
        }
    }
    return 1;
}

static int calls_ok(void)
{
    return transform_calls == 1
        && transform_arg == &camera
        && transform_state_ok
        && project_calls == 1
        && project_arg == &camera
        && camera_state == &camera;
}

int main(void)
{
    float expected;

    reset(0x3f000000);
    set_basis(1, 1);
    set_position(100, 200, 300);
    set_velocity(1, 2, 3);
    set_transform(11, 22, 33, 7, 8, 9);
    camera_update(&camera);
    set_bits_float(&expected, 0x3f000000);
    put(calls_ok()
        && saved_position[0] == 11
        && saved_position[1] == 22
        && saved_position[2] == 33
        && cleared_ok()
        && basis_ok()
        && same_float(scaled_basis[0], expected) ? '0' : 'x');
    set_bits_float(&expected, 0x40000000);
    put(same_float(scaled_basis[3], expected) ? '1' : 'x');
    set_bits_float(&expected, 0x40900000);
    put(same_float(scaled_basis[8], expected) ? '2' : 'x');

    reset(0xc0000000);
    set_basis(1, 1);
    set_position(100, 200, 300);
    set_velocity(1, 2, 3);
    set_transform(-1, -2, -3, 4, 5, 6);
    camera_update(&camera);
    put(calls_ok()
        && saved_position[0] == -1
        && saved_position[1] == -2
        && saved_position[2] == -3
        && cleared_ok()
        && basis_ok() ? '3' : 'x');
    set_bits_float(&expected, 0xc0000000);
    put(same_float(scaled_basis[0], expected) ? '4' : 'x');
    set_bits_float(&expected, 0xc1900000);
    put(same_float(scaled_basis[8], expected) ? '5' : 'x');

    reset(0x3f800000);
    set_basis(-1, -1);
    set_position(100, 200, 300);
    set_velocity(1, 2, 3);
    set_transform(0, 0, 0, 0, 0, 0);
    camera_update(&camera);
    put(calls_ok()
        && saved_position[0] == 0
        && saved_position[1] == 0
        && saved_position[2] == 0
        && cleared_ok()
        && basis_ok() ? '6' : 'x');
    set_bits_float(&expected, 0xbf800000);
    put(same_float(scaled_basis[0], expected) ? '7' : 'x');
    set_bits_float(&expected, 0xc1100000);
    put(same_float(scaled_basis[8], expected) ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
