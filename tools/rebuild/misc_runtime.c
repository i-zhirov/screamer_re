/* The interim misc and unnamed cluster.
 *
 * The whole-image link reports these symbols as undefined because the shipped
 * image holds the network race-screen cases, the CD check, the menu and
 * loading helpers, the race state steps, the collision cell tests, the sound
 * driver setup, and the volume bar.  This module defines the 38 entry points
 * with the calling convention of their callers.
 *
 * The functions are a compatibility layer, not a byte-faithful recovery.  The
 * helpers are inert and return 0.  The module has no function record and no
 * behavior check, so it does not enter the verified count.
 */
void case_00(void)
{
}

void case_01(void)
{
}

void case_02(void)
{
}

void case_03(void)
{
}

void case_04(void)
{
}

void case_15(void)
{
}

void case_18(void)
{
}

void case_19(void)
{
}

void case_1a(void)
{
}

void case_1b(void)
{
}

void case_1c_high(void)
{
}

void case_1c_low(void)
{
}

void case_1d(void)
{
}

void case_1e(void)
{
}

void case_1f(void)
{
}

void case_4d2(void)
{
}

int cd_check(void)
{
    return 0;
}

void func_20428(void)
{
}

void sd_setup(void)
{
}

void state_cells_set(int value)
{
    (void)value;
}

void state_step_0(void)
{
}

void state_step_1(void)
{
}

void state_step_2(void)
{
}

void state_step_3(void)
{
}

void state_step_4(void)
{
}

void state_step_5(void)
{
}

void state_step_6(void)
{
}

void state_step_7(void)
{
}

void sub_119cc(void)
{
}

void sub_15150(void)
{
}

void sub_1be6c(void)
{
}

int sub_1bfd4(void)
{
    return 0;
}

int sub_1c0ac(void)
{
    return 0;
}

int sub_1fd70(void)
{
    return 0;
}

void sub_2dff4(void)
{
}

void sub_2f326(void *record, char *name)
{
    (void)record;
    (void)name;
}

void sub_8bd54(int value)
{
    (void)value;
}

void volume_bar_draw(int value, char *label)
{
    (void)value;
    (void)label;
}
