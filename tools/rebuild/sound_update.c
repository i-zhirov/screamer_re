/* The rebuilt `sound_update` module.
 *
 * The original starts at linear 0x81a70 in STARTH.EXE.  The real body holds 16
 * instructions and 62 bytes up to the jump at 0x81aac.  The two bytes at
 * 0x81aae are padding, and the next entry `sound_effects_update` starts at
 * 0x81ab0.  The body sweep of `le.function_body` runs past the jump to the
 * first `ret` at 0x81b9a, because the jump is not the first instruction of the
 * function.  The plan row counted that longer sweep before this rebuild.
 *
 * The entry stores the driver pointer in the global `0x7451c`, saves all
 * registers and the four segment registers, calls the reset helper at `0x82e0c`,
 * clears the active count `0x72488`, loads the destination `0x40330` in `edi`,
 * and jumps to the shared service body at `0x81ac6`.
 *
 * The entry `sound_effects_update` at `0x81ab0` joins the same body at
 * `0x81ac6`.  That entry holds the body, so this module holds the `sound_update`
 * entry only and calls the body as `sound_service_body`.  The body starts with
 * the source update at `0x8a958`, which uses the destination pointer in `edi`.
 *
 * The original also computes `ecx` from `0x7a7f4` and `eax` from `0x7a7bc`.
 * The shared body overwrites both before it reads them, so the rebuild omits
 * both.  The original also sets DS, ES, FS, and GS; the flat target keeps one
 * data segment, so the rebuild omits the segment loads.
 */
extern int sound_driver_handle;
extern int sound_active_count;

extern void sound_reset(void);
extern void sound_service_body(void *destination);

void sound_update(int driver)
{
    sound_driver_handle = driver;
    sound_reset();
    sound_active_count = 0;
    sound_service_body((void *)0x40330);
}
