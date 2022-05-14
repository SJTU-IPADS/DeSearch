#ifndef __ZT_GLOBALS_ENCLAVE__
#define __ZT_GLOBALS_ENCLAVE__

#include "ZT_Globals.hpp"

#include "ORAM_Interface.hpp"

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <sgx_tcrypto.h>
#include "sgx_trts.h"
#include "../Enclave2_t.h"  /* print_string */

struct oram_request {
    uint32_t * id;
    uint32_t * level;
    uint32_t * d_lev;
    bool * recursion;
    bool * block;
};

struct oram_response {
    unsigned char * path;
    unsigned char * path_hash;
    unsigned char * new_path;
    unsigned char * new_path_hash;
};

struct nodev2 {
    unsigned char * serialized_block;
    struct nodev2 * next;
};

//TODO : Do we need this ?
struct request_parameters {
    char opType;
    uint32_t id;
    uint32_t position_in_id;
    uint32_t level;
};

void printf(const char * fmt, ...);

void displaySerializedBlock(unsigned char * serialized_result_block, uint32_t level, uint32_t recursion_levels, uint32_t x);

void aes_dec_serialized(unsigned char * encrypted_block, uint32_t data_size, unsigned char * decrypted_block, unsigned char * aes_key);
void aes_enc_serialized(unsigned char * decrypted_block, uint32_t data_size, unsigned char * encrypted_block, unsigned char * aes_key);
#endif
