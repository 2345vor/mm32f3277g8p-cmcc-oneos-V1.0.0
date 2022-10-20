#include "onetls_stub.h"
#include <stdlib.h>

#ifdef ONETLS_USING_PSA
    int onetls_psa_entropy_stub(void *data, uint8_t *output, size_t len, size_t *out_len)
    { 
        *out_len = 0;
        while ((*out_len) < len) {
            // 修改psa的熵源, 用伪随机
            output[(*out_len)++] = rand() & 0xff;
        }
        return ONETLS_SUCCESS;
    }
#endif
