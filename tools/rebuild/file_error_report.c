/* The rebuilt `file_error_report` module.
 *
 * The original sits at linear 0x4bf88 in STARTH.EXE and holds 88 instructions
 * and 261 bytes: the six register saves, the frame, the three frame locals, the
 * status read at `+0xac`, the five message bodies, the jump-table dispatch at
 * `0x4c04c`, and the final `exit(-2)` call at `0x300be`.
 *
 * The status field selects one message.  The message goes through the printer
 * at `0x4bf64` with the stream `0x2fd2`.  The name field at `+0x78` is the
 * engine interface name:
 *
 *     1        "Cannot open main file: %s" with the name
 *     2        "File %s not present in main file." with the name
 *     3        "Out of memory."
 *     4        "Out of file domain."
 *     5        "Out of memory to decode file."
 *     other    "Unknown error in Engine interface <%s>.\n" with the name
 *
 * The five status values are dense, so the compiler builds a jump table and
 * not a comparison tree.  The original holds the EBP frame, so the module
 * builds with `-of+`.
 */
extern void __cdecl report(char *stream, char *format, ...);
extern void exit(int status);

typedef struct file_interface {
    char reserved_00[0x78];
    char name[0x34];
    int status;
} file_interface;

void file_error_report(file_interface *file)
{
    if (file->status == 1) {
        report((char *)0x2fd2, "Cannot open main file: %s",
               (char *)file + 0x78);
    } else if (file->status == 2) {
        report((char *)0x2fd2, "File %s not present in main file.",
               (char *)file + 0x78);
    } else if (file->status == 3) {
        report((char *)0x2fd2, "Out of memory.");
    } else if (file->status == 4) {
        report((char *)0x2fd2, "Out of file domain.");
    } else if (file->status == 5) {
        report((char *)0x2fd2, "Out of memory to decode file.");
    } else {
        report((char *)0x2fd2, "Unknown error in Engine interface <%s>.\n",
               (char *)file + 0x78);
    }
    exit(-2);
}
