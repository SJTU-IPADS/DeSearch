#if 0
#include "enclave_common.hpp"

#include "sgx_eid.h"
#include "Enclave2_t.h"

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

void ecall_querier_setup(const char * b_index_input, const int b_index_input_size)
{
    // clear history
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
        _this_vec = q_index_tbl[q_wordID_list[i]];
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
            q_results_list.push_back(q_digest_tbl[*docID - 1]); // offset by one
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
#endif