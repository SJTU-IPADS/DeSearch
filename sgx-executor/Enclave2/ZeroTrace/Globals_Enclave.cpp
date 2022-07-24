#include "Globals_Enclave.hpp"

void displaySerializedBlock(unsigned char* serialized_result_block, uint32_t level, uint32_t recursion_levels, uint32_t x)
{
    uint32_t i = 0;
    unsigned char* data_ptr;
    printf("Fetched Result_block : (%d,%d) : \n", getId(serialized_result_block), getTreeLabel(serialized_result_block));
    if (level != recursion_levels) {
        data_ptr = (unsigned char*)getDataPtr(serialized_result_block);
        for (i = 0; i < x; i++) {
            printf("%c,", (*data_ptr));
            data_ptr++;
        }
        printf("\n");
    }
}

void aes_dec_serialized(unsigned char* encrypted_block, uint32_t data_size, unsigned char* decrypted_block, unsigned char* aes_key)
{
    unsigned char* ctr = (unsigned char*)malloc(NONCE_LENGTH);
    unsigned char* encrypted_block_ptr = encrypted_block + NONCE_LENGTH;
    unsigned char* decrypted_block_ptr = decrypted_block + NONCE_LENGTH;
    memcpy(ctr, encrypted_block, NONCE_LENGTH);

    // 8 from 4 bytes for id and 4 bytes for treelabel
    uint32_t ciphertext_size = data_size + 8;
    sgx_status_t ret = SGX_SUCCESS;
    uint32_t ctr_inc_bits = 16;
    ret = sgx_aes_ctr_decrypt((const sgx_aes_ctr_128bit_key_t*)aes_key,
        encrypted_block_ptr,
        ciphertext_size,
        ctr,
        ctr_inc_bits,
        decrypted_block_ptr);
    free(ctr);
}

void aes_enc_serialized(unsigned char* decrypted_block, uint32_t data_size, unsigned char* encrypted_block, unsigned char* aes_key)
{
    //Add generate_randomness() for nonce.
    unsigned char* ctr = (unsigned char*)malloc(NONCE_LENGTH);
    memcpy(encrypted_block, ctr, NONCE_LENGTH);

    unsigned char* decrypted_block_ptr = decrypted_block + NONCE_LENGTH;
    unsigned char* encrypted_block_ptr = encrypted_block + NONCE_LENGTH;

    sgx_status_t ret = SGX_SUCCESS;
    uint32_t ctr_inc_bits = 16;

    // 8 from 4 bytes for id and 4 bytes for treelabel
    uint32_t input_size = data_size + 8;
    ret = sgx_aes_ctr_encrypt((const sgx_aes_ctr_128bit_key_t*)aes_key,
        decrypted_block_ptr,
        input_size,
        ctr,
        ctr_inc_bits,
        encrypted_block_ptr);

    free(ctr);
}
