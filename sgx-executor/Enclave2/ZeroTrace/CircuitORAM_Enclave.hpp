#ifndef __ZT_CIRCUITORAM__

#include <stdint.h>
#include <string.h>

#include "Globals_Enclave.hpp"
#include "Enclave_utils.hpp"
#include "Block.hpp"
#include "Bucket.hpp"
#include "ORAMTree.hpp"

class CircuitORAM: public ORAMTree, public ORAM_Interface {
    public:
    // Specific to CircuitORAM
    uint32_t * deepest;
    uint32_t * target;
    int32_t * deepest_position;
    int32_t * target_position;
    unsigned char * serialized_block_hold;
    unsigned char * serialized_block_write;
    // For deterministic reverse lexicographic eviction
    uint32_t * access_counter;

    CircuitORAM() {};

    void Initialize(uint32_t instance_id, uint8_t oram_type, uint8_t pZ, uint32_t pmax_blocks, uint32_t pdata_size, uint32_t pstash_size, uint32_t poblivious_flag, uint32_t precursion_data_size, uint8_t precursion_levels);

    uint32_t CircuitORAM_Access(char opType, uint32_t id, uint32_t position_in_id, uint32_t leaf, uint32_t newleaf, uint32_t newleaf_nextlevel, unsigned char * decrypted_path,
        unsigned char * path_hash, uint32_t level, unsigned char * data_in, unsigned char * data_out);
    // void Access_temp(uint32_t id, char opType, unsigned char* data_in, unsigned char* data_out);	
    uint32_t access(uint32_t id, int32_t position_in_id, char opType, uint8_t level, unsigned char * data_in, unsigned char * data_out, uint32_t * prev_sampled_leaf);
    uint32_t access_oram_level(char opType, uint32_t leaf, uint32_t id, uint32_t position_in_id, uint32_t level, uint32_t newleaf, uint32_t newleaf_nextleaf, unsigned char * data_in, unsigned char * data_out);

    void CircuitORAM_FetchBlock(uint32_t * return_value, uint32_t leaf, uint32_t newleaf, char opType,
        uint32_t id, uint32_t position_in_id, uint32_t newleaf_nextlevel, uint32_t level, unsigned char * data_in, unsigned char * data_out);

    void CircuitORAM_RebuildPath(unsigned char * decrypted_path_ptr, uint32_t data_size, uint32_t block_size, uint32_t leaf, uint32_t level);

    void EvictionRoutine(uint32_t leaf, uint32_t level);

    // Additional CircuitORAM functions
    uint32_t * prepare_target(uint32_t leaf, unsigned char * serialized_path, uint32_t block_size, uint32_t level, uint32_t * deepest, int32_t * target_position);
    uint32_t * prepare_deepest(uint32_t leaf, unsigned char * serialized_path, uint32_t block_size, uint32_t level, int32_t * deepest_position);
    void EvictOnceFast(uint32_t * deepest, uint32_t * target, int32_t * deepest_position, int32_t * target_position, unsigned char * serialized_path, unsigned char * path_hash, uint32_t level, unsigned char * new_path_hash, uint32_t leaf);

    // Virtual functions, inherited from ORAMTree class 
    void Create(uint32_t instance_id, uint8_t oram_type, uint8_t pZ, uint32_t pmax_blocks, uint32_t pdata_size, uint32_t pstash_size, uint32_t poblivious_flag, uint32_t precursion_data_size, uint8_t precursion_levels) override;
    void Access(uint32_t id, char opType, unsigned char * data_in, unsigned char * data_out) override;

};

#define __ZT_CIRCUITORAM__
#endif
