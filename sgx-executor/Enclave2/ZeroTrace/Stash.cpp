#include "Stash.hpp"

// Oblivious Functions needed :
/*
1) omove_buffer()
2)

*/

// Other Functions needed :
/*
1) getID()
2) getDataPtr(), isBlockDummy, getLabel, 
3) ^ All of those permuting functions ! 
*/

Stash::Stash() { }

Stash::Stash(uint32_t param_stash_data_size, uint32_t param_STASH_SIZE, uint32_t param_gN)
{
    stash_data_size = param_stash_data_size;
    STASH_SIZE = param_STASH_SIZE;
    gN = param_gN;
}

void Stash::setParams(uint32_t param_stash_data_size, uint32_t param_STASH_SIZE, uint32_t param_gN)
{
    stash_data_size = param_stash_data_size;
    STASH_SIZE = param_STASH_SIZE;
    gN = param_gN;
}

void Stash::PerformAccessOperation(char opType, uint32_t id, uint32_t newleaf, unsigned char* data_in, unsigned char* data_out)
{
    struct nodev2* iter = getStart();
    uint8_t cntr = 1;
    uint32_t flag_id = 0, flag_w = 0, flag_r = 0;
    unsigned char* data_ptr;
    uint32_t* leaflabel_ptr;

#ifdef PAO_DEBUG
    bool flag_found = false;
#endif

#ifdef RESULTS_DEBUG
    printf("Before PerformAccess, datasize = %d, Fetched Data :", stash_data_size);
    for (uint32_t j = 0; j < stash_data_size; j++) {
        printf("%c", data_out[j]);
    }
    printf("\n");
#endif

    while (iter && cntr <= STASH_SIZE) {
        data_ptr = (unsigned char*)getDataPtr(iter->serialized_block);
        leaflabel_ptr = getTreeLabelPtr(iter->serialized_block);
        flag_id = (getId(iter->serialized_block) == id);
#ifdef PAO_DEBUG
        if (flag_id == true) {
            flag_found = true;
            printf("Found, has data: \n");
            unsigned char* data_ptr = getDataPtr(iter->serialized_block);
            for (uint32_t j = 0; j < stash_data_size; j++) {
                printf("%c", data_ptr[j]);
            }
            //setTreeLabel(iter->serialized_block,newleaf);
            printf("\n");
        }
#endif

        //Replace leaflabel in block with newleaf
        oassign_newlabel(leaflabel_ptr, newleaf, flag_id);
        //omove_buffer(leaflabel_ptr, &newleaf, ID_SIZE_IN_BYTES, flag_id);
        flag_w = (flag_id && opType == 'w');
        omove_buffer((unsigned char*)data_ptr, data_in, stash_data_size, flag_w);
        flag_r = (flag_id && opType == 'r');
#ifdef PAO_DEBUG
        if (flag_found) {
            printf("flag_id = %d, optype=%c, flag_r = %d\n", flag_id, opType, flag_r);
        }
#endif
        omove_buffer(data_out, (unsigned char*)data_ptr, stash_data_size, flag_r);

        iter = iter->next;
        cntr++;
    }
#ifdef RESULTS_DEBUG
    printf("After PerformAccess, datasize = %d, Fetched Data :", stash_data_size);
    for (uint32_t j = 0; j < stash_data_size; j++) {
        printf("%c", data_out[j]);
    }
    printf("\n");
#endif
#ifdef PAO_DEBUG
    if (flag_found == false) {
        printf("BLOCK NOT FOUND IN STASH\n");
    }
#endif
}

void Stash::ObliviousFillResultData(uint32_t id, unsigned char* result_data)
{
    struct nodev2* iter = getStart();
    uint8_t cntr = 1;
    uint32_t flag = 0;
    uint32_t* data_ptr;
    while (iter && cntr <= STASH_SIZE) {
        flag = (getId(iter->serialized_block) == id);
        data_ptr = (uint32_t*)getDataPtr(iter->serialized_block);
        omove_buffer(result_data, (unsigned char*)data_ptr, stash_data_size, flag);
        iter = iter->next;
        cntr++;
    }
}

uint32_t Stash::displayStashContents(uint64_t nlevel, bool recursive_block)
{
    uint32_t count = 0, cntr = 1;
    nodev2* iter = getStart();
    printf("Stash Contents : \n");
    while (iter && cntr <= STASH_SIZE) {
        unsigned char* tmp;
        if ((!isBlockDummy(iter->serialized_block, gN))) {
            printf("loc = %d, (%d,%d) : ", cntr, getId(iter->serialized_block), getTreeLabel(iter->serialized_block));
            tmp = iter->serialized_block + 24;
            uint32_t pbuckets = getTreeLabel(iter->serialized_block);
            count++;
            while (pbuckets >= 1) {
                printf("%d, ", pbuckets);
                pbuckets = pbuckets >> 1;
            }
            printf("\n");
            printf("Data: ");
            if (recursive_block) {
                uint32_t* data_ptr = (uint32_t*)tmp;
                for (uint32_t j = 0; j < stash_data_size / (sizeof(uint32_t)); j++) {
                    printf("%d,", *data_ptr);
                    data_ptr++;
                }
            } else {
                unsigned char* data_ptr = tmp;
                for (uint32_t j = 0; j < stash_data_size; j++) {
                    printf("%c", data_ptr[j]);
                }
            }
            printf("\n");
        }
        iter = iter->next;
        cntr++;
    }
    printf("\n");
    return count;
}

uint32_t Stash::stashOccupancy()
{
    uint32_t count = 0, cntr = 1;
    nodev2* iter = getStart();
    while (iter && cntr <= STASH_SIZE) {
        if ((!isBlockDummy(iter->serialized_block, gN))) {
            count++;
        }
        iter = iter->next;
        cntr++;
    }
    return count;
}

struct nodev2* Stash::getStart()
{
    return start;
}

void Stash::setStart(struct nodev2* new_start)
{
    start = new_start;
}

void Stash::setup(uint32_t pstash_size, uint32_t pdata_size, uint32_t pgN)
{
    gN = pgN;
    STASH_SIZE = pstash_size;
    stash_data_size = pdata_size;
    current_size = 0;
    for (uint32_t i = 0; i < STASH_SIZE; i++) {
        insert_new_block();
    }
}

void Stash::setup_nonoblivious(uint32_t pdata_size, uint32_t pgN)
{
    gN = pgN;
    stash_data_size = pdata_size;
}

void Stash::insert_new_block()
{
    Block block(stash_data_size, gN);
    struct nodev2* new_node = (struct nodev2*)malloc(sizeof(struct nodev2));

    if (current_size == STASH_SIZE) {
        printf("%d, Stash Overflow here\n", current_size);
    } else {
        unsigned char* serialized_block = block.serialize(stash_data_size);
        new_node->serialized_block = serialized_block;
        new_node->next = getStart();
        setStart(new_node);
        //printf("Inserted block (%d, %d) into stash\n", getId(serialized_block), getTreeLabel(serialized_block));
        current_size += 1;
    }
}

void Stash::remove(nodev2* ptr, nodev2* prev_ptr)
{
    nodev2* temp;
    if (prev_ptr == NULL && current_size == 1) {
        start = NULL;
    } else if (prev_ptr == NULL) {
        start = ptr->next;
    } else if (ptr->next == NULL) {
        prev_ptr->next = NULL;
    } else {
        prev_ptr->next = ptr->next;
    }

    free(ptr->serialized_block);
    free(ptr);
    current_size--;
}

void Stash::pass_insert(unsigned char* serialized_block, bool is_dummy)
{
    struct nodev2* iter = start;
    bool block_written = false;
    uint8_t cntr = 1;
#ifdef STASH_OVERFLOW_DEBUG
    bool inserted = false;
#endif
    while (iter && cntr <= STASH_SIZE) {
        bool flag = (!is_dummy && (isBlockDummy(iter->serialized_block, gN)) && !block_written);
#ifdef STASH_OVERFLOW_DEBUG
        inserted = inserted || flag;
#endif
        stash_serialized_insert(iter->serialized_block, serialized_block, stash_data_size, flag, &block_written);
        iter = iter->next;
        cntr++;
    }
#ifdef STASH_OVERFLOW_DEBUG
    if (!is_dummy && !inserted) {
        printf("STASH OVERFLOW \n");
    }
#endif
}

void Stash::insert(unsigned char* serialized_block)
{
    struct nodev2* new_node = (struct nodev2*)malloc(sizeof(struct nodev2));
    struct nodev2* temp_start = getStart();

    if (current_size == STASH_SIZE) {
        printf("Stash Overflow %s %d\n", __func__, __LINE__);
    } else {
        new_node->serialized_block = serialized_block;
        new_node->next = getStart();
        setStart(new_node);
        current_size += 1;
    }
}
