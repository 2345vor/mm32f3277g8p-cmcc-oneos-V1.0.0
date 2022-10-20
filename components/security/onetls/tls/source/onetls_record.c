#include "onetls_record.h"
#include "onetls_lib.h"
#include "onetls_socket.h"
#include "onetls_alert.h"
#include "onetls_extension.h"
#include "onetls_tls.h"
#include "onetls_crypto.h"

static uint32_t onetls_tls_get_plain_data(onetls_ctx *ctx, uint8_t *in, uint32_t in_len, uint32_t *recv_len)
{
    uint32_t tmp_len = 0;
    if (ctx->message.plain_data == NULL) {
        return ONETLS_SUCCESS;
    }

    // 缓冲区的数据长度
    tmp_len = onetls_min(in_len - *recv_len, onetls_buffer_cursor_to_tail(ctx->message.plain_data));
    onetls_buffer_get_stream(ctx->message.plain_data, in, tmp_len);
    *recv_len += tmp_len;

    // 如果缓冲区数据用完了，可以清理了
    if (onetls_buffer_cursor_to_tail(ctx->message.plain_data) == 0) {
        onetls_buffer_del_safe(&(ctx->message.plain_data));
    }
    return tmp_len;
}

static uint32_t onetls_recv_nst_inner(onetls_ctx* ctx, onetls_buffer *buffer)
{
    onetls_resumption *res_data = NULL;
    uint32_t life_time = 0, age_add = 0, ret = 0;
    uint8_t nonce_len = 0, nonce[255] = { 0 };
    uint16_t ticket_len = 0, total_len = 0;

    onetls_buffer_get_uint32(buffer, &life_time);
    onetls_buffer_get_uint32(buffer, &age_add);
    onetls_buffer_get_uint8(buffer, &nonce_len);
    if (nonce_len > (onetls_buffer_cursor_to_size(buffer) - 2)) {
        onetls_check_errlog(ONETLS_RECORD_INVALID_LEN, "onetls_recv_nst_inner nonce len");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_RECORD_INVALID_LEN;
    }

    onetls_buffer_get_stream(buffer, nonce, nonce_len);
    onetls_buffer_get_uint16(buffer, &ticket_len);
    if (ticket_len > (onetls_buffer_cursor_to_size(buffer) - 2)) {
        onetls_check_errlog(ONETLS_RECORD_INVALID_LEN, "onetls_recv_nst_inner ticket len");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_RECORD_INVALID_LEN;
    }
    total_len = sizeof(onetls_resumption) + ticket_len;
    res_data = (onetls_resumption*)onetls_malloc(total_len);
    onetls_assert(res_data != NULL);
    memset(res_data, 0, sizeof(onetls_resumption) + ticket_len);

    res_data->life_time = life_time;
    res_data->age_add = age_add;
    res_data->ticket_len = ticket_len;
    onetls_buffer_get_stream(buffer, res_data->ticket, ticket_len);

    ret = onetls_deal_with_nst(ctx, buffer, res_data, total_len, nonce, nonce_len);
    onetls_check_errlog(ret, "onetls_deal_with_nst");

    onetls_free(res_data);
    return ret;
}

static uint32_t onetls_recv_nst(onetls_ctx* ctx, onetls_buffer *buffer)
{
    uint32_t pkt_len = 0;
    if (ctx->resumption.ticket_cb == NULL) {    // 不要报错
        return ONETLS_SUCCESS;        
    }
    onetls_buffer_get_uint24(buffer, &pkt_len);
    if ((onetls_buffer_cursor_to_tail(buffer) < 16) || (pkt_len != onetls_buffer_cursor_to_tail(buffer))) {
        onetls_check_errlog(ONETLS_EXCEPTION_MSG, "onetls_recv_nst len");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_EXCEPTION_MSG;
    }
    return onetls_recv_nst_inner(ctx, buffer);
}

static uint32_t onetls_tls_post_hs_msg(onetls_ctx *ctx)
{
    uint8_t mt = 0;
    uint32_t ret = 0;
    onetls_buffer *buffer = ctx->message.plain_data;

    // 获取二级消息类型
    onetls_buffer_get_uint8(buffer, &mt);
    switch (mt) {
    case ONETLS_SERVER_NEW_SESSION_TICKET:
        ret = onetls_recv_nst(ctx, buffer);
        onetls_check_errlog(ret, "onetls_recv_nst");
        break;
    case ONETLS_KEY_UPDATE:
        ret = onetls_tls_recv_key_update(ctx, buffer);
        onetls_check_errlog(ret, "onetls_tls_recv_key_update");
        break;
    default:    // 其他的消息都不支持
        onetls_check_errlog(mt, "onetls_tls_deal_with_hs_msg");
        ret = ONETLS_EXCEPTION_MSG;
        break;
    }
    return ret;
}

static uint32_t onetls_check_recv(onetls_ctx *ctx, uint8_t mt)
{
    uint32_t ret = ONETLS_EXCEPTION_MSG;
    onetls_buffer *buffer = ctx->message.plain_data;
    switch (mt) {
        case ONETLS_MT_APPLICATION_DATA: // 数据报文不用处理，直接返回给应用层
            return ONETLS_SUCCESS;  // must return
        case ONETLS_MT_HANDSHAKE:   // post handshake message
            ret = onetls_tls_post_hs_msg(ctx);
            onetls_check_errlog(ret, "onetls_tls_post_hs_msg");
            break;  
        case ONETLS_MT_ALERT:   // 告警报文
            onetls_buffer_get_uint8(buffer, ctx->message.alert);
            onetls_buffer_get_uint8(buffer, ctx->message.alert + 1);
            ret = onetls_recv_alert(ctx);
            onetls_check_errlog(ret, "onetls_recv_alert");
            break; 
        case ONETLS_MT_CCS: // 忽略
        case ONETLS_DTLS_OK:
            ret = ONETLS_SUCCESS;
            break; 
        default:
            onetls_check_errlog(ONETLS_EXCEPTION_MSG, "onetls_check_recv unkown type[%d]", mt);
            onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_UNEXPECTED_MESSAGE);
            break;
    }

    onetls_buffer_del_safe(&(ctx->message.plain_data));
    return ret;
}

static uint32_t onetls_tls_recv_record_header(onetls_ctx *ctx, onetls_buffer *buffer, uint16_t *record_len)
{
    uint32_t recv_len = 0, ret = 0;
    uint16_t version = 0, legacy_version = onetls_is_dtls(ctx) ? ONETLS_VERSION_DTLS_12 : ONETLS_VERSION_TLS_12;

    if (onetls_buffer_get_tail(buffer) < ONETLS_RECORD_HEADER_LEN) {
        ret = onetls_sock_recv(ctx, 
                               onetls_buffer_get_tail_p(buffer), ONETLS_RECORD_HEADER_LEN - onetls_buffer_get_tail(buffer), 
                               &recv_len, ONETLS_SOCKET_WAIT_TIME);
        onetls_buffer_setup_tail(buffer, recv_len); // 得先平移游标，在判断是否操作成功
        if (ret != ONETLS_SUCCESS) {
            return ret;
        }        
    }

    // 检查报文长度是否合法
    onetls_buffer_set_cursor(buffer, 1);
    onetls_buffer_get_uint16(buffer, &version);
    if (version < legacy_version) {
        onetls_check_errlog(ONETLS_WRONG_VERSION, "onetls_tls_recv_record_header ver[%d]", version);
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_PROTOCOL_VERSION);
        return ONETLS_WRONG_VERSION;        
    }

    onetls_buffer_get_uint16(buffer, record_len);
    if (*record_len > onetls_buffer_cursor_to_size(buffer)) {
        onetls_check_errlog(ONETLS_RECORD_INVALID_LEN, "onetls_tls_recv_record_header len[%d]", *record_len);
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_RECORD_OVERFLOW);
        return ONETLS_RECORD_INVALID_LEN;
    }
    return ONETLS_SUCCESS;
}

static uint32_t onetls_tls_recv_record_payload(onetls_ctx *ctx, onetls_buffer *buffer, uint16_t record_len)
{
    uint32_t ret = 0, recv_len = 0;
    
    record_len += ONETLS_RECORD_HEADER_LEN;    // 总消息长度
    if (record_len <= onetls_buffer_get_tail(buffer)) { // 记录层携带的长度不合理
        onetls_check_errlog(record_len, "onetls_tls_recv_record_payload");
        return ONETLS_RECORD_INVALID_LEN;
    }

    ret = onetls_sock_recv(ctx, 
                           onetls_buffer_get_tail_p(buffer), record_len - onetls_buffer_get_tail(buffer), 
                           &recv_len, ONETLS_SOCKET_WAIT_TIME);
    onetls_buffer_setup_tail(buffer, recv_len); // 得先平移游标，在判断是否操作成功
    if (ret != ONETLS_SUCCESS) {
        return ret;
    }
    onetls_buffer_set_cursor(buffer, 0);
    return ONETLS_SUCCESS;
}

static uint32_t onetls_tls_recv_record(onetls_ctx *ctx, onetls_buffer *buffer)
{
    // 如果没有足够的头部了，先解析出总长度
    uint16_t record_len = 0;
    uint32_t ret = onetls_tls_recv_record_header(ctx, buffer, &record_len);
    if (ret != ONETLS_SUCCESS) {
        return ret;
    }
    ret = onetls_tls_recv_record_payload(ctx, buffer, record_len);
    if (ret != ONETLS_SUCCESS) {
        return ret;
    }
    return ONETLS_SUCCESS;    
}

static uint32_t onetls_cache_recv_packet(onetls_ctx *ctx, onetls_buffer *buffer, uint8_t content_type, uint16_t epoch, uint64_t seq_num)
{
    uint32_t length = 0, offset = 0, offset_len = 0;
    uint16_t msg_seq = 0;

    if ((ctx->state >= ONETLS_STATE_OK) || (ctx->handshake == NULL)) {
        return ONETLS_SUCCESS;
    }

    onetls_list_node *tmp_node = NULL;
    onetls_packet_node *cur_node = NULL;
    
    onetls_buffer_set_cursor(buffer, ONETLS_RECORD_HEADER_LEN);
    
    if (content_type == ONETLS_MT_HANDSHAKE) {  // 获取msg_seq
        onetls_buffer_setup_cursor(buffer, 1);  // MT
        onetls_buffer_get_uint24(buffer, &length);
        onetls_buffer_get_uint16(buffer, &msg_seq);
        onetls_buffer_get_uint24(buffer, &offset);
        onetls_buffer_get_uint24(buffer, &offset_len);
        
        if ((offset != 0) || (length != offset_len)) {
            // TODO: 暂不支持分片
            return ONETLS_RECORD_INVALID_LEN;
        }
    }

    cur_node = (onetls_packet_node *)onetls_malloc(sizeof(onetls_packet_node));
    memset(cur_node, 0, sizeof(onetls_packet_node));
    cur_node->message_seq = msg_seq;
    cur_node->packet = buffer;
    cur_node->epoch = epoch;
    cur_node->seq_num = seq_num;
    cur_node->type = content_type;
    cur_node->time_out = onetls_get_time();

    onetls_list_init(&(cur_node->list));
    cur_node->list.v_ptr = cur_node;

    tmp_node = ctx->handshake->recv_list.next;
    while (tmp_node != &(ctx->handshake->recv_list)) {
        onetls_packet_node *packet_node = (onetls_packet_node *)(tmp_node->v_ptr);
        onetls_assert(packet_node != NULL);

        if (epoch == packet_node->epoch) {
            if ((seq_num == packet_node->seq_num) || 
                    ((content_type == ONETLS_MT_HANDSHAKE) && 
                    (msg_seq == packet_node->message_seq))) {
                onetls_free(cur_node);
                onetls_check_errlog(ONETLS_SEQ_NUM_HITED_AGAIN, "onetls_cache_recv_packet");
                return ONETLS_SEQ_NUM_HITED_AGAIN;
            }
        }

        if (epoch < packet_node->epoch) {
            break;
        }
        tmp_node = tmp_node->next;
    }

    onetls_list_add(tmp_node->prev, &(cur_node->list));
    ctx->handshake->recv_list.v_byte ++;    // 存放报文缓存个数
    return ONETLS_SUCCESS;
}

static uint32_t onetls_dtls_recv_packet_check_full(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint8_t content_type = 0;
    uint16_t version = 0, epoch = 0, length = 0;
    uint64_t seq_num = 0;
    onetls_buffer *one_packet = NULL;

    onetls_buffer_get_uint8(buffer, &content_type);    
    onetls_buffer_get_uint16(buffer, &version);
    if (version < ONETLS_VERSION_DTLS_12) {  // 如果版本号不对。直接弃这个消息, 断链，记录层其他消息已经在收报时候校验了
        onetls_check_errlog(ONETLS_WRONG_VERSION, "onetls_dtls_recv_packet_check_full");
        return ONETLS_WRONG_VERSION;
    }

    onetls_buffer_get_uint16(buffer, &epoch);   // 简单校验
    if ((epoch < ctx->security.dtls.record_num_rd.epoch) || 
        (epoch > (ctx->security.dtls.record_num_rd.epoch + ONETLS_DTLS_EPOCH_LIMIT))) {
        onetls_check_errlog(ONETLS_EPOCH_NOT_IN_WINDOWS, "onetls_dtls_recv_packet_check epoch");
        return ONETLS_EPOCH_NOT_IN_WINDOWS;  // 整个报文都不要了。
    }
    onetls_buffer_get_uint48(buffer, &seq_num);
    if (epoch == ctx->security.dtls.record_num_rd.epoch) {  // 不相等的话，就暂时不校验这个报文的seq_num
        // 校验seq的范围，必须在窗口里面
        if (onetls_abs(seq_num, onetls_trans_seq_num(ctx->security.dtls.record_num_rd.seq_num, 6)) > ONETLS_DTLS_WINDOWS_SIZE) {
            onetls_check_errlog(ONETLS_SEQ_NUM_NOT_IN_WINDOWS, "onetls_dtls_recv_packet_check seq");
            return ONETLS_SEQ_NUM_NOT_IN_WINDOWS;
        }
    } else if (seq_num > ONETLS_DTLS_WINDOWS_SIZE) {
        onetls_check_errlog(ONETLS_SEQ_NUM_NOT_IN_WINDOWS, "onetls_dtls_recv_packet_check seq");
        return ONETLS_SEQ_NUM_NOT_IN_WINDOWS;
    }

    onetls_buffer_get_uint16(buffer, &length);   // 简单校验
    if (length > onetls_buffer_cursor_to_tail(buffer)) {
        onetls_check_errlog(ONETLS_SEQ_NUM_NOT_IN_WINDOWS, "onetls_dtls_recv_packet_check len");
        return ONETLS_RECORD_INVALID_LEN;
    }

    if (content_type == ONETLS_MT_ALERT) {  // 如果是告警，就直接处理了
        onetls_buffer_get_uint8(buffer, ctx->message.alert);
        onetls_buffer_get_uint8(buffer, ctx->message.alert + 1);
        onetls_buffer_reset(buffer);    // 全都不要了
        // 处理告警
        onetls_recv_alert(ctx);
        return ONETLS_RECV_ALERT;
    }

    if (content_type == ONETLS_MT_CCS) {    // 收到了ccs报文的话，直接将epoch增加，然后不缓存这个报文了。
        ctx->handshake->msg_tag.server_ccs ++;
        ctx->security.dtls.record_num_rd.epoch ++;
        onetls_seq_num_reset(ctx->security.dtls.record_num_rd.seq_num, 6);
        onetls_buffer_setup_cursor(buffer, length);
        return ONETLS_SUCCESS;
    }

    // 基本校验过了。
    onetls_buffer_setback_cursor(buffer, ONETLS_RECORD_HEADER_LEN);
    one_packet = onetls_buffer_new(length + ONETLS_RECORD_HEADER_LEN);

    // 将这一个报文拷贝出来
    onetls_buffer_get_stream(buffer, onetls_buffer_get_data(one_packet), length + ONETLS_RECORD_HEADER_LEN);
    onetls_buffer_set_tail(one_packet, length + ONETLS_RECORD_HEADER_LEN);

    if (onetls_cache_recv_packet(ctx, one_packet, content_type, epoch, seq_num)) {
        onetls_buffer_del(one_packet);
        return ONETLS_FAIL;
    }
    return ONETLS_SUCCESS;
}

static uint32_t onetls_dtls_recv_packet_check_cslee(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint8_t hdr_cslee = 0, short_seq = 0;
    uint16_t length = 0, hdr_len = 0, long_seq = 0, epoch = 0;
    onetls_buffer *one_packet = NULL;
    uint32_t cur_cursor = onetls_buffer_get_cursor(buffer); // 记住这里，一会儿要回来拷贝
    uint64_t seq_num = 0;

    if (onetls_buffer_cursor_to_tail(buffer) < ctx->cid_len) {
        onetls_check_errlog(ONETLS_RECORD_INVALID_LEN, "onetls_dtls_recv_packet_check_cslee");
        return ONETLS_RECORD_INVALID_LEN;
    }

    onetls_buffer_get_uint8(buffer, &hdr_cslee);
    if (hdr_cslee & 0x10) { // 是否有cid
        if (ctx->cid_len == 0) {
            onetls_check_errlog(ONETLS_SHOULD_NOT_GET_CID, "onetls_dtls_recv_packet_check_cslee");
            return ONETLS_SHOULD_NOT_GET_CID;
        }
        onetls_buffer_setup_cursor(buffer, ctx->cid_len);
    } else if (ctx->cid_len != 0) {
        onetls_check_errlog(ONETLS_SHOULD_GET_CID, "onetls_dtls_recv_packet_check_cslee");
        return ONETLS_SHOULD_GET_CID;
    }
    
    if (hdr_cslee & 0x08) { // seq
        onetls_buffer_get_uint16(buffer, &long_seq);
        seq_num = long_seq;
    } else {
        onetls_buffer_get_uint8(buffer, &short_seq);
        seq_num = short_seq;
    }

    if (hdr_cslee & 0x04) {
        onetls_buffer_get_uint16(buffer, &length);
    } else {
        length = onetls_buffer_cursor_to_tail(buffer);
    }
    hdr_len = onetls_buffer_get_cursor(buffer) - cur_cursor;
    onetls_buffer_set_cursor(buffer, cur_cursor);

    if (length > onetls_buffer_cursor_to_tail(buffer)) {
        onetls_check_errlog(ONETLS_RECORD_INVALID_LEN, "onetls_dtls_recv_packet_check_cslee");
        return ONETLS_RECORD_INVALID_LEN;
    }

    epoch = hdr_cslee & 0x03;
    one_packet = onetls_buffer_new(length + hdr_len);
    // 将这一个报文拷贝出来
    onetls_buffer_get_stream(buffer, onetls_buffer_get_data(one_packet), length + hdr_len);
    onetls_buffer_set_tail(one_packet, length + hdr_len);

    if (onetls_cache_recv_packet(ctx, one_packet, hdr_cslee, epoch, seq_num)) {
        onetls_buffer_del(one_packet);
        return ONETLS_FAIL;
    }
    return ONETLS_SUCCESS;
}

static uint32_t onetls_dtls_recv_packet_check(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint8_t content_type = 0;
    uint32_t ret = 0;

    onetls_buffer_set_cursor(buffer, 0);

    if ((ctx->state >= ONETLS_STATE_OK) || (ctx->handshake == NULL)) {
        return ONETLS_SUCCESS;
    }  

    // 可能有多个报文
    do {
        onetls_buffer_get_uint8(buffer, &content_type);
        onetls_buffer_setback_cursor(buffer, 1);    // 获取一下报文类型就行，偏移不能动
        if ((content_type & 0xE0) == 0x20) {    // 00100000 11100000
            ret = onetls_dtls_recv_packet_check_cslee(ctx, buffer);
        } else {
            ret = onetls_dtls_recv_packet_check_full(ctx, buffer);
        }
    } while ((onetls_buffer_cursor_to_tail(buffer) > 0) && (ret == ONETLS_SUCCESS));
    return ONETLS_SUCCESS;
}

static uint32_t onetls_dtls_recv_record(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint32_t ret = 0, recv_len = 0;
    // 先去缓冲队列里面找，如果有合适的。就暂时不去收包

    onetls_check_dtls_hs_msg(ctx);
    if (onetls_buffer_get_tail(ctx->message.msg_in)) {
        return ONETLS_SUCCESS;
    }

    ret = onetls_sock_recv(ctx, 
                           onetls_buffer_get_tail_p(buffer), ctx->mtu - onetls_buffer_get_tail(buffer), 
                           &recv_len, ONETLS_SOCKET_WAIT_TIME);
    onetls_buffer_setup_tail(buffer, recv_len);
    if (ret != ONETLS_SUCCESS) {
        return ret;
    }
    if (recv_len < ONETLS_RECORD_HEADER_LEN) {
        onetls_check_errlog(ONETLS_SOCKET_PACKET_LEN, "onetls_dtls_recv_record[len:%d]", recv_len);
        return ONETLS_SOCKET_PACKET_LEN;
    }
    ret = onetls_dtls_recv_packet_check(ctx, buffer);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_dtls_recv_packet_check");
        return ret;
    }
    if (ctx->state < ONETLS_STATE_OK) {
        onetls_buffer_reset(buffer);    // 受方向的报文，会在外面check，从缓冲区提出来
    }    
    return ONETLS_SUCCESS;
}

static uint32_t onetls_dtls_send_inner(onetls_ctx *ctx, uint8_t *out, uint32_t out_len, uint32_t *send_len)
{
    uint32_t ret = 0;

    if ((onetls_is_dtls(ctx)) && (out_len > ctx->mtu)) {
        onetls_check_errlog(ONETLS_TOO_MUCH_DATA, "onetls_check_data_channel len");
        return ONETLS_TOO_MUCH_DATA;
    }

    onetls_buffer_reset(ctx->message.msg_out);

    ret = onetls_send_encode_data(ctx, ctx->message.msg_out, out, out_len, ONETLS_MT_APPLICATION_DATA);
    if (ret != ONETLS_SUCCESS) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        onetls_check_errlog(ret, "onetls_dtls_send_inner onetls_send_encode_i");
        return ret;
    }

    ret = onetls_send_record(ctx, ctx->message.msg_out);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_dtls_send_inner onetls_send_record");
        return ret;
    }    

    *send_len = out_len;
    return ONETLS_SUCCESS;
}

static uint32_t onetls_tls_send_inner(onetls_ctx *ctx, uint8_t *out, uint32_t out_len, uint32_t *send_len)
{
    uint32_t ret = 0;
    uint32_t slice_len = 0;
    do {
        if (onetls_buffer_cursor_to_tail(ctx->message.msg_out) != 0) {    // 内部有数据
            ret = onetls_send_record(ctx, ctx->message.msg_out);
            if (ret != ONETLS_SUCCESS) {
                return ret;
            }
            continue;
        }

        // 能发送的长度
        if (*send_len >= out_len) {
            break;
        }
        slice_len = onetls_min(out_len - *send_len, ONETLS_MAX_PACKET_LEN);
        ret = onetls_send_encode_data(ctx, ctx->message.msg_out, out + *send_len, slice_len, ONETLS_MT_APPLICATION_DATA);
        if (ret != ONETLS_SUCCESS) {
            onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
            onetls_check_errlog(ret, "onetls_tls_send_inner onetls_send_encode_i");
            return ret;
        }
        *send_len += slice_len;
    } while (!ctx->shutdown);
    return ONETLS_SUCCESS;
}

static uint32_t onetls_dtls_recv_inner(onetls_ctx *ctx, uint8_t *in, uint32_t in_len, uint32_t *recv_len)
{
    uint32_t ret = 0;
    uint8_t mt = 0;
    
    // 收取一个分片
    onetls_buffer_reset(ctx->message.msg_in);
    ret = onetls_recv_record(ctx, ctx->message.msg_in);
    if (ret != ONETLS_SUCCESS) {
        return ret;  // 可能超时，可能断链
    }
    
    // 简单校验一下分片
    ret = onetls_want_a_msg(ctx, ONETLS_MT_APPLICATION_DATA);
    if (ret != ONETLS_SUCCESS) {
        return ret;
    }

    // 解密
    ctx->message.plain_data = onetls_buffer_new(onetls_buffer_get_tail(ctx->message.msg_in));
    onetls_assert(ctx->message.plain_data != NULL);

    ret = onetls_recv_decode_buffer(ctx, ctx->message.plain_data, ctx->message.msg_in, &mt);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_recv_decode[%d]", mt);
        onetls_buffer_del(ctx->message.plain_data);
        ctx->message.plain_data = NULL;
        return ret;
    }

    // 查看报文内容，是不是我们要得数据报文，有可能是其他的协议报文
    ret = onetls_check_recv(ctx, mt);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_check_recv[%d]", mt);
        return ret;
    }

    *recv_len = 0;
    if (ctx->message.plain_data != NULL) {
        *recv_len = onetls_min(in_len, onetls_buffer_cursor_to_tail(ctx->message.plain_data));
        onetls_buffer_get_stream(ctx->message.plain_data, in, *recv_len);
        onetls_buffer_del_safe(&(ctx->message.plain_data)); 
    }
    return ONETLS_SUCCESS;
}

static uint32_t onetls_tls_recv_inner(onetls_ctx *ctx, uint8_t *in, uint32_t in_len, uint32_t *recv_len)
{
    uint32_t ret = 0;
    uint8_t hit = 0, mt = 0;

    do {
        // 看下缓冲区中是否有数据
        if (onetls_tls_get_plain_data(ctx, in, in_len, recv_len) != 0) {
            continue;
        }
        onetls_assert(ctx->message.plain_data == NULL);
        // 已经收到了部分数据
        if (hit) {
            break;
        }
        
        // 收取一个分片
        onetls_buffer_reset(ctx->message.msg_in);
        ret = onetls_recv_record(ctx, ctx->message.msg_in);
        if (ret != ONETLS_SUCCESS) {
            return ret;  // 可能超时，可能断链
        }
        hit = 1;

        // 简单校验一下分片
        ret = onetls_want_a_msg(ctx, ONETLS_MT_APPLICATION_DATA);
        if (ret == ONETLS_SOCKET_TRYAGAIN) continue; // 可能收到了告警或者其他不需要的报文
        if (ret != ONETLS_SUCCESS) {
            onetls_check_errlog(ret, "onetls_tls_recv_inner onetls_want_a_msg");
            onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_UNEXPECTED_MESSAGE);
            return ret;
        }

        // 解密
        ctx->message.plain_data = onetls_buffer_new(onetls_buffer_get_tail(ctx->message.msg_in));
        onetls_assert(ctx->message.plain_data != NULL);

        ret = onetls_recv_decode_buffer(ctx, ctx->message.plain_data, ctx->message.msg_in, &mt);
        if (ret != ONETLS_SUCCESS) {
            onetls_check_errlog(ret, "onetls_recv_decode[%d]", mt);
            onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_DECRYPT_ERROR);
            onetls_buffer_del(ctx->message.plain_data);
            ctx->message.plain_data = NULL;
            return ret;
        }

        // 查看报文内容，是不是我们要得数据报文，有可能是其他的协议报文
        ret = onetls_check_recv(ctx, mt);
        if (ret != ONETLS_SUCCESS) {
            onetls_check_errlog(ret, "onetls_check_recv[%d]", mt);
            return ret;
        }
    } while ((*recv_len < in_len) && (!ctx->shutdown));
    return ret;
}

static void onetls_dtls_makeup_seq_num(onetls_ctx *ctx, uint8_t sending, uint8_t *seq_num, uint8_t *mask, uint8_t mask_len)
{
    uint16_t epoch = (sending) ? 
                     ctx->security.dtls.record_num_wr.epoch : 
                     ctx->security.dtls.record_num_rd.epoch;

    seq_num[0] = (epoch >> 8) & 0xff;
    seq_num[1] = (epoch) & 0xff;

    if (mask_len == 2) {
        seq_num[6] ^= mask[0];
        seq_num[7] ^= mask[1];
    } else {
        seq_num[7] ^= mask[0];
    }
}

static uint32_t onetls_dtls_send_encode(onetls_ctx *ctx, onetls_buffer *cipher_text, uint8_t *buffer, uint32_t buffer_len, uint8_t mt)
{
    uint8_t nonce[ONETLS_MAX_IV_LEN] = { 0 }, offset = ctx->cipher->iv_len - 8, loop = 0;
    uint8_t seq_no[8] = { 0 }, mask[16] = { 0 };
    uint32_t ret, cipher_len = buffer_len + ctx->cipher->tag_len + 1 + 5;

    onetls_dtls_makeup_seq_num(ctx, 1, seq_no, ctx->security.dtls.record_num_wr.seq_num + 4, 2);
    memcpy(nonce, ctx->security.wr_iv, offset); // 静态iv
    for (loop = 0; loop < 8; loop ++) {         // 生成nonce
        nonce[offset + loop] = (ctx->security.wr_iv[offset + loop]) ^ (seq_no[loop]);
    }

    // 不支持CID
    if (onetls_buffer_set_tail(cipher_text, cipher_len) != cipher_len) {    // 判断空间够不够
        onetls_check_errlog(ONETLS_MEM_NO_ZONE, "onetls_dtls_send_encode");
        return ONETLS_MEM_NO_ZONE;
    }

    onetls_buffer_set_cursor(cipher_text, 0); // 放置到头部
    onetls_buffer_put_uint8(cipher_text, 0x2C | (seq_no[1] & 0x03));
    onetls_buffer_put_uint8(cipher_text, seq_no[6]);
    onetls_buffer_put_uint8(cipher_text, seq_no[7]);
    onetls_buffer_put_uint16(cipher_text, cipher_len - 5);

    ret = onetls_aead_encrypt(ctx, nonce, // nonce，长度由算法决定
                              onetls_buffer_get_data(cipher_text), 5,
                              buffer, buffer_len, &mt,
                              onetls_buffer_get_data(cipher_text) + 5, &cipher_len);
    if (ret != ONETLS_SUCCESS) {    // 如果是函数申请的内存，该释放就释放。
        onetls_check_errlog(ret, "onetls_aead_encrypt");
        return ret;
    }

    ret = onetls_aes_ecb_encrypt(ctx->security.c_sn_key, ctx->cipher->key_len, onetls_buffer_get_data(cipher_text) + 5, mask);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_aes_ecb_encrypt");
        return ret;
    }

    onetls_buffer_set_cursor(cipher_text, 1);
    onetls_buffer_put_uint8(cipher_text, seq_no[6] ^ mask[0]);
    onetls_buffer_put_uint8(cipher_text, seq_no[7] ^ mask[1]);
    onetls_buffer_set_cursor(cipher_text, 0);
    onetls_seq_num_add(ctx->security.dtls.record_num_wr.seq_num, 6);
    return ret;
}

static uint32_t onetls_tls_send_encode(onetls_ctx *ctx, onetls_buffer *cipher_text, uint8_t *buffer, uint32_t buffer_len, uint8_t mt)
{
    uint8_t nonce[ONETLS_MAX_IV_LEN] = { 0 }, offset = ctx->cipher->iv_len - 8, loop = 0;
    uint32_t ret = 0, cipher_len = buffer_len + ctx->cipher->tag_len + 1 + ONETLS_RECORD_HEADER_LEN;

    memcpy(nonce, ctx->security.wr_iv, offset); // 静态iv
    for (loop = 0; loop < 8; loop ++) {         // 生成nonce
        nonce[offset + loop] = (ctx->security.wr_iv[offset + loop]) ^ (ctx->security.wr_seq[loop]);
    }

    // 带上记录层的头
    if (onetls_buffer_set_tail(cipher_text, cipher_len) != cipher_len) {    // 判断空间够不够
        onetls_check_errlog(ONETLS_MEM_NO_ZONE, "onetls_tls_send_encode");
        return ONETLS_MEM_NO_ZONE;
    }

    onetls_buffer_set_cursor(cipher_text, 0); // 放置到头部
    onetls_buffer_put_uint8(cipher_text, ONETLS_MT_APPLICATION_DATA);
    onetls_buffer_put_uint16(cipher_text, ONETLS_VERSION_TLS_12);
    onetls_buffer_put_uint16(cipher_text, cipher_len - ONETLS_RECORD_HEADER_LEN);

    ret = onetls_aead_encrypt(ctx, nonce, // nonce，长度由算法决定
                              onetls_buffer_get_data(cipher_text), ONETLS_RECORD_HEADER_LEN,
                              buffer, buffer_len, &mt,
                              onetls_buffer_get_data(cipher_text) + ONETLS_RECORD_HEADER_LEN, &cipher_len);
    if (ret != ONETLS_SUCCESS) {    // 如果是函数申请的内存，该释放就释放。
        onetls_check_errlog(ret, "onetls_aead_encrypt");
        return ret;
    }
    onetls_buffer_set_cursor(cipher_text, 0);
    onetls_seq_num_add(ctx->security.wr_seq, sizeof(ctx->security.wr_seq));
    return ret;
}

static uint32_t onetls_dtls_get_seq_offset(onetls_ctx *ctx, onetls_buffer *buffer, uint32_t *offset, uint32_t *cipher_len, uint8_t *seqno, uint8_t *add, uint32_t *add_len)
{
    uint8_t mask[16] = { 0 }, type = 0, mask_len = 0;
    uint16_t length = 0;
    uint32_t ret = 0;

    onetls_buffer_set_cursor(buffer, 0);
    onetls_buffer_get_uint8(buffer, &type);
    *offset = 1;
    add[*add_len] = type;
    *add_len += 1;

    if (type & 0x10) {
        *offset += ctx->cid_len;    // 暂不支持
        onetls_buffer_setup_cursor(buffer, ctx->cid_len);
    }
    if (type & 0x08) {
        *offset += 2;
        *add_len += 2;
        mask_len = 2;
        onetls_buffer_get_uint8(buffer, seqno + 6);
        onetls_buffer_get_uint8(buffer, seqno + 7);
    } else {
        *offset += 1;
        mask_len = 1;
        *add_len += 1;
        onetls_buffer_get_uint8(buffer, seqno + 7);
    }

    if (type & 0x04) {
        *offset += 2;
        onetls_buffer_get_uint16(buffer, &length);
        *cipher_len = length;
        add[*add_len] = (length >> 8) & 0xff;
        *add_len += 1;
        add[*add_len] = (length) & 0xff;
        *add_len += 1;
    } else {
        *cipher_len = onetls_buffer_cursor_to_tail(buffer);
    }

    ret = onetls_aes_ecb_encrypt(ctx->security.s_sn_key, ctx->cipher->key_len, onetls_buffer_get_data(buffer) + *offset, mask);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_aes_ecb_encrypt");
        return ret;
    }

    onetls_dtls_makeup_seq_num(ctx, 0, seqno, mask, mask_len);
    if (mask_len == 1) {
        add[1] = seqno[7];
    } else {
        add[1] = seqno[6];
        add[2] = seqno[7];
    }
    return ONETLS_SUCCESS;
}

static uint32_t onetls_dtls_recv_decode(onetls_ctx *ctx, onetls_buffer *plain_text, onetls_buffer *buffer, uint8_t *mt)
{
    uint8_t seqno[8] = { 0 };
    uint8_t add[21] = { 0 };
    uint8_t nonce[ONETLS_MAX_IV_LEN] = { 0 }, offset_iv = ctx->cipher->iv_len - 8, loop = 0;
    uint32_t ret = 0, offset = 0, plain_len = onetls_buffer_get_tail(buffer) - ctx->cipher->tag_len - 1;
    uint32_t cipher_len = 0, add_len = 0;

    ret = onetls_dtls_get_seq_offset(ctx, buffer, &offset, &cipher_len, seqno, add, &add_len);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_dtls_get_seq_offset");
        return ret;
    }

    memcpy(nonce, ctx->security.rd_iv, ctx->cipher->iv_len);    // 静态iv
    for (loop = 0; loop < 8; loop ++) {
        nonce[offset_iv + loop] ^= seqno[loop];
    }

    ret = onetls_aead_decrypt(ctx, nonce,
                              add, add_len,
                              onetls_buffer_get_data(buffer) + offset, onetls_buffer_get_tail(buffer) - offset,
                              mt, onetls_buffer_get_data(plain_text), &plain_len);
    if (ret != ONETLS_SUCCESS) {    // 如果是函数申请的内存，该释放就释放。
        onetls_check_errlog(ret, "onetls_aead_decrypt");
        return ret;
    }
    onetls_buffer_set_tail(plain_text, plain_len);
    onetls_buffer_set_cursor(plain_text, 0);
    onetls_seq_num_add(ctx->security.dtls.record_num_rd.seq_num, 6);
    return ONETLS_SUCCESS;
}

static uint32_t onetls_tls_recv_decode(onetls_ctx *ctx, onetls_buffer *plain_text, onetls_buffer *buffer, uint8_t *mt)
{
    uint8_t nonce[ONETLS_MAX_IV_LEN] = { 0 }, offset = ctx->cipher->iv_len - 8, loop = 0;
    uint32_t ret = 0, plain_len = onetls_buffer_get_tail(buffer) - ctx->cipher->tag_len - ONETLS_RECORD_HEADER_LEN - 1;
    
    memcpy(nonce, ctx->security.rd_iv, offset);    // 静态iv
    for (loop = 0; loop < 8; loop ++) {
        nonce[offset + loop] = (ctx->security.rd_iv[offset + loop]) ^ (ctx->security.rd_seq[loop]);
    }
    
    // 带上记录层的头
    if (onetls_buffer_set_tail(plain_text, plain_len) != plain_len) {    // 判断空间够不够
        onetls_check_errlog(ONETLS_MEM_NO_ZONE, "onetls_tls_recv_decode");
        return ONETLS_MEM_NO_ZONE;
    }

    ret = onetls_aead_decrypt(ctx, nonce,
                              onetls_buffer_get_data(buffer), ONETLS_RECORD_HEADER_LEN,
                              onetls_buffer_get_data(buffer) + ONETLS_RECORD_HEADER_LEN, onetls_buffer_get_tail(buffer) - ONETLS_RECORD_HEADER_LEN,
                              mt, onetls_buffer_get_data(plain_text), &plain_len);
    if (ret != ONETLS_SUCCESS) {    // 如果是函数申请的内存，该释放就释放。
        onetls_check_errlog(ret, "onetls_aead_decrypt");
        return ret;
    }
    
    // 明文的最后一个字节是MT
    onetls_buffer_set_cursor(plain_text, 0);    
    onetls_seq_num_add(ctx->security.rd_seq, sizeof(ctx->security.rd_seq));
    return ret; 
}

uint32_t onetls_release_cache_list(onetls_handshake *handshake)
{
    onetls_list_node *cur_node = NULL, *next_node = NULL;
    if (handshake == NULL) {
        return ONETLS_SUCCESS;
    }
    onetls_buffer_del_safe(&(handshake->send_cache));

    cur_node = handshake->recv_list.next;
    while (cur_node != &(handshake->recv_list)) {
        next_node = cur_node->next;

        onetls_packet_node *packet_node = (onetls_packet_node *)(cur_node->v_ptr);
        onetls_assert(packet_node != NULL);
        onetls_list_del(cur_node);

        onetls_buffer_del(packet_node->packet);
        onetls_free(packet_node);

        cur_node = next_node;
    }
    handshake->recv_list.v_byte = 0;
    return ONETLS_SUCCESS;
}

uint32_t onetls_want_a_msg(onetls_ctx* ctx, uint8_t mt)
{
    onetls_buffer *buffer = ctx->message.msg_in;
    uint8_t content_type = 0;

    onetls_buffer_set_cursor(buffer, 0);
    onetls_buffer_get_uint8(buffer, &content_type);

    // 收到了告警消息
    if (content_type == ONETLS_MT_ALERT) {
        // 把消息拷贝给告警缓冲区，继续读
        onetls_buffer_setup_cursor(buffer, 4);
        if (onetls_is_dtls(ctx)) {
            onetls_buffer_setup_cursor(buffer, 8);  // 序列号和epoch在记录层校验
        }
        onetls_buffer_get_uint8(buffer, ctx->message.alert);
        onetls_buffer_get_uint8(buffer, ctx->message.alert + 1);
        onetls_buffer_reset(buffer);
        // 处理告警
        onetls_recv_alert(ctx);
        if (ctx->handshake) {
            ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_READ;
        }
        return ONETLS_SOCKET_TRYAGAIN;
    }

    if (content_type == ONETLS_MT_CCS) {   // 兼容报文
        // 重置收报缓冲区
        onetls_buffer_reset(buffer);
        onetls_tls_recv_server_ccs(ctx);
        if (ctx->handshake) {
            ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_READ;
        }
        return ONETLS_SOCKET_TRYAGAIN;
    }
    if (onetls_is_dtls(ctx) && (content_type & 0x20)) {
        content_type = ONETLS_MT_APPLICATION_DATA;
    }
    return (content_type == mt) ? ONETLS_SUCCESS : ONETLS_FAIL;
}

uint32_t onetls_recv_record(onetls_ctx *ctx, onetls_buffer *buffer)
{
    if (onetls_is_dtls(ctx)) {
        return onetls_dtls_recv_record(ctx, buffer);
    }
    return onetls_tls_recv_record(ctx, buffer);
}

uint32_t onetls_send_record(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint32_t send_len = 0, ret = 0;
    if ((buffer == NULL) || (onetls_buffer_cursor_to_tail(buffer) == 0)) {
        return ONETLS_SUCCESS;
    }

    ret = onetls_sock_send(ctx, 
                           onetls_buffer_get_cursor_p(buffer), onetls_buffer_cursor_to_tail(buffer), 
                           &send_len, ONETLS_SOCKET_WAIT_TIME);
    onetls_buffer_setup_cursor(buffer, send_len); // 得先平移游标，在判断是否操作成功
    if (ret != ONETLS_SUCCESS) {
        return ret;
    }
    // 发送完成
    onetls_buffer_reset(buffer);
    return ONETLS_SUCCESS;
}

uint32_t onetls_send_inner(onetls_ctx *ctx, uint8_t *out, uint32_t out_len, uint32_t *send_len)
{
    if (onetls_is_dtls(ctx)) {
        return onetls_dtls_send_inner(ctx, out, out_len, send_len);
    }
     return onetls_tls_send_inner(ctx, out, out_len, send_len);
}

uint32_t onetls_recv_inner(onetls_ctx *ctx, uint8_t *in, uint32_t in_len, uint32_t *recv_len)
{
    if (onetls_is_dtls(ctx)) {
        return onetls_dtls_recv_inner(ctx, in, in_len, recv_len);
    }
    return onetls_tls_recv_inner(ctx, in, in_len, recv_len);
}

uint32_t onetls_send_encode_data(onetls_ctx *ctx, onetls_buffer *cipher_text, uint8_t *buffer, uint32_t buffer_len, uint8_t mt)
{
    if (onetls_is_dtls(ctx)) {
        return onetls_dtls_send_encode(ctx, cipher_text, buffer, buffer_len, mt);
    }
    return onetls_tls_send_encode(ctx, cipher_text, buffer, buffer_len, mt);
}

uint32_t onetls_send_encode_buffer(onetls_ctx *ctx, onetls_buffer *cipher_text, onetls_buffer *buffer, uint8_t mt)
{
    return onetls_send_encode_data(ctx, cipher_text, onetls_buffer_get_cursor_p(buffer), onetls_buffer_cursor_to_tail(buffer), mt);
}

uint32_t onetls_recv_decode_buffer(onetls_ctx *ctx, onetls_buffer *plain_text, onetls_buffer *buffer, uint8_t *mt)
{
    if (onetls_is_dtls(ctx)) {
        return onetls_dtls_recv_decode(ctx, plain_text, buffer, mt);
    }
    return onetls_tls_recv_decode(ctx, plain_text, buffer, mt);
}
