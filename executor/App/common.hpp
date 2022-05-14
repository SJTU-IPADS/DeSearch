#pragma once

#include "config.hpp"

// C++ STL
#include <set>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <unistd.h>

#include <cstdint>
#include <chrono>
#include <string>
#include <thread>
#include <cstring>

// stdio
#include <sstream>
#include <iostream>
#include <fstream>

using namespace std;

// redis as kanban
#include <sw/redis++/redis++.h>
using namespace sw::redis;

// json for encode/decode
#include "json.hpp"
using json::JSON;

#include "Utility.hpp"

// remove beginning and ending \" in a JSON string
static inline string trim_string(string original_string)
{
    return original_string.substr(1, original_string.length()-2);
}

//////////////////////////////////////////////////////////////////////////

// utility functions

bool ed25519_verify(uint8_t *_public, uint8_t *msg, uint32_t msg_len, uint8_t *signature);

void ed25519_sig_to_string(const uint8_t *sig, char outputBuffer[129]);
void ed25519_string_to_sig(const char inputBuffer[129], uint8_t *sig);
void ed25519_key_to_string(const uint8_t *key, char outputBuffer[65]);
void ed25519_string_to_key(const char inputBuffer[65], uint8_t *key);

void crawler_loop(void);
void indexer_loop(void);
void reducer_loop(void);
void querier_loop(const char *addr, int port);
void querier_setup(void);

//////////////////////////////////////////////////////////////////////////

// Kanban Mailbox

static auto kb = Redis(KANBAN_ADDR);

int get_epoch_int(void);

static inline
bool o_check_key(string key)
{
    auto value = kb.get(key);
    if (value) {
//        cout << key << " item already exist\n";
        return true;
    }
    return false;
}

static inline
void o_put_key(const string key, const char * value, const size_t value_size)
{
    kb.set(key, StringView(value, value_size));
}

// using blocking I/O model
static inline
void o_get_key(const string key, char * & o_value, int & o_sz)
{
    for (;;) {
        auto value = kb.get(key);
        if (value) {
            o_sz = value->size();
            o_value = new char[o_sz];
            memcpy(o_value, value->data(), o_sz);
            break;
        }
        sleep(1);
    }
}

static inline
void o_append_witness(const string & epoch_value, const char * witness_value)
{
    // use the witness OUT part as the witness key
    string witness_key = JSON::Load(witness_value)["OUT"].dump();
    witness_key = witness_key.substr(1, witness_key.length()-2); // trim \"

    // all witnesses are located in the same folder for quick browsing
    kb.hset("witness", witness_key, witness_value);
    // printf("witness [%d] %s %s\n", strlen(witness_value), witness_key.data(), witness_value);

    // push the witness->key part to epoch for authentication later on
    string epoch_witness_key = "WITNESS-#EPOCH" + epoch_value;
    kb.sadd(epoch_witness_key, witness_key);
    printf("%s %s\n", epoch_witness_key.data(), witness_key.data());
}
