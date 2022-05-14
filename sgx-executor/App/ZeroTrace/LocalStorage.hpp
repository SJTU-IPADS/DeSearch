#include "ZT_Globals.hpp"

#include "../Enclave2/Enclave2_u.h"

#include <string>
#include <iostream>
#include <fstream>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>

#pragma once

class LocalStorage {

    public:
    uint32_t Z;
    // TODO: Probably should switch D out for D_level version?
    uint32_t D;
    uint8_t recursion_levels;
    bool inmem;
    uint64_t gN;

    unsigned char ** inmem_tree_l;
    unsigned char ** inmem_hash_l;

    uint64_t * blocks_in_level;
    uint64_t * buckets_in_level;
    uint64_t * real_max_blocks_level;
    uint32_t * D_level;

    uint32_t bucket_size;

    uint32_t data_block_size;
    uint32_t recursion_block_size;

    // Variables for Hybrid Storage mechanism
    uint32_t levels_on_disk = 0;
    uint32_t objectkeylimit;

    LocalStorage();
    LocalStorage(LocalStorage & ls);

    void setParams(uint32_t max_blocks, uint32_t D, uint32_t Z, uint32_t stash_size, uint32_t data_size, bool inmem, uint32_t recursion_block_size, uint8_t recursion_levels);

    void fetchHash(uint32_t bucket_id, unsigned char * hash_buffer, uint32_t hash_size, uint8_t level);

    // downloadBucket() is never used, as we typically operate with Paths
    // uploadBucket() on the other hand is used, for initializing the ORAM tree, a bucket at a time.
    // (So that we can initialize without having to maintain the entire ORAM tree in PRM space.)
    uint8_t uploadBucket(uint32_t bucket_id, unsigned char * serialized_bucket, uint32_t bucket_size, unsigned char * hash, uint32_t hash_size, uint8_t level);
    uint8_t downloadBucket(uint32_t bucket_id, unsigned char * bucket, uint32_t bucket_size, unsigned char * hash, uint32_t hash_size, uint8_t level);

    uint8_t uploadPath(uint32_t leaf_label, unsigned char * path, unsigned char * path_hash, uint8_t level, uint32_t D);
    unsigned char * downloadPath(uint32_t leaf_label, unsigned char * path, unsigned char * path_hash, uint32_t path_hash_size, uint8_t level, uint32_t D);

    void showPath_reverse(unsigned char * decrypted_path, uint8_t Z, uint32_t d, uint32_t data_size);

    void deleteObject();
    void copyObject();
};
