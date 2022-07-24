#include "ORAMTree.hpp"

ORAMTree::ORAMTree()
{
    aes_key = (unsigned char*)malloc(KEY_LENGTH);
    sgx_read_rand(aes_key, KEY_LENGTH);
}

void ORAMTree::SampleKey()
{
    printf("\n\nORAMTREE SAMPLEKEY WAS CALLED\n\n");
    aes_key = (unsigned char*)malloc(KEY_LENGTH);
    sgx_read_rand(aes_key, KEY_LENGTH);
}

ORAMTree::~ORAMTree()
{
    free(aes_key);
}

void ORAMTree::print_pmap0()
{
    uint32_t p = 0;
    printf("Pmap0 = \n");
    while (p < real_max_blocks_level[0]) {
        printf("(%d,%d)\n", p, posmap[p]);
        p++;
    }
    printf("\n");
}

void ORAMTree::verifyPath(unsigned char* path_array, unsigned char* path_hash, uint32_t leaf, uint32_t D, uint32_t block_size, uint8_t level)
{
    unsigned char* path_array_iter = path_array;
    unsigned char* path_hash_iter = path_hash;
    sgx_sha256_hash_t parent_hash;
    sgx_sha256_hash_t child;
    sgx_sha256_hash_t lchild;
    sgx_sha256_hash_t rchild;
    sgx_sha256_hash_t lchild_retrieved;
    sgx_sha256_hash_t rchild_retrieved;
    sgx_sha256_hash_t parent_hash_retrieved;
    uint32_t temp = leaf;
    uint32_t cmp1, cmp2, cmp;
    int32_t i;
    // uint32_t D = (uint32_t) ceil(log((double)max_blocks)/log((double)2));

    for (i = D - 1; i >= 0; i--) {
        if (i == (D - 1)) {
            // No child hashes to compute
            sgx_sha256_msg(path_array_iter, (block_size * Z), (sgx_sha256_hash_t*)child);
            path_array_iter += (block_size * Z);
            memcpy((uint8_t*)lchild_retrieved, path_hash_iter, HASH_LENGTH);
            path_hash_iter += HASH_LENGTH;
            memcpy((uint8_t*)rchild_retrieved, path_hash_iter, HASH_LENGTH);
            path_hash_iter += HASH_LENGTH;

            if (temp % 2 == 0) {
                cmp1 = memcmp((uint8_t*)child, (uint8_t*)lchild_retrieved, HASH_LENGTH);
            } else {
                cmp1 = memcmp((uint8_t*)child, (uint8_t*)rchild_retrieved, HASH_LENGTH);
            }

#ifdef DEBUG_INTEGRITY
            if (cmp1 == 0) {
                printf("Verification Successful at leaf\n");
            } else {
                printf("\n Computed child hash value : ");
                for (uint8_t l = 0; l < HASH_LENGTH; l++)
                    printf("%c", (child[l] % 26) + 'A');

                printf("\n Lchild_retrieved :");
                for (uint8_t l = 0; l < HASH_LENGTH; l++)
                    printf("%c", (lchild_retrieved[l] % 26) + 'A');

                printf("\n Rchild_retrieved :");
                for (uint8_t l = 0; l < HASH_LENGTH; l++)
                    printf("%c", (rchild_retrieved[l] % 26) + 'A');

                printf("\nFAIL_1:%d", cmp1);
                printf("\n");
            }
#endif
        } else if (i == 0) {
            //No sibling child
            sgx_sha_state_handle_t p_sha_handle;
            sgx_sha256_init(&p_sha_handle);
            sgx_sha256_update(path_array_iter, (block_size * Z), p_sha_handle);
            path_array_iter += (block_size * Z);
            sgx_sha256_update((uint8_t*)lchild_retrieved, SGX_SHA256_HASH_SIZE, p_sha_handle);
            sgx_sha256_update((uint8_t*)rchild_retrieved, SGX_SHA256_HASH_SIZE, p_sha_handle);
            sgx_sha256_get_hash(p_sha_handle, (sgx_sha256_hash_t*)parent_hash);
            sgx_sha256_close(p_sha_handle);

            //Fetch retreived root hash :
            memcpy((uint8_t*)parent_hash_retrieved, path_hash_iter, HASH_LENGTH);
            path_hash_iter += HASH_LENGTH;
            // Test if retrieved merkle_root_hash of tree matches internally stored merkle_root_hash
            // If retrieved matches internal, then computed merkle_root_hash should match as well for free.

            cmp = memcmp((uint8_t*)parent_hash_retrieved, (uint8_t*)merkle_root_hash_level[level], HASH_LENGTH);

#ifdef DEBUG_INTEGRITY
            if (cmp != 0) {
                printf("\nROOT_COMPUTED:");
                for (uint8_t l = 0; l < HASH_LENGTH; l++)
                    printf("%c", (parent_hash[l] % 26) + 'A');

                printf("\nROOT_RETRIEVED:");
                for (uint8_t l = 0; l < HASH_LENGTH; l++)
                    printf("%c", (parent_hash_retrieved[l] % 26) + 'A');

                printf("\nROOT_MERKLE_LOCAL:");
                for (uint8_t l = 0; l < HASH_LENGTH; l++) {
                    printf("%c", (merkle_root_hash_level[level][l] % 26) + 'A');
                }
                printf("\nFAIL_ROOT:%d\n", cmp);
            } else {
                printf("\nVERI-SUCCESS!\n");

                printf("\nROOT_COMPUTED:");
                for (uint8_t l = 0; l < HASH_LENGTH; l++)
                    printf("%c", (parent_hash[l] % 26) + 'A');

                printf("\nROOT_RETRIEVED:");
                for (uint8_t l = 0; l < HASH_LENGTH; l++)
                    printf("%c", (parent_hash_retrieved[l] % 26) + 'A');

                printf("\nROOT_MERKLE_LOCAL:");
                for (uint8_t l = 0; l < HASH_LENGTH; l++)
                    printf("%c", (merkle_root_hash_level[level][l] % 26) + 'A');

                printf("\n");
            }
#endif
        } else {
            sgx_sha_state_handle_t p_sha_handle;
            sgx_sha256_init(&p_sha_handle);
            sgx_sha256_update(path_array_iter, (block_size * Z), p_sha_handle);
            path_array_iter += (block_size * Z);
            sgx_sha256_update((uint8_t*)lchild_retrieved, SGX_SHA256_HASH_SIZE, p_sha_handle);
            sgx_sha256_update((uint8_t*)rchild_retrieved, SGX_SHA256_HASH_SIZE, p_sha_handle);
            sgx_sha256_get_hash(p_sha_handle, (sgx_sha256_hash_t*)parent_hash);
            sgx_sha256_close(p_sha_handle);

            // Children hashes for next round
            memcpy((uint8_t*)lchild_retrieved, path_hash_iter, HASH_LENGTH);
            path_hash_iter += HASH_LENGTH;
            memcpy((uint8_t*)rchild_retrieved, path_hash_iter, HASH_LENGTH);
            path_hash_iter += HASH_LENGTH;

#ifdef DEBUG_INTEGRITY
            printf("\nlchild_of_next_round=");
            for (uint8_t l = 0; l < HASH_LENGTH; l++) {
                printf("%c", (lchild_retrieved[l] % 26) + 'A');
            }

            printf("\nrchild_of_next_round=");
            for (uint8_t l = 0; l < HASH_LENGTH; l++) {
                printf("%c", (rchild_retrieved[l] % 26) + 'A');
            }

            printf("\ncomputed_parent:");
            for (uint8_t l = 0; l < HASH_LENGTH; l++)
                printf("%c", (parent_hash[l] % 26) + 'A');
            printf("\n");
#endif

            if (temp % 2 == 0)
                cmp = memcmp((uint8_t*)lchild_retrieved, (uint8_t*)parent_hash, HASH_LENGTH);
            else
                cmp = memcmp((uint8_t*)rchild_retrieved, (uint8_t*)parent_hash, HASH_LENGTH);
        }

        temp = temp >> 1;
    }
}

void ORAMTree::decryptPath(unsigned char* path_array, unsigned char* decrypted_path_array, uint32_t num_of_blocks_on_path, uint32_t data_size)
{
    unsigned char* path_iter = path_array;
    unsigned char* decrypted_path_iter = decrypted_path_array;

    for (uint32_t i = 0; i < num_of_blocks_on_path; i++) {
#ifdef ENCRYPTION_ON
        aes_dec_serialized(path_iter, data_size, decrypted_path_iter, aes_key);
#endif

        path_iter += (data_size + ADDITIONAL_METADATA_SIZE);
        decrypted_path_iter += (data_size + ADDITIONAL_METADATA_SIZE);
    }
}

void ORAMTree::encryptPath(unsigned char* path_array, unsigned char* encrypted_path_array, uint32_t num_of_blocks_on_path, uint32_t data_size)
{
    unsigned char* path_iter = path_array;
    unsigned char* encrypted_path_iter = encrypted_path_array;

    for (uint32_t i = 0; i < num_of_blocks_on_path; i++) {
#ifdef ENCRYPTION_ON
#ifdef AEI
//block.cwf_aes_ebc_dec(extblock_size-24);
#else
        aes_enc_serialized(path_iter, data_size, encrypted_path_iter, aes_key);
#endif
#endif
        path_iter += (data_size + ADDITIONAL_METADATA_SIZE);
        encrypted_path_iter += (data_size + ADDITIONAL_METADATA_SIZE);
    }
}

/*
  ORAMTree::BuildTreeLevel
  
  The ORAMTree at recursion_level = level  has to store 
  max_blocks_level[level] blocks in it.
   
  util_divisor = Z is a parameter that determines how packed
  the leaf nodes are at initialization, and effectively determines
  the height of the tree instantiated.
  
  The tree has ceil(max_blocks_level[level]/util_divisor) leaf nodes.
*/

uint32_t* ORAMTree::BuildTreeLevel(uint8_t level, uint32_t* prev_pmap)
{
    uint32_t tdata_size;
    uint32_t block_size;
    sgx_sha256_hash_t current_bucket_hash;

    uint32_t util_divisor = Z;
    uint32_t pD_temp = ceil((double)max_blocks_level[level] / (double)util_divisor);
    uint32_t pD = (uint32_t)ceil(log((double)pD_temp) / log((double)2));
    uint32_t pN = (int)pow((double)2, (double)pD);
    uint32_t ptreeSize = 2 * pN - 1;
    //+1 to depth pD, since ptreeSize = 2 *pN
    D_level[level] = pD + 1;
    N_level[level] = pN;
    printf("In ORAMTree:BuildTreeLevel N_level[%d]=%d\n", level, N_level[level]);

#ifdef BUILDTREE_DEBUG
    printf("\n\nBuildTreeLevel,\nLevel : %d, Params - D = %d, N = %d, treeSize = %d, x = %d\n", level, pD, pN, ptreeSize, x);
#endif

    if (level == (recursion_levels - 1) || level == 0) {
        tdata_size = data_size;
        block_size = (data_size + ADDITIONAL_METADATA_SIZE);
    } else {
        tdata_size = recursion_data_size;
        block_size = recursion_data_size + ADDITIONAL_METADATA_SIZE;
    }

    uint32_t* posmap_l = (uint32_t*)malloc(max_blocks_level[level] * sizeof(uint32_t));
    if (posmap_l == NULL) {
        printf("Failed to allocate\n");
    }

    uint32_t hashsize = HASH_LENGTH;
    unsigned char* hash_lchild = (unsigned char*)malloc(HASH_LENGTH);
    unsigned char* hash_rchild = (unsigned char*)malloc(HASH_LENGTH);
    uint32_t blocks_per_bucket_in_ll = real_max_blocks_level[level] / pN;

#ifdef BUILDTREE_DEBUG
    printf("Posmap Size = %f MB\n", float(max_blocks_level[level] * sizeof(uint32_t)) / (float(1024 * 1024)));
    for (uint8_t jk = 0; jk < recursion_levels; jk++) {
        printf("real_max_blocks_level[%d] = %d \n", jk, real_max_blocks_level[jk]);
    }
    printf("\n");
    printf("pN = %d, level = %d, real_max_blocks_level[level] = %d, blocks_per_bucket_in_ll = %d\n", pN, level, real_max_blocks_level[level], blocks_per_bucket_in_ll);
#endif

    uint32_t c = real_max_blocks_level[level] - (blocks_per_bucket_in_ll * pN);
    uint32_t cnt = 0;

    Bucket temp(Z);
    temp.initialize(tdata_size, gN);

    //Build Last Level of Tree
    uint32_t label = 0;
    for (uint32_t i = pN; i <= ptreeSize; i++) {

        temp.reset_blocks(tdata_size, gN);

        uint32_t blocks_in_this_bucket = blocks_per_bucket_in_ll;
        if (cnt < c) {
            blocks_in_this_bucket += 1;
            cnt += 1;
        }

#ifdef BUILDTREE_DEBUG
        printf("Bucket : %d\n", i);
#endif

        for (uint8_t q = 0; q < blocks_in_this_bucket; q++) {
            temp.blocks[q].id = label;
            //treeLabel will be the bucket_id of that leaf = nlevel[level] + leaf
            temp.blocks[q].treeLabel = (pN) + (i - pN);

            if (level < recursion_levels - 1 && level > 0) {
                temp.blocks[q].fill_recursion_data(&(prev_pmap[(label)*x]), recursion_data_size);
            }
            posmap_l[temp.blocks[q].id] = temp.blocks[q].treeLabel;
            label++;
        }

#ifdef BUILDTREE_DEBUG
        for (uint8_t q = 0; q < Z; q++) {
            if (level < recursion_levels - 1 && level > 0) {
                printf("(%d, %d): ", temp.blocks[q].id, temp.blocks[q].treeLabel);
                for (uint8_t p = 0; p < x; p++) {
                    printf("%d,", (prev_pmap[(label * x) + p]));
                }
                printf("\n");
            } else {
                printf("(%d, %d): ", temp.blocks[q].id, temp.blocks[q].treeLabel);
                unsigned char* ptr = temp.blocks[q].getDataPtr();
                for (uint64_t p = 0; p < data_size; p++) {
                    printf("%c", ptr[p]);
                }
                printf("\n");
            }
        }
        printf("\n");
#endif

#ifdef ENCRYPTION_ON
        temp.aes_encryptBlocks(tdata_size, aes_key);
#endif

        //TODO: No need to create and release memory for serialized bucket again and again
        unsigned char* serialized_bucket = temp.serialize(tdata_size);
        uint8_t ret;

        //Hash / Integrity Tree
        sgx_sha256_msg(serialized_bucket, block_size * Z, (sgx_sha256_hash_t*)&(current_bucket_hash));

        //Upload Bucket
        uploadBucket_OCALL(&ret, instance_id, oram_type, serialized_bucket, Z * block_size, i, (unsigned char*)&(current_bucket_hash), HASH_LENGTH, block_size, level);

#ifdef BUILDTREE_VERIFICATION_DEBUG
        printf("Level = %d, Bucket no = %d, Hash = ", level, i);
        for (uint8_t l = 0; l < HASH_LENGTH; l++)
            printf("%c", (current_bucket_hash[l] % 26) + 'A');
        printf("\n");
#endif

        free(serialized_bucket);
    }

    //Build Upper Levels of Tree
    for (uint32_t i = pN - 1; i >= 1; i--) {
        temp.reset_blocks(tdata_size, gN);

#ifdef BUILDTREE_DEBUG
        for (uint8_t q = 0; q < Z; q++) {
            if (level < recursion_levels - 1 && level > 0) {
                printf("(%d, %d): ", temp.blocks[q].id, temp.blocks[q].treeLabel);
                for (uint8_t p = 0; p < x; p++) {
                    printf("%d,", (prev_pmap[(label * x) + p]));
                }
                printf("\n");
            } else {
                printf("(%d, %d): ", temp.blocks[q].id, temp.blocks[q].treeLabel);
                unsigned char* ptr = temp.blocks[q].getDataPtr();
                for (uint64_t p = 0; p < data_size; p++) {
                    printf("%c", ptr[p]);
                }
                printf("\n");
            }
        }
#endif

#ifdef ENCRYPTION_ON
        temp.aes_encryptBlocks(tdata_size, aes_key);
#endif

        unsigned char* serialized_bucket = temp.serialize(tdata_size);
        uint8_t ret;

        //Hash
        build_fetchChildHash(instance_id, oram_type, i * 2, i * 2 + 1, hash_lchild, hash_rchild, HASH_LENGTH, level);
        sgx_sha_state_handle_t p_sha_handle;
        sgx_sha256_init(&p_sha_handle);
        sgx_sha256_update(serialized_bucket, block_size * Z, p_sha_handle);
        sgx_sha256_update(hash_lchild, SGX_SHA256_HASH_SIZE, p_sha_handle);
        sgx_sha256_update(hash_rchild, SGX_SHA256_HASH_SIZE, p_sha_handle);
        sgx_sha256_get_hash(p_sha_handle, (sgx_sha256_hash_t*)merkle_root_hash_level[level]);
        sgx_sha256_close(p_sha_handle);

        //Upload Bucket
        uploadBucket_OCALL(&ret, instance_id, oram_type, serialized_bucket, Z * block_size, i, (unsigned char*)&(merkle_root_hash_level[level]), HASH_LENGTH, block_size, level);

#ifdef BUILDTREE_VERIFICATION_DEBUG
        printf("Level = %d, Bucket no = %d, Hash = ", level, i);
        for (uint8_t l = 0; l < HASH_LENGTH; l++)
            printf("%c", (merkle_root_hash_level[level][l] % 26) + 'A');
        printf("\n");
#endif

        free(serialized_bucket);
    }

#ifdef BUILDTREE_DEBUG
    //TEST MODULE: Retrieve buckets and check if they were stored correctly:
    printf("\n\nTEST MODULE (BuildTreeLevel) \n\n");
    uint8_t ret;
    unsigned char* serialized_bucket = (unsigned char*)malloc(Z * block_size);
    unsigned char* temp_hash = (unsigned char*)malloc(HASH_LENGTH);
    printf("tdata_size = %d, block_size = %d\n", tdata_size, block_size);

    for (uint32_t i = 1; i < ptreeSize; i++) {
        downloadBucket_OCALL(&ret, instance_id, oram_type, serialized_bucket, Z * block_size, i, temp_hash, HASH_LENGTH, block_size, level);
        unsigned char* bucket_iter = serialized_bucket;
        printf("Bucket %d\n", i);
        for (uint8_t j = 0; j < Z; j++) {
            unsigned char* data_ptr = bucket_iter + ADDITIONAL_METADATA_SIZE;
            uint32_t* data_iter = (uint32_t*)(bucket_iter + ADDITIONAL_METADATA_SIZE);
            uint32_t no = (tdata_size) / sizeof(uint32_t);

            printf("(%d,%d) :", getId(bucket_iter), getTreeLabel(bucket_iter));
            if (getId(bucket_iter) == gN || level == recursion_levels - 1) {
                for (uint8_t q = 0; q < tdata_size; q++)
                    printf("%c", data_ptr[q]);
            } else {
                for (uint8_t q = 0; q < no; q++)
                    printf("%d,", data_iter[q]);
            }
            bucket_iter += block_size;
            printf("\n");
        }
        printf("\n");
    }
    free(serialized_bucket);
    free(temp_hash);
#endif

    free(hash_lchild);
    free(hash_rchild);
    return (posmap_l);
}

void ORAMTree::BuildTreeRecursive(uint8_t level, uint32_t* prev_pmap)
{
    if (level == 0) {
#ifdef BUILDTREE_DEBUG
        printf("BUILDTREE_DEBUG : Level 0 :\n");
#endif

        uint32_t* posmap_l;

        if (recursion_levels == 1) {
            posmap_l = BuildTreeLevel(level, NULL);
        } else {
            posmap_l = (uint32_t*)malloc(real_max_blocks_level[level] * sizeof(uint32_t));
            memcpy(posmap_l, prev_pmap, real_max_blocks_level[level] * sizeof(uint32_t));
            D_level[level] = 0;
            N_level[level] = max_blocks_level[level];

#ifdef DEBUG_INTEGRITY
            if (recursion_levels != -1) {
                printf("The Merkle Roots are :\n");
                for (uint32_t i = 0; i < recursion_levels; i++) {
                    printf("Level %d : ", i);
                    for (uint32_t l = 0; l < HASH_LENGTH; l++) {
                        printf("%c", (merkle_root_hash_level[i][l] % 26) + 'A');
                    }
                    printf("\n");
                }
            }
#endif
        }
        posmap = posmap_l;
    } else {
        uint32_t* posmap_level = BuildTreeLevel(level, prev_pmap);
        BuildTreeRecursive(level - 1, posmap_level);
        free(posmap_level);
    }
}

void ORAMTree::Initialize()
{
    N_level = (uint64_t*)malloc((recursion_levels) * sizeof(uint64_t));
    D_level = (uint32_t*)malloc((recursion_levels) * sizeof(uint64_t));
    recursive_stash = (Stash*)malloc((recursion_levels) * sizeof(Stash));

    // Fix stash_size for each level
    // 2.19498 log2(N) + 1.56669 * lambda - 10.98615

    for (uint32_t i = 0; i < recursion_levels; i++) {
        // printf("recursion_level i=%d, gN = %d\n",i, gN);

        if (i != recursion_levels - 1 && recursion_levels != 1) {
            if (oblivious_flag)
                recursive_stash[i].setup(stash_size, recursion_data_size, gN);
            else
                recursive_stash[i].setup_nonoblivious(recursion_data_size, gN);
        } else {
            if (oblivious_flag) {
                recursive_stash[i].setup(stash_size, data_size, gN);
            } else
                recursive_stash[i].setup_nonoblivious(data_size, gN);
        }
    }

    BuildTreeRecursive(recursion_levels - 1, NULL);

    uint32_t d_largest;
    if (recursion_levels == 0)
        d_largest = D_level[0];
    else
        d_largest = D_level[recursion_levels - 1];

    // printf("Initialize , d_largest == %d!\n",d_largest);

    // Allocate encrypted_path and decrypted_path to be the largest path sizes the ORAM would ever need
    // So that we dont have to have costly malloc and free within access()
    // Since ZT is currently single threaded, these are shared across all ORAM instances
    // Will have to redesign these to be comoponents of the ORAM_Instance class in a multi-threaded setting.

    uint64_t largest_path_size = Z * (data_size + ADDITIONAL_METADATA_SIZE) * (d_largest);
    // printf("Z=%d, data_size=%d, d_largest=%d, Largest_path_size = %ld\n", Z, data_size, d_largest, largest_path_size);
    encrypted_path = (unsigned char*)malloc(largest_path_size);
    decrypted_path = (unsigned char*)malloc(largest_path_size);
    fetched_path_array = (unsigned char*)malloc(largest_path_size);
    path_hash = (unsigned char*)malloc(HASH_LENGTH * 2 * (d_largest));
    new_path_hash = (unsigned char*)malloc(HASH_LENGTH * 2 * (d_largest));
    serialized_result_block = (unsigned char*)malloc(data_size + ADDITIONAL_METADATA_SIZE);
}

void ORAMTree::uploadPath(uint32_t leaf, unsigned char* path, uint64_t path_size, unsigned char* path_hash, uint64_t path_hash_size, uint8_t level)
{
    uint32_t d = D_level[level];
    uint8_t ret;

#ifdef ENCRYPTION_ON
    uploadPath_OCALL(&ret, instance_id, oram_type, encrypted_path, path_size, leaf, path_hash, path_hash_size, level, d);
#else
    uploadPath_OCALL(&ret, instance_id, oram_type, decrypted_path, path_size, leaf, path_hash, path_hash_size, level, d);
#endif
}

// For non-recursive level = 0
unsigned char* ORAMTree::downloadPath(uint32_t leaf, unsigned char* path_hash, uint8_t level)
{
    uint32_t temp = leaf;
    uint8_t rt;
    uint32_t tdata_size;
    uint32_t path_size, path_hash_size;
    uint32_t d = D_level[level];

    if (level == recursion_levels - 1 || recursion_levels == 1) {
        tdata_size = data_size;
    } else {
        tdata_size = recursion_data_size;
    }
    path_size = Z * (tdata_size + ADDITIONAL_METADATA_SIZE) * (d);
    path_hash_size = HASH_LENGTH * 2 * (d);

    downloadPath_OCALL(&rt, instance_id, oram_type, fetched_path_array, path_size, leaf, path_hash, path_hash_size, level, d);

#ifndef PASSIVE_ADVERSARY
    verifyPath(fetched_path_array, path_hash, leaf, d, tdata_size + ADDITIONAL_METADATA_SIZE, level);
#endif

#ifdef ACCESS_DEBUG
    printf("Verified path \n");
#endif

#ifdef ENCRYPTION_ON
    decryptPath(fetched_path_array, decrypted_path, (Z * (d)), tdata_size);
#else
    decrypted_path = fetched_path_array;
#endif

#ifdef ACCESS_DEBUG
    printf("Decrypted path \n");
#endif

#ifdef ENCRYPTION_ON
    return decrypted_path;
#else
    return fetched_path_array;
#endif
}

void ORAMTree::createNewPathHash(unsigned char* path_ptr, unsigned char* old_path_hash, unsigned char* new_path_hash, uint32_t leaf, uint32_t block_size, uint8_t level)
{
    uint32_t d = D_level[level];
    uint32_t leaf_temp = leaf;
    uint32_t leaf_temp_prev = leaf;
    unsigned char* new_path_hash_trail = new_path_hash;

    for (uint8_t i = 0; i < d + 1; i++) {
        if (i == 0) {
            sgx_sha256_msg(path_ptr, (block_size * Z), (sgx_sha256_hash_t*)new_path_hash);
            path_ptr += (block_size * Z);
            new_path_hash_trail = new_path_hash;
            new_path_hash += HASH_LENGTH;
        } else {
            sgx_sha_state_handle_t sha_handle;
            sgx_sha256_init(&sha_handle);
            sgx_sha256_update(path_ptr, (block_size * Z), sha_handle);
            path_ptr += (block_size * Z);
            if (leaf_temp_prev % 2 == 0) {
                sgx_sha256_update(new_path_hash_trail, HASH_LENGTH, sha_handle);
                old_path_hash += HASH_LENGTH;
                sgx_sha256_update(old_path_hash, HASH_LENGTH, sha_handle);
                old_path_hash += HASH_LENGTH;
            } else {
                sgx_sha256_update(old_path_hash, HASH_LENGTH, sha_handle);
                old_path_hash += (2 * HASH_LENGTH);
                sgx_sha256_update(new_path_hash_trail, HASH_LENGTH, sha_handle);
            }
            sgx_sha256_get_hash(sha_handle, (sgx_sha256_hash_t*)new_path_hash);
            if (i == d) {
                memcpy(merkle_root_hash_level[level], new_path_hash_trail, HASH_LENGTH);
            }
            new_path_hash_trail += HASH_LENGTH;
            new_path_hash += HASH_LENGTH;
            sgx_sha256_close(sha_handle);
        }
        leaf_temp_prev = leaf_temp;
        leaf_temp = leaf_temp >> 1;
    }
}

void ORAMTree::PushBlocksFromPathIntoStash(unsigned char* decrypted_path_ptr, uint8_t level, uint32_t data_size, uint32_t block_size, uint32_t id, uint32_t position_in_id, uint32_t leaf, uint32_t* nextLeaf, uint32_t newleaf, uint32_t sampledLeaf, int32_t newleaf_nextlevel)
{
    uint32_t d = D_level[level];
    uint32_t i;
#ifdef ACCESS_DEBUG
    printf("Fetched Path in PushBlocksFromPathIntoStash, data_size = %d : \n", data_size);
    //showPath(decrypted_path, Z, d, data_size);
    showPath_reverse(decrypted_path, Z, d, data_size, leaf);
#endif

    // FetchBlock Module :
    for (i = 0; i < (Z * (d)); i++) {
        bool dummy_flag = getId(decrypted_path_ptr) == gN;
        if (oblivious_flag) {
            recursive_stash[level].pass_insert(decrypted_path_ptr, isBlockDummy(decrypted_path_ptr, gN));
            setId(decrypted_path_ptr, gN);
        } else {
            if (!(isBlockDummy(decrypted_path_ptr, gN))) {
                if (getId(decrypted_path_ptr) == id) {
                    setTreeLabel(decrypted_path_ptr, newleaf);

                    //NOTE: if write operator, Write new data to block here.
                    if (level != recursion_levels) {
                        uint32_t* temp_block_ptr = (uint32_t*)getDataPtr(decrypted_path_ptr);
                        *nextLeaf = temp_block_ptr[position_in_id];
                        if (*nextLeaf > gN || *nextLeaf < 0) {
                            //Pull a random leaf as a temp fix.
                            *nextLeaf = sampledLeaf;
                            //printf("NEXTLEAF : %d, replaced with: %d\n",nextLeaf,newleaf_nextlevel);
                        }
                        temp_block_ptr[position_in_id] = newleaf_nextlevel;
                    }
                }
                recursive_stash[level].insert(decrypted_path_ptr);
            }
        }
        decrypted_path_ptr += block_size;
    }

#ifdef ACCESS_DEBUG
//printf("End of PushBlocksFromPathIntoStash, Path : \n");
//showPath_reverse(decrypted_path, Z, d, data_size);
#endif
}

//Scan over the stash and fix recustion leaf label
void ORAMTree::OAssignNewLabelToBlock(uint32_t id, uint32_t position_in_id, uint8_t level, uint32_t newleaf, uint32_t newleaf_nextlevel, uint32_t* nextLeaf)
{
    uint32_t k;
    nodev2* listptr_t;
    listptr_t = recursive_stash[level].getStart();

    for (k = 0; k < stash_size; k++) {
        bool flag1, flag2 = false;
        flag1 = ((getId(listptr_t->serialized_block) == id) && (!isBlockDummy(listptr_t->serialized_block, gN)));
        oassign_newlabel(getTreeLabelPtr(listptr_t->serialized_block), newleaf, flag1);

#ifdef ACCESS_DEBUG
        if (level != recursion_levels && recursion_levels != -1) {
            if (getId(listptr_t->serialized_block) == id)
                printf(" New Treelabel = %d\n", getTreeLabel(listptr_t->serialized_block));
        }
#endif

        if (level != recursion_levels && recursion_levels != -1) {
            for (uint8_t p = 0; p < x; p++) {
                flag2 = (flag1 && (position_in_id == p));
                ofix_recursion(&(listptr_t->serialized_block[24 + p * 4]), flag2, newleaf_nextlevel, nextLeaf);
            }
        }
        listptr_t = listptr_t->next;
    }
}

uint32_t ORAMTree::FillResultBlock(uint32_t id, unsigned char* result_data, uint32_t block_size)
{
    recursive_stash[recursion_levels - 1].ObliviousFillResultData(id, result_data);
}

// if target[i] != _|_, then one block should be moved from path[i] to path[target[i]]

// Debug Function to display the count and     stash occupants
void ORAMTree::print_stash_count(uint32_t level, uint32_t nlevel)
{
    uint32_t stash_oc;
    if (recursion_levels > 0) {
        stash_oc = recursive_stash[level].stashOccupancy();
        printf("Level : %d, stash_occupancy :%d\n", level, stash_oc);
        recursive_stash[level].displayStashContents(nlevel, level != (recursion_levels - 1));
    }
}

void ORAMTree::showPath(unsigned char* decrypted_path, uint8_t Z, uint32_t d, uint32_t data_size)
{
    unsigned char* decrypted_path_iter = decrypted_path;
    uint32_t block_size = data_size + ADDITIONAL_METADATA_SIZE;
    uint32_t num_of_blocks_on_path = Z * d;
    printf("\n\nshowPath() (From leaf to root) (Z=%d):  \n", Z);

    if (data_size == recursion_data_size) {
        for (uint32_t i = 0; i < num_of_blocks_on_path; i++) {
            if (i % Z == 0) {
                printf("\n");
            }
            printf("(%d,%d) :", getId(decrypted_path_iter), getTreeLabel(decrypted_path_iter));
            uint32_t no = (data_size) / sizeof(uint32_t);
            uint32_t* data_iter = (uint32_t*)(decrypted_path_iter + ADDITIONAL_METADATA_SIZE);
            unsigned char* data_ptr = (unsigned char*)(decrypted_path_iter + ADDITIONAL_METADATA_SIZE);

            if (getId(decrypted_path_iter) == gN) {
                for (uint8_t q = 0; q < data_size; q++)
                    printf("%c", data_ptr[q]);
            } else {
                for (uint8_t q = 0; q < no; q++)
                    printf("%d,", data_iter[q]);
            }
            printf("\n");
            decrypted_path_iter += block_size;
        }
    } else {
        for (uint32_t i = 0; i < num_of_blocks_on_path; i++) {
            if (i % Z == 0) {
                printf("\n");
            }
            printf("(%d,%d) :", getId(decrypted_path_iter), getTreeLabel(decrypted_path_iter));
            unsigned char* data_ptr = decrypted_path_iter + ADDITIONAL_METADATA_SIZE;
            for (uint8_t q = 0; q < data_size; q++)
                printf("%c", data_ptr[q]);
            printf("\n");
            decrypted_path_iter += (block_size);
        }
    }
}

// Debug Function to show a tree path in reverse
void ORAMTree::showPath_reverse(unsigned char* decrypted_path, uint8_t Z, uint32_t d, uint32_t data_size, uint32_t bucket_id_of_leaf)
{
    printf("\n\nshowPath_reverse (Root to leaf): \n");
    uint32_t block_size = data_size + ADDITIONAL_METADATA_SIZE;
    unsigned char* decrypted_path_iter = decrypted_path + ((uint64_t)((Z * (d - 1))) * uint64_t(block_size));
    uint32_t temp = bucket_id_of_leaf;
    uint32_t bucket_ids[d];
    for (uint32_t i = 0; i < d; i++) {
        bucket_ids[i] = (temp >> (d - i - 1));
    }

    if (data_size == recursion_data_size) {
        for (uint32_t i = 0; i < d; i++) {
            unsigned char* bucket_iter = decrypted_path_iter;

            printf("Bucket id %d:\n", bucket_ids[i]);
            for (uint32_t j = 0; j < Z; j++) {
                printf("(%d,%d) :", getId(bucket_iter), getTreeLabel(bucket_iter));
                uint32_t no = (data_size) / sizeof(uint32_t);
                uint32_t* data_iter = (uint32_t*)(bucket_iter + ADDITIONAL_METADATA_SIZE);
                unsigned char* data_ptr = (unsigned char*)(bucket_iter + ADDITIONAL_METADATA_SIZE);

                if (getId(bucket_iter) == gN) {
                    for (uint8_t q = 0; q < data_size; q++)
                        printf("%c", data_ptr[q]);
                } else {
                    for (uint8_t q = 0; q < no; q++)
                        printf("%d,", data_iter[q]);
                }

                printf("\n");
                bucket_iter += block_size;
            }
            printf("\n");
            decrypted_path_iter -= (Z * block_size);
        }
    } else {
        for (uint32_t i = 0; i < d; i++) {
            unsigned char* bucket_iter = decrypted_path_iter;

            printf("Bucket id %d:\n", bucket_ids[i]);
            temp = temp >> 1;
            for (uint32_t j = 0; j < Z; j++) {
                printf("(%d,%d) :", getId(bucket_iter), getTreeLabel(bucket_iter));

                unsigned char* data_ptr = (unsigned char*)(bucket_iter + ADDITIONAL_METADATA_SIZE);
                for (uint8_t q = 0; q < data_size; q++)
                    printf("%c", data_ptr[q]);

                printf("\n");
                bucket_iter += (block_size);
            }
            printf("\n");
            decrypted_path_iter -= (Z * block_size);
        }
    }
}

void ORAMTree::SetParams(uint32_t s_instance_id, uint8_t s_oram_type, uint8_t pZ, uint32_t s_max_blocks, uint32_t s_data_size, uint32_t s_stash_size, uint32_t oblivious, uint32_t s_recursion_data_size, uint8_t precursion_levels)
{
    instance_id = s_instance_id;
    oram_type = s_oram_type;
    data_size = s_data_size;
    stash_size = s_stash_size;
    oblivious_flag = (oblivious == 1);
    recursion_data_size = s_recursion_data_size;
    recursion_levels = precursion_levels;
    x = recursion_data_size / sizeof(uint32_t);
    Z = pZ;

    uint64_t size_pmap0 = s_max_blocks * sizeof(uint32_t);
    uint64_t cur_pmap0_blocks = s_max_blocks;
    while (size_pmap0 > MEM_POSMAP_LIMIT) {
        cur_pmap0_blocks = (uint64_t)ceil((double)cur_pmap0_blocks / (double)x);
        size_pmap0 = cur_pmap0_blocks * sizeof(uint32_t);
    }

    max_blocks_level = (uint64_t*)malloc((recursion_levels) * sizeof(uint64_t));
    real_max_blocks_level = (uint64_t*)malloc((recursion_levels) * sizeof(uint64_t));

    uint8_t max_recursion_level_index = recursion_levels - 1;
    real_max_blocks_level[max_recursion_level_index] = s_max_blocks;
    int32_t lev = max_recursion_level_index - 1;
    while (lev >= 0) {
        real_max_blocks_level[lev] = ceil((double)real_max_blocks_level[lev + 1] / (double)x);
        lev--;
    }

#ifdef SET_PARAMETERS_DEBUG
    for (uint8_t j = 0; j <= max_recursion_level_index; j++) {
        printf("real_max_blocks_level[%d] = %d \n", j, real_max_blocks_level[j]);
    }
    printf("\n");
#endif

    max_blocks_level[0] = cur_pmap0_blocks;
    for (uint32_t i = 1; i <= max_recursion_level_index; i++) {
        max_blocks_level[i] = max_blocks_level[i - 1] * x;
    }

#ifdef SET_PARAMETERS_DEBUG
    for (uint32_t i = 0; i <= max_recursion_level_index; i++) {
        printf("ENCLAVE:Level : %d, Blocks : %d\n", i, max_blocks_level[i]);
    }
    printf("\n");
#endif

    gN = max_blocks_level[max_recursion_level_index];
    merkle_root_hash_level = (sgx_sha256_hash_t*)malloc((max_recursion_level_index + 1) * sizeof(sgx_sha256_hash_t));
}
