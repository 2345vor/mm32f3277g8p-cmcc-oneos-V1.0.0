#include "onetls_alert.h"
#include "onetls_lib.h"
#include "onetls_record.h"

static uint32_t onetls_tls_form_hs_alert(onetls_ctx *ctx, onetls_buffer *buffer, uint8_t level, uint8_t desc)
{
    onetls_buffer_put_uint8(buffer, ONETLS_MT_ALERT);
    onetls_buffer_put_uint16(buffer, ONETLS_VERSION_TLS_12);
    onetls_buffer_put_uint16(buffer, 2);
    onetls_buffer_put_uint8(buffer, level);
    onetls_buffer_put_uint8(buffer, desc);
    onetls_buffer_set_cursor(buffer, 0);
    return ONETLS_SUCCESS;
}

static uint32_t onetls_tls_form_traffic_alert(onetls_ctx *ctx, onetls_buffer *buffer, uint8_t level, uint8_t desc)
{
    uint8_t alert_data[] = {level, desc};
    uint32_t ret = onetls_send_encode_data(ctx, buffer, alert_data, sizeof(alert_data), ONETLS_MT_ALERT);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_tls_form_traffic_alert");
        return ret;
    }
    onetls_buffer_set_cursor(buffer, 0);
    return ONETLS_SUCCESS;
}

static uint32_t onetls_tls_send_alert(onetls_ctx *ctx, uint8_t level, uint8_t desc)
{
    uint32_t ret = 0;
    onetls_buffer *buffer = onetls_buffer_new(ONETLS_ADDITIONAL_CIPHER_LEN + 2);
    if (ctx->state == ONETLS_STATE_OK) {
        onetls_tls_form_traffic_alert(ctx, buffer, level, desc);
    } else {
        onetls_tls_form_hs_alert(ctx, buffer, level, desc);
    }

    // 如果是致命的，直接shutdown
    if (level == ONETLS_ALERT_LEVEL_FATAL) {
        ctx->shutdown = desc & ONETLS_ALERT_MASK;
    }

    // 告警优先级最高
    ret = onetls_flush(ctx);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_flush");
        onetls_buffer_del(buffer);
        return ret;
    }

    onetls_send_record(ctx, buffer);
    onetls_buffer_del(buffer);
    return ONETLS_SUCCESS;
}

static uint32_t onetls_dtls_send_alert(onetls_ctx *ctx, uint8_t level, uint8_t desc)
{
    // TODO:
    return ONETLS_FAIL;
}

uint32_t onetls_send_alert(onetls_ctx *ctx, uint8_t level, uint8_t desc)
{
    if (onetls_is_dtls(ctx)) {
        return onetls_dtls_send_alert(ctx, level, desc);
    }
    return onetls_tls_send_alert(ctx, level, desc);
}

uint32_t onetls_recv_alert(onetls_ctx *ctx)
{
    onetls_check_errlog(1, "recv alert level[%d] desc[%d]", ctx->message.alert[0], ctx->message.alert[1]);

    if ((ctx->message.alert[0] == ONETLS_ALERT_LEVEL_FATAL) || (ctx->message.alert[1] == ONETLS_ALERT_DESCRIPTION_CLOSE_NOTIFY)) {
        ctx->shutdown = ctx->message.alert[1] & ONETLS_ALERT_MASK;
        ctx->state = ONETLS_STATE_INIT;
        if (ctx->state == ONETLS_STATE_OK) {
            onetls_shutdown_inner(ctx);
        }
    }
    ctx->message.alert[0] = 0;
    ctx->message.alert[1] = 0;
    return ONETLS_SUCCESS;
}

void onetls_notify_close(onetls_ctx *ctx)
{
    uint32_t ret = onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_WARNING, ONETLS_ALERT_DESCRIPTION_CLOSE_NOTIFY);
    onetls_check_errlog(ret, "onetls_notify_close");
}