#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>

#include "onetls_socket.h"
#include "onetls_util.h"
#include "onetls_lib.h"
#include "onetls_alert.h"
#include <sys/errno.h>

uint32_t onetls_sock_test(int fd, uint8_t op, uint32_t timeout)
{ 
    struct timeval tv;
    fd_set fd_s;
    int ret = 0, width = sizeof(fd_s) * 8;
    if (fd >= width) {
        onetls_check_errlog(ret, "onetls_sock_test suggest using poll/epoll");
        return ONETLS_FAIL;
    }

    FD_ZERO(&fd_s);   
    FD_SET(fd, &fd_s);  

    tv.tv_sec = timeout / 1000;  
    tv.tv_usec = 0;
    ret = select((fd + 1), 
                 (op == ONETLS_SOCK_RD) ? &fd_s : NULL, 
                 (op == ONETLS_SOCK_WR) ? &fd_s : NULL, 
                 NULL, 
                 &tv);
    if (ret < 0) {
        onetls_check_errlog(ret, "select[%d]", onetls_get_syscall_errno());
        return ONETLS_FAIL;
    }
    if (ret == 0) {
        return ONETLS_SOCKET_TRYAGAIN;
    }
    return ONETLS_SUCCESS;
}

uint32_t onetls_sock_recv(onetls_ctx *ctx, uint8_t *buf, uint32_t len, uint32_t *recv_len, uint32_t time_out)
{  
    int ret = 0;
    
    *recv_len = 0;
    while (*recv_len < len) {
        ret = onetls_sock_test(ctx->recv_fd, ONETLS_SOCK_RD, time_out);
        if (ret) {
            return ret;
        }

        // 调用底层的接收接口
        ret = recv(ctx->recv_fd, buf + *recv_len, len - *recv_len, 0);
        if (ret < 0) {
            if (onetls_get_syscall_errno() == EINTR) {
                continue;
            }
            if (onetls_get_syscall_errno() == EAGAIN) {
                return ONETLS_SOCKET_TRYAGAIN;
            }
            onetls_check_errlog(ret, "recv");
            return ONETLS_SOCKET_READ_FAIL;
        }
        *recv_len += ret;

        if (onetls_is_dtls(ctx)) {
            break;  // udp一次就是一个包。多的不要，少了外面会解析校验
        } else {
            if (ret == 0) { // 关闭了socket
                ctx->shutdown &= ONETLS_ALERT_MASK;
                return ONETLS_SOCKET_CLOSE;
            }
        }
    }
    return ONETLS_SUCCESS;
}

uint32_t onetls_sock_send(onetls_ctx *ctx, uint8_t *buf, uint32_t len, uint32_t *send_len, uint32_t time_out)
{
    int ret = 0;
    
    *send_len = 0;
    while (*send_len < len) {
        ret = onetls_sock_test(ctx->send_fd, ONETLS_SOCK_WR, time_out);
        if (ret) {
            return ret;
        }

        // 调用底层的发送接口
        ret = send(ctx->send_fd, buf + *send_len, len - *send_len, 0);
        if (ret < 0) {
            if (onetls_get_syscall_errno() == EINTR) {
                continue;
            }
            if (onetls_get_syscall_errno() == EAGAIN) {
                return ONETLS_SOCKET_TRYAGAIN;
            }
            onetls_check_errlog(ret, "send");
            return ONETLS_SOCKET_SEND_FAIL;
        }
        *send_len += ret;
        if (onetls_is_dtls(ctx)) {
            if (*send_len != len) {
                onetls_check_errlog(ret, "onetls_sock_send[send:%d][real_send:%d][errno:%d]", len, *send_len, onetls_get_syscall_errno());
                return ONETLS_SOCKET_SEND_FAIL;
            }
            break;
        } else {
            if (ret == 0) { // 关闭了socket
                ctx->shutdown &= ONETLS_ALERT_MASK;
                return ONETLS_SOCKET_CLOSE;
            }
        }
    }
    return ONETLS_SUCCESS;
}