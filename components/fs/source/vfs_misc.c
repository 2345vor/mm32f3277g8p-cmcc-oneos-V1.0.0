#include <string.h>
#include <os_memory.h>
#include <os_mutex.h>
#include <os_assert.h>
#include <vfs.h>

static os_mutex_t fs_cwd_lock;

#define VFS_WORKDIR_INIT()          os_mutex_init(&fs_cwd_lock, "fscwdlock", OS_FALSE)
#define VFS_WORKDIR_LOCK()          os_mutex_lock(&fs_cwd_lock, OS_WAIT_FOREVER)
#define VFS_WORKDIR_UNLOCK()        os_mutex_unlock(&fs_cwd_lock)

static char working_dir[VFS_PATH_MAX] = {"/"};

void working_dir_init(void)
{
    VFS_WORKDIR_INIT();
    memset(working_dir, 0, sizeof(working_dir));
    working_dir[0] = '/';
}

char *working_dir_get(char *buf, size_t size)
{
    char *ptr;

    VFS_WORKDIR_LOCK();
    strncpy(buf, &working_dir[0], size);
    VFS_WORKDIR_UNLOCK();

    return ptr;
}

void working_dir_set(char *path)
{
    VFS_WORKDIR_LOCK();
    strncpy(working_dir, path, VFS_PATH_MAX);
    VFS_WORKDIR_UNLOCK();
}

static char *_vfs_conjunction_path(const char *dirpath, const char *filepath)
{
    char *fullpath = OS_NULL;
    char *fulldir = OS_NULL;
    os_bool_t dir_path_alloc = OS_FALSE;
    os_uint32_t length;

    if (filepath[0] == '/') /* file is an absolute path, use it directly */
    {
        length = strlen(filepath) + 1;
        fullpath = os_malloc(length);
        if (fullpath)
        {
            strncpy(fullpath, filepath, length);
        }
    }
    else                    /* file is not an absolute path, add it follow direcotry. */
    {
        VFS_WORKDIR_LOCK();
        if (!dirpath)
        {
            fulldir = working_dir;
        }
        else if (dirpath[0] == '/')
        {
            fulldir = (char *)dirpath;
        }
        else
        {
            /* Convert to absolute dir. */
            length = strlen(working_dir) + strlen(dirpath) + 2;
            fulldir = os_malloc(length);
            if (fulldir)
            {
                dir_path_alloc = OS_TRUE;
                os_snprintf(fulldir, length, "%s/%s", working_dir, dirpath);
            }
        }

        if (fulldir)
        {
            length = strlen(fulldir) + strlen(filepath) + 2;
            fullpath = os_malloc(length);
            if (fullpath) /* Join path and file name */
            {
                os_snprintf(fullpath, length, "%s/%s", fulldir, filepath);
            }

            if (dir_path_alloc == OS_TRUE)
            {
                os_free(fulldir);
            }
        }
        VFS_WORKDIR_UNLOCK();
    }

     return fullpath;
}

static char *_vfs_normalize_path(char *fullpath)
{
    char *dst0, *dst, *src;

    src = fullpath;
    dst = fullpath;
    dst0 = fullpath;
    while (1)
    {
        char c = *src;

        if (c == '.')
        {
            if (!src[1])
            {
                    src ++; /* '.' and ends */
            }
            else if (src[1] == '/')
            {
                /* './' case */
                src += 2;

                while ((*src == '/') && (*(src+1) != '\0'))
                {
                    src ++;
                }
                continue;
            }
            else if (src[1] == '.')
            {
                if (!src[2])
                {
                    /* '..' and ends case */
                    src += 2;
                    goto up_one;
                }
                else if (src[2] == '/')
                {
                    /* '../' case */
                    src += 3;

                    while ((*src == '/') && (*(src+1) != '\0'))
                    {
                        src ++;
                    }
                    goto up_one;
                }
            }
        }

        /* Copy up the next '/' and erase all '/' */
        while ((c = *src++) != '\0' && c != '/')
        {
            *dst ++ = c;
        }

        if (c == '/')
        {
            *dst ++ = '/';
            while (c == '/')
            {
                c = *src++;
            }

            src --;
        }
        else if (!c)
        {
            break;
        }

        continue;

up_one:
        dst --;
        if (dst < dst0)
        {
            return OS_NULL;
        }
        while (dst0 < dst && dst[-1] != '/')
        {
            dst --;
        }
    }

    *dst = '\0';

    /* Remove '/' in the end of path if exist */
    dst --;
    if ((dst != fullpath) && (*dst == '/'))
    {
        *dst = '\0';
    }

    /* Final check fullpath is not empty, for the special path of lwext "/.." */
    if ('\0' == fullpath[0])
    {
        fullpath[0] = '/';
        fullpath[1] = '\0';
    }

    return fullpath;
}

const char *vfs_create_absolute_path(const char *dirpath, const char *filepath)
{
    char *fullpath;
    char *normalizepath;

    OS_ASSERT(filepath);

    normalizepath = OS_NULL;
    fullpath = _vfs_conjunction_path(dirpath, filepath);
    if (fullpath)
    {
        normalizepath = _vfs_normalize_path(fullpath);
        if (!normalizepath)
        {
            os_free(fullpath);
        }
    }

    return normalizepath;
}

const char *vfs_get_rel_mnt_path(const char *mntpath, const char *abspath)
{
    const char *path;

    if (strlen(mntpath) == strlen(abspath))
    {
        path = "/";
    }
    else
    {
        path = abspath + strlen(mntpath);
        if ((*path != '/') && (path != abspath))
        {
            path --;
        }
    }

    return path;
}

const char *vfs_create_rel_mnt_path(const char *mntpath, const char *abspath)
{
    const char *path;
    char *path_new;

    path = vfs_get_rel_mnt_path(mntpath, abspath);
    /* path_new = strdup(path); */
    path_new = (char *)os_malloc(strlen(path) + 1);
    if (path_new)
    {
        strcpy(path_new, path);
    }

    return path_new;
}

const char *vfs_get_path_lastname(const char *path)
{
    const char *ptr;

    ptr = strrchr(path, '/');
    if (ptr)
    {
        /* found '/', Skip character '/'. */
        ptr++;
    }
    else
    {
        /* not found '/', use path directly. */
        ptr = path;
    }

    return ptr;
}

