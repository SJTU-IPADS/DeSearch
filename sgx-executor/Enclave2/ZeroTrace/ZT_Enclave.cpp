#include "ZT_Enclave.hpp"

uint32_t getNewORAMInstanceID(uint8_t oram_type) {
    return coram_instance_id++;
}

uint8_t createNewORAMInstance(uint32_t instance_id, uint32_t max_blocks, uint32_t data_size, uint32_t stash_size, uint32_t oblivious_flag, uint32_t recursion_data_size, int8_t recursion_levels, uint8_t oram_type, uint8_t pZ) {

    CircuitORAM * new_coram_instance = new CircuitORAM();
    coram_instances.push_back(new_coram_instance);

    new_coram_instance -> Create(instance_id, oram_type, pZ, max_blocks, data_size, stash_size, oblivious_flag, recursion_data_size, recursion_levels);

    return 0;
}

void accessInterface(uint32_t instance_id, uint8_t oram_type, unsigned char *request, unsigned char *response, uint32_t request_size, uint32_t response_size) {
    //TODO : Would be nice to remove this dynamic allocation.
    CircuitORAM * coram_current_instance;

    unsigned char * data_in, * data_out;
    uint32_t id, opType;

    //Extract Request Id and OpType
    opType = request[0];
    unsigned char * request_ptr = request + 1;
    memcpy( & id, request_ptr, ID_SIZE_IN_BYTES);
    data_in = request_ptr + ID_SIZE_IN_BYTES;

    coram_current_instance = coram_instances[instance_id];
    coram_current_instance -> Access(id, opType, data_in, response);
}

void accessBulkReadInterface(uint32_t instance_id, uint8_t oram_type, uint32_t no_of_requests, unsigned char * request, unsigned char * response, uint32_t request_size, uint32_t response_size) {
    //TODO : Would be nice to remove this dynamic allocation.
    CircuitORAM * coram_current_instance;

    unsigned char * data_in, * request_ptr, * response_ptr;
    uint32_t id;
    char opType = 'r';

    uint32_t tdata_size;
    coram_current_instance = coram_instances[instance_id];
    tdata_size = coram_current_instance -> data_size;

    request_ptr = request;
    response_ptr = response;

    for (int l = 0; l < no_of_requests; l++) {
        // Extract Request Ids
        memcpy( & id, request_ptr, ID_SIZE_IN_BYTES);
        data_in = request_ptr + ID_SIZE_IN_BYTES;
        request_ptr += ID_SIZE_IN_BYTES;

        // TODO: Fix Instances issue.
        coram_current_instance -> Access(id, opType, data_in, response_ptr);
        response_ptr += (tdata_size);
    }
}
