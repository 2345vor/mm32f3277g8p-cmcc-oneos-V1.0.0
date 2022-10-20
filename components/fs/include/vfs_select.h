#ifndef __VFS_SELECT_H__
#define __VFS_SELECT_H__

#include <oneos_config.h>
#include <sys/time.h>
#include <sys/select.h>

#ifdef OS_USING_IO_MULTIPLEXING
extern int vfs_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
#endif

#endif 

