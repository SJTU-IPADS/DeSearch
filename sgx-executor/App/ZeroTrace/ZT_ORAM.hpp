#include "../Enclave2/Enclave2_u.h"

#include "ZT_Globals.hpp"
#include "LocalStorage.hpp"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include <pwd.h>
#include <map>
#include <vector>

using namespace std;

#define MAX_PATH FILENAME_MAX

#include <sgx_urts.h>

#define CIRCUIT_ORAM
#define MILLION 1E6
#define HASH_LENGTH 32

// Global Variables Declarations
uint32_t aes_key_size = 16;
uint32_t hash_size = 32;
#define ADDITIONAL_METADATA_SIZE 24
uint32_t oram_id = 0;

// Timing variables
uint32_t recursion_levels_e = 0;

/* Global EID shared by multiple threads */
bool resume_experiment = false;
bool inmem_flag = true;

// Storage Backends:
// TODO: Switch to LS for each LSORAM, Path, Circuit
LocalStorage ls;
std::map < uint32_t, LocalStorage * > ls_PORAM;
std::map < uint32_t, LocalStorage * > ls_CORAM;

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

struct thread_data {
    struct oram_request * req;
    struct oram_response * resp;
};

struct thread_data td;
struct oram_request req_struct;
struct oram_response resp_struct;
unsigned char * data_in;
unsigned char * data_out;

uint8_t uploadPath_OCALL(uint32_t instance_id, uint8_t oram_type, unsigned char * path_array, uint32_t path_size, uint32_t leaf_label, unsigned char * path_hash, uint32_t path_hash_size, uint8_t level, uint32_t D_level) {

    LocalStorage * ls;
    auto search = ls_CORAM.find(instance_id);
    if (search != ls_CORAM.end()) {
        ls = search -> second;
    }
    ls -> uploadPath(leaf_label, path_array, path_hash, level, D_level);
    return 1;
}

uint8_t downloadPath_OCALL(uint32_t instance_id, uint8_t oram_type, unsigned char * path_array, uint32_t path_size, uint32_t leaf_label, unsigned char * path_hash, uint32_t path_hash_size, uint8_t level, uint32_t D_level) {

    LocalStorage * ls;
    auto search = ls_CORAM.find(instance_id);
    if (search != ls_CORAM.end()) {
        ls = search -> second;
    }
    ls -> downloadPath(leaf_label, path_array, path_hash, path_hash_size, level, D_level);
    return 1;
}

uint8_t uploadBucket_OCALL(uint32_t instance_id, uint8_t oram_type, unsigned char * serialized_bucket, uint32_t bucket_size, uint32_t label, unsigned char * hash, uint32_t hashsize, uint32_t size_for_level, uint8_t recursion_level) {

    LocalStorage * ls;
    auto search = ls_CORAM.find(instance_id);
    if (search != ls_CORAM.end()) {
        ls = search -> second;
    }
    ls -> uploadBucket(label, serialized_bucket, size_for_level, hash, hashsize, recursion_level);
    return 1;
}

uint8_t downloadBucket_OCALL(uint32_t instance_id, uint8_t oram_type, unsigned char * serialized_bucket, uint32_t bucket_size, uint32_t label, unsigned char * hash, uint32_t hashsize, uint32_t size_for_level, uint8_t recursion_level) {

    LocalStorage * ls;
    auto search = ls_CORAM.find(instance_id);
    if (search != ls_CORAM.end()) {
        ls = search -> second;
    }
    ls -> downloadBucket(label, serialized_bucket, size_for_level, hash, hashsize, recursion_level);
    return 1;
}

void build_fetchChildHash(uint32_t instance_id, uint8_t oram_type, uint32_t left, uint32_t right, unsigned char * lchild, unsigned char * rchild, uint32_t hash_size, uint32_t recursion_level) {

    LocalStorage * ls;
    auto search = ls_CORAM.find(instance_id);
    if (search != ls_CORAM.end()) {
        ls = search -> second;
    }
    ls -> fetchHash(left, lchild, hash_size, recursion_level);
    ls -> fetchHash(right, rchild, hash_size, recursion_level);
}

uint8_t computeRecursionLevels(uint32_t max_blocks, uint32_t recursion_data_size, uint64_t onchip_posmap_memory_limit) {
    uint8_t recursion_levels = 1;
    uint8_t x;

    if (recursion_data_size != 0) {
        recursion_levels = 1;
        x = recursion_data_size / sizeof(uint32_t);
        uint64_t size_pmap0 = max_blocks * sizeof(uint32_t);
        uint64_t cur_pmap0_blocks = max_blocks;

        while (size_pmap0 > onchip_posmap_memory_limit) {
            cur_pmap0_blocks = (uint64_t) ceil((double) cur_pmap0_blocks / (double) x);
            recursion_levels++;
            size_pmap0 = cur_pmap0_blocks * sizeof(uint32_t);
        }

        #ifdef RECURSION_LEVELS_DEBUG
        printf("IN App: max_blocks = %d\n", max_blocks);
        printf("Recursion Levels : %d\n", recursion_levels);
        #endif
    }
    return recursion_levels;
}

uint32_t ZT_New(uint32_t max_blocks, uint32_t data_size, uint32_t stash_size, uint32_t oblivious_flag, uint32_t recursion_data_size, uint32_t oram_type, uint8_t pZ) {
    sgx_status_t sgx_return = SGX_SUCCESS;
    int8_t rt;
    uint8_t urt;
    uint32_t instance_id;
    int8_t recursion_levels;
    LocalStorage * ls_oram = new LocalStorage();

    // RecursionLevels is really number of levels of ORAM
    // So if no recursion, recursion_levels = 1 
    recursion_levels = computeRecursionLevels(max_blocks, recursion_data_size, MEM_POSMAP_LIMIT);

    uint32_t D = (uint32_t) ceil(log((double) max_blocks / pZ) / log((double) 2));
    printf("Parmas for LS : (%d, %d, %d, %d, %d, %d, %d, %d)\n",
        max_blocks, D, pZ, stash_size, data_size + ADDITIONAL_METADATA_SIZE, inmem_flag, recursion_data_size + ADDITIONAL_METADATA_SIZE, recursion_levels);

    // LocalStorage Module, just works with recursion_levels 0 to recursion_levels 
    // And functions without treating recursive and non-recursive backends differently
    // Hence recursion_levels passed = recursion_levels,

    ls_oram -> setParams(max_blocks, D, pZ, stash_size, data_size + ADDITIONAL_METADATA_SIZE, inmem_flag, recursion_data_size + ADDITIONAL_METADATA_SIZE, recursion_levels);

    sgx_return = Enclave2_getNewORAMInstanceID(e2_enclave_id, & instance_id, oram_type);

    ls_CORAM.insert(std::make_pair(instance_id, ls_oram));
    printf("Inserted instance_id = %d into ls_CORAM\n", instance_id);

    uint8_t ret;
    sgx_return = Enclave2_createNewORAMInstance(e2_enclave_id, & ret, instance_id, max_blocks, data_size, stash_size, oblivious_flag, recursion_data_size, recursion_levels, oram_type, pZ);

    #ifdef DEBUG_PRINT
    printf("initialize_oram Successful\n");
    #endif
    return (instance_id);
}

void ZT_Access(uint32_t instance_id, uint8_t oram_type, unsigned char * request, unsigned char * response, uint32_t request_size, uint32_t response_size) {

    Enclave2_accessInterface(e2_enclave_id, instance_id, oram_type, request, response, request_size, response_size);

}

void ZT_Bulk_Read(uint32_t instance_id, uint8_t oram_type, uint32_t no_of_requests, unsigned char * request, unsigned char * response, uint32_t request_size, uint32_t response_size) {

    Enclave2_accessBulkReadInterface(e2_enclave_id, instance_id, oram_type, no_of_requests, request, response, request_size, response_size);

}
