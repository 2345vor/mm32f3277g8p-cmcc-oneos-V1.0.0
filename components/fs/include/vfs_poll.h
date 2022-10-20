#ifndef __VFS_POLL_H__
#define __VFS_POLL_H__

#include <oneos_config.h>
#include <poll.h>
#include <os_types.h>
#include <os_list.h>
#include <os_sem.h>

struct vfs_pollfd
{
    int             fd;      /* File descriptor */
    os_uint16_t     events;  /* Requested events */
    os_uint16_t     revents; /* Returned events */

    os_sem_t       *sem;     /* Semaphore used to post returned events */
    os_list_node_t  node;    /* Used by dirver. */
};

#ifdef OS_USING_IO_MULTIPLEXING
extern int vfs_poll(struct vfs_pollfd *fds, unsigned int nfds, int timeout);
extern os_err_t vfs_poll_notify(struct vfs_pollfd *poll_fd, os_uint16_t revents);
#endif

#endif /* __VFS_POLL_H__ */

