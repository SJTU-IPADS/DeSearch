#include "ZT_Globals.hpp"

#include "ZT.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <cstdint>
#include <random>
#include <vector>
#include <algorithm>

class myZT {
    public:
    uint32_t DATA_SIZE;
    uint32_t MAX_BLOCKS;

    uint32_t STASH_SIZE;
    uint32_t OBLIVIOUS_FLAG;
    uint32_t RECURSION_DATA_SIZE;
    uint32_t ORAM_TYPE;

    uint8_t Z;

    myZT();
    uint32_t myZT_New(uint32_t data_size, uint32_t block_size);
    void myZT_Access(uint32_t instance_id, uint32_t block_id, char op_type, unsigned char * data_in, unsigned char * data_out, uint32_t block_size);
    void myZT_Bulk_Access(uint32_t instance_id, uint32_t * block_list, uint32_t batch_size, unsigned char * data_in, unsigned char * data_out, uint32_t block_size);

};

myZT::myZT() {
    // params
    STASH_SIZE = 10;
    OBLIVIOUS_FLAG = 1;
    RECURSION_DATA_SIZE = 128;
    // path
    ORAM_TYPE = 1;
    Z = 2;
}

uint32_t myZT::myZT_New(uint32_t data_size, uint32_t block_size) {
    return ZT_New(block_size, data_size, STASH_SIZE, OBLIVIOUS_FLAG, RECURSION_DATA_SIZE, ORAM_TYPE, Z);
}

static inline
void myZT_serializeRequest(uint32_t request_id, char op_type, unsigned char * data, uint32_t data_size, unsigned char * serialized_request) {
    unsigned char * request_ptr = serialized_request;
    * request_ptr = op_type;
    request_ptr += 1;
    memcpy(request_ptr, (unsigned char * ) & request_id, ID_SIZE_IN_BYTES);
    request_ptr += ID_SIZE_IN_BYTES;
    memcpy(request_ptr, data, data_size);
}

void myZT::myZT_Access(uint32_t instance_id, uint32_t block_id, char op_type, unsigned char * data_in, unsigned char * data_out, uint32_t block_size) {

    // Prepare Request:
    uint32_t request_size = 1 + ID_SIZE_IN_BYTES + block_size;
    unsigned char serialized_request[request_size];
    myZT_serializeRequest(block_id, op_type, data_in, block_size, serialized_request);

    // Process Request:
    uint32_t response_size = block_size;
    unsigned char *response = data_out;
    ZT_Access(instance_id, ORAM_TYPE, serialized_request, response, request_size, response_size);

}

void myZT::myZT_Bulk_Access(uint32_t instance_id, uint32_t * block_list, uint32_t batch_size, unsigned char * data_in, unsigned char * data_out, uint32_t block_size) {

    // Prepare Request:
    uint32_t req_counter = 0;
    uint32_t request_size = block_size * batch_size;
    
    unsigned char serialized_request[batch_size * ID_SIZE_IN_BYTES];
    unsigned char * serialized_request_ptr = serialized_request;

    for (int i = 0; i < batch_size; i++) {
        memcpy(serialized_request_ptr, & (block_list[req_counter + i]), ID_SIZE_IN_BYTES);
        serialized_request_ptr += ID_SIZE_IN_BYTES;
    }

    // Process Request:
    uint32_t response_size = block_size * batch_size;
    unsigned char *response = data_out;
    ZT_Bulk_Read(instance_id, ORAM_TYPE, batch_size, serialized_request, response, request_size, response_size);

}
