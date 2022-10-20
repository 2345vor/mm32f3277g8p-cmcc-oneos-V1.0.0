#include <string.h>
#include <os_memory.h>
#include <os_assert.h>
#include <dlog.h>
#include <vfs.h>
#include "vfs_private.h"
#include <os_mutex.h>

#define FD_TO_FDTABID(fd)       (fd - VFS_FD_OFFSET)
#define FDTABID_TO_FD(id)       (id + VFS_FD_OFFSET)

struct fd_entry   {
    os_uint16_t     ref_cnt;            /* Descriptor reference count */
    struct vfs_file file;
};

static struct fd_entry fdtable[VFS_FD_MAX];
os_mutex_t fs_lock;

int fd_alloc(void)
{
    int id = 0;
    int fd = -1;

    VFS_LOCK();
    for (id = 0; id < VFS_FD_MAX; id++)
    {
        if (fdtable[id].ref_cnt == 0)
        {
            fdtable[id].ref_cnt++;
            break;
        }
    }
    VFS_UNLOCK();

    if (id < VFS_FD_MAX)
    {
        fd = FDTABID_TO_FD(id);
    }
    else
    {
        LOG_E(VFS_TAG, "fd table is not enough, you may enlarge VFS_FD_MAX");
    }

    return fd;
}

void fd_ref_inc(int fd)
{
    int id;

    id = FD_TO_FDTABID(fd);
    OS_ASSERT((id >= 0) && (id < VFS_FD_MAX));

    VFS_LOCK();
    fdtable[id].ref_cnt++;
    VFS_UNLOCK();
}

void fd_ref_dec(int fd)
{
    int id;

    id = FD_TO_FDTABID(fd);
    OS_ASSERT((id >= 0) && (id < VFS_FD_MAX));

    VFS_LOCK();
    OS_ASSERT(fdtable[id].ref_cnt > 0);
    if (--fdtable[id].ref_cnt == 0)
    {
        if (fdtable[id].file.path)
        {
            os_free((void *)fdtable[id].file.path);
        }
        memset(&fdtable[id].file, 0, sizeof(struct vfs_file));
    }
    VFS_UNLOCK();
}

void fd_free(int fd)
{
    fd_ref_dec(fd);
}

struct vfs_file *fd_to_fp(int fd)
{
    int id;
    struct vfs_file *fp = OS_NULL;

    id = FD_TO_FDTABID(fd);
    if ((id >= 0) && (id < VFS_FD_MAX))
    {
        if (&fdtable[id].ref_cnt > 0)
        {
            fp = &fdtable[id].file;
        }
    }

    return fp;
}

struct vfs_dir *dp_alloc(void)
{
    int id = 0;
    struct vfs_dir *dp = OS_NULL;

    dp = os_malloc(sizeof(struct vfs_dir));
    if (dp)
    {
        memset(dp, 0, sizeof(struct vfs_dir));
        VFS_LOCK();
        for (id = 0; id < VFS_FD_MAX; id++)
        {
            if (fdtable[id].ref_cnt == 0)
            {
                fdtable[id].ref_cnt++;
                dp->fp = &fdtable[id].file;
                break;
            }
        }
        VFS_UNLOCK();

        if (!dp->fp)
        {
            os_free(dp);
            dp = OS_NULL;
            LOG_E(VFS_TAG, "fd table is not enough, you may enlarge VFS_FD_MAX");
        }
    }

    return dp;
}


void dp_ref_inc(struct vfs_dir *dp)
{
    struct fd_entry *fd_entry = OS_NULL;

    OS_ASSERT(dp && dp->fp);
    fd_entry = os_container_of(dp->fp, struct fd_entry, file);
    OS_ASSERT_EX((fd_entry >= &fdtable[0]) &&  (fd_entry <= &fdtable[VFS_FD_MAX - 1]), "Invalid directory stream");
    VFS_LOCK();
    fd_entry->ref_cnt++;
    VFS_UNLOCK();
}

void dp_ref_dec(struct vfs_dir *dp)
{
    struct fd_entry *fd_entry = OS_NULL;

    OS_ASSERT(dp && dp->fp);
    fd_entry = os_container_of(dp->fp, struct fd_entry, file);
    OS_ASSERT_EX((fd_entry >= &fdtable[0]) &&  (fd_entry <= &fdtable[VFS_FD_MAX - 1]), "Invalid directory stream");
    VFS_LOCK();
    OS_ASSERT(fd_entry->ref_cnt > 0);
    if (--fd_entry->ref_cnt == 0)
    {
        if (dp->fp->path)
        {
            os_free((void *)dp->fp->path);
        }
        memset(dp->fp, 0, sizeof(struct vfs_file));
        os_free(dp);
    }
    VFS_UNLOCK();
}

void dp_free(struct vfs_dir *dp)
{
    dp_ref_dec(dp);
}

void fd_table_init(void)
{
    memset(fdtable, 0, sizeof(fdtable));
}

#ifdef OS_USING_SHELL
#include <shell.h>

static char *fd_type_str[FT_TYPEMAX] = {
    "INVALID",
    "FILE   ",
    "DIR    ",
    "DEV    ",
};

os_err_t sh_fdshow(os_int32_t argc, char **argv)
{
    os_uint32_t i;
    struct vfs_file *fp;

    OS_UNREFERENCE(argc);
    OS_UNREFERENCE(argv);

    os_kprintf("  fd    type    mnt_path    path\r\n");
    os_kprintf("  --   ------   --------    ------\r\n");
    VFS_LOCK();
    for (i = 0; i < VFS_FD_MAX; i++)
    {
        fp = &fdtable[i].file;
        if (fp->mnt_point)
        {
            OS_ASSERT(fp->type < FT_TYPEMAX);
            os_kprintf("  %2d    %s    %s    %s\r\n", 
                       FDTABID_TO_FD(i), fd_type_str[fp->type], fp->mnt_point->mnt_path, fp->path);
        }
    }
    VFS_UNLOCK();

    return OS_EOK;
}
SH_CMD_EXPORT(fd_show, sh_fdshow, "show fd information");
#endif

