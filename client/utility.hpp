#include "Hacl_Ed25519.h"

// ed25519 utils

static
void ed25519_sign(uint8_t *signature, uint8_t *secret, uint8_t *msg, uint32_t msg_len)
{
    Hacl_Ed25519_sign(signature, secret, msg, msg_len);
}

static
bool ed25519_verify(uint8_t *_public, uint8_t *msg, uint32_t msg_len, uint8_t *signature)
{
    return Hacl_Ed25519_verify(_public, msg, msg_len, signature);
}

static
void ed25519_sig_to_string(const uint8_t *sig, char outputBuffer[129])
{
    for(int i = 0; i < 64; i++)
    {
        snprintf(outputBuffer + (i * 2), 4, "%02x", sig[i]);
    }
    outputBuffer[128] = 0;
}

static
void ed25519_string_to_sig(const char inputBuffer[129], uint8_t *sig)
{
    char temp[3] = "";
    for(int i = 0; i < 64; i++) {
        temp[0] = inputBuffer[i * 2];
        temp[1] = inputBuffer[i * 2 + 1];
        sig[i] = strtol(temp, 0, 16);
    }
}

static
void ed25519_key_to_string(const uint8_t *key, char outputBuffer[65])
{
    for(int i = 0; i < 32; i++)
    {
        snprintf(outputBuffer + (i * 2), 4, "%02x", key[i]);
    }
    outputBuffer[64] = 0;
}

static
void ed25519_string_to_key(const char inputBuffer[65], uint8_t *key)
{
    char temp[3] = "";
    for(int i = 0; i < 32; i++) {
        temp[0] = inputBuffer[i * 2];
        temp[1] = inputBuffer[i * 2 + 1];
        key[i] = strtol(temp, 0, 16);
    }
}

#include "openssl/sha.h"
#include "openssl/aes.h"

static
void sha256_string(const void *start, size_t size, char outputBuffer[65])
{
    unsigned char hash[SHA256_DIGEST_LENGTH];

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, start, size);
    SHA256_Final(hash, &sha256);

    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
}

#include "json.hpp"
using json::JSON;

static
bool check_witness(string & s_witness)
{
    JSON original_witness = JSON::Load(s_witness);

    JSON json_witness;
    json_witness["TAG"]  = original_witness["TAG"];
    json_witness["IN"]   = original_witness["IN"];
    json_witness["OUT"]  = original_witness["OUT"];
    json_witness["FUNC"] = original_witness["FUNC"];

    uint8_t sig[65]  = "";
    string _string_sig = original_witness["SIG"].ToString();
    ed25519_string_to_sig(_string_sig.data(), sig);

    return ed25519_verify(pkey, (uint8_t *)json_witness.dump().data(), json_witness.dump().length(), sig);
}

#include <set>
#include <string>
#include <algorithm>
#include <sw/redis++/redis++.h>
using namespace sw::redis;

// use Kanban to simulate ledger
static auto kb = Redis(KANBAN_ADDR);

static
string get_epoch(void)
{
    // get epoch string from Kanban
    auto _epoch_packet = kb.get("EPOCH");
    if (_epoch_packet) {
    } else {
        cerr << "The system hasn't been init'ed yet ..." << endl;
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

static
bool check_root_hash(int epoch)
{
    string epoch_witness_key = "WITNESS-#EPOCH" + to_string(epoch);

    // summary the all keys
    set <string> witness_keys;
    kb.smembers(epoch_witness_key, ::inserter(witness_keys, witness_keys.begin()));

    // turn to a sorted vector
    vector <string> string_witness_keys;
    string_witness_keys.assign(witness_keys.begin(), witness_keys.end());
    sort(string_witness_keys.begin(), string_witness_keys.end(),
        [] (string &s1, string &s2) { return s1.back() < s2.back(); });

    string omni_string = "";
    for (auto & s : string_witness_keys) omni_string += s;
    char _root_hash[65] = "";
    sha256_string(omni_string.data(), omni_string.size(), _root_hash);
 
    // XXX: using Kanban as the blockchain in this implementation
    string chain_key = "ROOT-HASH-#EPOCH" + to_string(epoch);
    auto root_hash = kb.get(chain_key);
 
    return strncmp((*root_hash).data(), _root_hash, 64) == 0;
}
