#include "enclave_common.hpp"

#include "sgx_eid.h"
#include "Enclave2_t.h"

// in-enclave item list
static vector <string> item_candidates;

// [IN] items ---> [OUT] documents
static
void create_cralwer_witness(const vector<string> &inputs,
    const string &output, string &witness)
{
    witness_context crawler_context;
    witness_init(crawler_context, "STEEMIT CRAWLER");

    for (int pos = 0; pos < inputs.size(); pos++) {
        witness_input(crawler_context, inputs[pos].data(), inputs[pos].length());
    }
    witness_output(crawler_context, output.data(), output.length());
    witness_func(crawler_context, CRAWLER_TAG, sizeof(CRAWLER_TAG));
    witness_finalize(crawler_context, witness);
}

// User Defined Functions: parsing items

static
void parse_fake_item(const vector <string> & inputs, string & output)
{
    int pos = 0;
    JSON item_arr;
    for ( auto & item : inputs ) {
        item_arr[pos++] = item;
    }
    output = item_arr.dump();
}

static
void parse_steemit_item(const vector <string> & inputs, string & output)
{
    // parse the steemit block transactions
    vector <string> post_candidates;
    for ( auto & _item : inputs) {
        JSON item = JSON::Load(_item);
        JSON transactions = item["result"]["block"]["transactions"];
        for ( auto & txn : transactions.ArrayRange() ) {
            JSON op = txn["operations"];
            for ( auto & post : op.ArrayRange() ) {
                if ("comment_operation" == post["type"].ToString()) {
                    string one_post = post["value"]["title"].ToString();
                    one_post += "  " + post["value"]["body"].ToString();
                    // cout << one_post << "\n";
                    post_candidates.push_back(one_post);
                }
            }
        }
    }

    // trick: if we get zero documents from the Steemit blocks, then we use a hash string
    // that is pointed to the block contents, which will be hardly queried
    // if we use a constant string, there will be circular witness graph that's hard to parse
    // if we use a random string, it will introduce non-determinism
    if (0 == post_candidates.size()) {
        JSON item = JSON::Load(inputs[0]); // only choose the 1st item
        char _sha256[65] = "";
        uint8_t output_hash[SGX_SHA256_HASH_SIZE];
        sgx_sha256_msg((const uint8_t *)item.dump().data(), item.dump().size(), (sgx_sha256_hash_t*)&output_hash[0]);
        sgx_sha256_string(output_hash, _sha256);
        string s_sha256 = _sha256;
        post_candidates.push_back(s_sha256);
    }

    int post_i = 0;
    JSON post_arr;
    for ( auto & post : post_candidates ) {
        post_arr[post_i++] = post;
    }
    output = post_arr.dump();
}

// items:string[] => document:string
void ecall_parse_items(
    void * app_input, int app_input_size,
    void * app_output, int app_output_size,
    void * app_witness, int app_witness_size
    )
{
    string output, witness;

    // recover input list
    Binary2Digest((const char *)app_input, app_input_size, item_candidates);

#ifdef USE_STEEMIT_ITEM
    parse_steemit_item(item_candidates, output);
#else
    parse_fake_item(item_candidates, output);
#endif

    create_cralwer_witness(item_candidates, output, witness);

    // make the results visible to outside
    if (app_output_size > output.size()) {
        memcpy(app_output, output.data(), output.size());
    }
    if (app_witness_size > witness.size()) {
        memcpy(app_witness, witness.data(), witness.size());
    }

    // stateless
    item_candidates.clear();
}
