#include <string.h>
#include <os_util.h>
#include <os_memory.h>
#include <shell.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <vfs_fs.h>
#include <vfs_posix.h>
#include "vfs_private.h"

#define CP_BUF_SIZE         2048
#define CAT_BUF_SIZE        81

static void _cat(const char *path)
{
    char buf[CAT_BUF_SIZE];
    int fd;
    int size;

    fd = vfs_open(path, O_RDONLY);
    if (fd >= 0)
    {
        do
        {
            memset(buf, 0, sizeof(buf));
            size = vfs_read(fd, buf, CAT_BUF_SIZE - 1);
            if (size > 0)
            {
                os_kprintf("%s", buf);
            }
            else if (size < 0)
            {
                os_kprintf("read file %s failed\r\n", path);
            }
            else
            {
                /* Do nothing.*/
            }
        }
        while (size > 0);
        os_kprintf("\r\n");

        vfs_close(fd);
    }
    else
    {
        os_kprintf("Open file %s failed\r\n", path);
    }
}

static void _echo(char *path, int flag, char *buf)
{
    int fd = -1;
    int result;

    fd = vfs_open(path, flag, 0);
    if (fd >= 0)
    {
        result = vfs_write(fd, buf, strlen(buf));
        if ((result < 0) && (strlen(buf) > 0))
        {
            os_kprintf("write file %s failed\r\n", path);
        }
        vfs_close(fd);
    }
    else
    {
        os_kprintf("open file %s failed\r\n", path);
    }
}

//TODO the path should be dir/file or only file?
static void _ls(const char *path)
{
    FS_DIR *dp;
    struct dirent *dent;
    char *abs_path;
    struct stat entry_stat;

    dp = vfs_opendir(path);
    if (dp)
    {
        os_kprintf("Directory %s:\r\n", path);
        do
        {
            dent = vfs_readdir(dp);
            if (dent)
            {
                memset(&entry_stat, 0, sizeof(struct stat));

                abs_path = vfs_create_absolute_path(path, dent->d_name);
                if (!abs_path)
                {
                    break;
                }

                if (vfs_stat(abs_path, &entry_stat) == 0)
                {
                    os_kprintf("%-20s    ", dent->d_name);
                    if (S_ISDIR(entry_stat.st_mode))
                    {
                        os_kprintf("%-25s\r\n", "<DIR>");
                    }
                    else
                    {
                        os_kprintf("%-25lu\r\n", entry_stat.st_size);
                    }
                }
                else
                {
                    os_kprintf("BAD file: %s\r\n", dent->d_name);
                }
                os_free(abs_path);
            }
        }
        while (dent);
        vfs_closedir(dp);
    }
    else
    {
        os_kprintf("No such directory\r\n");
    }
}

static void _copyfile(const char *src, const char *dst)
{
    os_uint8_t *buf;
    int read_size;
    int write_size;
    int fd_src;
    int fd_dst;
    int result;
    char *src_abspath = OS_NULL;
    char *dst_abspath = OS_NULL;

    result = 0;
    fd_src = -1;
    fd_dst = -1;
    src_abspath = vfs_create_absolute_path(OS_NULL, src);
    if (src_abspath)
    {
        dst_abspath = vfs_create_absolute_path(OS_NULL, dst);
        if (dst_abspath)
        {
            if (strcmp(src_abspath, dst_abspath) == 0)
            {
                os_kprintf("cp failed, %s and %s are the same file\r\n", src, dst);
                result = -1;
            }
            os_free(dst_abspath);
            os_free(src_abspath);
        }
        else
        {
            os_kprintf("out of memory\r\n");
            os_free(src_abspath);
            result = -1;
        }
    }
    else
    {
        os_kprintf("out of memory\r\n");
        result = -1;
    }

    if (0 == result)
    {
        fd_src = vfs_open(src, O_RDONLY);
        if (fd_src >= 0)
        {
            /* If dst file exist, O_TRUNC will clear it first. */
            fd_dst = vfs_open(dst, O_WRONLY | O_CREAT | O_TRUNC);
            if (fd_dst)
            {
                buf = os_malloc(CP_BUF_SIZE);
                if (!buf)
                {
                    vfs_close(fd_src);
                    vfs_close(fd_dst);
                    result = -1;
                    os_kprintf("out of memory\r\n");
                }
            }
            else
            {
                vfs_close(fd_src);
                result = -1;
                os_kprintf("open %s failed\r\n", dst);
            }
        }
        else
        {
            result = -1;
            os_kprintf("open %s failed\r\n", src);
        }
    }

    if (0 == result)
    {
        do
        {
            read_size = vfs_read(fd_src, buf, CP_BUF_SIZE);
            if (read_size > 0)
            {
                write_size = vfs_write(fd_dst, buf, read_size);
                if (write_size != read_size)
                {
                    if (write_size < 0)
                    {
                        os_kprintf("Write file data failed\r\n");
                    }
                    else
                    {
                        os_kprintf("Write file data failed, not copy all the data\r\n");
                    }
                    break;
                }
            }
        }
        while (read_size > 0);

        vfs_close(fd_src);
        vfs_close(fd_dst);
        os_free(buf);
    }
}

static void _copydir(const char *src, const char *dst)
{
    struct stat stat_buf;
    FS_DIR *dp;
    struct dirent *dent;
    char *src_abspath = OS_NULL;
    char *dst_abspath = OS_NULL;
    int result;

    dp = vfs_opendir(src);

    if (dp)
    {
        do
        {
            dent = vfs_readdir(dp);
            if (dent)
            {
                if (strcmp(dent->d_name, "..") == 0 || strcmp(dent->d_name, ".") == 0)
                {
                    continue;
                }
                src_abspath = vfs_create_absolute_path(src, dent->d_name);
                if (!src_abspath)
                {
                    os_kprintf("out of memory\r\n");
                    break;
                }
                dst_abspath = vfs_create_absolute_path(dst, dent->d_name);
                if (!dst_abspath)
                {
                    os_free(src_abspath);
                    os_kprintf("out of memory\r\n");
                    break;
                }

                memset(&stat_buf, 0, sizeof(struct stat));
                if (vfs_stat(src_abspath, &stat_buf) != 0)
                {
                    os_free(src_abspath);
                    os_free(dst_abspath);
                    os_kprintf("stat %s failed\r\n", dent->d_name);
                    continue;
                }

                if (S_ISDIR(stat_buf.st_mode))
                {
                    result = vfs_mkdir(dst_abspath, 0);
                    if (result >= 0)
                    {
                        _copydir(src_abspath, dst_abspath);
                    }
                    else
                    {
                        os_kprintf("mkdir %s failed\r\n", dst_abspath);
                    }
                }
                else
                {
                    _copyfile(src_abspath, dst_abspath);
                }
                os_free(src_abspath);
                os_free(dst_abspath);
            }
        }
        while (dent);
    }
    else
    {
        os_kprintf("open %s failed\r\n", src);
    }

    vfs_closedir(dp);
}

static void _copy(const char *src, const char *dst)
{
    struct stat stat_src;
    struct stat stat_dst;
    char *file_dst;
    char *dir_dst;
    int result_src = 0;
    int result_dst = 0;
    int result;
    os_bool_t dst_exist = OS_FALSE;

    memset(&stat_src, 0, sizeof(struct stat));
    memset(&stat_dst, 0, sizeof(struct stat));
    result_src = vfs_stat(src, &stat_src);
    if (result_src >= 0)
    {
        result_dst = vfs_stat(dst, &stat_dst);
        if (result_dst >= 0)
        {
            dst_exist = OS_TRUE;
        }
    }
    else
    {
        os_kprintf("copy failed, bad %s\r\n", src);
    }

    if (result_src >= 0)
    {
        if (S_ISREG(stat_src.st_mode))
        {
            if (S_ISREG(stat_dst.st_mode))
            {
                /* If dst is file, truncate it and copy. */
                _copyfile(src, dst);
            }
            else if (S_ISDIR(stat_dst.st_mode))
            {
                /* If dst is directory, copy src file under dst directory. */
                file_dst = vfs_create_absolute_path(dst, vfs_get_path_lastname(src));
                if (file_dst)
                {
                    _copyfile(src, file_dst);
                    os_free(file_dst);
                }
                else
                {
                    os_kprintf("out of memory\r\n");
                }
            }
            else
            {
                /* If dst not exist, create it and copy. */
                if (OS_FALSE == dst_exist)
                {
                    _copyfile(src, dst);
                }
                else
                {
                    /* Dst exist, but type is invaild, do nothing. */
                    os_kprintf("cp faild, %s is not regular file or directory.\r\n", dst);
                }
            }
        }
        else if (S_ISDIR(stat_src.st_mode))
        {
            if (S_ISREG(stat_dst.st_mode))
            {
                /* If dst is file, not permit copy dir to file. */
                os_kprintf("cp faild, cp dir to file is not permitted\r\n");
            }
            else if (S_ISDIR(stat_dst.st_mode))
            {
                /* If dst is directory, copy src directory under dst directory. */
                dir_dst = vfs_create_absolute_path(dst, vfs_get_path_lastname(src));
                if (dir_dst)
                {
                    result = vfs_mkdir(dir_dst, 0);
                    if (result >= 0)
                    {
                        _copydir(src, dir_dst);
                    }
                    else
                    {
                        os_kprintf("mkdir %s failed\r\n", dir_dst);
                    }
                    os_free(dir_dst);
                }
                else
                {
                    os_kprintf("out of memory\r\n");
                }
            }
            else
            {
                /* If dst not exist, create dst directory, the copy directory. */
                if (OS_FALSE == dst_exist)
                {
                    result = vfs_mkdir(dst, 0);
                    if (result >= 0)
                    {
                        _copydir(src, dst);
                    }
                    else
                    {
                        os_kprintf("mkdir %s failed\r\n", dst);
                    }
                }
                else
                {
                    os_kprintf("cp faild, %s is not regular file or directory.\r\n", dst);
                }
            }
        }
        else
        {
            /* Src type is invaild, do nothing. */
            os_kprintf("cp faild, %s is not regular file or directory.\r\n", src);
        }
   }
}

static void _move(const char *src, const char *dst)
{
    struct stat stat_src;
    struct stat stat_dst;
    char *dest = OS_NULL;
    char *src_tmp;
    int result_src = 0;
    int result_dst = 0;
    os_bool_t dst_exist = OS_FALSE;

    memset(&stat_src, 0, sizeof(struct stat));
    memset(&stat_dst, 0, sizeof(struct stat));
    result_src = vfs_stat(src, &stat_src);
    if (result_src >= 0)
    {
        result_dst = vfs_stat(dst, &stat_dst);
        if (result_dst >= 0)
        {
            dst_exist = OS_TRUE;
        }
    }
    else
    {
        os_kprintf("move failed, bad source: %s\r\n", src);
    }

    if (result_src >= 0)
    {
        os_kprintf("%s => %s\r\n", src, dst);

        if (S_ISDIR(stat_dst.st_mode))
        {
            /* If dst is directory, move src file/directory under dst directory. */
            dest = (char *)os_malloc(VFS_PATH_MAX);
            if (dest)
            {
                src_tmp = (char *)src + strlen(src);
                while (src_tmp != src)
                {
                    if (*src_tmp == '/')
                    {
                        break;
                    }
                    src_tmp --;
                }

                os_snprintf(dest, VFS_PATH_MAX - 1, "%s/%s", dst, src_tmp);
                if (S_ISREG(stat_src.st_mode) || S_ISDIR(stat_src.st_mode))
                {
                    vfs_rename(src, dest);
                }
                else
                {
                    os_kprintf("mv failed, %s is not regular file or directory.\r\n", src);
                }
                os_free(dest);
            }
            else
            {
                os_kprintf("out of memory\r\n");
            }
        }
        else if (S_ISREG(stat_dst.st_mode))
        {
            /* If dst is file, delete it first. */
            if (S_ISREG(stat_src.st_mode))
            {
                vfs_unlink(dst);
                vfs_rename(src, dst);
            }
            else if (S_ISDIR(stat_src.st_mode))
            {
                os_kprintf("mv failed, mv a dir to a file is not permitted\r\n");
            }
            else
            {
                os_kprintf("mv failed, %s is not regular file or directory.\r\n", src);
            }
        }
        else
        {
            /* If dst not exist, rename it directly. */
            if (OS_FALSE == dst_exist)
            {
                vfs_rename(src, dst);
            }
            else
            {
                os_kprintf("mv failed, %s is not regular file or directory.\r\n", dst);
            }
        }
    }
}

static void _df(const char *path)
{
    int result;
    int minor = 0;
    long long cap;
    struct statfs buffer;
    int unit_index = 0;
    char *unit_str[] = {"KB", "MB", "GB"};

    if(path)
    {
        result = vfs_statfs(path, &buffer);
        if (result == 0)
        {
            cap = ((long long)buffer.f_bsize) * ((long long)buffer.f_bfree) / 1024LL;
            for (unit_index = 0; unit_index < 2; unit_index ++)
            {
                if (cap < 1024) 
                {
                    break;
                }

                minor = (cap % 1024) * 10 / 1024; /* Only one decimal point */
                cap = cap / 1024;
            }

            os_kprintf("disk free: %d.%d %s [ %d block, %d bytes per block ]\r\n",
                       (unsigned long)cap, 
                       minor, 
                       unit_str[unit_index], 
                       buffer.f_bfree, 
                       buffer.f_bsize);
        }
        else
        {
            os_kprintf("statfs failed\r\n");
        }
    }
    else
    {
        os_kprintf("invalid path\r\n");
    }
}

static os_err_t sh_cat(os_int32_t argc, char **argv)
{
    int i;

    if (argc > 1)
    {
        for (i = 1; i < argc; i ++)
        {
            _cat(argv[i]);
        }
    }
    else
    {
        os_kprintf("Usage: cat <FILE>...\r\n");
        os_kprintf("Concatenate file(s)\r\n");
    }

    return OS_EOK;
}
SH_CMD_EXPORT(cat, sh_cat, "Concatenate FILE(s)");

static os_err_t sh_echo(os_int32_t argc, char **argv)
{
    os_err_t result = OS_EOK;

    if (argc == 4)
    {
        if (strcmp(argv[2], ">") == 0)
        {
            _echo(argv[3], O_RDWR | O_CREAT | O_TRUNC, argv[1]);
        }
        else if (strcmp(argv[2], ">>") == 0)
        {
            _echo(argv[3], O_RDWR | O_CREAT | O_APPEND, argv[1]);
        }
        else
        {
            result = OS_EINVAL;
        }
    }
    else
    {
        result = OS_EINVAL;
    }

    if (result != OS_EOK)
    {
        os_kprintf("Usage: echo <STRING> > <FILE>\r\n");
        os_kprintf("Usage: echo <STRING> >> <FILE>\r\n");
    }

    return OS_EOK;
}
SH_CMD_EXPORT(echo, sh_echo, "echo string to file");

static os_err_t sh_ls(os_int32_t argc, char **argv)
{
    char *cwd;

    if (argc == 1)
    {
        cwd = os_malloc(VFS_PATH_MAX);
        if (cwd)
        {
            vfs_getcwd(cwd, VFS_PATH_MAX);
            _ls(cwd);
            os_free(cwd);
        }
        else
        {
            os_kprintf("out of memory\r\n");
        }
    }
    else if (argc == 2)
    {
        _ls(argv[1]);
    }
    else
    {
        os_kprintf("Usage: ls [DIRNAME]\r\n");
    }

    return OS_EOK;
}
SH_CMD_EXPORT(ls, sh_ls, "List information about the FILEs.");

static os_err_t sh_cd(os_int32_t argc, char **argv)
{
    int result = 0;

    if (argc == 1)
    {
        vfs_chdir("/");
    }
    else if (argc == 2)
    {
        result = vfs_chdir(argv[1]);
        if (result != 0)
        {
            os_kprintf("No such directory: %s\r\n", argv[1]);
        }
    }
    else
    {
        os_kprintf("Usage: cd [PATH]\r\n");
    }

    return OS_EOK;
}
SH_CMD_EXPORT(cd, sh_cd, "Change the shell working directory.");

static os_err_t sh_pwd(os_int32_t argc, char **argv)
{
    char *buf;

    OS_UNREFERENCE(argc);
    OS_UNREFERENCE(argv);

    buf = os_malloc(VFS_PATH_MAX);
    if (buf)
    {
        vfs_getcwd(buf, VFS_PATH_MAX);
        os_kprintf("%s\r\n", buf);
        os_free(buf);
    }
    else
    {
        os_kprintf("out of memory\r\n");
    }

    return OS_EOK;
}
SH_CMD_EXPORT(pwd, sh_pwd, "Print the name of the current working directory.");

static os_err_t sh_mkdir(os_int32_t argc, char **argv)
{
    int result;

    if (argc == 2)
    {
        result = vfs_mkdir(argv[1], 0);
        if (result < 0)
        {
            os_kprintf("mkdir %s failed\r\n", argv[1]);
        }
    }
    else
    {
        os_kprintf("Usage: mkdir <DIRNAME>\r\n");
        os_kprintf("Create the DIRECTORY, if it not exist.\r\n");
    }

    return OS_EOK;
}
SH_CMD_EXPORT(mkdir, sh_mkdir, "Create the DIRECTORY.");

static os_err_t sh_rm(os_int32_t argc, char **argv)
{
    int i;
    int result;

    if (argc > 1)
    {
        for (i = 1; i < argc; i ++)
        {
            result = vfs_unlink(argv[i]);
            if (result < 0)
            {
                os_kprintf("rm %s failed\r\n", argv[i]);
            }
        }
    }
    else
    {
        os_kprintf("Usage: rm <FILE>...\r\n");
        os_kprintf("Remove (unlink) the FILE(s).\r\n");
    }

    return OS_EOK;
}
SH_CMD_EXPORT(rm, sh_rm, "Remove(unlink) the FILE(s).");

static os_err_t sh_cp(os_int32_t argc, char **argv)
{
    if (argc == 3)
    {
        _copy(argv[1], argv[2]);
    }
    else
    {
        os_kprintf("Usage: cp <SOURCE> <DEST> \r\n");
        os_kprintf("Copy SOURCE to DEST.\r\n");
    }

    return OS_EOK;
}
SH_CMD_EXPORT(cp, sh_cp, "Copy SOURCE to DEST.");

static os_err_t sh_mv(os_int32_t argc, char **argv)
{
    if (argc == 3)
    {
        _move(argv[1], argv[2]);
    }
    else
    {
        os_kprintf("Usage: mv <SOURCE> <DEST> \r\n");
        os_kprintf("Rename SOURCE to DEST, or move SOURCE(s) to DIRECTORY.\r\n");
    }

    return OS_EOK;
}
SH_CMD_EXPORT(mv, sh_mv, "Rename SOURCE to DEST.");

static os_err_t sh_df(os_int32_t argc, char **argv)
{
    if (argc == 1)
    {
        _df("/");
    }
    else if (argc == 2)
    {
        _df(argv[1]);
    }
    else
    {
        os_kprintf("Usage: df [PATH]\r\n");
    }

    return OS_EOK;
}
SH_CMD_EXPORT(df, sh_df, "disk free");

static os_err_t sh_mkfs(os_int32_t argc, char **argv)
{
    int result = 0;

    if ((argc == 4) && (strcmp(argv[1], "-t") == 0))
    {
        result = vfs_mkfs(argv[2], argv[3]);
        if (result != 0)
        {
            os_kprintf("mkfs failed, result=%d\r\n", result);
        }
    }
    else
    {
        os_kprintf("Usage: mkfs -t <FS_TYPE> <DEVICENAME>\r\n");
    }

    return OS_EOK;
}
SH_CMD_EXPORT(mkfs, sh_mkfs, "format disk with file system");

