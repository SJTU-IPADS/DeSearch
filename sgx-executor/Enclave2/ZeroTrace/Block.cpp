#include "Block.hpp"

Block::Block()
{
    data = NULL;
    r = NULL;
    treeLabel = 0;
    id = 0;
}

Block::Block(uint32_t gN)
{
    data = NULL;
    r = NULL;
    treeLabel = 0;
    id = gN;
}

Block::Block(uint32_t data_size, uint32_t gN)
{
    data = NULL;
    r = NULL;
    generate_data(data_size);
    generate_r();
    treeLabel = 0;
    id = gN;
}

void Block::initialize(uint32_t data_size, uint32_t gN)
{
    data = NULL;
    r = NULL;
    generate_data(data_size);
    generate_r();
    treeLabel = 0;
    id = gN;
}

Block::Block(Block& b, uint32_t g_data_size)
{
    data = (unsigned char*)malloc(g_data_size);
    r = (uint8_t*)malloc(NONCE_LENGTH);
    memcpy(r, b.r, NONCE_LENGTH);
    memcpy(data, b.data, g_data_size);

    treeLabel = b.treeLabel;
    id = b.id;
}

Block::Block(uint32_t set_id, uint8_t* set_data, uint32_t set_label)
{
    //data = ; Generate datablock of appropriate datasize for new block.
    //treeLabel = 0;
    id = set_id;
    data = set_data;
    treeLabel = set_label;
}

Block::Block(unsigned char* serialized_block, uint32_t blockdatasize)
{
    unsigned char* ptr = serialized_block;
    r = (uint8_t*)malloc(NONCE_LENGTH);

    memcpy(r, ptr, NONCE_LENGTH);
    ptr += NONCE_LENGTH;

    memcpy((void*)&id, ptr, ID_SIZE_IN_BYTES);
    ptr += ID_SIZE_IN_BYTES;
    memcpy((void*)&treeLabel, ptr, ID_SIZE_IN_BYTES);
    ptr += ID_SIZE_IN_BYTES;

    data = (unsigned char*)malloc(blockdatasize);
    memcpy(data, ptr, blockdatasize);
    ptr += blockdatasize;
}

Block::~Block()
{
    if (r)
        free(r);
    if (data)
        free(data);
}

void Block::generate_data(uint32_t fdata_size)
{
    // TODO : Fix to initiate with some value
    if (data == NULL)
        data = (uint8_t*)malloc(fdata_size);

#ifdef RAND_DATA
    sgx_read_rand(data, fdata_size);
#else
    for (uint32_t i = 0; i < fdata_size; i++) {
        data[i] = (i % 26) + 65; // ASCII 'A'
    }
    data[fdata_size - 1] = '\0';
#endif
}

void Block::reset(uint32_t data_size, uint32_t gN)
{
    id = gN;
    treeLabel = 0;

    if (data == NULL)
        data = (uint8_t*)malloc(data_size);

    for (uint32_t i = 0; i < data_size; i++) {
        data[i] = (i % 26) + 65; // ASCII 'A'
    }
    data[data_size - 1] = '\0';
}

void Block::generate_r()
{
    if (r == NULL)
        r = (uint8_t*)malloc(NONCE_LENGTH);

#ifdef RAND_DATA
    sgx_read_rand(r, NONCE_LENGTH);
#else
    for (uint8_t i = 0; i < NONCE_LENGTH; i++) {
        r[i] = 'A';
    }
#endif
}

bool Block::isDummy(uint32_t gN)
{
    return (id == gN);
}

void Block::fill(unsigned char* serialized_block, uint32_t data_size)
{
    unsigned char* ptr = serialized_block;
    r = (uint8_t*)malloc(NONCE_LENGTH);

    memcpy(r, ptr, NONCE_LENGTH);
    ptr += NONCE_LENGTH;

    memcpy((void*)&id, ptr, ID_SIZE_IN_BYTES);
    ptr += ID_SIZE_IN_BYTES;
    memcpy((void*)&treeLabel, ptr, ID_SIZE_IN_BYTES);
    ptr += ID_SIZE_IN_BYTES;

    data = (unsigned char*)malloc(data_size);
    memcpy(data, ptr, data_size);
    ptr += data_size;
}

void Block::fill(uint32_t data_size)
{
    generate_data(data_size);
    generate_r();
}

void Block::fill_recursion_data(uint32_t* pmap, uint32_t recursion_data_size)
{
    memcpy(data, pmap, recursion_data_size);
}

unsigned char* Block::serialize(uint32_t data_size)
{
    uint32_t tdata_size = data_size + ADDITIONAL_METADATA_SIZE;
    unsigned char* serialized_block = (unsigned char*)malloc(tdata_size);
    unsigned char* ptr = serialized_block;
    memcpy(ptr, (void*)r, NONCE_LENGTH);
    ptr += NONCE_LENGTH;
    memcpy(ptr, (void*)&id, sizeof(id));
    ptr += sizeof(id);
    memcpy(ptr, (void*)&treeLabel, sizeof(treeLabel));
    ptr += sizeof(treeLabel);
    memcpy(ptr, data, data_size);
    ptr += data_size;
    return serialized_block;
}

void Block::serializeToBuffer(unsigned char* serialized_block, uint32_t data_size)
{
    unsigned char* ptr = serialized_block;
    memcpy(ptr, (void*)r, NONCE_LENGTH);
    ptr += NONCE_LENGTH;
    memcpy(ptr, (void*)&id, sizeof(id));
    ptr += sizeof(id);
    memcpy(ptr, (void*)&treeLabel, sizeof(treeLabel));
    ptr += sizeof(treeLabel);
    memcpy(ptr, data, data_size);
    ptr += data_size;
}

void Block::serializeForAes(unsigned char* buffer, uint32_t bDataSize)
{
    memcpy(buffer, (void*)&id, sizeof(id));
    memcpy(buffer + ID_SIZE_IN_BYTES, (void*)&treeLabel, sizeof(treeLabel));
    memcpy(buffer + ID_SIZE_IN_BYTES * 2, data, bDataSize);
}

void Block::displayBlock()
{
    printf("(ID = %d, Label = %d)\n", id, treeLabel);
    if (r) {
        printf("Nonce = %s\n", r);
    } else {
        printf("r = NULL\n");
    }

    if (data) {
        printf("Data = %s\n", data);
    } else {
        printf("DATA_PTR = NULL\n");
    }
}

void Block::aes_enc(uint32_t data_size, unsigned char* aes_key)
{
    generate_r();
    uint32_t input_size = data_size + 2 * ID_SIZE_IN_BYTES;
    unsigned char* ctr = (unsigned char*)malloc(NONCE_LENGTH);
    unsigned char* ciphertext = (unsigned char*)malloc(input_size);
    unsigned char* input_buffer = (unsigned char*)malloc(input_size);
    serializeForAes(input_buffer, data_size);

    memcpy(ctr, r, NONCE_LENGTH);
    sgx_status_t ret = SGX_SUCCESS;
    uint32_t ctr_inc_bits = 16;

    ret = sgx_aes_ctr_encrypt((const sgx_aes_ctr_128bit_key_t*)aes_key,
        input_buffer,
        input_size,
        ctr,
        ctr_inc_bits,
        ciphertext);

    memcpy((void*)&id, ciphertext, ID_SIZE_IN_BYTES);
    memcpy((void*)&treeLabel, ciphertext + ID_SIZE_IN_BYTES, ID_SIZE_IN_BYTES);
    memcpy(data, ciphertext + ID_SIZE_IN_BYTES * 2, data_size);

    free(input_buffer);
    free(ciphertext);
    free(ctr);
}

void Block::aes_dec(uint32_t data_size, unsigned char* aes_key)
{
    uint32_t ciphertext_size = data_size + 2 * ID_SIZE_IN_BYTES;
    unsigned char* ctr = (unsigned char*)malloc(NONCE_LENGTH);
    unsigned char* ciphertext = (unsigned char*)malloc(ciphertext_size);
    unsigned char* input_buffer = (unsigned char*)malloc(ciphertext_size);
    memcpy(ctr, r, NONCE_LENGTH);

    serializeForAes(ciphertext, data_size);
    sgx_status_t ret = SGX_SUCCESS;
    uint32_t ctr_inc_bits = 16;

    ret = sgx_aes_ctr_decrypt((const sgx_aes_ctr_128bit_key_t*)aes_key,
        ciphertext,
        ciphertext_size,
        ctr,
        ctr_inc_bits,
        input_buffer);

    memcpy((void*)&id, input_buffer, ID_SIZE_IN_BYTES);
    memcpy((void*)&treeLabel, input_buffer + ID_SIZE_IN_BYTES, ID_SIZE_IN_BYTES);
    memcpy(data, input_buffer + ID_SIZE_IN_BYTES * 2, data_size);

    free(input_buffer);
    free(ciphertext);
    free(ctr);
}

unsigned char* Block::getDataPtr()
{
    return data;
}
