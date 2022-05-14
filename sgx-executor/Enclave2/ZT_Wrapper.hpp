static inline
uint8_t computeRecursionLevels(uint32_t max_blocks, uint32_t recursion_data_size, uint64_t onchip_posmap_memory_limit) {
    uint8_t recursion_levels = 1;
    uint8_t x;

    if (recursion_data_size != 0) {
        recursion_levels = 1;
        x = recursion_data_size / sizeof(uint32_t);
        uint64_t size_pmap0 = max_blocks * sizeof(uint32_t);
        uint64_t cur_pmap0_blocks = max_blocks;

        while (size_pmap0 > onchip_posmap_memory_limit) {
            cur_pmap0_blocks = (uint64_t) ceil((double) cur_pmap0_blocks / (double) x);
            recursion_levels++;
            size_pmap0 = cur_pmap0_blocks * sizeof(uint32_t);
        }

    }
    return recursion_levels;
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

static inline
void ZT_Access(uint32_t instance_id, uint8_t oram_type, unsigned char * request, unsigned char * response, uint32_t request_size, uint32_t response_size) {
    accessInterface(instance_id, oram_type, request, response, request_size, response_size);
}

static
void myZT_Access(uint32_t instance_id, uint32_t block_id, char op_type, unsigned char * data_in, unsigned char * data_out, uint32_t block_size) {

    // Prepare Request:
    uint32_t request_size = 1 + ID_SIZE_IN_BYTES + block_size;
    unsigned char serialized_request[request_size];
    myZT_serializeRequest(block_id, op_type, data_in, block_size, serialized_request);

    // Process Request:
    uint32_t response_size = block_size;
    unsigned char *response = data_out;
    ZT_Access(instance_id, 1, serialized_request, response, request_size, response_size);

}