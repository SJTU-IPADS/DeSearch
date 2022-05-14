#ifndef __ZT_BLOCK__
#define __ZT_BLOCK__

#include "ZT_Globals.hpp"

#include "Globals_Enclave.hpp"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

class Block {
    public:
        unsigned char * data;
    uint32_t id;
    uint32_t treeLabel;
    uint8_t * r;

    Block();
    Block(uint32_t gN);
    Block(uint32_t data_size, uint32_t gN);
    Block(Block & b, uint32_t g_data_size);
    Block(uint32_t set_id, uint8_t * set_data, uint32_t set_label);
    Block(unsigned char * serialized_block, uint32_t blockdatasize);
    ~Block();

    void initialize(uint32_t data_size, uint32_t gN);
    void generate_data(uint32_t data_size);
    void generate_r();
    bool isDummy(uint32_t gN);

    // reset_block sets the block to be a dummy block, and updates 
    // the data part to be A-Z looped as a dummy data entry
    void reset(uint32_t data_size, uint32_t gN);

    void fill(unsigned char * serialized_block, uint32_t data_size);
    void fill();
    void fill(uint32_t data_size);
    void fill_recursion_data(uint32_t * pmap, uint32_t recursion_data_size);

    unsigned char * serialize(uint32_t data_size);
    void serializeToBuffer(unsigned char * serialized_block, uint32_t data_size);
    void serializeForAes(unsigned char * buffer, uint32_t bDataSize);
    void aes_enc(uint32_t data_size, unsigned char * aes_key);
    void aes_dec(uint32_t data_size, unsigned char * aes_key);

    unsigned char * getDataPtr();

    //Debug Functions
    void displayBlock();
};

#endif
