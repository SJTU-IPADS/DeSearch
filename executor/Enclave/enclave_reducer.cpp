#include "enclave_common.hpp"

static
void create_reducer_witness(
    const void * g_index, size_t g_index_sz,
    const void * new_index, size_t new_index_sz,
    const void * g_new_index, size_t g_new_index_sz,
    string &o_witness)
{
    witness_context reducer_context;
    witness_init(reducer_context, "STEEMIT REDUCER");

    witness_input(reducer_context, g_index, g_index_sz);
    witness_input(reducer_context, new_index, new_index_sz);

    witness_output(reducer_context, g_new_index, g_new_index_sz);
    witness_func(reducer_context, REDUCER_TAG, sizeof(REDUCER_TAG));
    witness_finalize(reducer_context, o_witness);
}

// [IN] global index + new index ---> [OUT] new global index
void ecall_do_merge(
    const char * index1, int sz_1,
    const char * index2, int sz_2,
    char * new_index, int * new_sz,
    void * reducer_witness, int reducer_witness_size)
{
    vector <string> digest_tbl1, digest_tbl2;
    unordered_map <DWORD, vector <DWORD> > index_tbl1, index_tbl2;

    // retrieve internal index length
    int index_sz1 = 0, index_sz2 = 0;
    memcpy(&index_sz1, index1, sizeof(int));
    memcpy(&index_sz2, index2, sizeof(int));

    // retrieve internal index
    Binary2Index(index1 + sizeof(int), index_sz1, index_tbl1);
    Binary2Index(index2 + sizeof(int), index_sz2, index_tbl2);

    // retrieve intenal digest
    int digest_sz1 = sz_1 - ( sizeof(int) + index_sz1 );
    int digest_sz2 = sz_2 - ( sizeof(int) + index_sz2 );
    Binary2Digest(index1 + sizeof(int) + index_sz1, digest_sz1, digest_tbl1);
    Binary2Digest(index2 + sizeof(int) + index_sz2, digest_sz2, digest_tbl2);

    // merge digests
    vector <string> new_digest_tbl;
    new_digest_tbl.insert(new_digest_tbl.end(), digest_tbl1.begin(), digest_tbl1.end());
    new_digest_tbl.insert(new_digest_tbl.end(), digest_tbl2.begin(), digest_tbl2.end());
    // dump_digest(new_digest_tbl);

    // merge indexes
    unordered_map <DWORD, vector <DWORD> > new_index_tbl;
    new_index_tbl = index_tbl1;
    int offset = digest_tbl1.size();
    for (auto & index: index_tbl2) {
        int wordID = index.first;
        for (auto & docID : index.second) {
            new_index_tbl[wordID].push_back(docID + offset);
        }
    }
    // dump_index(new_index_tbl);

    int _new_sz = 0;
    char *_new_index = Merge2Binary(new_digest_tbl, new_index_tbl, &_new_sz);

    string witness;
    create_reducer_witness(index1, sz_1, index2, sz_2, _new_index, _new_sz, witness);

    // make the results visible to outside
    if (*new_sz > _new_sz) {
        memcpy(new_index, _new_index, _new_sz);
    }
    if (reducer_witness_size > witness.size()) {
        memcpy(reducer_witness, witness.data(), witness.size());
    }
    *new_sz = _new_sz;
    delete _new_index;
}