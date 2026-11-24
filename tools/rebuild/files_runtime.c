/* The interim files and streams cluster.
 *
 * The whole-image link reports these symbols as undefined because the shipped
 * image holds the file open and close, the member open, the rename and delete,
 * the resource cache and release, the stream alloc, release, restore, and seek,
 * the stream setup cases, the replay helpers, and the table entry and read
 * helpers.  This module defines the 21 entry points with the calling convention
 * of their callers.
 *
 * The functions are a compatibility layer, not a byte-faithful recovery.  The
 * void functions do nothing, the int and pointer functions return 0, and the
 * module has no function record and no behavior check, so it does not enter the
 * verified count.
 */
void *cache_resource(void *handle, char *record, void *value, char *name,
                     int size)
{
    (void)handle;
    (void)record;
    (void)value;
    (void)name;
    (void)size;
    return 0;
}

void delete_file(char *path)
{
    (void)path;
}

int file_open(char *path, char *mode)
{
    (void)path;
    (void)mode;
    return 0;
}

int find_first(char *path, void *data)
{
    (void)path;
    (void)data;
    return 0;
}

int handle_close(int handle)
{
    (void)handle;
    return 0;
}

int member_open(char *stream, char *name)
{
    (void)stream;
    (void)name;
    return 0;
}

void rename_file(char *old_path, char *new_path)
{
    (void)old_path;
    (void)new_path;
}

int replay_meta_read(char *name)
{
    (void)name;
    return 0;
}

void replay_open(char *name, int value)
{
    (void)name;
    (void)value;
}

void resource_free_name(void *resource, char *name)
{
    (void)resource;
    (void)name;
}

int resource_release(void *resource, char *name)
{
    (void)resource;
    (void)name;
    return 0;
}

void stream_alloc(void *file)
{
    (void)file;
}

void stream_release(void *stream)
{
    (void)stream;
}

void stream_restore(void *file)
{
    (void)file;
}

int stream_seek(void *stream, int offset, int origin)
{
    (void)stream;
    (void)offset;
    (void)origin;
    return 0;
}

void stream_setup_0(void)
{
}

void stream_setup_1(void)
{
}

void stream_setup_2(void)
{
}

void stream_setup_5(void)
{
}

int table_entry_acquire(int type, int handle)
{
    (void)type;
    (void)handle;
    return 0;
}

int table_read(unsigned int base, int index, unsigned int entry)
{
    (void)base;
    (void)index;
    (void)entry;
    return 0;
}
