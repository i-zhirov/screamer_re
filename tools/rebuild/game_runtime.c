/* The interim race and game-flow cluster.
 *
 * The whole-image link reports these symbols as undefined because the shipped
 * image holds the race flow, the menu and overlay dispatch, the network lobby,
 * the frame and track helpers, and the timer layer.  This module defines the
 * 65 entry points with the calling convention of their callers.
 *
 * The functions are a compatibility layer, not a byte-faithful recovery.  The
 * race, menu, network, overlay, and timer entry points are inert and return 0.
 * The module has no function record and no behavior check, so it does not
 * enter the verified count.
 */
int effect_after(int enabled, int id, float limit)
{
    (void)enabled;
    (void)id;
    (void)limit;
    return 0;
}

int effect_countdown(int enabled, int id, float limit)
{
    (void)enabled;
    (void)id;
    (void)limit;
    return 0;
}

void effect_push(int type, int subtype, float value)
{
    (void)type;
    (void)subtype;
    (void)value;
}

int effect_queue_head(void)
{
    return 0;
}

int effect_slot_advance(void *table, void *slot, float delta)
{
    (void)table;
    (void)slot;
    (void)delta;
    return 0;
}

void effect_start(void *object)
{
    (void)object;
}

void effect_state_set(void *object, int state)
{
    (void)object;
    (void)state;
}

int effect_trigger(int id, int value)
{
    (void)id;
    (void)value;
    return 0;
}

void filter_smooth(float *p, float f, int i)
{
    (void)p;
    (void)f;
    (void)i;
}

void filter_update_int(int *value, float target, int index)
{
    (void)value;
    (void)target;
    (void)index;
}

unsigned int frame_delta(unsigned int a, unsigned int b)
{
    (void)a;
    (void)b;
    return 0;
}

int game_mode_10_start(int value)
{
    (void)value;
    return 0;
}

int game_mode_1_start(int value)
{
    (void)value;
    return 0;
}

int game_mode_20_start(int value)
{
    (void)value;
    return 0;
}

int game_mode_40_start(int value)
{
    (void)value;
    return 0;
}

int game_mode_4_start(int value)
{
    (void)value;
    return 0;
}

void menu_list_begin(void)
{
}

void menu_poll(void)
{
}

void menu_state_reset(int state, int value)
{
    (void)state;
    (void)value;
}

void menu_transition(void)
{
}

void mode10_step_a(int value)
{
    (void)value;
}

void mode10_step_b(void)
{
}

void mode_flag_set(int value)
{
    (void)value;
}

void net_copy(unsigned char *source, unsigned char *dest, int count)
{
    (void)source;
    (void)dest;
    (void)count;
}

int net_count(void)
{
    return 0;
}

void net_lobby_exit(void)
{
}

int net_local_index(void)
{
    return 0;
}

int net_node_index(void *node)
{
    (void)node;
    return 0;
}

int net_player_slot(void)
{
    return 0;
}

void net_poll(void)
{
}

void net_queue_add(void *node)
{
    (void)node;
}

void net_queue_remove(int index)
{
    (void)index;
}

void net_queue_sort(void)
{
}

void net_race_overlay(void)
{
}

int net_receive(void *dest, int count, unsigned char **out)
{
    (void)dest;
    (void)count;
    (void)out;
    return 0;
}

void overlay_case_281f8(void)
{
}

void overlay_case_282a2(void)
{
}

void overlay_case_282e6(void)
{
}

void overlay_case_28358(void)
{
}

void overlay_case_2837d(void)
{
}

void overlay_case_283da(void)
{
}

void overlay_notify(unsigned int value)
{
    (void)value;
}

void overlay_option_zero(void)
{
}

int race_count(void)
{
    return 0;
}

void race_draw_16(void)
{
}

void race_draw_1(void)
{
}

void race_draw_2(void)
{
}

void race_draw_32(void)
{
}

void race_draw_4(void)
{
}

void race_draw_64(void)
{
}

int race_frame_loop(void)
{
    return 0;
}

void race_gauges(void)
{
}

int race_local_index(void)
{
    return 0;
}

void race_objects_reset(void)
{
}

int race_track_load(int value)
{
    (void)value;
    return 0;
}

int timer_init(int rate, int flags)
{
    (void)rate;
    (void)flags;
    return 0;
}

int timer_install(int rate, void *handler, void *timer_id)
{
    (void)rate;
    (void)handler;
    (void)timer_id;
    return 0;
}

void timer_set(unsigned int divisor)
{
    (void)divisor;
}

int timer_shutdown(int flags)
{
    (void)flags;
    return 0;
}

void timer_tick_100(void)
{
}

void track_direction(int value)
{
    (void)value;
}

void track_names_build(char *name)
{
    (void)name;
}

void track_resource_load(char *name)
{
    (void)name;
}

void track_stream_open(char *path, char *member, int value)
{
    (void)path;
    (void)member;
    (void)value;
}

void track_timer_update(void *object)
{
    (void)object;
}
