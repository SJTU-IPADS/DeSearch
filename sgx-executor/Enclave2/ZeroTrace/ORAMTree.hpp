#ifndef __ZT_ORAMTREE__

#include <string.h>

#include "Globals_Enclave.hpp"
#include "Enclave_utils.hpp"
#include "Bucket.hpp"
#include "Stash.hpp"

class ORAMTree {
    public:

    // Basic Params
    uint8_t Z;
    uint32_t data_size;
    uint32_t stash_size;
    bool oblivious_flag;
    uint8_t recursion_levels;
    uint32_t recursion_data_size;

    // Params to tie ORAM to corresponding LS backend
    uint32_t instance_id;
    uint8_t oram_type;

    // Buffers
    unsigned char * encrypted_path;
    unsigned char * decrypted_path;
    unsigned char * fetched_path_array;
    unsigned char * path_hash;
    unsigned char * new_path_hash;
    unsigned char * serialized_result_block;

    // Computed Params
    uint32_t x;
    uint64_t gN;
    uint32_t treeSize;

    // PositionMap
    uint32_t * posmap;

    // Stash components
    Stash * recursive_stash;

    // Parameters for recursive ORAMs (these are per arrays for corresponding parameters for each level of recursion)
    uint64_t * max_blocks_level; //The total capacity of blocks in a level
    uint64_t * real_max_blocks_level; //The real blocks used out of that total capacity
    // This is an artifact of recursion, in non-recursive they are the same.
    uint64_t * N_level; //For non-recursive, N = N_level[0]
    uint32_t * D_level; //For non-recursive, D = D_level[0]
    sgx_sha256_hash_t * merkle_root_hash_level;
    // For non-recursive, merkle_root = merkle_root_hash_level[0] 

    // Key components		
    unsigned char * aes_key;

    // Debug Functions
    void print_stash_count(uint32_t level, uint32_t nlevel);
    void print_pmap0();
    void showPath(unsigned char * decrypted_path, uint8_t Z, uint32_t d, uint32_t data_size);
    void showPath_reverse(unsigned char * decrypted_path, uint8_t Z, uint32_t d, uint32_t data_size, uint32_t leaf_bucket_id);

    // Initialize/Build Functions
    // Helper function for BuildTree Recursive
    uint32_t * BuildTreeLevel(uint8_t level, uint32_t * prev_posmap);
    // For non-recursive, simply invoke BuildTreeRecursive(0,NULL)
    void BuildTreeRecursive(uint8_t level, uint32_t * prev_pmap);
    void Initialize();
    void SetParams(uint32_t instance_id, uint8_t oram_type, uint8_t pZ, uint32_t pmax_blocks, uint32_t pdata_size, uint32_t pstash_size, uint32_t poblivious_flag, uint32_t precursion_data_size, uint8_t precursion_levels);
    void SampleKey();

    // Constructor & Destructor
    ORAMTree();
    ~ORAMTree();

    // Path Function
    void verifyPath(unsigned char * path_array, unsigned char * path_hash, uint32_t leaf, uint32_t D, uint32_t block_size, uint8_t level);
    void decryptPath(unsigned char * path_array, unsigned char * decrypted_path_array, uint32_t num_of_blocks_on_path, uint32_t data_size);
    void encryptPath(unsigned char * path_array, unsigned char * encrypted_path_array, uint32_t num_of_blocks_on_path, uint32_t data_size);

    unsigned char * downloadPath(uint32_t leaf, unsigned char * path_hash, uint8_t level);
    void uploadPath(uint32_t leaf, unsigned char * path, uint64_t path_size, unsigned char * path_hash, uint64_t path_hash_size, uint8_t level);

    // Access Functions
    void createNewPathHash(unsigned char * path_ptr, unsigned char * old_path_hash, unsigned char * new_path_hash, uint32_t leaf, uint32_t block_size, uint8_t level);
    void addToNewPathHash(unsigned char * path_iter, unsigned char * old_path_hash, unsigned char * new_path_hash_trail, unsigned char * new_path_hash, uint32_t level_in_path, uint32_t leaf_temp_prev, uint32_t block_size, uint8_t level);
    void PushBlocksFromPathIntoStash(unsigned char * decrypted_path_ptr, uint8_t level, uint32_t data_size, uint32_t block_size, uint32_t id, uint32_t position_in_id, uint32_t leaf, uint32_t * nextLeaf, uint32_t newleaf, uint32_t sampledLeaf, int32_t newleaf_nextlevel);
    uint32_t access_oram_level(char opType, uint32_t leaf, uint32_t id, uint32_t position_in_id, uint8_t level, uint32_t newleaf, uint32_t newleaf_nextleaf, unsigned char * data_in, unsigned char * data_out);

    // Misc                
    void OAssignNewLabelToBlock(uint32_t id, uint32_t position_in_id, uint8_t level, uint32_t newleaf, uint32_t newleaf_nextlevel, uint32_t * nextLeaf);
    uint32_t FillResultBlock(uint32_t id, unsigned char * result_data, uint32_t block_size);
};

#define __ZT_ORAMTREE__
#endif
