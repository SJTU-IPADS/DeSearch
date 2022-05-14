#ifndef __ZT_BUCKET__

#include "Globals_Enclave.hpp"
#include "Block.hpp"

class Bucket {
    public:
    Block * blocks;
    uint8_t Z;

    Bucket(unsigned char * serialized_bucket, uint32_t data_size, uint8_t Z);
    Bucket(uint8_t Z);
    Bucket();
    ~Bucket();

    void initialize(uint32_t data_size, uint32_t gN);
    void reset_blocks(uint32_t data_size, uint32_t gN);
    void sampleRandomness();
    void aes_encryptBlocks(uint32_t data_size, unsigned char * aes_key);
    void aes_decryptBlocks(uint32_t data_size, unsigned char * aes_key);

    unsigned char * serialize(uint32_t data_size);

    void serializeToBuffer(unsigned char * serializeBuffer, uint32_t data_size);

    void displayBlocks();
    void fill();
    void fill(Block * b, uint32_t pos, uint32_t g_data_size);
    void fill(unsigned char * serialized_block, uint32_t pos, uint32_t g_data_size);
    void fill(uint32_t data_size);
};

#define __ZT_BUCKET__
#endif
