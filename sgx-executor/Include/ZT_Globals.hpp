#ifndef __ZT_CONFIG__
#define __ZT_CONFIG__

#include <stdint.h>

// The PRM memory limit for Position Map of a ZeroTrace ORAM Instance in bytes.
// (The recursion levels of the ORAM is paramterized by this value)
#define MEM_POSMAP_LIMIT (8 * 1024 * 1024)

#define REVERSE_LEXICOGRAPHIC_EVICTION 1

#define ENCRYPTION_ON 1

#define HSORAM_OBLIVIOUS_TYPE_ORAM 1
#define HSORAM_RECURSION_DATA_SIZE 128

#define HSORAM_ORAM_TYPE 1
#define HSORAM_STASH_SIZE 100
#define HSORAM_Z 4

#define ADDITIONAL_METADATA_SIZE 24
#define HASH_LENGTH 32
#define NONCE_LENGTH 16
#define KEY_LENGTH 16
#define MILLION 1E6
#define IV_LENGTH 12
#define ID_SIZE_IN_BYTES 4
#define EC_KEY_SIZE 32
#define KEY_LENGTH 16
#define TAG_SIZE 16
#define CLOCKS_PER_MS (CLOCKS_PER_SEC / 1000)
#define AES_GCM_BLOCK_SIZE_IN_BYTES 16
#define PRIME256V1_KEY_SIZE 32

//Other Global variables
const char SHARED_AES_KEY[KEY_LENGTH] = {
    "AAAAAAAAAAAAAAA"
};
const char HARDCODED_IV[IV_LENGTH] = {
    "AAAAAAAAAAA"
};

enum LSORAM_STORAGE_MODES {
    INSIDE_PRM,
    OUTSIDE_PRM
};
enum LSORAM_OBLV_MODES {
    ACCESS_OBLV,
    FULL_OBLV
};
enum LSORAM_ERRORCODES {
    KEY_SIZE_OUT_OF_BOUND,
    VALUE_SIZE_OUT_OF_BOUND
};

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef struct {
    unsigned char* key;
    unsigned char* value;
} tuple;

typedef struct detailed_microbenchmarks {
    double posmap_time;
    double download_path_time;
    double fetch_block_time;
    double eviction_time;
    double upload_path_time;
    double total_time;
} det_mb;

//Inline Functions
inline uint32_t iBitsPrefix(uint32_t n, uint32_t w, uint32_t i)
{
    return (~((1 << (w - i)) - 1)) & n;
}

inline uint32_t ShiftBy(uint32_t n, uint32_t w)
{
    return (n >> w);
}

inline uint32_t noOfBitsIn(uint32_t local_deepest)
{
    uint32_t count = 0;
    while (local_deepest != 0) {
        local_deepest = local_deepest >> 1;
        count++;
    }
    return count;
}

inline bool isBlockDummy(unsigned char* serialized_block, uint64_t gN)
{
    bool dummy_flag = *((uint32_t*)(serialized_block + 16)) == gN;
    return dummy_flag;
}

inline uint32_t getId(unsigned char* serialized_block)
{
    uint32_t id = *((uint32_t*)(serialized_block + 16));
    return id;
}

inline uint32_t* getIdPtr(unsigned char* serialized_block)
{
    uint32_t* id = ((uint32_t*)(serialized_block + 16));
    return id;
}

inline void setId(unsigned char* serialized_block, uint32_t new_id)
{
    *((uint32_t*)(serialized_block + 16)) = new_id;
}

inline uint32_t getTreeLabel(unsigned char* serialized_block)
{
    uint32_t treeLabel = *((uint32_t*)(serialized_block + 20));
    return treeLabel;
}

inline uint32_t* getTreeLabelPtr(unsigned char* serialized_block)
{
    uint32_t* labelptr = ((uint32_t*)(serialized_block + 20));
    return labelptr;
}

inline void setTreeLabel(unsigned char* serialized_block, uint32_t new_treelabel)
{
    *((uint32_t*)(serialized_block + 20)) = new_treelabel;
}

inline unsigned char* getDataPtr(unsigned char* decrypted_path_ptr)
{
    return (unsigned char*)(decrypted_path_ptr + 24);
}

#define __GLOBALS__
#endif
