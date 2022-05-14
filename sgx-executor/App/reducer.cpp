#include "common.hpp"

#include "../Enclave1/Enclave1_u.h"
#include "../Enclave2/Enclave2_u.h"
#include "../Enclave3/Enclave3_u.h"

#include "sgx_eid.h"
#include "sgx_urts.h"

static sgx_status_t status;

extern sgx_enclave_id_t e1_enclave_id;
extern sgx_enclave_id_t e2_enclave_id;
extern sgx_enclave_id_t e3_enclave_id;

#if 0
void ecall_do_merge(
    const char * index1, int sz_1,
    const char * index2, int sz_2,
    char * new_index, int * new_sz,
    void * reducer_witness, int reducer_witness_size
);
#endif

// Reducer merges a new index to an old one
static
void do_reduce(void)
{
    int int_current_epoch = get_epoch_int();
    if (-1 == int_current_epoch) return;
    if (int_current_epoch <= 2) return; // only work when EPOCH == 3
    string current_epoch = to_string(int_current_epoch);

    char *global_index = nullptr, *new_index = nullptr, *new_global_index = nullptr;
    int global_index_sz = 0, new_index_sz = 0, new_global_index_sz = 0;

    for (int i = 1; i <= INDEX_POOL_MAX; ++i) {

        // first check whether the index key exists; if yes, skip it, cuz others did this
        string new_global_index_key = "GLOBAL-INDEX-#EPOCH" + current_epoch + "-v" + to_string(i);
        if (true == o_check_key(new_global_index_key)) {
            continue;
        }

        // get global index of last epoch
        string global_index_key = "GLOBAL-INDEX-#EPOCH" + to_string(int_current_epoch-1) + "-v" + to_string(i);
        o_get_key(global_index_key, global_index, global_index_sz);

        // get new index
        string new_index_key = "INDEX-#EPOCH" + current_epoch + "-v" + to_string(i);
        o_get_key(new_index_key, new_index, new_index_sz);

        // [FUNC] merge these two, new_global_index <- global_index + new_index
#define REDUCER_OUTPUT_MAX_LENGTH     (256*1024*1024) // 256MB

        char * reducer_output = new char [REDUCER_OUTPUT_MAX_LENGTH];
        memset(reducer_output, 0x0, REDUCER_OUTPUT_MAX_LENGTH);
        new_global_index = reducer_output;
        new_global_index_sz = REDUCER_OUTPUT_MAX_LENGTH;

        char * reducer_witness = new char [WITNESS_MAX_LENGTH];
        memset(reducer_witness, 0x0, WITNESS_MAX_LENGTH);

        /*
        ecall_do_merge(global_index, global_index_sz, new_index, new_index_sz,
            new_global_index, &new_global_index_sz, reducer_witness, WITNESS_MAX_LENGTH);
        */
        status = Enclave2_ecall_do_merge(e2_enclave_id,
            global_index, global_index_sz,
            new_index, new_index_sz,
            new_global_index, &new_global_index_sz,
            reducer_witness, WITNESS_MAX_LENGTH
            );
        if (status != SGX_SUCCESS) {
            printf("Enclave2_ecall_do_merge failed: Error code is %x\n", status);
        }
        printf("===== %s %d %d %d\n", global_index_key.data(), global_index_sz, new_index_sz, new_global_index_sz);

        // [OUT] upload global index and corresponding witnesses to kanban
        o_put_key(new_global_index_key, new_global_index, new_global_index_sz);
        o_append_witness(current_epoch, reducer_witness);

        global_index_sz = new_index_sz = new_global_index_sz = 0;
        delete global_index, new_index, new_global_index;
    }
}

// a reducer executor simply runs as a daemon
void reducer_loop(void)
{
    for (;;) {
        do_reduce();
        sleep(1);
    }
}