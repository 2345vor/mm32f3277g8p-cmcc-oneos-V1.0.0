#include <string.h>
#include <os_util.h>
#include <os_task.h>
#include <os_memory.h>
#include <fcntl.h>
#include <vfs.h>
#include <sys/errno.h>
#include "vfs_private.h"
#include <vfs_posix.h>

int vfs_open(const char *path, int flags, ...)
{
    int fd;
    int ret = 0;
    int result = 0;
    char *abs_path = OS_NULL;
    struct vfs_file *fp = OS_NULL;

    if (path)
    {
        fd = fd_alloc();
        if (fd >= 0)
        {
            fp = fd_to_fp(fd);
            if (!fp)
            {
                fd_free(fd);
                VFS_SET_ERRNO(-EBADF);
                ret = -1;
            }
        }
        else
        {
            VFS_SET_ERRNO(-ENOMEM);
            ret = -1;
        }
    }
    else
    {
        VFS_SET_ERRNO(-EINVAL);
        ret = -1;
    }

    if (ret == 0)
    {
        abs_path = vfs_create_absolute_path(OS_NULL, path);
        if (abs_path)
        {
            fd_ref_inc(fd);
            result = do_openfile(fp, abs_path, flags);
            fd_ref_dec(fd);
            if (result < 0)
            {
                fd_free(fd);
                VFS_SET_ERRNO(result);
                ret = -1;
            }
            os_free(abs_path);
        }
        else
        {
            fd_free(fd);
            VFS_SET_ERRNO(-ENOMEM);
            ret = -1;
        }
    }

    if (ret == -1)
    {
        fd = -1;
    }
    return fd;
}

int vfs_close(int fd)
{
    int ret = 0;
    int result = 0;
    struct vfs_file *fp = OS_NULL;

    fp = fd_to_fp(fd);
    if (fp)
    {
        fd_ref_inc(fd);
        result = do_closefile(fp);
        fd_ref_dec(fd);

        /* No matter close success or fail, both free fd. */
        fd_free(fd);
        if (result < 0)
        {
            VFS_SET_ERRNO(result);
            ret = -1;
        }
    }
    else
    {
        VFS_SET_ERRNO(-EBADF);
        ret = -1;
    }

    return ret;
}

int vfs_read(int fd, void *buf, size_t len)
{
    int result = -1;
    struct vfs_file *fp = OS_NULL;

    fp = fd_to_fp(fd);
    if (fp)
    {
        fd_ref_inc(fd);
        result = do_readfile(fp, buf, len);
        fd_ref_dec(fd);
        if (result < 0)
        {
            VFS_SET_ERRNO(result);
            result = -1;
        }
    }
    else
    {
        VFS_SET_ERRNO(-EBADF);
    }

    return result;
}

int vfs_write(int fd, const void *buf, size_t len)
{
    int result = -1;
    struct vfs_file *fp = OS_NULL;

    fp = fd_to_fp(fd);
    if (fp)
    {
        fd_ref_inc(fd);
        result = do_writefile(fp, buf, len);
        fd_ref_dec(fd);
        if (result < 0)
        {
            VFS_SET_ERRNO(result);
            result = -1;
        }
    }
    else
    {
        VFS_SET_ERRNO(-EBADF);
    }

    return result;
}

off_t vfs_lseek(int fd, off_t offset, int whence)
{
    int result = -1;
    struct vfs_file *fp = OS_NULL;

    fp = fd_to_fp(fd);
    if (fp)
    {
        fd_ref_inc(fd);
        result = do_lseekfile(fp, offset, whence);
        fd_ref_dec(fd);
        if (result < 0)
        {
            VFS_SET_ERRNO(result);
            result = -1;
        }
    }
    else
    {
        VFS_SET_ERRNO(-EBADF);
    }

    return result;
}

int vfs_fsync(int fd)
{
    int ret = 0;
    int result = 0;
    struct vfs_file *fp = OS_NULL;

    fp = fd_to_fp(fd);
    if (fp)
    {
        fd_ref_inc(fd);
        result = do_syncfile(fp);
        fd_ref_dec(fd);
        if (result < 0)
        {
            VFS_SET_ERRNO(result);
            ret = -1;
        }
    }
    else
    {
        VFS_SET_ERRNO(-EBADF);
        ret = -1;
    }

    return ret;
}

int vfs_fcntl(int fd, int cmd, ...)
{
    int ret = 0;
    int result = 0;
    struct vfs_file *fp = OS_NULL;
    va_list ap;
    void *arg;

    fp = fd_to_fp(fd);
    if (fp)
    {
        va_start(ap, cmd);
        arg = va_arg(ap, void *);
        va_end(ap);
        fd_ref_inc(fd);
        result = do_fcntl(fp, cmd, arg);
        fd_ref_dec(fd);
        if (result >= 0)
        {
            ret = result;
        }
        else
        {
            VFS_SET_ERRNO(result);
            ret = -1;
        }
    }
    else
    {
        VFS_SET_ERRNO(-EBADF);
        ret = -1;
    }

    return ret;
}

int vfs_ioctl(int fd, unsigned long request, ...)
{
    int ret = 0;
    int result = 0;
    struct vfs_file *fp = OS_NULL;
    va_list ap;
    void *arg;

    fp = fd_to_fp(fd);
    if (fp)
    {
        va_start(ap, request);
        arg = va_arg(ap, void *);
        va_end(ap);
        fd_ref_inc(fd);
        result = do_ioctl(fp, request, arg);
        fd_ref_dec(fd);
        if (result >= 0)
        {
            ret = result;
        }
        else
        {
            VFS_SET_ERRNO(result);
            ret = -1;
        }
    }
    else
    {
        VFS_SET_ERRNO(-EBADF);
        ret = -1;
    }

    return ret;
}

int vfs_mkdir(const char *path, mode_t mode)
{
    int ret = 0;
    int result = 0;
    char *abs_path = OS_NULL;
    struct vfs_dir *dp = OS_NULL;

    if (path)
    {
        abs_path = vfs_create_absolute_path(OS_NULL, path);
        if (abs_path)
        {
            dp = dp_alloc();
            if (dp)
            {
                dp_ref_inc(dp);
                result = do_opendir(dp, abs_path, O_CREAT);
                if (result >= 0)
                {
                    result = do_closedir(dp);
                }
                dp_ref_dec(dp);
                dp_free(dp);
                if (result < 0)
                {
                    VFS_SET_ERRNO(result);
                    ret = -1;
                }
            }
            else
            {
                VFS_SET_ERRNO(-ENOMEM);
                ret = -1;
            }
            os_free(abs_path);
        }
        else
        {
            VFS_SET_ERRNO(-ENOMEM);
            ret = -1;
        }
    }
    else
    {
        VFS_SET_ERRNO(-EINVAL);
        ret = -1;
    }

    return ret;
}

int vfs_rmdir(const char *path)
{
    int ret = 0;
    int result = 0;
    char *abs_path = OS_NULL;

    if (path)
    {
        abs_path = vfs_create_absolute_path(OS_NULL, path);
        if (abs_path)
        {
            result = do_unlink(abs_path);
            if (result < 0)
            {
                VFS_SET_ERRNO(result);
                ret = -1;
            }
            os_free(abs_path);
        }
        else
        {
            VFS_SET_ERRNO(-ENOMEM);
            ret = -1;
        }
    }
    else
    {
        VFS_SET_ERRNO(-EINVAL);
        ret = -1;
    }

    return ret;
}

FS_DIR *vfs_opendir(const char *path)
{
    int result = 0;
    char *abs_path = OS_NULL;
    struct vfs_dir *dp = OS_NULL;

    if (path)
    {
        abs_path = vfs_create_absolute_path(OS_NULL, path);
        if (abs_path)
        {
            dp = dp_alloc();
            if (dp)
            {
                dp_ref_inc(dp);
                result = do_opendir(dp, abs_path, 0);
                dp_ref_dec(dp);
                if (result < 0)
                {
                    dp_free(dp);
                    dp = OS_NULL;
                    VFS_SET_ERRNO(result);
                }
            }
            else
            {
                VFS_SET_ERRNO(-ENOMEM);
            }
            os_free(abs_path);
        }
        else
        {
            VFS_SET_ERRNO(-ENOMEM);
        }
    }
    else
    {
        VFS_SET_ERRNO(-EINVAL);
    }

    return dp;
}

int vfs_closedir(FS_DIR *dp)
{
    int ret = 0;
    int result = 0;

    if (dp)
    {
        dp_ref_inc(dp);
        result = do_closedir(dp);
        dp_ref_dec(dp);

        /* No matter close success or fail, both free dp. */
        dp_free(dp);
        if (result < 0)
        {
            VFS_SET_ERRNO(result);
            ret = -1;
        }
    }
    else
    {
        VFS_SET_ERRNO(-EBADF);
        ret = -1;
    }

    return ret;
}

/*
    In POSIX.1-2008, readdir is not thread-safe. In the GNU C Library implementation, it is safe to call
    readdir concurrently on different dirstreams, but multiple threads accessing the same dirstream 
    result in undefined behavior. readdir_r is a fully thread-safe alternative, but suffers from poor 
    portability. It is recommended that you use readdir, with externallocking if multiple 
    threads access the same dirstream.
*/
struct dirent *vfs_readdir(FS_DIR *dp)
{
    int result = 0;
    struct dirent *dentry = OS_NULL;

    if (dp)
    {
        dp_ref_inc(dp);
        result = do_readdir(dp, &dp->entry);
        dp_ref_dec(dp);
        if (result > 0)
        {
            dentry = &dp->entry;
        }
        else
        {
            VFS_SET_ERRNO(result);
        }
    }
    else
    {
        VFS_SET_ERRNO(-EBADF);
    }

    return dentry;
}

long vfs_telldir(FS_DIR *dp)
{
    int result = 0;

    if (dp)
    {
        dp_ref_inc(dp);
        result = do_telldir(dp);
        dp_ref_dec(dp);
        if (result < 0)
        {
            VFS_SET_ERRNO(result);
        }
    }
    else
    {
        VFS_SET_ERRNO(-EBADF);
    }

    return result;
}

void vfs_seekdir(FS_DIR *dp, off_t offset)
{
    int result = 0;

    if (dp)
    {
        dp_ref_inc(dp);
        result = do_seekdir(dp, offset);
        dp_ref_dec(dp);
        if (result < 0)
        {
            VFS_SET_ERRNO(result);
        }
    }
    else
    {
        VFS_SET_ERRNO(-EBADF);
    }
}

void vfs_rewinddir(FS_DIR *dp)
{
    int result = 0;

    if (dp)
    {
        dp_ref_inc(dp);
        result = do_seekdir(dp, 0);
        dp_ref_dec(dp);
        if (result < 0)
        {
            VFS_SET_ERRNO(result);
        }
    }
    else
    {
        VFS_SET_ERRNO(-EBADF);
    }
}

int vfs_unlink(const char *path)
{
    int ret = 0;
    int result = 0;
    char *abs_path = OS_NULL;

    if (path)
    {
        abs_path = vfs_create_absolute_path(OS_NULL, path);
        if (abs_path)
        {
            result = do_unlink(abs_path);
            if (result < 0)
            {
                VFS_SET_ERRNO(result);
                ret = -1;
            }
            os_free(abs_path);
        }
        else
        {
            VFS_SET_ERRNO(-ENOMEM);
            ret = -1;
        }
    }
    else
    {
        VFS_SET_ERRNO(-EINVAL);
        ret = -1;
    }

    return ret;
}

int vfs_rename(const char *oldpath, const char *newpath)
{
    int ret = 0;
    int result = 0;
    char *old_abs_path = OS_NULL;
    char *new_abs_path = OS_NULL;

    if (oldpath && newpath)
    {
        old_abs_path = vfs_create_absolute_path(OS_NULL, oldpath);
        new_abs_path = vfs_create_absolute_path(OS_NULL, newpath);
        if (old_abs_path && new_abs_path)
        {
            result = do_rename(old_abs_path, new_abs_path);
            if (result < 0)
            {
                VFS_SET_ERRNO(result);
                ret = -1;
            }
        }
        else
        {
            VFS_SET_ERRNO(-ENOMEM);
            ret = -1;
        }
        if (old_abs_path)
        {
            os_free(old_abs_path);
        }
        if (new_abs_path)
        {
            os_free(new_abs_path);
        }
    }
    else
    {
        VFS_SET_ERRNO(-EINVAL);
        ret = -1;
    }

    return ret;
}


int vfs_stat(const char *path, struct stat *buf)
{
    int ret = 0;
    int result = 0;
    char *abs_path = OS_NULL;

    if (path && buf)
    {
        abs_path = vfs_create_absolute_path(OS_NULL, path);
        if (abs_path)
        {
            result = do_stat(abs_path, buf);
            if (result < 0)
            {
                VFS_SET_ERRNO(result);
                ret = -1;
            }
            os_free(abs_path);
        }
        else
        {
            VFS_SET_ERRNO(-ENOMEM);
            ret = -1;
        }
    }
    else
    {
        VFS_SET_ERRNO(-EINVAL);
        ret = -1;
    }

    return ret;
}

int vfs_fstat(int fd, struct stat *buf)
{
    int ret = 0;
    int result = 0;
    char *abs_path = OS_NULL;
    struct vfs_file *fp = OS_NULL;

    if (fd && buf)
    {
        
        fp = fd_to_fp(fd);
        if (fp && fp->mnt_point)
        {
            abs_path = vfs_create_absolute_path(fp->mnt_point->mnt_path, fp->path);
        }
        if (abs_path)
        {
            result = do_stat(abs_path, buf);
            if (result < 0)
            {
                VFS_SET_ERRNO(result);
                ret = -1;
            }
            os_free(abs_path);
        }
        else
        {
            VFS_SET_ERRNO(-ENOMEM);
            ret = -1;
        }
    }
    else
    {
        VFS_SET_ERRNO(-EINVAL);
        ret = -1;
    }

    return ret;
}

int vfs_chdir(const char *path)
{
    int ret = 0;
    int result = 0;
    FS_DIR *dir;
    char *abs_path = OS_NULL;

    if (path)
    {
        dir = vfs_opendir(path);
        if (dir)
        {
            result = vfs_closedir(dir);
            if (result >= 0)
            {
                abs_path = vfs_create_absolute_path(OS_NULL,path);
                if (abs_path)
                {
                    working_dir_set(abs_path);
                    os_free(abs_path);
                }
                else
                {
                    ret = -1;
                }
            }
            else
            {
                ret = -1;
            }
        }
        else
        {
            ret = -1;
        }
    }
    else
    {
        ret = -1;
    }

    return ret;
}

char *vfs_getcwd(char *buf, size_t size)
{
    working_dir_get(buf, size);

    return buf;
}

int vfs_access(const char *path, int amode)
{
    int ret = 0;
    struct stat st;

    ret = vfs_stat(path, &st);

    return ret;
}

int vfs_statfs(const char *path, struct statfs *buf)
{
    int ret = 0;
    int result = 0;
    char *abs_path = OS_NULL;
    int fd = -1;
    FS_DIR *fs_dir = OS_NULL;

    if (path && buf)
    {
        fd = vfs_open(path, O_RDONLY);
        if (fd >= 0)
        {
            vfs_close(fd);
        }
        else
        {
            fs_dir = vfs_opendir(path);
            if (fs_dir)
            {
                vfs_closedir(fs_dir);
            }
        }

        if ((fd >= 0) || (fs_dir))
        {
            abs_path = vfs_create_absolute_path(OS_NULL, path);
            if (abs_path)
            {
                result = do_statfs(abs_path, buf);
                if (result < 0)
                {
                    VFS_SET_ERRNO(result);
                    ret = -1;
                }
                os_free(abs_path);
            }
            else
            {
                VFS_SET_ERRNO(-ENOMEM);
                ret = -1;
            }
        }
        else
        {
            VFS_SET_ERRNO(-EINVAL);
            ret = -1;
        }
    }
    else
    {
        VFS_SET_ERRNO(-EINVAL);
        ret = -1;
    }

    return ret;
}

