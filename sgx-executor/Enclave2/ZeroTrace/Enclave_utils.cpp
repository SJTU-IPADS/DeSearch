#include "Enclave_utils.hpp"

void oarray_search(uint32_t* array, uint32_t loc, uint32_t* leaf, uint32_t newLabel, uint32_t N_level)
{
    for (uint32_t i = 0; i < N_level; i++) {
        omove(i, &(array[i]), loc, leaf, newLabel);
    }
    return;
}

void displayKey(unsigned char* key, uint32_t key_size)
{
    printf("<");
    for (int t = 0; t < key_size; t++) {
        char pc = 'A' + (key[t] % 26);
        printf("%c", pc);
    }
    printf(">\n");
}
