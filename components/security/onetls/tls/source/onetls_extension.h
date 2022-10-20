#ifndef __ONETLS_EXTENSION_H__
#define __ONETLS_EXTENSION_H__
#include "onetls_client.h"
#include "onetls_buffer.h"

typedef enum {
    onetls_ext_server_name = (0),                                         /* RFC 6066 */
    onetls_ext_max_fragment_length = (1),                                 /* RFC 6066 */
    onetls_ext_status_request = (5),                                      /* RFC 6066 */
    onetls_ext_supported_groups = (10),                                   /* RFC 8422, 7919 */
    onetls_ext_signature_algorithms = (13),                               /* RFC 8446 */
    onetls_ext_use_srtp = (14),                                           /* RFC 5764 */
    onetls_ext_heartbeat = (15),                                          /* RFC 6520 */
    onetls_ext_application_layer_protocol_negotiation = (16),             /* RFC 7301 */
    onetls_ext_signed_certificate_timestamp = (18),                       /* RFC 6962 */
    onetls_ext_client_certificate_type = (19),                            /* RFC 7250 */
    onetls_ext_server_certificate_type = (20),                            /* RFC 7250 */
    onetls_ext_padding = (21),                                            /* RFC 7685 */
    onetls_ext_session_ticket = (35),                                     /* RFC 5077 */
    onetls_ext_pre_shared_key = (41),                                     /* RFC 8446 */
    onetls_ext_early_data = (42),                                         /* RFC 8446 */
    onetls_ext_supported_versions = (43),                                 /* RFC 8446 */
    onetls_ext_cookie = (44),                                             /* RFC 8446 */
    onetls_ext_psk_exchange_modes = (45),                                 /* RFC 8446 */
    onetls_ext_certificate_authorities = (47),                            /* RFC 8446 */
    onetls_ext_oid_filters = (48),                                        /* RFC 8446 */
    onetls_ext_post_handshake_auth = (49),                                /* RFC 8446 */
    onetls_ext_signature_algorithms_cert = (50),                          /* RFC 8446 */
    onetls_ext_key_share = (51),                                          /* RFC 8446 */
} onetls_ext_extension_type;

typedef uint32_t(*onetls_extension_func)(onetls_ctx *ctx, onetls_buffer *buffer);
typedef struct {
    uint16_t    iana_id;
    onetls_extension_func func;
} onetls_extension;

uint32_t onetls_construct_client_hello_extension(onetls_ctx *ctx);
uint32_t onetls_tls_parse_server_extension(onetls_ctx *ctx, onetls_buffer *buffer, uint8_t state);
uint32_t onetls_ext_send_support_version(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_recv_support_version(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_send_psk_ex(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_recv_psk_ex(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_send_psk_ex_mode(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_recv_server_name(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_send_server_name(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_send_support_group(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_recv_support_group(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_send_key_share(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_recv_key_share(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_send_early_data(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_recv_early_data(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_recv_early_data_empty(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_send_session_tickets(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_send_sign_alg(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_send_max_fragment(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_recv_max_fragment(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_recv_cookie(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_ext_send_cookie(onetls_ctx *ctx, onetls_buffer *buffer);

#endif
