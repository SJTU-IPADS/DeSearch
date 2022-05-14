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
void ecall_build_index(
    void * index_inputs, int index_inputs_size,
    char * index_output, int * index_output_size,
    void * index_witness, int index_witness_size
);
#endif

static
void do_index(void)
{
    int int_current_epoch = get_epoch_int();
    if (-1 == int_current_epoch) return;
    if (int_current_epoch <= 1) { // only work when EPOCH >= 2
        return;
    }
    string current_epoch = to_string(int_current_epoch);

    vector <string> items_candidates;
    int items_pos = 1;

    for (int i = 1; i <= INDEX_POOL_MAX; ++i) {

        // must check if the index exists; skip ones that other did
        // format: INDEX-#EPOCH[n]-v[m]
        string index_key = "INDEX-#EPOCH" + current_epoch + "-v" + to_string(i);
        if (true == o_check_key(index_key)) {
            continue;
        }

        // [IN] get INDEX_BULK_MAX items from last epoch
        vector <string> inputs;
        for (int j = items_pos; j <= items_pos + INDEX_BULK_MAX-1; ++j) {
            string items_key = "ITEMS-#EPOCH" + to_string(int_current_epoch-1) + "-v" + to_string(j);

            int items_length = -1;
            char * items_value;
            o_get_key(items_key, items_value, items_length);
            items_value[items_length] = 0;

            inputs.push_back( items_value );

            delete items_value;
        }

        // [FUNC] tokenize and generate index for ITEMS_BULK_MAX items
#define INDEX_OUTPUT_MAX_LENGTH     (64*1024*1024) // 64MB
        int index_inputs_size = 0;
        char * index_inputs = Digest2Binary(inputs, index_inputs_size);

        char * index_output = new char [INDEX_OUTPUT_MAX_LENGTH];
        memset(index_output, 0x0, INDEX_OUTPUT_MAX_LENGTH);

        char * indexer_witness = new char [WITNESS_MAX_LENGTH];
        memset(indexer_witness, 0x0, WITNESS_MAX_LENGTH);

        int index_output_size = INDEX_OUTPUT_MAX_LENGTH;
        /*
        ecall_build_index(
            index_inputs, index_inputs_size,
            index_output, &index_output_size,
            indexer_witness, WITNESS_MAX_LENGTH);
        */
        status = Enclave2_ecall_build_index(e2_enclave_id,
            index_inputs, index_inputs_size,
            index_output, &index_output_size,
            indexer_witness, WITNESS_MAX_LENGTH
            );
        if (status != SGX_SUCCESS) {
            printf("Enclave2_ecall_parse_items failed: Error code is %x\n", status);
        }

        // [OUT] upload index and corresponding witnesses to kanban
        o_put_key(index_key, index_output, index_output_size);
        o_append_witness(current_epoch, indexer_witness);

        // extra step: init global index when EPOCH == 2
        if (2 == int_current_epoch) { // copy
            string global_index_key = "GLOBAL-INDEX-#EPOCH" + current_epoch + "-v" + to_string(i);
            o_put_key(global_index_key, index_output, index_output_size);
        }

        delete index_output, indexer_witness;

        items_pos += INDEX_BULK_MAX;
    }
}

// an indexer executor simply runs as a daemon
void indexer_loop(void)
{
    for (;;) {
        do_index();
        sleep(1);
    }
}