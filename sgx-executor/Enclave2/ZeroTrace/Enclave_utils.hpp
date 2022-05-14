#ifndef __ZT_ENCLAVE_UTILS__

#include "Globals_Enclave.hpp"

#include "oasm_lib.h"

void oarray_search(uint32_t * array, uint32_t loc, uint32_t * leaf, uint32_t newLabel, uint32_t N_level);

void displayKey(unsigned char * key, uint32_t key_size);

void enclave_sha256(char * string, uint32_t str_len);

#define __ZT_ENCLAVE_UTILS__

#endif
