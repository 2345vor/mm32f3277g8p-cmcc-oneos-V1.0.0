#include "onetls_extension.h"
#include "onetls_lib.h"
#include "onetls_alert.h"
#include "onetls_crypto.h"
#include "onetls_util.h"

#define ONETLS_MAX_RECV_EXT_NUM (8)    // 根据配置，可以适当调整一下

static onetls_extension g_onetls_client_hello_ext[] = {
    {onetls_ext_supported_versions,   onetls_ext_send_support_version},
    {onetls_ext_cookie,               onetls_ext_send_cookie},
    {onetls_ext_server_name,          onetls_ext_send_server_name},
    {onetls_ext_psk_exchange_modes,   onetls_ext_send_psk_ex_mode},
    {onetls_ext_supported_groups,     onetls_ext_send_support_group},
    {onetls_ext_key_share,            onetls_ext_send_key_share},
    {onetls_ext_early_data,           onetls_ext_send_early_data},
    {onetls_ext_session_ticket,       onetls_ext_send_session_tickets},
    {onetls_ext_signature_algorithms, onetls_ext_send_sign_alg},
    {onetls_ext_max_fragment_length,  onetls_ext_send_max_fragment},
    {onetls_ext_pre_shared_key,       onetls_ext_send_psk_ex}, /* MUST be last */
};

static onetls_extension g_onetls_server_hello_ext[] = {
    {onetls_ext_cookie,              onetls_ext_recv_cookie},
    {onetls_ext_supported_versions,  onetls_ext_recv_support_version},
    {onetls_ext_key_share,           onetls_ext_recv_key_share},
    {onetls_ext_pre_shared_key,      onetls_ext_recv_psk_ex},
};

static onetls_extension g_onetls_server_ee_ext[] = {
    {onetls_ext_server_name,         onetls_ext_recv_server_name},
    {onetls_ext_supported_groups,    onetls_ext_recv_support_group},
    {onetls_ext_early_data,          onetls_ext_recv_early_data_empty},
    {onetls_ext_max_fragment_length, onetls_ext_recv_max_fragment},
};

static onetls_extension g_onetls_server_nst_ext[] = {
    {onetls_ext_early_data,          onetls_ext_recv_early_data},
};

static const onetls_extension *onetls_get_ext_byid(const onetls_extension *exts, uint32_t num, uint16_t id)
{   // 查找extid
    uint32_t offset = 0;
    if (exts == NULL) {
        return NULL;
    }
    for (offset = 0; offset < num; offset ++) {
        if (exts[offset].iana_id == id) return &(exts[offset]);
    }
    return NULL;
}

static void onetls_tls_get_server_exts(uint8_t state, const onetls_extension **exts, uint32_t *num)
{
    switch (state) {
        case ONETLS_STATE_RECV_SVR_HELLO:
            *num = sizeof(g_onetls_server_hello_ext) / sizeof(onetls_extension);
            *exts = g_onetls_server_hello_ext;
            break;
        case ONETLS_STATE_RECV_SVR_EE:
            *num = sizeof(g_onetls_server_ee_ext) / sizeof(onetls_extension);
            *exts = g_onetls_server_ee_ext;
            break;
        case ONETLS_STATE_RECV_SVR_NST:
            *num = sizeof(g_onetls_server_nst_ext) / sizeof(onetls_extension);
            *exts = g_onetls_server_nst_ext;
            break;            
        default:
            *exts = NULL;
            break;
    }
}

static uint32_t onetls_tls_recv_ext_repeat_check(uint16_t *recv_exts, uint16_t check_ext)
{
    uint32_t offset = 0;
    do {
        if (recv_exts[offset] == 0) {
            recv_exts[offset] = check_ext;
            break;
        }
        if (recv_exts[offset] == check_ext) {
            return ONETLS_EXT_REPEAT;
        }
    } while ((++offset) < ONETLS_MAX_RECV_EXT_NUM);
    return ONETLS_SUCCESS;
}

uint32_t onetls_tls_parse_server_extension(onetls_ctx *ctx, onetls_buffer *buffer, uint8_t state)
{
    const onetls_extension *extension = NULL;
    uint16_t ext_id = 0, ext_total_len = 0, recv_exts[ONETLS_MAX_RECV_EXT_NUM] = { 0 };
    uint32_t ret = 0, num = 0;
    const onetls_extension *exts = NULL;
    onetls_tls_get_server_exts(state, &exts, &num);

    // 开始解析
    onetls_buffer_get_uint16(buffer, &ext_total_len);
    if (ext_total_len != onetls_buffer_cursor_to_tail(buffer)) { // 总长度校验，可以为零
        onetls_check_errlog(ONETLS_EXT_LEN_WRONG, "onetls_tls_parse_server_ext wrong len!");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_EXT_LEN_WRONG;
    }

    while ((ret == ONETLS_SUCCESS) && (onetls_buffer_cursor_to_tail(buffer) != 0)) {
        // 获取一个extension id
        onetls_buffer_get_uint16(buffer, &ext_id);
        extension = onetls_get_ext_byid(exts, num, ext_id);
        if (extension == NULL) {
            onetls_check_errlog(ONETLS_EXT_LEN_WRONG, "onetls_get_ext_byid[%d]", ext_id);
            onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_UNSUPPORTED_EXTENSION);
            return ONETLS_EXT_UNKNOWN;
        }

        ret = onetls_tls_recv_ext_repeat_check(recv_exts, ext_id);
        if (ret != ONETLS_SUCCESS) {    // 重复性检查
            onetls_check_errlog(ONETLS_EXT_REPEAT, "onetls_tls_recv_ext_repeat_check[%d]", ext_id);
            onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
            return ONETLS_EXT_REPEAT;
        }

        ret = extension->func(ctx, buffer);
        onetls_check_errlog(ret, "onetls_tls_parse_server_ext[%d]", extension->iana_id);
    }
    return ret;
}

uint32_t onetls_construct_client_hello_extension(onetls_ctx *ctx)
{
    uint32_t ret = 0, total_len_cursor = 0, total_len = 0;
    uint8_t num = sizeof(g_onetls_client_hello_ext) / sizeof(onetls_extension), offset = 0;

    onetls_buffer *buffer = ctx->message.msg_out;
    onetls_buffer_set_cursor(buffer, -1);
    
    // 长度区域预留，等填充完了，再来反写
    total_len_cursor = onetls_buffer_get_cursor(buffer);
    onetls_buffer_put_uint16(buffer, 0);

    // 添加扩展
    for (offset = 0; offset < num; offset ++) {
        onetls_buffer_set_cursor(buffer, -1);
        ret = g_onetls_client_hello_ext[offset].func(ctx, buffer);
        if (ret != ONETLS_SUCCESS) {
            onetls_check_errlog(ret, "client hello extension[ext_id = %d]", g_onetls_client_hello_ext[offset].iana_id);
            return ret;
        }        
    }
    
    total_len = onetls_buffer_get_tail(buffer) - total_len_cursor - 2;
    // 刷新扩展总长度
    onetls_buffer_set_cursor(buffer, total_len_cursor);
    onetls_buffer_put_uint16(buffer, total_len);
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_send_session_tickets(onetls_ctx *ctx, onetls_buffer *buffer)
{
    if (ctx->resumption.ticket_cb == NULL) {
        return ONETLS_SUCCESS;
    }
    // 放置MT
    onetls_buffer_put_uint16(buffer, onetls_ext_session_ticket);
    onetls_buffer_put_uint16(buffer, 0);
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_send_support_version(onetls_ctx *ctx, onetls_buffer *buffer)
{
    // 放置MT
    onetls_buffer_put_uint16(buffer, onetls_ext_supported_versions);

    // 当前因为只支持一个TLS13版本，不计算了。固定为3  
    onetls_buffer_put_uint16(buffer, 3);

    // 当前支持最新的tls13,暂不支持其他的，就一个，固定为2
    onetls_buffer_put_uint8(buffer, 2);

    if (onetls_is_dtls(ctx)) {
        onetls_buffer_put_uint16(buffer, ONETLS_VERSION_DTLS_13);
    } else {
        onetls_buffer_put_uint16(buffer, ONETLS_VERSION_TLS_13);
    }
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_recv_support_version(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint16_t length = 0, version = 0;
    onetls_buffer_get_uint16(buffer, &length);
    if (length != 2) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_EXT_LEN_WRONG;
    }

    onetls_buffer_get_uint16(buffer, &version);
    // 收方向必须有一个，且最低支持版本为tls1.3
    if (version < ONETLS_VERSION_TLS_13) {
        onetls_check_errlog(ONETLS_EXT_WRONG_VER, "onetls_ext_recv_support_version not support");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_PROTOCOL_VERSION);
        return ONETLS_EXT_WRONG_VER;
    }
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_send_psk_ex(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint16_t binder_len = 0, psk_id_len = 0;
    
    if (ctx->psk_info.ticket != NULL) {
        binder_len += onetls_hash_size(ctx->cipher->hkdf_hash) + 1;
        psk_id_len += onetls_buffer_get_tail(ctx->psk_info.ticket) + 6;
    }

    if (ctx->psk_info.hint != NULL) {
        binder_len += onetls_hash_size(ctx->cipher->hkdf_hash) + 1;
        psk_id_len += onetls_buffer_get_tail(ctx->psk_info.hint) + 6;
    }

    // 先注入identity
    onetls_buffer_put_uint16(buffer, onetls_ext_pre_shared_key);    // 放置MT
    onetls_buffer_put_uint16(buffer, psk_id_len + binder_len + 4);
    onetls_buffer_put_uint16(buffer, psk_id_len);

    if (ctx->psk_info.ticket != NULL) {
        onetls_buffer_put_uint16(buffer, onetls_buffer_get_tail(ctx->psk_info.ticket));
        onetls_buffer_put_stream(buffer, onetls_buffer_get_data(ctx->psk_info.ticket), onetls_buffer_get_tail(ctx->psk_info.ticket));
        onetls_buffer_put_uint32(buffer, 0);  // TODO: 需要计算，暂时填写0
    }

    if (ctx->psk_info.hint != NULL) {
        onetls_buffer_put_uint16(buffer, onetls_buffer_get_tail(ctx->psk_info.hint));
        onetls_buffer_put_stream(buffer, onetls_buffer_get_data(ctx->psk_info.hint), onetls_buffer_get_tail(ctx->psk_info.hint));
        onetls_buffer_put_uint32(buffer, 0);
    }
    onetls_buffer_put_uint16(buffer, binder_len);

    // binder在后续注入，内容在后面注入，反正这是最后一个项目了
    onetls_buffer_setup_cursor(buffer, binder_len);
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_recv_psk_ex(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint16_t lenth = 0, psk_id = 0;
    onetls_buffer_get_uint16(buffer, &lenth);
    if (lenth != 2) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_FAIL;
    }
    onetls_buffer_get_uint16(buffer, &psk_id);   

    if (psk_id > 1) { // 当前最多也就支持两个。0号是ticket/psk，1号是psk
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_UNKNOWN_PSK_IDENTITY);
        return ONETLS_FAIL;        
    }

    ctx->handshake->secret.select_psk = psk_id;
    if ((ctx->psk_info.ticket != NULL) && (psk_id == 0)) {
        ctx->handshake->secret.select_psk = 1;
    }
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_recv_server_name(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint16_t name_len = 0;
    onetls_buffer_get_uint16(buffer, &name_len);
    if ((name_len == 0) || (name_len > 255)) {
        onetls_check_errlog(ONETLS_FAIL, "onetls_ext_recv_server_name[%d]", name_len);
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_FAIL;   
    }

    onetls_buffer_del(ctx->server_name);
    ctx->server_name = onetls_buffer_new(name_len + 1); // 就怕上面字符串操作
    onetls_assert(ctx->server_name != NULL);

    onetls_buffer_get_stream(buffer, onetls_buffer_get_data(ctx->server_name), name_len);
    onetls_buffer_setup_cursor(ctx->server_name, name_len);
    onetls_buffer_put_uint8(ctx->server_name, 0);
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_send_server_name(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint16_t name_len = strlen(ONETLS_NAME);
    // 放置MT
    onetls_buffer_put_uint16(buffer, onetls_ext_server_name);
    onetls_buffer_put_uint16(buffer, name_len + 5);
    onetls_buffer_put_uint16(buffer, name_len + 3);

    // type, rfc6606上面描述好像当前只有一个枚举就是host_name=0    
    onetls_buffer_put_uint8(buffer, 0);
    // 放置真实的长度
    onetls_buffer_put_uint16(buffer, name_len);
    onetls_buffer_put_stream(buffer, (const uint8_t *)ONETLS_NAME, name_len);
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_send_psk_ex_mode(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint16_t length = 0;
    uint32_t group_num = 0;
    onetls_get_support_groups(&group_num);

    // 放置MT
    onetls_buffer_put_uint16(buffer, onetls_ext_psk_exchange_modes);
    length = (group_num > 0) ? 2 : 1;

    // 放置总长度，因为当前两种模式都支持，psk-only psk-dhe，所以就是定的
    onetls_buffer_put_uint16(buffer, length + 1);
    // 两个模式
    onetls_buffer_put_uint8(buffer, length);
    onetls_buffer_put_uint8(buffer, ONETLS_DH_PSK_ONLY); // psk-only
    if (group_num > 0) {
        onetls_buffer_put_uint8(buffer, ONETLS_DH_PSK_DHE); // psk-dhe
    }
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_send_support_group(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint32_t group_num = 0, offset = 0;
    const onetls_dh_groups *groups = onetls_get_support_groups(&group_num);
    if (group_num == 0) {
        return ONETLS_SUCCESS;
    }    
    // 放置MT
    onetls_buffer_put_uint16(buffer, onetls_ext_supported_groups);
    onetls_buffer_put_uint16(buffer, group_num * 2 + 2);
    onetls_buffer_put_uint16(buffer, group_num * 2);
    for (offset = 0; offset < group_num; offset++) {
        onetls_buffer_put_uint16(buffer, groups[offset].iana_id);
    }
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_send_sign_alg(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint32_t sign_num = 0, offset = 0;
    const onetls_sign_alg *signs = onetls_get_sign_algs(&sign_num);
    if (sign_num == 0) {
        return ONETLS_SUCCESS;
    }    
    // 放置MT
    onetls_buffer_put_uint16(buffer, onetls_ext_signature_algorithms);
    onetls_buffer_put_uint16(buffer, sign_num * 2 + 2);
    onetls_buffer_put_uint16(buffer, sign_num * 2);
    for (offset = 0; offset < sign_num; offset++) {
        onetls_buffer_put_uint16(buffer, signs[offset].iana_id);
    }
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_recv_support_group(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint16_t tlen = 0, dlen = 0, no_use_id = 0;
    uint8_t offset = 0;
    onetls_buffer_get_uint16(buffer, &tlen);
    onetls_buffer_get_uint16(buffer, &dlen);
    if (tlen != (dlen + 2)) {
        onetls_check_errlog(ONETLS_EXT_LEN_WRONG, "onetls_ext_recv_support_group len");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_BAD_RECORD_MAC);
        return ONETLS_EXT_LEN_WRONG;
    }
    for (offset = 0; offset < dlen / 2; offset++) {
        onetls_buffer_get_uint16(buffer, &no_use_id);
        // TODO: 当前无实际使用场景，后续有
    }
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_send_key_share(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint32_t offset = 0, key_share_len = 0, total_len_cursor = 0, group_num = 0;
    onetls_get_support_groups(&group_num);
    if (group_num == 0) {
        return ONETLS_SUCCESS;
    }  
    onetls_assert(ctx->handshake->dh_para.key_share != NULL);
    onetls_buffer_put_uint16(buffer, onetls_ext_key_share);   

    // 总长度 + key_share 长度，预留，等计算完之后再填充
    total_len_cursor = onetls_buffer_get_cursor(buffer);
    onetls_buffer_put_uint32(buffer, 0);

    // 将每一个key_share拷贝到报文
    for (offset = 0; offset < group_num; offset++) {
        if (ctx->handshake->dh_para.server_selected_id != 0) {
            if (ctx->handshake->dh_para.key_share[offset].group->iana_id != ctx->handshake->dh_para.server_selected_id) {
                continue;
            }
        }
        onetls_buffer_put_uint16(buffer, ctx->handshake->dh_para.key_share[offset].group->iana_id);
        onetls_buffer_put_uint16(buffer, ctx->handshake->dh_para.key_share[offset].public_len);
        onetls_buffer_put_stream(buffer, 
                                 ctx->handshake->dh_para.key_share[offset].public_key, 
                                 ctx->handshake->dh_para.key_share[offset].public_len);
    }
    onetls_buffer_set_cursor(buffer, total_len_cursor);
    key_share_len = onetls_buffer_get_tail(buffer) - total_len_cursor - 4;

    // 写总长度
    onetls_buffer_put_uint16(buffer, key_share_len + 2);
    onetls_buffer_put_uint16(buffer, key_share_len);
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_recv_key_share(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint16_t total_length = 0, group_length = 0, group_id = 0;
    uint32_t group_num = 0, ret = 0, offset = 0;
    
    onetls_get_support_groups(&group_num);
    if (group_num == 0) {
        onetls_check_errlog(ONETLS_EXT_NO_NEED, "onetls_ext_recv_key_share onetls_get_support_groups");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_UNEXPECTED_MESSAGE);
        return ONETLS_EXT_NO_NEED;
    }
    
    onetls_buffer_get_uint16(buffer, &total_length);
    if (total_length < 4) {
        onetls_check_errlog(ONETLS_EXT_LEN_WRONG, "onetls_ext_recv_key_share total_length");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_EXT_LEN_WRONG;
    }
    
    // 解析group
    onetls_buffer_get_uint16(buffer, &group_id);  // 可能是一个链表。做简单点，第一个必须命中
    onetls_buffer_get_uint16(buffer, &group_length);
    if ((group_length > ONETLS_DH_PB_KEY_LEN) || (total_length != (group_length + 4))) {
        onetls_check_errlog(ONETLS_EXT_LEN_WRONG, "onetls_ext_recv_key_share group_length");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_EXT_LEN_WRONG;
    }

    ret = ONETLS_NO_KEY_SHARE_MATCH;
    if ((ctx->handshake->dh_para.server_selected_id != 0) && (ctx->handshake->dh_para.server_selected_id != group_id)) {
        onetls_check_errlog(ONETLS_NO_KEY_SHARE_MATCH, "onetls_ext_recv_key_share server_selected_id[%d]", ctx->handshake->dh_para.server_selected_id);
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_NO_KEY_SHARE_MATCH;
    }

    for (offset = 0; offset < group_num; offset++) {
        if (group_id != ctx->handshake->dh_para.key_share[offset].group->iana_id) continue;

        ctx->handshake->dh_para.server_public_len = group_length;
        ctx->handshake->dh_para.server_selected_id = group_id;
        onetls_buffer_get_stream(buffer, ctx->handshake->dh_para.server_public_key, group_length);

        ret = onetls_gen_kex_with_pub_key(ctx, 
                                          &(ctx->handshake->dh_para.key_share[offset]), 
                                          ctx->handshake->dh_para.server_public_key, 
                                          ctx->handshake->dh_para.server_public_len);
        if (ret != ONETLS_SUCCESS) {
            onetls_check_errlog(ONETLS_EXT_LEN_WRONG, "onetls_ext_recv_key_share onetls_gen_kex_dh_key");
            onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
            return ONETLS_EXT_LEN_WRONG;
        }
        break;
    }
    return ret;
}

uint32_t onetls_ext_send_early_data(onetls_ctx *ctx, onetls_buffer *buffer)
{
    if (ctx->early_data.early_buffer == NULL) {
        return ONETLS_SUCCESS;
    }
    // 放置MT
    onetls_buffer_put_uint16(buffer, onetls_ext_early_data);
    onetls_buffer_put_uint16(buffer, 0);
    (ctx->handshake->msg_tag.client_ext_early_data)++;
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_recv_early_data_empty(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint16_t lenth = 0;
    onetls_buffer_get_uint16(buffer, &lenth);
    if (lenth != 0) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_INVALID_PARA;
    }    
    if (ctx->handshake->msg_tag.client_ext_early_data == 0) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_INVALID_PARA;
    }
    (ctx->handshake->msg_tag.server_ext_early_data)++;
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_recv_early_data(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint32_t lenth = 0;
    uint16_t len_v = 0;
    onetls_buffer_get_uint16(buffer, &len_v);
    if (len_v != sizeof(uint32_t)) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        onetls_check_errlog(ONETLS_INVALID_PARA, "onetls_ext_recv_early_data");
        return ONETLS_INVALID_PARA;
    }    
    onetls_buffer_get_uint32(buffer, &lenth);

    ctx->resumption.max_early_data_len_tmp = lenth;
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_send_max_fragment(onetls_ctx *ctx, onetls_buffer *buffer)
{
#if 0    
    uint8_t mfl = onetls_tran_max_fragment_len(ctx);
    if (mfl == ONETLS_MAX_FRAG_LEN_ULIMIT) {
        return ONETLS_SUCCESS;
    }

    // 放置MT
    onetls_buffer_put_uint16(buffer, onetls_ext_max_fragment_length);
    onetls_buffer_put_uint16(buffer, 1);
    onetls_buffer_put_uint8(buffer, mfl);
#endif    
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_recv_max_fragment(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint16_t lenth = 0;
    uint8_t mfl = 0;
    onetls_buffer_get_uint16(buffer, &lenth);
    if (lenth != 1) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_INVALID_PARA;
    } 

    onetls_buffer_get_uint8(buffer, &mfl);
    if (mfl != onetls_tran_max_fragment_len()) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        onetls_check_errlog(ONETLS_INVALID_PARA, "onetls_ext_recv_max_fragment[%d]", mfl);
        return ONETLS_INVALID_PARA;
    }
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_recv_cookie(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint16_t lenth_a = 0, lenth_b = 0;

    if (!(ctx->handshake->msg_tag.server_hello_retry)) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_INVALID_PARA;
    }

    onetls_buffer_get_uint16(buffer, &lenth_a);
    onetls_buffer_get_uint16(buffer, &lenth_b);
    if ((lenth_a != (lenth_b + 2)) || (lenth_b > onetls_buffer_cursor_to_tail(buffer))) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_INVALID_PARA;
    }     

    onetls_buffer_del_safe(&(ctx->handshake->cookie));
    ctx->handshake->cookie = onetls_buffer_new(lenth_b);
    
    onetls_buffer_get_stream(buffer, onetls_buffer_get_data(ctx->handshake->cookie), lenth_b);
    onetls_buffer_setup_cursor(ctx->handshake->cookie, lenth_b);
    return ONETLS_SUCCESS;
}

uint32_t onetls_ext_send_cookie(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint32_t cookie_len = 0;
    if ((!(ctx->handshake->msg_tag.server_hello_retry)) || (ctx->handshake->cookie == NULL)) {
        return ONETLS_SUCCESS;
    }

    cookie_len = onetls_buffer_get_tail(ctx->handshake->cookie);

    onetls_buffer_put_uint16(buffer, onetls_ext_cookie);
    onetls_buffer_put_uint16(buffer, cookie_len + 2);
    onetls_buffer_put_uint16(buffer, cookie_len);
    onetls_buffer_put_stream(buffer, onetls_buffer_get_data(ctx->handshake->cookie), cookie_len);
    return ONETLS_SUCCESS;
}