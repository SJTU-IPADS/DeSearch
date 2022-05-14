#include "enclave_common.hpp"

#include "sgx_eid.h"
#include "Enclave2_t.h"

#define USE_ORAM

#if 0
// [IN] query + index ---> [OUT] answer
static
void create_querier_witness(const string &query, const void *index, size_t index_sz,
    const string &output, string &q_witness)
{
    string o_witness;
    witness_context querier_context;
    witness_init(querier_context, "STEEMIT QUERIER");

    witness_input(querier_context, index, index_sz);
    witness_input(querier_context, query.data(), query.length());

    witness_output(querier_context, output.data(), output.length());
    witness_func(querier_context, QUERIER_TAG, sizeof(QUERIER_TAG));
    witness_finalize(querier_context, o_witness);

    q_witness = o_witness;
}
#endif

// exception is that querier is (somewhat) stateful
// to avoid always re-hashing the input from scratch
// we compose the querier's witness using 2-stage
static witness_context querier_context_template;

// in-enclave data structures
static vector <string> q_digest_tbl;
static unordered_map <DWORD, vector <DWORD> > q_index_tbl;

//////////////////////////////////////////////////////////////////////////

#ifdef USE_ORAM

#include "ZT_Globals.hpp"

// For ORAM setup, we ensure all blocks in the same size
// hence we can concatenate two digests in one ORAM block

#define DIGEST_LENGTH (512) // one ORAM block for two digests
#define DIGEST_BLOCK_NUM (1024 * 16)

#define INDEX_BLOCK_SIZE (512)
#define INDEX_BLOCK_NUM (1024 * 8)

uint32_t digest_oram, index_oram;

#include "ZT_Wrapper.hpp"

// a compressed index using VLB encoding
static unordered_map<DWORD, _VLB_coder> vlb_index_tbl;

// linear ORAM metadata
static unordered_map <DWORD, DWORD> g_offset; // wordID -> offset
static unordered_map <DWORD, DWORD> g_length; // wordID -> length
static unordered_map <DWORD, unsigned char *> g_VLB_buffer; // wordID -> VLB blob ptr

int create_digest_oram()
{
    unsigned char data_in[DIGEST_LENGTH] = "";
    unsigned char data_out[DIGEST_LENGTH] = "";

    int8_t recursion_levels = computeRecursionLevels(DIGEST_BLOCK_NUM, 128, MEM_POSMAP_LIMIT);

    digest_oram = getNewORAMInstanceID(1); // CO-ORAM

    createNewORAMInstance(digest_oram, DIGEST_BLOCK_NUM /*maxBlocks*/, DIGEST_LENGTH /*dataSize*/, 10 /*stashSize*/, 1 /*oblivious_flag*/, 128 /*recursion_data_size*/, recursion_levels, 1 /*oram_type*/, 2 /*pZ*/);

    int block_id = 0;
    for (block_id = 0; block_id < q_digest_tbl.size(); ++block_id) {
        if (block_id % 2 == 0) {
            memcpy(data_in, (unsigned char *)q_digest_tbl[block_id].data(), DIGEST_LENGTH/2);
        }
        else if (block_id % 2 == 1) {
            memcpy(data_in + DIGEST_LENGTH/2, (unsigned char *)q_digest_tbl[block_id].data(), DIGEST_LENGTH/2);
            myZT_Access(digest_oram, block_id/2, 'w', data_in, data_out, DIGEST_LENGTH);
        }
    }
    myZT_Access(digest_oram, block_id/2, 'w', data_in, data_out, DIGEST_LENGTH);

    return 0;
}

/*
 * [wordID] --> [blockID] --> [index blob]
 */
int create_index_oram()
{
    int total_length = 0;

    // step-1: malshall all VLB indexes to blobs, then Zlib them
    for (auto i = vlb_index_tbl.begin(); i != vlb_index_tbl.end(); ++i) {

        // from VLB to buffer
        int index_size = 0;
        unsigned char * index_buf = marshall_VLB_to_buffer(i->second, index_size);

        // bookkeeping
        g_VLB_buffer[i->first] = index_buf;

        g_offset[i->first] = total_length;
        g_length[i->first] = index_size;
        
        total_length += index_size;
    }

    printf(">>> Total ORAM Block Size: %ld\n", total_length);
    
    // step-2: start to copy to ORAM blocks
    // a linear buffer
    unsigned char *temp = (unsigned char *)calloc(1, total_length + 1);
    int offset = 0;
    for (auto i = vlb_index_tbl.begin(); i != vlb_index_tbl.end(); ++i) {
        auto blob = g_VLB_buffer[i->first];
        memcpy(temp + offset, blob, g_length[i->first]);
        free(blob);
        offset += g_length[i->first];
    }
    
    int8_t recursion_levels = computeRecursionLevels(INDEX_BLOCK_NUM, 128, MEM_POSMAP_LIMIT);
    index_oram = getNewORAMInstanceID(1); // CO-ORAM
    createNewORAMInstance(index_oram, INDEX_BLOCK_NUM/*maxBlocks*/, INDEX_BLOCK_SIZE /*dataSize*/, 10 /*stashSize*/, 1 /*oblivious_flag*/, 128 /*recursion_data_size*/, recursion_levels, 1 /*oram_type*/, 2 /*pZ*/);

    int block_id = 0;
    unsigned char *block_ptr = temp;
    unsigned char data_out[INDEX_BLOCK_SIZE] = "";
    for (int i = 0; i <= total_length; i += INDEX_BLOCK_SIZE) {
        myZT_Access(index_oram, block_id, 'w', block_ptr, data_out, INDEX_BLOCK_SIZE);
        block_id ++;
        block_ptr += INDEX_BLOCK_SIZE;
    }

    free(temp);
    g_VLB_buffer.clear();

    return 0;
}
#endif

//////////////////////////////////////////////////////////////////////////

void ecall_querier_setup(const char * b_index_input, const int b_index_input_size)
{
    // clear all history
    g_offset.clear();
    g_length.clear();
    g_VLB_buffer.clear();

    q_digest_tbl.clear();
    q_index_tbl.clear();

    // retrieve index len
    int b_index_size = 0;
    memcpy(&b_index_size, b_index_input, sizeof(int));

    // retrieve index
    Binary2Index(b_index_input + sizeof(int), b_index_size, q_index_tbl);

    // retrieve digest
    int b_digest_size = b_index_input_size - ( sizeof(int) + b_index_size );
    Binary2Digest(b_index_input + sizeof(int) + b_index_size, b_digest_size, q_digest_tbl);

    // XXX: WITNESS: Part-1
    witness_clear(querier_context_template); // refresh the witness template
    witness_init(querier_context_template, "STEEMIT QUERIER");
    witness_input(querier_context_template, b_index_input, b_index_input_size);

    // dump_digest(q_digest_tbl);
    // dump_index(q_index_tbl);

#ifdef USE_ORAM

    // transform index table into a VLB style
    for (auto i = q_index_tbl.begin(); i != q_index_tbl.end(); ++i) {
        int wordID = i->first;

        vector<DWORD> docID_list = i->second;
        for (DWORD const &docID : docID_list) {
            (vlb_index_tbl[wordID]).Zip(docID); // append to the docID list
        }
    }

    create_digest_oram();
    create_index_oram();
    // release all digests and indexes to save memory
    q_digest_tbl.clear();
    q_index_tbl.clear();
    vlb_index_tbl.clear();
    printf(">>> enable ORAM-based digest and index\n");

#endif
}

// answer query
void ecall_do_query(
    const char * querier_request, int querier_request_size,
    char * querier_response, int querier_response_size)
{
    // STEP 0: parse the JSON request, split keywords and page number
    string s_request = querier_request;
    JSON j_keywords = JSON::Load(s_request)["keywords"];
    JSON j_page = JSON::Load(s_request)["page"];
    string q_keywords = j_keywords.dump();
    string q_page = trim_string(j_page.dump());

    // check if valid inputs
    if (0 == q_keywords.size() || 0 == q_page.size()) return;
    int page = strtol(q_page.data(), nullptr, 10);

    string q = q_keywords;
    vector<DWORD> q_wordID_list;

    // STEP 1: tokenize the query words and CRC32 them into wordID
    ISphTokenizer * pTok = CreateTokenizer();
    pTok->SetBuffer ( (BYTE*)q.data(), q.length() );

    BYTE * sTok;
    while ( ( sTok = pTok->GetToken() )!=NULL ) {
        DWORD wordID = sphCRC32( (char*)sTok, strlen((char*)sTok ));
        q_wordID_list.push_back(wordID);
    }
    SafeDelete ( pTok );

    // only serve 32 keywords at most (Google's maximum)
    if (q_wordID_list.size() > 32) {
        q_wordID_list.resize(32);
    }

    // STEP 2: retrieve corresponding docID lists
    vector <vector <DWORD> > docID_lists;
    vector <DWORD> _this_vec;
    for (int i = 0; i < q_wordID_list.size(); ++i) {
#ifdef USE_ORAM

        _this_vec.clear();

        int offset = g_offset[ q_wordID_list[i] ];
        int length = g_length[ q_wordID_list[i] ];
        
        if (length) {
        
            int block_start = offset / INDEX_BLOCK_SIZE;
            int block_offset = offset % INDEX_BLOCK_SIZE;
            int block_end = (offset + length) / INDEX_BLOCK_SIZE;

            unsigned char data_in[INDEX_BLOCK_SIZE] = "";
            unsigned char *data_out = (unsigned char *)malloc( INDEX_BLOCK_SIZE*(block_end-block_start+1) );
            unsigned char *temp = data_out;

            // retrieve the block list
            for (int block_id = block_start; block_id <= block_end; block_id++) {
                myZT_Access(index_oram, block_id, 'r', data_in, temp, INDEX_BLOCK_SIZE);
                temp += INDEX_BLOCK_SIZE;
            }
            VLB_coder VLB;
            unmarshall_buffer_VLB(data_out + block_offset, length, VLB);
            Dump_VLB(VLB, _this_vec);
            
            free(data_out);
        
        }

#else
        _this_vec = q_index_tbl[q_wordID_list[i]];
#endif
        docID_lists.push_back(_this_vec);
    }

    // STEP 3: do intersection of multiple vectors
    for (int i = 0; i < docID_lists.size(); ++i) {
        if (i > 0) {
            _this_vec = intersection( docID_lists[i-1], docID_lists[i] );
        } else {
            _this_vec = docID_lists[0];
        }
    }

    // STEP 4: return most 10 as final results
    int left = 10;
    vector <string> q_results_list;
    if (_this_vec.begin() + (page-1)*10 <  _this_vec.end()) {
        for (auto docID = _this_vec.begin() + (page-1)*10; docID != _this_vec.end(); ++docID) {
#ifdef USE_ORAM

        uint32_t block_id = (*docID - 1);  // offset by one

        unsigned char data_in[DIGEST_LENGTH] = "";
        unsigned char data_out[DIGEST_LENGTH + 1] = "";

        myZT_Access(digest_oram, block_id/2, 'r', data_in, data_out, DIGEST_LENGTH);

        unsigned char data_result[DIGEST_LENGTH/2 + 1] = "";

        if (block_id % 2 == 0) {
            memcpy(data_result, data_out, DIGEST_LENGTH/2);
        }
        else if (block_id % 2 == 1) {
            memcpy(data_result, data_out + DIGEST_LENGTH/2, DIGEST_LENGTH/2);
        }

        string temp = (char * ) data_result;
        q_results_list.push_back(temp);

#else
            q_results_list.push_back(q_digest_tbl[*docID - 1]); // offset by one
#endif
            if (0 == --left) break;
        }
    }

    // STEP 5: add dummy results for exactly 10 results
    string dummy;
    dummy.resize(DIGEST_MAX_LENGTH, 'X');
    while (left--) {
        q_results_list.push_back(dummy);
    }

    // compose a JSON style result list
    JSON response1;
    response1["results"] = json::Array();
    for (int i = 0; i < 10; i++) {
        JSON result;
        result["number"] = to_string(i + 1);
        result["answer"] = q_results_list[i];
        response1["results"].append(result);
    }

    // WITNESS: Part-2
    string querier_witness;
    witness_context querier_context = querier_context_template; // copy on write
    witness_input(querier_context, querier_request, querier_request_size);
    witness_output(querier_context, response1.dump().data(), response1.dump().size());
    witness_func(querier_context, QUERIER_TAG, sizeof(QUERIER_TAG));
    witness_finalize(querier_context, querier_witness);

    // compose the answer and witness together
    JSON final_response;
    final_response["response"] = response1;
    final_response["witness"] = JSON::Load(querier_witness);
    string final_all = final_response.dump();

    // make the results visible to outside
    if (querier_response_size > final_all.size()) {
        memcpy(querier_response, final_all.data(), final_all.size());
    }
}