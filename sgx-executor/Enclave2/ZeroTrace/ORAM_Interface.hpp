#ifndef __ZT_ORAM_INTERFACE__

#include <stdint.h>

class ORAM_Interface {
    public:
    virtual void Create(uint32_t instance_id, uint8_t oram_type, uint8_t pZ, uint32_t max_blocks, uint32_t data_size, uint32_t stash_size, uint32_t oblivious_flag, uint32_t recursion_data_size, uint8_t recursion_levels) = 0;
    virtual void Access(uint32_t id, char opType, unsigned char * data_in, unsigned char * data_out) = 0;
};

#define __ZT_ORAM_INTERFACE__
#endif
