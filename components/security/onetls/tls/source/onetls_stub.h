#ifndef __ONETLS_STUB_H__
#define __ONETLS_STUB_H__
#include "onetls_client.h"

#ifdef ONETLS_USING_PSA
    int onetls_psa_entropy_stub(void *data, uint8_t *output, size_t len, size_t *out_len);
#endif

#endif
