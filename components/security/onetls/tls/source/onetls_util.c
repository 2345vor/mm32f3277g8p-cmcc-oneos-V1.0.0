#include <time.h>

#include "onetls_util.h"
#include "onetls_lib.h"

uint32_t onetls_get_time()
{   
    return (uint32_t)time(0);
}

void onetls_seq_num_add(uint8_t *seq, uint32_t seq_size)
{
    uint8_t loop = 0;
    for (loop = seq_size; loop > 0; loop--) {
        seq[loop - 1] ++;
        if (seq[loop - 1] != 0) {
            break;
        }
    }
}

void onetls_seq_num_reset(uint8_t *seq, uint32_t seq_size)
{
    memset(seq, 0, seq_size);
}

uint64_t onetls_trans_seq_num(uint8_t *seq, uint32_t seq_size)
{
    uint64_t tmp = 0;
    uint32_t offset = seq_size;
    while (offset > 0) {
        tmp <<= 8;
        tmp += seq[seq_size - offset];
        offset --;
    }
    return tmp;
}

uint32_t onetls_get_random(onetls_ctx *ctx)
{
    uint32_t t = onetls_get_time();
    ctx->handshake->random_c[0] = (t & 0xff000000) >> 24;
    ctx->handshake->random_c[1] = (t & 0x00ff0000) >> 16;
    ctx->handshake->random_c[2] = (t & 0x0000ff00) >> 8;
    ctx->handshake->random_c[3] = (t & 0x000000ff);
    onetls_get_rnd(ctx->handshake->random_c + 4, 28);
    return ONETLS_SUCCESS;
}

uint8_t onetls_tran_max_fragment_len()
{
    uint32_t step_len = ONETLS_MAX_PACKET_LEN / 512;
    if (step_len <= 1) {
        return ONETLS_MAX_FRAG_LEN_512;
    }
    if (step_len <= 2) {
        return ONETLS_MAX_FRAG_LEN_1024;
    }
    if (step_len <= 4) {
        return ONETLS_MAX_FRAG_LEN_2048;
    }
    if (step_len <= 8) {
        return ONETLS_MAX_FRAG_LEN_4096;
    }
    return ONETLS_MAX_FRAG_LEN_ULIMIT;
}

uint32_t onetls_check_data_empty(const uint8_t *data, uint32_t len)
{
    uint32_t offset = 0;
    for (offset = 0; offset < len; offset++) {
        if (data[offset] != 0) {
            return 0;
        }
    }
    return 1;
}