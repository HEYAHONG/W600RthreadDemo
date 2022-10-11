#include <stdio.h>
#include <string.h>
#include <RC.h>
/*
替换oneshot apweb模式下的文件系统,自定义oneshot的web界面。利用$Sub$$实现替换。
*/

#define ONESHOT_RC_DIR "oneshot/"
#define OVERRIDE(x) $Sub$$##x

struct fs_file
{
    char *data;
    int len;
    int index;
    int ReadIndex;
    void *pextension;
};

struct fs_file *OVERRIDE(fs_open)(char *name)
{
    if (name == NULL || strlen(name) == 0)
    {
        return NULL;
    }
    struct fs_file *file = (struct fs_file *)malloc(sizeof(struct fs_file));
    memset(file, 0, sizeof(struct fs_file));

    char *rc_name = (char *)malloc(strlen(name) + sizeof(ONESHOT_RC_DIR) + 1);
    memset(rc_name, 0, strlen(name) + sizeof(ONESHOT_RC_DIR) + 1);
    strcat(rc_name, ONESHOT_RC_DIR);
    {
        for (size_t i = 0; i < strlen(name); i++)
        {
            if (name[i] == '/' && rc_name[strlen(rc_name) - 1] == '/')
            {
                continue;
            }
            rc_name[strlen(rc_name)] = name[i];
        }
    }

    file->data = (char *)RCGetHandle(rc_name);
    file->len = RCGetSize(rc_name);
    file->index = file->len;

    free(rc_name);

    return file;
}
void OVERRIDE(fs_close)(struct fs_file *file)
{
    if (file == NULL)
    {
        return;
    }
    free(file);
}

int OVERRIDE(fs_read)(struct fs_file *file, char *buffer, int count)
{
    if (file == NULL || file->data == NULL)
    {
        return -1;
    }

    if (file->index >= file ->len)
    {
        return -1;
    }

    if (count >= (file->len - file->index))
    {
        count = (file->len - file->index);
    }

    memcpy(buffer, file->data + file->index, count);
    file->index += count;

    return count;

}



