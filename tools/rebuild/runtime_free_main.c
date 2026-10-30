/* The test harness for the rebuilt run-time `free` module.
 *
 * The harness holds the free-list head, the end pointer, the largest free size,
 * the heap flag, and the heap stub.  The stub records its two arguments.  The
 * harness builds a two-node free list and calls the module with a block between
 * the two nodes, then with a block that matches the end pointer:
 *
 *     0  the walk stopped at the first node and the stub received the block
 *     1  the largest free size rose to the node value and the flag cleared
 *     2  the end-pointer guard kept the largest free size
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void free(void *p);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void heap_free(void *block_pointer, void *list);
#pragma aux heap_free parm [eax] [ebx];

void *free_list;
void *free_list_end;
unsigned int largest_free;
unsigned char heap_flag;

static char text[8];
static unsigned int heap[0x40];
static unsigned int *node_a;
static unsigned int *node_b;
static unsigned int *block;
static void *got_block;
static void *got_node;
static int heap_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void heap_free(void *block_pointer, void *list)
{
    got_block = block_pointer;
    got_node = list;
    heap_calls = heap_calls + 1;
}

static void build_list(void)
{
    int index;

    for (index = 0; index < 0x40; ++index) {
        heap[index] = 0;
    }
    node_a = &heap[0x10];
    node_b = &heap[0x20];
    block = &heap[0x18];
    node_a[2] = (unsigned int)node_b;
    node_b[2] = 0;
    node_a[5] = 0x200;
    node_b[5] = 0x100;
    free_list = node_a;
    free_list_end = 0;
    largest_free = 0x100;
    heap_flag = 1;
    heap_calls = 0;
}

int main(void)
{
    build_list();
    free(block);
    put(heap_calls == 1 && got_block == block && got_node == node_a ? '0' : 'x');
    put(largest_free == 0x200 && heap_flag == 0 ? '1' : 'x');

    build_list();
    free_list_end = node_a;
    largest_free = 0x100;
    heap_flag = 1;
    free(block);
    put(heap_calls == 1 && largest_free == 0x100 && heap_flag == 0 ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
