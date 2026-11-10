/* The rebuilt `array_sort` module.
 *
 * The original sits at linear 0x83500 in STARTH.EXE and holds 39 instructions
 * and 93 bytes up to the `ret` at 0x8355c.  The next store record starts at
 * 0x835c0, so the record keeps the 192-byte extent and sets no separate
 * function size.  The identity check uses 39 instructions.
 *
 * The only direct callers are `sound_effects_update` at 0x81ab0, with the call
 * site at 0x81af0, and `sound_frame_prepare` at 0x8af08, with the call sites at
 * 0x8af64 and 0x8af9a.  Every caller puts the array base in EDI, the element
 * count in EAX, and the element size 4 in EDX before the call, so the function
 * sorts an array of signed dwords in place.  The array at 0x45790 and the array
 * at 0x7a6e4 belong to the sound code, so the record takes the section
 * object_update_all callees.
 *
 * The function is a quicksort:
 *
 *     range  take the element at the middle of the range as the pivot and swap
 *            it to the start of the range
 *     split  move every element below the pivot to the front part of the range
 *            and keep the pivot at the start
 *     left   process the start part, the pivot and the lower elements, first
 *     right  keep the end part, the pivot and the higher elements, for later
 *
 * The original keeps the pending end parts on the machine stack.  It writes
 * the entry ESP into the immediate of a `cmp esp, imm32`, so the test at the
 * end of a range compares ESP with the entry value: an equal value means the
 * pending stack is empty.  The rebuilt C function recurses on the two parts,
 * which keeps the same order and the same sorted result.
 *
 * The original has no symbol, so the data flow supports the name array_sort.
 * The record keeps the name with the confidence inferred.  The original saves
 * no register and has no EBP frame, and the function calls no helper and uses
 * no floating-point instruction.  The module uses no assembly implementation.
 */

void array_sort(int *values, int count)
{
    int *low;
    int *high;
    int *mid;
    int *left;
    int *scan;
    int pivot;
    int value;

    if (count < 2) {
        return;
    }
    low = values;
    high = values + count;
    mid = low + (count >> 1);
    pivot = *mid;
    *mid = *low;
    *low = pivot;
    left = low + 1;
    for (scan = low + 1; scan < high; scan++) {
        value = *scan;
        if (pivot > value) {
            *scan = *left;
            *left = value;
            left++;
        }
    }
    array_sort(values, (int)(left - low));
    array_sort(left, (int)(high - left));
}
