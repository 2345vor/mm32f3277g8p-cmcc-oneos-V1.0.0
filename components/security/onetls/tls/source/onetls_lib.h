#ifndef __ONETLS_LIB_H__
#define __ONETLS_LIB_H__
#include "onetls_client.h"
#include "onetls_log.h"
#include "onetls_mem.h"
#include "onetls_util.h"
#include "onetls_crypto.h"
#include "onetls_socket.h"

#define ONETLS_NAME "onetls"

// 一些通用大小
#define ONETLS_TLS_KEY_UPDATE_LEN       (5)
#define ONETLS_TLS_END_OF_EARLY_LEN     (4)
#define ONETLS_HANSHAKE_SH_MIN_LEN      (64)

#define ONETLS_HANDSHAKE_IO_WANT_NOTHING (0)
#define ONETLS_HANDSHAKE_IO_WANT_READ    (0x01)
#define ONETLS_HANDSHAKE_IO_WANT_SEND    (0x10)

// 报文类型
#define ONETLS_MT_CCS              0x14
#define ONETLS_MT_ALERT            0x15
#define ONETLS_MT_HANDSHAKE        0x16
#define ONETLS_MT_APPLICATION_DATA 0x17

/* From RFC 8446: https://tools.ietf.org/html/rfc8446#appendix-B.3 */
#define ONETLS_HELLO_REQUEST              0
#define ONETLS_CLIENT_HELLO               1
#define ONETLS_SERVER_HELLO               2
#define ONETLS_SERVER_NEW_SESSION_TICKET  4
#define ONETLS_CLIENT_END_OF_EARLY_DATA   5
#define ONETLS_ENCRYPTED_EXTENSIONS       8
#define ONETLS_CERTIFICATE               11
#define ONETLS_SERVER_KEY                12
#define ONETLS_CERT_REQ                  13
#define ONETLS_SERVER_HELLO_DONE         14
#define ONETLS_CERT_VERIFY               15
#define ONETLS_CLIENT_KEY                16
#define ONETLS_FINISHED                  20
#define ONETLS_SERVER_CERT_STATUS        22
#define ONETLS_SERVER_SESSION_LOOKUP     23
#define ONETLS_KEY_UPDATE                24
#define ONETLS_DTLS_OK                   26
#define ONETLS_MESSAGE_HASH              254

// TLS的版本状态，当前不支持配置，因为只支持最高的TLS/DTLS 1.3
#define ONETLS_VERSION_TLS_12     0x0303
#define ONETLS_VERSION_TLS_13     0x0304
#define ONETLS_VERSION_DTLS_12    0xFEFD
#define ONETLS_VERSION_DTLS_13    0x7f26        // 0xffff - ONETLS_VERSION_TLS_13 + 0x0201;

enum {
    ONETLS_EPOCH_FOR_CH_SH_HRR = 0,
    ONETLS_EPOCH_FOR_EARLY_DATA,
    ONETLS_EPOCH_FOR_HS_MSG,
    ONETLS_EPOCH_FOR_APP_DATA,
};

enum {
    ONETLS_MAX_FRAG_LEN_ULIMIT = 0, // 16384
    ONETLS_MAX_FRAG_LEN_512,
    ONETLS_MAX_FRAG_LEN_1024,
    ONETLS_MAX_FRAG_LEN_2048,
    ONETLS_MAX_FRAG_LEN_4096,
};

// TLS/DTLS状态机
enum en_onetls_state {
    ONETLS_STATE_INIT = 0,
    ONETLS_STATE_SEND_CNT_HELLO,
    ONETLS_STATE_GEN_C_EARLY_TRAFFIC_SECRET,
    ONETLS_STATE_SEND_EARLY_DATA,
    ONETLS_STATE_RECV_SVR_HELLO,
    ONETLS_STATE_GEN_HS_SECRET,
    ONETLS_STATE_GEN_S_HS_SECRET,
    ONETLS_STATE_RECV_SVR_CCS, // 保留报文，可能没有。有也不做啥
    ONETLS_STATE_SEND_CNT_CCS, // 保留报文，可能没有。有也不做啥
    ONETLS_STATE_RECV_SVR_EE,
    ONETLS_STATE_RECV_SVR_FINISH,
    ONETLS_STATE_SEND_EOE,
    ONETLS_STATE_GEN_C_HS_SECRET,
    ONETLS_STATE_SEND_CNT_FINISH,
    ONETLS_STATE_GEN_APP_SECRET,
    ONETLS_STATE_PRE_OK,
    ONETLS_STATE_OK,

    // 不用专门的状态机处理
    ONETLS_STATE_RECV_SVR_NST,  // 任何状态下都有可能处理
};

enum en_onetls_sub_state {
    ONETLS_SUB_STATE_PREPARING = 0,
    ONETLS_SUB_STATE_SENDING,
    ONETLS_SUB_STATE_WAITING,
    ONETLS_SUB_STATE_FINISHED,
};

// 回话恢复相关数据
typedef struct {
    uint32_t life_time;         // 收到ticket的时间
    uint32_t age_add;
    uint32_t max_early_data;    // 最多能发送的early_data数量
    
    uint16_t cipher_id;         // 待恢复的算法
    uint8_t master_key[ONETLS_MAX_MD_LEN];  // key决定长度

    uint16_t ticket_len;
    uint8_t  ticket[0];
} onetls_resumption;

typedef uint32_t(*onetls_state_func)(onetls_ctx*);
typedef struct {
    uint8_t random_c[32];
    uint8_t random_s[32];

    onetls_buffer *cookie;
    onetls_buffer *send_cache;
    uint32_t hs_msg_timer;
    uint32_t hs_msg_retry_times;

    onetls_list_node recv_list;

    struct {    // 计算摘要时，做的一些缓存，后续可能用得到
        onetls_buffer *handshake_data;            // 缓存client hello
        uint8_t ch[ONETLS_MAX_MD_LEN];
        uint8_t ch_to_sh[ONETLS_MAX_MD_LEN];
        uint8_t ch_to_sf[ONETLS_MAX_MD_LEN];
        uint8_t ch_to_cf[ONETLS_MAX_MD_LEN];
    } handshake_packet;

    struct {    // 存放各种secret信息，各个长度，由算法决定
        uint8_t early_secret[2][ONETLS_MAX_MD_LEN];      // 1号用于会话恢复, 0号用于正常的PSK
        uint8_t select_psk;
        uint8_t early_traffic_secret[ONETLS_MAX_MD_LEN];
        uint8_t handshake_secret[ONETLS_MAX_MD_LEN];
        uint8_t client_handshake_secret[ONETLS_MAX_MD_LEN];
        uint8_t server_handshake_secret[ONETLS_MAX_MD_LEN];
        uint8_t c_finish_key[ONETLS_MAX_MD_LEN];
        uint8_t s_finish_key[ONETLS_MAX_MD_LEN];
    } secret;

    struct {    // dh参数相关
        onetls_key_share    *key_share;
        onetls_buffer       *dh_key;        // 协商完之后，最终的dh共享参数
        uint16_t server_selected_id;        // 服务的选择的id，主要用于retry
        uint16_t server_public_len;
        uint8_t server_public_key[ONETLS_DH_PB_KEY_LEN];
    } dh_para;

    struct {    // 消息标识。又来判断。收发了那些报文
        uint16_t message_seq_wr;
        uint16_t message_seq_rd;
        uint8_t server_hello;               // 最多一次cookie，防止被无限cookie
        uint8_t server_hello_retry;         // 收到retry
        uint8_t server_ccs;                 // 对面发送了。我才需要回复
        uint8_t server_ee;                  // 收到ee
        uint8_t server_finish;              // 收到server finish
        uint8_t client_ext_early_data;      // 我发送了，对面的扩展才能携带。不然就是异常的。
        uint8_t server_ext_early_data;      // 对面有这个扩展，我就必须得回复eoe报文
    } msg_tag;
    uint8_t want_io;                        // 当前的io状态
} onetls_handshake;

typedef struct {
    uint16_t epoch;
    uint8_t  seq_num[6];
} onetls_record_number;

struct st_onetls_ctx {
    onetls_handshake *handshake;    // 握手状态机
    onetls_buffer *sess_id;         // 缓存回话相关
    onetls_buffer *server_name;

    uint8_t state;                  // 当前的上下文状态位
    uint8_t sub_state;              // 在udp 模式下，需要等待到合理的报文之后。才可以继续
    uint8_t shutdown;               // 是否需要断链
    uint8_t mode;                   // 当前的工作模式
    uint16_t mtu;

    int recv_fd;
    int send_fd;

    const onetls_cipher *cipher;
    void *en_ctx;   // 加密上下文
    void *de_ctx;   // 解密上下文

    uint16_t cid_len;   // 为0标识不协商，具体数字标识协商出来的长度

    struct {
        onetls_recv_nst_callback ticket_cb;
        uint32_t max_early_data_len_tmp;
    } resumption;

    struct {
        uint8_t resume_master_secret[ONETLS_MAX_MD_LEN];
        uint8_t master_key[ONETLS_MAX_MD_LEN];
        uint8_t master_secret[ONETLS_MAX_MD_LEN];
        uint8_t c_app_secret[ONETLS_MAX_MD_LEN];
        uint8_t s_app_secret[ONETLS_MAX_MD_LEN];

        // 序列号
        uint8_t wr_seq[8];
        uint8_t wr_key[ONETLS_MAX_MD_LEN];
        uint8_t wr_iv[ONETLS_MAX_MD_LEN];

        uint8_t rd_seq[8];
        uint8_t rd_key[ONETLS_MAX_MD_LEN];
        uint8_t rd_iv[ONETLS_MAX_MD_LEN];
        
        uint8_t s_sn_key[ONETLS_MAX_MD_LEN];
        uint8_t c_sn_key[ONETLS_MAX_MD_LEN];

        struct {
            onetls_record_number record_num_wr;
            onetls_record_number record_num_rd;
        } dtls;
    } security;

    struct {
        onetls_buffer *hint;    // 存放hint数据
        onetls_buffer *out_band_psk;

        onetls_buffer *ticket;  // 存放ticket
    } psk_info;  

    struct {
        uint8_t alert[2];       // 告警数据
        onetls_buffer *plain_data;
        onetls_buffer *msg_in;
        onetls_buffer *msg_out;
    } message;

    struct {    // 存放早起数据
        uint32_t max_early_data; // 0标识不限制
        onetls_buffer *early_buffer;
    } early_data;
};


uint32_t onetls_is_dtls(onetls_ctx* ctx);
void onetls_shutdown_inner(onetls_ctx *ctx);

#endif
