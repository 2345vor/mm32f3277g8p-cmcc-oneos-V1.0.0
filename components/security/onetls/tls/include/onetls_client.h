#ifndef __ONETLS_CLIENT_H__
#define __ONETLS_CLIENT_H__
#include "onetls_config.h"
#include "onetls_err.h"

#define ONETLS_MODE_TLS  0
#define ONETLS_MODE_DTLS 1

#define ONETLS_KEY_UPDATE_NOT_REQ       (0)
#define ONETLS_KEY_UPDATE_REQUEST       (1)

// 会话管理器，所有配置、参数、数据、通信都基于上下文来处理的
struct st_onetls_ctx;
typedef struct st_onetls_ctx onetls_ctx;

typedef struct {
    const char *name;
    uint32_t v;
    uint32_t r;
    uint32_t c;
    uint32_t b;
} onetls_version_vrcb;

typedef uint32_t(*onetls_recv_nst_callback)(uint8_t *ticket,        // 之前获取到nst(ticket)格式化数据
                                            uint32_t ticket_len);    // ticket长度

// onetls的库初始化
uint32_t onetls_init(void);

// 获取当前协议栈的版本
const char *onetls_version(onetls_version_vrcb *vrcb);

// 申请一个会话管理上下文
onetls_ctx *onetls_ctx_new(uint8_t mode);
void onetls_ctx_del(onetls_ctx *ctx);

// 配置socket
void onetls_set_socket(onetls_ctx *ctx, int recv_fd, int send_fd);

// 配置psk_hint
uint32_t onetls_set_outband_psk(onetls_ctx *ctx, uint8_t *hint, uint32_t hint_len, uint8_t *key, uint32_t key_len);

// 发起连接
uint32_t onetls_connect(onetls_ctx *ctx);

// 断开一个连接
void onetls_shutdown(onetls_ctx *ctx);

// 数据收发
uint32_t onetls_send(onetls_ctx *ctx, uint8_t *out, uint32_t out_len, uint32_t *send_len);
uint32_t onetls_recv(onetls_ctx *ctx, uint8_t *in, uint32_t in_len, uint32_t *recv_len);

// 准备early data，将在connect的时候一同发送出去。openssl是在这里出发链接，这里还是将他们区分了
uint32_t onetls_send_early_data(onetls_ctx *ctx, uint8_t *out, uint32_t out_len);

// 清空发送缓冲区
uint32_t onetls_flush(onetls_ctx *ctx);

// 探测当前协议栈中是否有缓存数据，有就可以读取
uint32_t onetls_pending(onetls_ctx *ctx);

void onetls_set_recv_ticket_cb(onetls_ctx *ctx, onetls_recv_nst_callback cb);
// 配置ticket参数
uint32_t onetls_set_ticket(onetls_ctx *ctx, void *data, uint32_t data_len);

// 更新密钥
uint32_t onetls_update_key(onetls_ctx *ctx, uint8_t way);

// 配置mtu
uint32_t onetls_set_mtu(onetls_ctx *ctx, uint16_t mtu);
#endif
