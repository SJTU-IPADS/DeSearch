#include <set>
#include <string>
#include <unistd.h>
#include <stdint.h>
#include <iostream>
#include <algorithm>

#include "config.hpp"
#include "manager.hpp"
#include "utility.hpp"

using namespace std;

#include "json.hpp"
using json::JSON;

#include <sw/redis++/redis++.h>
using namespace sw::redis;

// hard coded Curve 25519 secret key
uint8_t skey[32] = {
    0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60,
    0xba, 0x84, 0x4a, 0xf4, 0x92, 0xec, 0x2c, 0xc4,
    0x44, 0x49, 0xc5, 0x69, 0x7b, 0x32, 0x69, 0x19,
    0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60,
};

// hard coded public keys
uint8_t pkey[32] = {
    0xd7, 0x5a, 0x98, 0x01, 0x82, 0xb1, 0x0a, 0xb7,
    0xd5, 0x4b, 0xfe, 0xd3, 0xc9, 0x64, 0x07, 0x3a,
    0x0e, 0xe1, 0x72, 0xf3, 0xda, 0xa6, 0x23, 0x25,
    0xaf, 0x02, 0x1a, 0x68, 0xf7, 0x07, 0x51, 0x1a,
};

// use Kanban to simulate ledger
static auto kb = Redis(KANBAN_ADDR);

static
string get_epoch(void)
{
    // get epoch string from Kanban
    auto _epoch_packet = kb.get("EPOCH");
    if (_epoch_packet) {
    } else {
        return "-1";
    }

    string string_epoch_packet = *_epoch_packet;
    JSON epoch_packet = JSON::Load(string_epoch_packet);

    // tear sig and epoch apart
    uint8_t _sig[64];
    string string_sig = epoch_packet["SIG"].ToString();
    string string_epoch = epoch_packet["EPOCH"].ToString();

    // verify the epoch packet
    ed25519_string_to_sig(string_sig.data(), _sig);
    bool res = ed25519_verify(pkey, (uint8_t *)string_epoch.data(), string_epoch.length(), _sig);
    // printf("EPOCH Verification %s\n", res ? "passed" : "failed");

    return res ? string_epoch : "-1";
}

static
int get_epoch_int(void)
{
    string string_epoch = get_epoch();
    return stoi(string_epoch, nullptr, 10);
}

// only invoked when no epoch at all from the ledger
void manager::init_epoch(void)
{
    if ("-1" != get_epoch()) return; // already init'ed

    uint8_t _sig[65]  = "";
    string first_epoch = "1";
    ed25519_sign(_sig, skey, (uint8_t *)first_epoch.data(), first_epoch.length());

    char sig[129] = "";
    ed25519_sig_to_string(_sig, sig);

    JSON epoch_packet;
    epoch_packet["EPOCH"] = first_epoch;
    epoch_packet["SIG"]   = sig;

    kb.set("EPOCH", epoch_packet.dump());

    cout << "manager::manager init epoch" << endl << endl;
}

// 'increment_epoch' is only available to the manager
void manager::increment_epoch(void)
{
    if ("-1" == get_epoch()) init_epoch();
    string current_epoch = get_epoch();
    int current_epoch_int = get_epoch_int();

    // make a root hash for all keys related to executors' witnesses

    set <string> witness_keys;
    for (;;) {
        
        string epoch_witness_key = "WITNESS-#EPOCH" + current_epoch;
        kb.smembers(epoch_witness_key, ::inserter(witness_keys, witness_keys.begin()));

        int expected_key_size = 0;
        if (1 == current_epoch_int) expected_key_size = ITEMS_POOL_MAX;
        else if (2 == current_epoch_int) expected_key_size = ITEMS_POOL_MAX + INDEX_POOL_MAX;
        else expected_key_size = ITEMS_POOL_MAX + INDEX_POOL_MAX * 2;

//        cout << epoch_witness_key << " witness_keys.size " << witness_keys.size() << endl;
//        cout << epoch_witness_key << " expected_key_size " << expected_key_size << endl;

        if (witness_keys.size() >= expected_key_size) break;
        witness_keys.clear();

        sleep(1);
    }

    // summary all keys
    vector <string> string_witness_keys;
    string_witness_keys.assign(witness_keys.begin(), witness_keys.end());
    // use a particular sorting algorithm
    sort(string_witness_keys.begin(), string_witness_keys.end(),
        [] (string &s1, string &s2) { return s1.back() < s2.back(); });
    // merge all keys into one string
    string omni_string = "";
    for (auto & s : string_witness_keys) omni_string += s;
    char root_hash[65] = "";
    sha256_string(omni_string.data(), omni_string.size(), root_hash);

    // XXX: use Kanban as the blockchain in this implementation
    string chain_key = "ROOT-HASH-#EPOCH" + current_epoch;
    kb.set(chain_key, root_hash);

    // increment the global epoch
    string new_epoch = to_string(get_epoch_int() + 1); // increment

    uint8_t _sig[65]  = "";
    ed25519_sign(_sig, skey, (uint8_t *)new_epoch.data(), new_epoch.length());

    char sig[129] = "";
    ed25519_sig_to_string(_sig, sig);

    JSON epoch_packet;
    epoch_packet["EPOCH"] = new_epoch;
    epoch_packet["SIG"]   = sig;

    kb.set("EPOCH", epoch_packet.dump());

    cout << "manager::manager another epoch " << current_epoch_int + 1 << endl << endl;
}

manager::manager(void)
{
    // TODO: an election protocol for everyone to be a master
    // high-level idea:
    // similar to PoET [USENIX SEC 2017], use a random number
    // roll the dice: am I elected to be the master?
    // if yes, get a list of all witnesses belonging to the current epoch
    // hash them into a single root hash
    // if someone else has committed the root hash, then discard the efforts
    // append to the blockchain (not considering failure transactions)
    // when done, increment the on-chain epoch
}

manager::~manager(void)
{

}