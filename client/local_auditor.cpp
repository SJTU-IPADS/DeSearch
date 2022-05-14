#include <set>
#include <string>
#include <stdint.h>

#include "config.hpp"

// hardcoded public keys
uint8_t pkey[32] = {
    0xd7, 0x5a, 0x98, 0x01, 0x82, 0xb1, 0x0a, 0xb7,
    0xd5, 0x4b, 0xfe, 0xd3, 0xc9, 0x64, 0x07, 0x3a,
    0x0e, 0xe1, 0x72, 0xf3, 0xda, 0xa6, 0x23, 0x25,
    0xaf, 0x02, 0x1a, 0x68, 0xf7, 0x07, 0x51, 0x1a,
};

using namespace std;

#include "utility.hpp"

// a process to demonstrate how to verify the whole witness tree
// in a WAN setup, a user only sends its IN to an external auditor
// or otherwise the searched keywords will be leaked

static vector <string> verify_cache;
static int verify_epoch;

// TODO: use incremental computation when invalidating cache
void do_local_verify(const string & needs_to_verify, string & verify_results)
{
    string current_epoch = get_epoch();
    int current_epoch_int = get_epoch_int();
    if (current_epoch_int < 3)
        return;

    // STEP-0: check if cache hit
    vector <string> scratch_pad;
    JSON q_in = JSON::Load(needs_to_verify)["IN"];
    for (auto & i : q_in.ArrayRange()) {
        scratch_pad.push_back(i.ToString());
    }
    int cnt_v = 0;
    for (auto & s : scratch_pad) {
        if (find(verify_cache.begin(), verify_cache.end(), s) != verify_cache.end())
            cnt_v ++;
    }
    if (get_epoch_int() == verify_epoch && 0 != verify_cache.size() && verify_cache.size() == cnt_v) {
        verify_results = "epoch " + current_epoch + ": end-to-end witness (completeness + freshness) verification pass";
        return;
    }
    if (get_epoch_int() != verify_epoch) verify_cache.clear();

    vector <string> tmp_verify_cache;
    tmp_verify_cache.assign(scratch_pad.begin(), scratch_pad.end());

    // STEP-1: download all witnesses and check authenticity one by one
    unordered_map <string, string> kb_witness;
    kb.hgetall("witness", ::inserter(kb_witness, kb_witness.begin()));

    for (auto & w : kb_witness) {
        if (false == check_witness(w.second)) {
            kb_witness.erase(kb_witness.find(w.first)); // delete illegal ones
            printf("fraud witness detected: %s\n", w.first.data());
        }
        // printf("%s\n", w.second.data());
    }

    // STEP-2: traverse the witness tree from [OUT] to [IN], collect all tree leaves
    set <string> leaves_set;
    while (scratch_pad.size() != 0) {
        string top = scratch_pad.back();
        scratch_pad.pop_back();
        if (kb_witness.find(top) == kb_witness.end()) {
            leaves_set.insert(top);
        } else {
            string parent = kb_witness[top];
            JSON q_in = JSON::Load(parent)["IN"];
            for (auto & i : q_in.ArrayRange()) {
                scratch_pad.push_back(i.ToString());
            }
        }
    }

    // STEP-3: download all root hashes and select witnesses whose keys are well
    set <string> witness_keys;
    // NOTE: we can only search 2 EPOCHes ago
    for (int e = 1; e <= current_epoch_int - 2; ++e) {
        // use root hash to check if they are tamper-resistant
        if (false == check_root_hash(e)) continue;
        string epoch_witness_key = "WITNESS-#EPOCH" + to_string(e);
        kb.smembers(epoch_witness_key, ::inserter(witness_keys, witness_keys.begin()));
    }

    unordered_map <string, string> well_witness;
    for (auto & k : witness_keys) {
        well_witness[k] = kb_witness[k];
    }

    // STEP-4: select whose [FUNC] are crawler
    set <string> items_set;
    char func[65] = "";
    sha256_string(CRAWLER_TAG, sizeof(CRAWLER_TAG), func);
    for (auto & k : well_witness) {
        string w = k.second; // get the witness string part
        if (w.find(func) != std::string::npos) {
            JSON jw = JSON::Load(w);
            JSON in = jw["IN"];
            for (auto & i : in.ArrayRange()) {
                items_set.insert(i.ToString());
            }
        }
    }

    // STEP-5: compare these two sets
    vector<string> v_diff;
    set_difference(items_set.begin(), items_set.end(),
                    leaves_set.begin(), leaves_set.end(), back_inserter(v_diff));

#if 0 // debug only
    cout << "expected set : " << endl;
    for (auto x : items_set) cout << x << endl;
    cout << "retrieved set : " << endl;
    for (auto y : leaves_set) cout << y << endl;
#endif

    verify_results = "epoch " + current_epoch + ( (0 == v_diff.size()) ?
        ": end-to-end witness (completeness + freshness) verification pass" :
        ": end-to-end witness (completeness + freshness) verification fail" );

    // STEP-6: assign to the global cache for a fast path if possible
    if (0 == v_diff.size()) {
        verify_cache.assign(tmp_verify_cache.begin(), tmp_verify_cache.end());
        verify_epoch = get_epoch_int();
    }
}
