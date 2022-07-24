#include "Bucket.hpp"

Bucket::Bucket(unsigned char* serialized_bucket, uint32_t data_size, uint8_t param_Z)
{
    //TODO: This might be wrong, recheck.
    blocks = (Block*)malloc(Z * sizeof(Block*));
    Z = param_Z;

    unsigned char* ptr = serialized_bucket + 24;
    for (uint8_t i = 0; i < Z; i++) {
        blocks[i].fill(ptr, data_size);
        ptr += (data_size + 24);
    }
}

Bucket::Bucket(uint8_t param_Z)
{
    Z = param_Z;
}

Bucket::~Bucket()
{
    if (blocks)
        free(blocks);
}

void Bucket::initialize(uint32_t data_size, uint32_t gN)
{
    blocks = (Block*)malloc(Z * sizeof(Block));
    for (uint8_t i = 0; i < Z; i++) {
        blocks[i].initialize(data_size, gN);
    }
}

void Bucket::reset_blocks(uint32_t data_size, uint32_t gN)
{
    for (uint8_t p = 0; p < Z; p++) {
        blocks[p].reset(data_size, gN);
    }
}

void Bucket::sampleRandomness()
{
    for (uint8_t i = 0; i < Z; i++) {
        blocks[i].generate_r();
    }
}

void Bucket::displayBlocks()
{
    for (uint8_t i = 0; i < Z; i++)
        printf("(%d,%d),", blocks[i].id, blocks[i].treeLabel);
    printf("\n");
}

void Bucket::aes_encryptBlocks(uint32_t data_size, unsigned char* aes_key)
{
    for (uint8_t e = 0; e < Z; e++) {
        blocks[e].aes_enc(data_size, aes_key);
    }
}

void Bucket::aes_decryptBlocks(uint32_t data_size, unsigned char* aes_key)
{
    for (uint8_t i = 0; i < Z; i++) {
        blocks[i].aes_dec(data_size, aes_key);
    }
}

unsigned char* Bucket::serialize(uint32_t data_size)
{
    uint32_t size_of_bucket = Z * (data_size + ADDITIONAL_METADATA_SIZE);
    uint32_t tdata_size = (data_size + ADDITIONAL_METADATA_SIZE);
    unsigned char* serialized_bucket = (unsigned char*)malloc(size_of_bucket);
    unsigned char* ptr = serialized_bucket;
    for (int i = 0; i < Z; i++) {
        unsigned char* serial_block = blocks[i].serialize(data_size);
        memcpy(ptr, serial_block, tdata_size);
        free(serial_block);
        ptr += tdata_size;
    }
    return serialized_bucket;
}

void Bucket::serializeToBuffer(unsigned char* serializeBuffer, uint32_t data_size)
{
    unsigned char* buffer_iter = serializeBuffer;
    uint32_t tdata_size = (data_size + 8 + NONCE_LENGTH);
    for (int i = 0; i < Z; i++) {
        blocks[i].serializeToBuffer(buffer_iter, data_size);
        buffer_iter += tdata_size;
    }
}

void Bucket::fill(Block* b, uint32_t pos, uint32_t g_data_size)
{
    blocks[pos].id = b->id;
    blocks[pos].treeLabel = b->treeLabel;
    memcpy(blocks[pos].data, b->data, g_data_size);
}

void Bucket::fill(unsigned char* serialized_block, uint32_t pos, uint32_t g_data_size)
{
    blocks[pos].id = getId(serialized_block);
    blocks[pos].treeLabel = getTreeLabel(serialized_block);
    memcpy(blocks[pos].data, serialized_block + 24, g_data_size);
}

void Bucket::fill(uint32_t data_size)
{
    for (uint8_t i = 0; i < Z; i++) {
        blocks[i].fill(data_size);
    }
}
