#include <string.h>
#include <oneos_config.h>
#include <os_memory.h>
#include <os_util.h>
#include <os_assert.h>
#include <dlog.h>
#include <vfs.h>
#include <fcntl.h>
#include <sys/errno.h>
#include "vfs_private.h"

#ifdef OS_USING_VFS_DEVFS
#include <vfs_devfs.h>
#endif

#define MNT_PT_INITED       (0x5A5A)
#define VFS_MKFS_MAX        VFS_MOUNTPOINT_MAX

enum vfs_dev_ref_stat
{
    DEV_REF_NONE,
    DEV_REF_MOUNT,
    DEV_REF_MKFS,
    DEV_REF_NO_SPACE
};

static const struct vfs_filesystem_ops *fs_ops_table[VFS_FILESYSTEM_TYPES_MAX];
static struct vfs_mountpoint mnt_point_table[VFS_MOUNTPOINT_MAX];
static os_device_t *dev_of_mkfs[VFS_MKFS_MAX];

static const struct vfs_filesystem_ops *_vfs_fs_ops_get(const char *fs_name)
{
    os_uint8_t i;
    const struct vfs_filesystem_ops *fs_ops = OS_NULL;

    VFS_LOCK();
    for (i = 0; i < VFS_FILESYSTEM_TYPES_MAX; i++)
    {
        if ((fs_ops_table[i]) && (strcmp(fs_ops_table[i]->fs_name, fs_name) == 0))
        {
            fs_ops = fs_ops_table[i];
            break;
        }
    }
    VFS_UNLOCK();

    return fs_ops;
}

static enum vfs_dev_ref_stat _vfs_mkfs_dev_ref(os_device_t *dev)
{
    os_uint8_t i;
    enum vfs_dev_ref_stat status;

    status = DEV_REF_NONE;

    VFS_LOCK();

    /* Check whether this device has been mounted. */
    for (i = 0; i < VFS_MOUNTPOINT_MAX; i++)
    {
        if (mnt_point_table[i].dev == dev)
        {
           status = DEV_REF_MOUNT;
           break;
        }
    }

    /* Check whether this device is doing mkfs. */
    if (DEV_REF_NONE == status)
    {
        for (i = 0; i < VFS_MKFS_MAX; i++)
        {
            if (dev_of_mkfs[i] == dev)
            {
                status = DEV_REF_MKFS;
                break;
            }
        }
    }

    /* Reference this device. */
    if (DEV_REF_NONE == status)
    {
        for (i = 0; i < VFS_MKFS_MAX; i++)
        {
            if (!dev_of_mkfs[i])
            {
                dev_of_mkfs[i] = dev;
                break;
            }
        }

        if (i >= VFS_MKFS_MAX)
        {
            status = DEV_REF_NO_SPACE;
        }
    }

    VFS_UNLOCK();

    return status;
}

static void _vfs_mkfs_dev_deref(os_device_t *dev)
{
    os_uint8_t i;

    VFS_LOCK();

    for (i = 0; i < VFS_MKFS_MAX; i++)
    {
        if (dev_of_mkfs[i] == dev)
        {
            dev_of_mkfs[i] = OS_NULL;
            break;
        }
    }

    VFS_UNLOCK();
}

static char *_vfs_mntpath_get(const char *path)
{
    char *mnt_path = OS_NULL;

    mnt_path = vfs_create_absolute_path(OS_NULL, path);
    if (mnt_path)
    {
        /* If not root dir and /dev, should check whether the directory exist. */
        if ((strcmp(mnt_path, "/") != 0) && (strcmp(mnt_path, DEVFS_PATH) != 0))
        {
            struct vfs_dir *dp;
            int result = 0;

            dp = dp_alloc();
            if (dp)
            {
                dp_ref_inc(dp);
                result = do_opendir(dp, mnt_path, 0);
                if (result >= 0)
                {
                    do_closedir(dp);
                }
                else
                {
                    os_free(mnt_path);
                    mnt_path = OS_NULL;
                }
                dp_ref_dec(dp);
                dp_free(dp);
            }
        }
    }

    return mnt_path;
}

static struct vfs_mountpoint *_vfs_mount_point_add(os_device_t *dev, char *mnt_path, const struct vfs_filesystem_ops *fs_ops)
{
    os_uint8_t i;
    struct vfs_mountpoint *mnt_point = OS_NULL;
    os_bool_t use_flag;

    use_flag = OS_FALSE;

    VFS_LOCK();

    for (i = 0; i < VFS_MKFS_MAX; i++)
    {
        if (dev && (dev_of_mkfs[i] == dev))
        {
            LOG_E(VFS_TAG, "ERROR. This device is doing mkfs, please try later.");
            use_flag = OS_TRUE;
            break;
        }
    }

    if (OS_FALSE == use_flag)
    {
        for (i = 0; i < VFS_MOUNTPOINT_MAX; i++)
        {
            if (mnt_point_table[i].ops)
            {
               /* Foreach item in mnt_point_table, if path already been mounted, return NULL. */
                if ((strcmp(mnt_point_table[i].mnt_path, mnt_path) == 0)
                    || (dev && (mnt_point_table[i].dev == dev)))
                {
                   LOG_E(VFS_TAG, "ERROR. Path or device has been mounted before.");
                   mnt_point = OS_NULL;
                   break;
                }
            }
            else
            {
               /* Use the first empty item. */
                if (!mnt_point)
                {
                    mnt_point = &mnt_point_table[i];
                }
            }
        }

        if (mnt_point)
        {
            mnt_point->mnt_path = mnt_path;
            mnt_point->ops = fs_ops;
            mnt_point->dev = dev;
        }
    }

    VFS_UNLOCK();

    return mnt_point;
}

static void _vfs_mount_point_set_init(struct vfs_mountpoint *mnt_point)
{
    VFS_LOCK();
    OS_ASSERT(mnt_point->is_inited != MNT_PT_INITED);
    mnt_point->is_inited = MNT_PT_INITED;
    VFS_UNLOCK();
}

static void _vfs_mount_point_del(struct vfs_mountpoint *mnt_point, char *mnt_path)
{
    if (mnt_point)
    {
        VFS_LOCK();
        memset(mnt_point, 0, sizeof(struct vfs_mountpoint));
        VFS_UNLOCK();
    }

    if (mnt_path)
    {
        os_free(mnt_path);
    }
}

static struct vfs_mountpoint *_vfs_mount_point_find_from_mntpath_and_ref(const char* mnt_path)
{
    os_uint16_t i = 0;
    struct vfs_mountpoint *mnt_point = OS_NULL;

    VFS_LOCK();
    for (i = 0; i < VFS_MOUNTPOINT_MAX; i++)
    {
        if ((mnt_point_table[i].mnt_path) && (strcmp(mnt_point_table[i].mnt_path, mnt_path) == 0))
        {
            mnt_point = &mnt_point_table[i];
            mnt_point->ref_cnt++;
            break;
        }
    }
    VFS_UNLOCK();

    return mnt_point;
}

static int _vfs_mount_point_checkref_and_unmount(struct vfs_mountpoint *mnt_point)
{
    int ret = 0;
    struct vfs_mountpoint mnt_point_tmp;

    VFS_LOCK();
    if (mnt_point->ref_cnt == 1)
    {
        /* Back up this mnt_point info to allow to call specify filesystem's unmount.*/
        memcpy(&mnt_point_tmp, mnt_point, sizeof(struct vfs_mountpoint));
        /* Delete this mnt_point to prevent other task access this mnt_point when we do unmout. */
        memset(mnt_point, 0, sizeof(struct vfs_mountpoint));
    }
    else
    {
        mnt_point->ref_cnt--;
        LOG_E(VFS_TAG, "Can't unmount now. This mount point was refereced, ref_cnt:%d.", mnt_point->ref_cnt);
        LOG_E(VFS_TAG, "You may use cmd: fd_show to see whether referenced by opened file/dir.");
        VFS_SET_ERRNO(-EBUSY);
        ret = -1;
    }
    VFS_UNLOCK();

    if (ret == 0)
    {
        ret = mnt_point_tmp.ops->unmount(&mnt_point_tmp);
        if (ret >= 0)
        {
            os_free(mnt_point_tmp.mnt_path);
        }
        else    /* If unfortunately unmount fail, we should restore it back to mnt_point_table. */
        {
            _vfs_mount_point_add(mnt_point_tmp.dev, mnt_point_tmp.mnt_path, mnt_point_tmp.ops);
        }
    }

    return ret;
}

struct vfs_mountpoint *vfs_mount_point_find_and_ref(const char* abspath)
{
    struct vfs_mountpoint *mnt_point = OS_NULL;
    os_uint32_t mnt_path_len;
    os_uint32_t path_len;
    os_uint32_t already_match_len = 0;
    int i = 0;

    if (abspath)
    {
        path_len = strlen(abspath);
        VFS_LOCK();
        for (i = 0; i < VFS_MOUNTPOINT_MAX; i++)
        {
            if ((!mnt_point_table[i].mnt_path) || (!mnt_point_table[i].ops) || (mnt_point_table[i].is_inited != MNT_PT_INITED))
            {
                continue;
            }

            mnt_path_len = strlen(mnt_point_table[i].mnt_path);
            if ((mnt_path_len < already_match_len) || (mnt_path_len > path_len))
            {
                continue;
            }

            /* check whether path have directory separator '/' at the the mount_path end.*/
            if ((mnt_path_len > 1) && (abspath[mnt_path_len] != '/') && (abspath[mnt_path_len] != '\0'))
            {
                continue;
            }

            if (strncmp(mnt_point_table[i].mnt_path, abspath, mnt_path_len) == 0)
            {
                mnt_point = &mnt_point_table[i];
                if (mnt_path_len == path_len)
                {
                    /* Find the best match mnt_path, break. */
                    break;
                }
                else/* mnt_path_len < path_len */
                {
                    /* Find a match mnt_path, but maybe not the longest match, 
                        record it, and then continue to search longger match.*/
                    already_match_len = mnt_path_len;
                }
            }
        }

        if (mnt_point)
        {
            mnt_point->ref_cnt++;
        }
        VFS_UNLOCK();
    }

    return mnt_point;
}

void vfs_mount_point_deref(struct vfs_mountpoint *mnt_point)
{
    VFS_LOCK();
    OS_ASSERT(mnt_point->ref_cnt > 0);
    mnt_point->ref_cnt--;
    VFS_UNLOCK();
}

int vfs_register(const struct vfs_filesystem_ops *fs_ops)
{
    os_uint8_t i;
    int ret = -1;

    OS_ASSERT(fs_ops);

    VFS_LOCK();
    for (i = 0; i < VFS_FILESYSTEM_TYPES_MAX; i++)
    {
        if (fs_ops_table[i])
        {
            if (strcmp(fs_ops_table[i]->fs_name, fs_ops->fs_name) == 0)
            {
                LOG_E(VFS_TAG, "Filesystem %s has already been registerd\r\n", fs_ops->fs_name);
                break;
            }
        }
        else
        {
            fs_ops_table[i] = fs_ops;
            ret = 0;
            break;
        }
    }
    VFS_UNLOCK();
    if (VFS_FILESYSTEM_TYPES_MAX == i)
    {
        LOG_E(VFS_TAG, "Filesytem ops table is not enough, you may enlarge VFS_FILESYSTEM_TYPES_MAX\r\n");
    }

    return ret;
}

int vfs_mount(const char *dev_name, const char *path, const char *fs_name, unsigned long mountflag, const void *data)
{
    const struct vfs_filesystem_ops *fs_ops = OS_NULL;
    struct vfs_mountpoint *mnt_point = OS_NULL;
    os_device_t *dev = OS_NULL;
    int ret = 0;
    char *mnt_path = OS_NULL;

    OS_ASSERT(path);
    OS_ASSERT(fs_name);

    /* For some fs, dev_name may not mandatory. But if need dev_name, we should check device exist. */
    if (dev_name)
    {
        dev = os_device_find(dev_name);
        if (!dev)
        {
            VFS_SET_ERRNO(-ENODEV);
            ret = -1;
        }
    }

    /* Check whether the path exist.*/
    if (ret == 0)
    {
        mnt_path = _vfs_mntpath_get(path);
        if (!mnt_path)
        {
            VFS_SET_ERRNO(-ENOTDIR);
            ret = -1;
        }
    }

    /* Find the fs ops. */
    if (ret == 0)
    {
        fs_ops = _vfs_fs_ops_get(fs_name);
        if ((!fs_ops) || (!fs_ops->mount))
        {
            VFS_SET_ERRNO(-ENOSYS);
            ret = -1;
        }
    }

    /* Get an entry to save the mount point info. */
    if (ret == 0)
    {
        mnt_point = _vfs_mount_point_add(dev, mnt_path, fs_ops);
        if (!mnt_point)
        {
            VFS_SET_ERRNO(-ENOSPC);
            ret = -1;
        }
    }

    /* Mount the specific filesystem. */
    if (ret == 0)
    {
        ret = fs_ops->mount(mnt_point, mountflag, data);
    }

    if (ret >= 0)
    {
        _vfs_mount_point_set_init(mnt_point);
        LOG_I(VFS_TAG, "Mount %s to %s", fs_name, path);
    }
    else
    {
        VFS_SET_ERRNO(ret);
        _vfs_mount_point_del(mnt_point, mnt_path);
    }

    return ret;
}

int vfs_unmount(const char *path)
{
    char *mnt_path;
    int ret = 0;
    struct vfs_mountpoint *mnt_point = OS_NULL;

    mnt_path = vfs_create_absolute_path(OS_NULL, path);
    if (mnt_path)
    {
        mnt_point = _vfs_mount_point_find_from_mntpath_and_ref(mnt_path);
        if (mnt_point && mnt_point->ops && mnt_point->ops->unmount)
        {
            ret = _vfs_mount_point_checkref_and_unmount(mnt_point);
            if (ret < 0)
            {
                VFS_SET_ERRNO(ret);
            }
        }
        else
        {
            VFS_SET_ERRNO(-ENOENT);
            ret = -1;
        }
        os_free(mnt_path);
    }
    else
    {
        VFS_SET_ERRNO(-ENOENT);
        ret = -1;
    }

    return ret;
}

int vfs_mkfs(const char *fs_name, const char *dev_name)
{
    int ret = 0;
    os_device_t *dev = OS_NULL;
    const struct vfs_filesystem_ops *fs_ops = OS_NULL;
    enum vfs_dev_ref_stat status;

    dev = os_device_find(dev_name);
    if (dev)
    {
        status = _vfs_mkfs_dev_ref(dev);
        if (DEV_REF_MOUNT == status)
        {
            LOG_E(VFS_TAG, "This device has been mounted, you should unmount it before mkfs");
            VFS_SET_ERRNO(-EBUSY);
            ret = -1;
        }
        else if (DEV_REF_MKFS == status)
        {
            LOG_E(VFS_TAG, "This device is already doing mkfs, you may try it later");
            VFS_SET_ERRNO(-EBUSY);
            ret = -1;
        }
        else if (DEV_REF_NO_SPACE == status)
        {
            LOG_E(VFS_TAG, "Too many device is doing mkfs, you may try it later");
            VFS_SET_ERRNO(-EBUSY);
            ret = -1;
        }
        else
        {
            ret = 0;
        }
    }
    else
    {
        LOG_E(VFS_TAG, "Device (%s) was not found", dev_name);
        VFS_SET_ERRNO(-ENODEV);
        ret = -1;
    }

    if (ret == 0)
    {
        fs_ops = _vfs_fs_ops_get(fs_name);
        if (fs_ops && fs_ops->mkfs)
        {
            ret = fs_ops->mkfs(dev);
        }
        else
        {
            LOG_E(VFS_TAG, "The file system (%s) mkfs function was not found", fs_name);
            VFS_SET_ERRNO(-ENOSYS);
            ret = -1;
        }
        _vfs_mkfs_dev_deref(dev);
    }

    return ret;
}

int vfs_init(void)
{
    VFS_LOCK_INIT();
    memset((void *)fs_ops_table, 0, sizeof(fs_ops_table));
    memset((void *)mnt_point_table, 0, sizeof(mnt_point_table));
    memset((void *)dev_of_mkfs, 0, sizeof(dev_of_mkfs));
    working_dir_init();
    fd_table_init();
#ifdef OS_USING_VFS_DEVFS
    vfs_devfs_init();
    vfs_mount(OS_NULL, DEVFS_PATH, "dev", 0, OS_NULL);
#endif
    return 0;
}
OS_POSTCORE_INIT(vfs_init, OS_INIT_SUBLEVEL_HIGH);

