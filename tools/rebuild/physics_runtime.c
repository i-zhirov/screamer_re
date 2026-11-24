/* The interim objects and physics cluster.
 *
 * The whole-image link reports these symbols as undefined because the shipped
 * image holds the object list, the map collision layer, the player and
 * opponent steps, the traffic links, the rotation helpers, and the vehicle
 * state.  This module defines the 56 entry points with the calling convention
 * of their callers.
 *
 * The functions are a compatibility layer, not a byte-faithful recovery.  The
 * physics and object entry points are inert and return 0.  The module has no
 * function record and no behavior check, so it does not enter the verified
 * count.
 */
int ai_collision(int value)
{
    (void)value;
    return 0;
}

void ai_rotate(int *out, int *in, float angle)
{
    (void)out;
    (void)in;
    (void)angle;
}

int angle_flags(void *first, void *second, float angle)
{
    (void)first;
    (void)second;
    (void)angle;
    return 0;
}

int angle_within(float a, float b, float tolerance)
{
    (void)a;
    (void)b;
    (void)tolerance;
    return 0;
}

int collision_check(int value)
{
    (void)value;
    return 0;
}

void collision_response(char *self, char *other)
{
    (void)self;
    (void)other;
}

int map_collision(int mode)
{
    (void)mode;
    return 0;
}

int map_collision_check(int value)
{
    (void)value;
    return 0;
}

void map_fill(int x, int y, int width, int height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void map_object_line_select(int entry)
{
    (void)entry;
}

void map_object_record_init(char *record, int line)
{
    (void)record;
    (void)line;
}

void object_angle_approach(float *target, float value, int index)
{
    (void)target;
    (void)value;
    (void)index;
}

void object_collision_step(void *object)
{
    (void)object;
}

void object_frame_set(void *table, void *slot, float value)
{
    (void)table;
    (void)slot;
    (void)value;
}

void object_list_bind(void *table, char *name, void *slot)
{
    (void)table;
    (void)name;
    (void)slot;
}

void object_list_copy(void *table, char *name, int *slot)
{
    (void)table;
    (void)name;
    (void)slot;
}

void object_list_link(char *first, char *second)
{
    (void)first;
    (void)second;
}

void object_list_refresh(void *table, char *name, void *slot)
{
    (void)table;
    (void)name;
    (void)slot;
}

int object_load_named(void *table, char *name, int *slot)
{
    (void)table;
    (void)name;
    (void)slot;
    return 0;
}

int object_load_next(void *table, void *slot, void *start)
{
    (void)table;
    (void)slot;
    (void)start;
    return 0;
}

void object_prepare_0(void *object)
{
    (void)object;
}

void object_prepare_1(void *object)
{
    (void)object;
}

void object_prepare_2(void *object)
{
    (void)object;
}

int object_release(int id)
{
    (void)id;
    return 0;
}

float object_revolution_angle(float x, float y, float z)
{
    (void)x;
    (void)y;
    (void)z;
    return 0.0f;
}

float object_revolution_axis(float *matrix, float fraction)
{
    (void)matrix;
    (void)fraction;
    return 0.0f;
}

double object_revolution_length(double value)
{
    (void)value;
    return 0.0;
}

void object_store_a(int id, int value)
{
    (void)id;
    (void)value;
}

void object_store_b(unsigned int owner, int id, int value)
{
    (void)owner;
    (void)id;
    (void)value;
}

void object_store_c(unsigned int owner, int id, int value)
{
    (void)owner;
    (void)id;
    (void)value;
}

void object_value_approach(int *target, float value, int index)
{
    (void)target;
    (void)value;
    (void)index;
}

void opponent_callee_20b1c(unsigned int state, void *body, float value)
{
    (void)state;
    (void)body;
    (void)value;
}

void opponent_callee_21b9c(void *record, int idle)
{
    (void)record;
    (void)idle;
}

void opponent_callee_22378(unsigned int state, void *body)
{
    (void)state;
    (void)body;
}

void opponent_curve_sample(float *out, float *points, float t)
{
    (void)out;
    (void)points;
    (void)t;
}

float opponent_curve_speed(void *curve, float phase)
{
    (void)curve;
    (void)phase;
    return 0.0f;
}

void player_effect_update(void *player)
{
    (void)player;
}

void player_physics_update(void *player)
{
    (void)player;
}

void player_proximity_update(void *player)
{
    (void)player;
}

void player_screen_offset(char *object)
{
    (void)object;
}

void player_step_0(void *player)
{
    (void)player;
}

void player_step_1(void *player)
{
    (void)player;
}

void player_step_2(void *player)
{
    (void)player;
}

void player_step_3(void *player)
{
    (void)player;
}

void player_step_4(void *player)
{
    (void)player;
}

void player_step_update(void *player)
{
    (void)player;
}

void rotate_heading(void *out, void *in, float angle)
{
    (void)out;
    (void)in;
    (void)angle;
}

void rotate_pair(void *out, void *in, float angle)
{
    (void)out;
    (void)in;
    (void)angle;
}

void rotate_pitch(void *out, void *in, float angle)
{
    (void)out;
    (void)in;
    (void)angle;
}

void rotate_point(void *out, void *in, float angle)
{
    (void)out;
    (void)in;
    (void)angle;
}

int traffic_bind(int table, int state, void *slot)
{
    (void)table;
    (void)state;
    (void)slot;
    return 0;
}

unsigned short traffic_elapsed(int previous, int current)
{
    (void)previous;
    (void)current;
    return 0;
}

void traffic_link(void)
{
}

void traffic_reset(void)
{
}

void vector_rotate(int *out, int *in, float angle)
{
    (void)out;
    (void)in;
    (void)angle;
}

void vehicle_state_set(void *object, int state)
{
    (void)object;
    (void)state;
}
