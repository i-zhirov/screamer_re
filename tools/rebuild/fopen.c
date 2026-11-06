/* The rebuilt `fopen` module.
 *
 * The original sits at linear 0x4c08d in STARTH.EXE and holds 48 instructions
 * and 155 bytes: the two register saves, the frame, the four argument slots,
 * the run-time open call at `0xc5fe5`, the three `strcpy` calls at `0xd47ee`,
 * the member search call at `0x4c128`, and the epilogue.
 *
 * The function opens the container path with the mode `rb` and stores the
 * handle at `stream + 0x00`.  When the open fails, it copies the path to
 * `stream + 0x78`, sets the state at `stream + 0xac` to 1, and returns -1.
 * Otherwise it copies the category to `stream + 0x20` and the path to
 * `stream + 0x24`, then searches the member through `0x4c128`.  A failed
 * search returns -1.  A found member returns 0.
 */
extern int file_open(char *path, char *mode);
extern char *strcpy(char *destination, char *source);
extern int member_open(char *stream, char *name);

int fopen(char *stream, char *path, char *name, char *category)
{
    int result;

    *(int *)stream = file_open(path, "rb");
    if (*(int *)stream == 0) {
        strcpy(stream + 0x78, path);
        *(int *)(stream + 0xac) = 1;
        result = -1;
    } else {
        strcpy(stream + 0x20, category);
        strcpy(stream + 0x24, path);
        if (member_open(stream, name) != 0) {
            result = -1;
        } else {
            result = 0;
        }
    }
    return result;
}
