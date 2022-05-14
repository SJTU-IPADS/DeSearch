#include "LocalStorage.hpp"

// Utilization Parameter is the number of blocks of a bucket that is filled at start state. ( 4 = MAX_OCCUPANCY )
// #define PASSIVE_ADVERSARY 1
// #define PRINT_BUCKETS 1

LocalStorage::LocalStorage() {}

void LocalStorage::setParams(uint32_t max_blocks, uint32_t set_D, uint32_t set_Z, uint32_t stash_size, uint32_t data_size_p, bool inmem_p, uint32_t recursion_block_size_p, uint8_t recursion_levels_p) {
    //Test and set directory name
    data_block_size = data_size_p;
    D = set_D;
    Z = set_Z;
    inmem = inmem_p;

    recursion_block_size = recursion_block_size_p;
    recursion_levels = recursion_levels_p;

    bucket_size = data_size_p * Z;
    //Datatree_size and Hashtree_size of the data-ORAM tree:
    uint64_t datatree_size = (pow(2, D + 1) - 1) * (bucket_size);
    uint64_t hashtree_size = ((pow(2, D + 1) - 1) * (HASH_LENGTH));

    #ifdef DEBUG_LS
    printf("\nIN LS : recursion_levels = %d, data_size = %d, recursion_block_size = %d\n\n", recursion_levels, data_block_size, recursion_block_size);
    printf("DataTree_size = %ld or %f GB, HashTree_size = %ld or %f GB\n", datatree_size, float(datatree_size) / (float(1024 * 1024 * 1024)), hashtree_size, float(hashtree_size) / (float(1024 * 1024 * 1024)));
    #endif

    {
        if (recursion_levels == 1) {
            #ifdef DEBUG_LS
            printf("DataTree_size = %ld, HashTree_size = %ld\n", datatree_size, hashtree_size);
            #endif

            inmem_tree_l = (unsigned char ** ) malloc(sizeof(unsigned char * ));
            inmem_hash_l = (unsigned char ** ) malloc(sizeof(unsigned char * ));
            inmem_tree_l[0] = (unsigned char * ) malloc(datatree_size);
            inmem_hash_l[0] = (unsigned char * ) malloc(hashtree_size);
            blocks_in_level = (uint64_t * ) malloc((recursion_levels) * sizeof(uint64_t * ));
            buckets_in_level = (uint64_t * ) malloc((recursion_levels) * sizeof(uint64_t * ));
            blocks_in_level[0] = max_blocks;
        } else {
            #ifdef RESUME_EXPERIMENT
            //Resuming mechanism for in-memory database
            #else

            uint32_t x = (recursion_block_size - ADDITIONAL_METADATA_SIZE) / sizeof(uint32_t);
            #ifdef DEBUG_LS
            printf("X = %d\n", x);
            #endif

            /*
            //Compute size for each level of recursion
            //and fill up blocks_in_level[]
            uint64_t pmap0_blocks = max_blocks; 				
            uint64_t cur_pmap0_blocks = max_blocks;
            uint32_t level = recursion_levels-1;
            blocks_in_level = (uint64_t*) malloc((recursion_levels) * sizeof(uint64_t*));
            blocks_in_level[recursion_levels-1] = pmap0_blocks;
             
            while(level > 0) {
              blocks_in_level[level-1] = ceil((double)blocks_in_level[level]/(double)x);
              level--;
            } 
            */

            //NEW RECURSION PARAMETERIZATION START
            uint64_t size_pmap0 = max_blocks * sizeof(uint32_t);
            uint64_t cur_pmap0_blocks = max_blocks;
            while (size_pmap0 > MEM_POSMAP_LIMIT) {
                cur_pmap0_blocks = (uint64_t) ceil((double) cur_pmap0_blocks / (double) x);
                size_pmap0 = cur_pmap0_blocks * sizeof(uint32_t);
            }

            blocks_in_level = (uint64_t * ) malloc((recursion_levels) * sizeof(uint64_t * ));
            buckets_in_level = (uint64_t * ) malloc((recursion_levels) * sizeof(uint64_t * ));
            real_max_blocks_level = (uint64_t * ) malloc((recursion_levels) * sizeof(uint64_t));

            uint8_t max_recursion_level_index = recursion_levels - 1;
            real_max_blocks_level[max_recursion_level_index] = max_blocks;
            int32_t lev = max_recursion_level_index - 1;
            while (lev >= 0) {
                real_max_blocks_level[lev] = ceil((double) real_max_blocks_level[lev + 1] / (double) x);
                lev--;
            }

            #ifdef SET_PARAMETERS_DEBUG
            for (uint8_t j = 0; j <= max_recursion_level_index; j++) {
                printf("LS: real_max_blocks_level[%d] = %ld \n", j, real_max_blocks_level[j]);
            }
            printf("\n");
            #endif

            blocks_in_level[0] = cur_pmap0_blocks;
            for (uint32_t i = 1; i <= max_recursion_level_index; i++) {
                blocks_in_level[i] = blocks_in_level[i - 1] * x;
            }

            #ifdef SET_PARAMETERS_DEBUG
            for (uint32_t i = 0; i <= max_recursion_level_index; i++) {
                printf("LS:Level : %d, Blocks : %ld\n", i, blocks_in_level[i]);
            }
            printf("\n");
            #endif

            gN = blocks_in_level[max_recursion_level_index];
            printf("gn = %ld\n", gN);
            //NEW RECURSION PARAMETERIZATION END

            inmem_tree_l = (unsigned char ** ) malloc((recursion_levels) * sizeof(unsigned char * ));
            inmem_hash_l = (unsigned char ** ) malloc((recursion_levels) * sizeof(unsigned char * ));

            for (uint32_t i = 0; i < recursion_levels; i++) {
                uint64_t level_size;
                // leaf_nodes in level = ceil(log_2(ceil(blocks_in_level[level] / util_divisor)) 
                // where util_divisor = Z
                // Total nodes in Tree = 2 *leaf_nodes -1
                uint32_t pD_temp = ceil((double) blocks_in_level[i] / (double) Z);
                uint32_t pD = (uint32_t) ceil(log((double) pD_temp) / log((double) 2));
                uint64_t pN = (int) pow((double) 2, (double) pD);
                uint64_t tree_size = 2 * pN - 1;
                buckets_in_level[i] = tree_size;

                if (i == recursion_levels - 1)
                    level_size = tree_size * ((uint64_t)(Z * (data_size_p + ADDITIONAL_METADATA_SIZE)));
                else
                    level_size = tree_size * ((uint64_t)(Z * (recursion_block_size + ADDITIONAL_METADATA_SIZE)));

                uint64_t hashtree_size = (uint64_t)(tree_size * (uint64_t)(HASH_LENGTH));

                //Setup Memory locations for hashtree and recursion block	
                inmem_tree_l[i] = (unsigned char * ) malloc(level_size);
                inmem_hash_l[i] = (unsigned char * ) malloc(hashtree_size);

                #ifdef SET_PARAMETERS_DEBUG
                printf("LS:Level : %d, Blocks : %ld, TreeSize = %ld, hashtree_size= %ld\n", i, blocks_in_level[i], tree_size, hashtree_size);
                #endif
            }
            #endif
        }

    }
}

LocalStorage::LocalStorage(LocalStorage & ls) {}

void LocalStorage::fetchHash(uint32_t bucket_id, unsigned char * hash, uint32_t hash_size, uint8_t recursion_level) {
    {
        memcpy(hash, inmem_hash_l[recursion_level] + ((bucket_id - 1) * HASH_LENGTH), HASH_LENGTH);
    }
}

uint8_t LocalStorage::uploadBucket(uint32_t bucket_id, unsigned char * bucket, uint32_t bucket_size, unsigned char * hash, uint32_t hash_size, uint8_t recursion_level) {
    #ifdef DEBUG_LS
    printf("LS:uploadBucket : level = %d, bucket_id = %d of buckets_in_level[%d] = %ld\n", recursion_level, bucket_id, recursion_level, buckets_in_level[recursion_level]);
    #endif

    uint64_t pos;

    {
        uint64_t pos = ((uint64_t)(Z * bucket_size)) * ((uint64_t)(bucket_id - 1));
        memcpy(inmem_tree_l[recursion_level] + (pos), bucket, (bucket_size * Z));
        memcpy(inmem_hash_l[recursion_level] + (HASH_LENGTH * (bucket_id - 1)), hash, HASH_LENGTH);
    }
    return 0;
}

void LocalStorage::showPath_reverse(unsigned char * decrypted_path, uint8_t Z, uint32_t d, uint32_t data_size) {
    //TODO: gN is hardcoded here for quick debugging
    uint32_t gN = 100;
    printf("\n\nIN LS: showPath_reverse (Root to leaf): \n");
    uint32_t block_size = data_size + ADDITIONAL_METADATA_SIZE;
    unsigned char * decrypted_path_iter = decrypted_path + ((uint64_t)((Z * (d - 1))) * uint64_t(block_size));

    if (data_size == recursion_block_size - 24) {
        for (uint32_t i = 0; i < d; i++) {
            unsigned char * bucket_iter = decrypted_path_iter;

            for (uint32_t j = 0; j < Z; j++) {
                printf("(%d,%d) :", getId(bucket_iter), getTreeLabel(bucket_iter));
                uint32_t no = (data_size) / sizeof(uint32_t);
                uint32_t * data_iter = (uint32_t * )(bucket_iter + ADDITIONAL_METADATA_SIZE);
                unsigned char * data_ptr = (unsigned char * )(bucket_iter + ADDITIONAL_METADATA_SIZE);

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
            unsigned char * bucket_iter = decrypted_path_iter;

            for (uint32_t j = 0; j < Z; j++) {
                printf("(%d,%d) :", getId(bucket_iter), getTreeLabel(bucket_iter));

                unsigned char * data_ptr = (unsigned char * )(bucket_iter + ADDITIONAL_METADATA_SIZE);
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

uint8_t LocalStorage::uploadPath(uint32_t leaf_label, unsigned char * path, unsigned char * path_hash, uint8_t level, uint32_t D_level) {
    std::string file_name_this, file_name_this_i;
    uint32_t size_for_level;
    uint64_t pos;
    {
        if (level == recursion_levels - 1)
            size_for_level = data_block_size;
        else
            size_for_level = recursion_block_size;
    }

    uint32_t temp = leaf_label;
    unsigned char * path_iter = path;
    unsigned char * path_hash_iter = path_hash;

    #ifdef DEBUG_LS
    showPath_reverse(path, Z, D_level, size_for_level - 24);
    #endif

    {
        for (uint8_t i = 0; i < D_level; i++) {
            memcpy(inmem_tree_l[level] + ((Z * size_for_level) * (temp - 1)), path_iter, (Z * size_for_level));
            #ifdef DEBUG_LS
            printf("In LS Upload, Bucket uploaded = %d\n", temp);
            printf("Blocks in bucket:\n");
            for (uint8_t q = 0; q < Z; q++) {
                printf("(%d,%d),", *((uint32_t * )(inmem_tree_l[level] + ((Z * size_for_level) * (temp - 1) + q * size_for_level + 16))),
                    *((uint32_t * )(inmem_tree_l[level] + ((Z * size_for_level) * (temp - 1) + q * size_for_level + 20))));

            }
            printf("\n");
            #endif

            #ifndef PASSIVE_ADVERSARY
            memcpy(inmem_hash_l[level] + (HASH_LENGTH * (temp - 1)), path_hash_iter, HASH_LENGTH);
            /*
            printf("LS_UploadPath : Level = %d, Bucket no = %d, Hash = ",level, temp);
            for(uint8_t l = 0;l<HASH_LENGTH;l++)
              printf("%c",(*(path_hash_iter + l) %26)+'A');
            printf("\n");
            */
            path_hash_iter += HASH_LENGTH;
            #endif
            path_iter += (Z * size_for_level);
            temp = temp >> 1;
        }
        //printf("UploadPath success\n");
    }
    return 0;
}

// unsigned char* downloadBucket(uint32_t bucket_id, unsigned char* data,  unsigned char *hash, uint32_t hash_size, uint8_t level, uint32_t D_lev);

uint8_t LocalStorage::downloadBucket(uint32_t bucket_id, unsigned char * bucket, uint32_t bucket_size, unsigned char * hash, uint32_t hash_size, uint8_t level) {
    printf("In LS::downloadBucket for Bucket %d\n", bucket_id);
    uint64_t pos;
    std::string file_name_this, file_name_this_i;
    uint32_t size_for_level;

    {
        if (level == recursion_levels - 1)
            size_for_level = data_block_size;
        else
            size_for_level = recursion_block_size;
    }

    memcpy(bucket, inmem_tree_l[level] + ((Z * size_for_level) * (bucket_id - 1)), (Z * size_for_level));
    memcpy(hash, inmem_hash_l[level] + (HASH_LENGTH * (bucket_id - 1)), HASH_LENGTH);

    return 0;
}

/*
LocalStorage::downloadPath() - returns requested path in *path

Requested path is returned leaf to root.
For each node on path, returned path_hash contains <L-hash, R-Hash> pairs with the exception of a single hash for root node

*/

unsigned char * LocalStorage::downloadPath(uint32_t leaf_label, unsigned char * path, unsigned char * path_hash, uint32_t path_hash_size, uint8_t level, uint32_t D_lev) {
    uint64_t pos;
    std::string file_name_this, file_name_this_i;
    uint32_t size_for_level;

    {
        if (level == recursion_levels - 1)
            size_for_level = data_block_size;
        else
            size_for_level = recursion_block_size;
    }

    uint32_t temp = leaf_label;
    unsigned char * path_iter = path;
    unsigned char * path_hash_iter = path_hash;

    {
        for (uint8_t i = 0; i < D_lev; i++) {
            //printf("LS: i = %d, temp = %d\n",i,temp);
            memcpy(path_iter, inmem_tree_l[level] + ((Z * size_for_level) * (temp - 1)), (Z * size_for_level));

            #ifndef PASSIVE_ADVERSARY
            if (i != D_lev - 1) {
                if (temp % 2 == 0) {
                    memcpy(path_hash_iter, inmem_hash_l[level] + (HASH_LENGTH * (temp - 1)), HASH_LENGTH);
                    path_hash_iter += HASH_LENGTH;
                    memcpy(path_hash_iter, inmem_hash_l[level] + (HASH_LENGTH * (temp)), HASH_LENGTH);
                    path_hash_iter += HASH_LENGTH;

                    #ifdef LS_DEBUG_INTEGRITY
                    printf("LS : Level = %d, Bucket no = %d, Hash = ", level, temp);
                    for (uint8_t l = 0; l < HASH_LENGTH; l++)
                        printf("%c", ( * (inmem_hash_l[level] + (HASH_LENGTH * (temp - 1)) + l) % 26) + 'A');
                    printf("\nLS : Level = %d, Bucket no = %d, Hash = ", level, temp + 1);
                    for (uint8_t l = 0; l < HASH_LENGTH; l++)
                        printf("%c", ( * (inmem_hash_l[level] + (HASH_LENGTH * (temp)) + l) % 26) + 'A');
                    printf("\n");
                    #endif
                } else {
                    memcpy(path_hash_iter, inmem_hash_l[level] + (HASH_LENGTH * (temp - 2)), HASH_LENGTH);
                    path_hash_iter += HASH_LENGTH;
                    memcpy(path_hash_iter, inmem_hash_l[level] + (HASH_LENGTH * (temp - 1)), HASH_LENGTH);
                    path_hash_iter += HASH_LENGTH;

                    #ifdef LS_DEBUG_INTEGRITY
                    printf("LS : Level = %d, Bucket no = %d, Hash = ", level, temp - 1);
                    for (uint8_t l = 0; l < HASH_LENGTH; l++)
                        printf("%c", ( * (inmem_hash_l[level] + (HASH_LENGTH * (temp - 2)) + l) % 26) + 'A');
                    printf("\nLS : Level = %d, Bucket no = %d, Hash = ", level, temp);
                    for (uint8_t l = 0; l < HASH_LENGTH; l++)
                        printf("%c", ( * (inmem_hash_l[level] + (HASH_LENGTH * (temp - 1)) + l) % 26) + 'A');
                    printf("\n");
                    #endif
                }
            } else {
                memcpy(path_hash_iter, inmem_hash_l[level] + (HASH_LENGTH * (temp - 1)), HASH_LENGTH);
                path_hash_iter += (HASH_LENGTH);
                #ifdef LS_DEBUG_INTEGRITY
                printf("LS : Level = %d, Bucket no = %d, Hash = ", level, temp);
                for (uint8_t l = 0; l < HASH_LENGTH; l++)
                    printf("%c", ( * (inmem_hash_l[level] + (HASH_LENGTH * (temp - 1)) + l) % 26) + 'A');
                printf("\n");
                #endif
            }
            #endif

            path_iter += (Z * size_for_level);
            temp = temp >> 1;
        }
    }
    return path;
}

void LocalStorage::deleteObject() {

}
void LocalStorage::copyObject() {

}
