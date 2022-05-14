#include "sgx_eid.h"
#include "Enclave1_t.h"

#include "sgx_trts.h"
#include "sgx_report.h"
#include "sgx_utils.h"

#include <sgx_tcrypto.h>
#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */
#include <cstring>

// for debug only
void printf(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}

void none(void)
{
    ;//
}

int retrieve_mr_enclave(uint8_t sha_exe[65])
{
    sgx_status_t err = SGX_ERROR_UNEXPECTED;
    sgx_attributes_t attribute_mask;
    uint16_t key_policy = SGX_KEYPOLICY_MRSIGNER;

    const sgx_report_t * report = sgx_self_report();

    printf("\n\tmr_enclave: ");
    for (int i = 0; i < sizeof(sgx_measurement_t); i++) {
          printf("%02x", report-> body.mr_enclave.m[i]);
    }
    printf("\n\n");

    memcpy(sha_exe, report-> body.mr_enclave.m, sizeof(sgx_measurement_t));

    return 0;
}

void sgx_sha256_string(const uint8_t *hash, char outputBuffer[65])
{
    for(int i = 0; i < SGX_SHA256_HASH_SIZE; i++)
    {
        snprintf(outputBuffer + (i * 2), 4, "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
}
