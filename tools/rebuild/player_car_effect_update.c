/* The rebuilt `player_car_effect_update` module.
 *
 * The original sits at linear 0x15bac in STARTH.EXE and holds 23 instructions
 * and 78 bytes up to the `ret` at 0x15bf9.  The next function starts at
 * 0x15bfc, so the store record keeps the 80-byte extent and sets no separate
 * function size.
 *
 * The five view groups at 0x2003c, 0x201d0, 0x202c0, 0x20360, and 0x203c4
 * call this function on the player object at object-3 offset 0xf91c, after
 * the player physics group at 0x15a80 and before the step group at 0x15ab8.
 * The function is the effect group of the player car update.
 *
 * The object pointer arrives in `eax`.  The function runs six player car
 * steps in order: the suspension update at 0x168b4, the skid update at
 * 0x17d14, the surface effect update at 0x1814c, the reset update at 0x18528,
 * the spin update at 0x188b8, and the wheel effect update at 0x19980.  It
 * then calls the car sound update at 0x242bc only when bit 0 of the byte
 * `DAT_00118e1c` is set.
 *
 * The call order, the object forward, and the effect steps support the name
 * `player_car_effect_update`.  The original has no symbol, so the record
 * keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and makes no math call, so the
 * module needs no FPU stub, and the module uses no assembly implementation.
 */
extern unsigned char DAT_00118e1c;

extern void player_car_suspension_update(void *object);
extern void player_car_skid_update(void *object);
extern void player_car_surface_effect_update(void *object);
extern void player_car_reset_update(void *object);
extern void player_car_spin_update(void *object);
extern void player_car_wheel_effect_update(void *object);
extern void car_sound_update(void *object);

void player_car_effect_update(void *object)
{
    player_car_suspension_update(object);
    player_car_skid_update(object);
    player_car_surface_effect_update(object);
    player_car_reset_update(object);
    player_car_spin_update(object);
    player_car_wheel_effect_update(object);
    if ((DAT_00118e1c & 1) != 0) {
        car_sound_update(object);
    }
}
