#include "enclave_common.hpp"

#include "Hacl_Ed25519.h"

// hard coded Curve 25519 secret key
uint8_t skey[32] = {
    0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60,
    0xba, 0x84, 0x4a, 0xf4, 0x92, 0xec, 0x2c, 0xc4,
    0x44, 0x49, 0xc5, 0x69, 0x7b, 0x32, 0x69, 0x19,
    0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60,
};

// ed25519 utils

void ed25519_sign(uint8_t *signature, uint8_t *secret, uint8_t *msg, uint32_t msg_len)
{
    Hacl_Ed25519_sign(signature, secret, msg, msg_len);
}

bool ed25519_verify(uint8_t *_public, uint8_t *msg, uint32_t msg_len, uint8_t *signature)
{
    return Hacl_Ed25519_verify(_public, msg, msg_len, signature);
}

void ed25519_sig_to_string(const uint8_t *sig, char outputBuffer[129])
{
    for(int i = 0; i < 64; i++)
    {
        snprintf(outputBuffer + (i * 2), 4, "%02x", sig[i]);
    }
    outputBuffer[128] = 0;
}

void ed25519_string_to_sig(const char inputBuffer[129], uint8_t *sig)
{
    char temp[3] = "";
    for(int i = 0; i < 64; i++) {
        temp[0] = inputBuffer[i * 2];
        temp[1] = inputBuffer[i * 2 + 1];
        sig[i] = strtol(temp, 0, 16);
    }
}

void ed25519_key_to_string(const uint8_t *key, char outputBuffer[65])
{
    for(int i = 0; i < 32; i++)
    {
        snprintf(outputBuffer + (i * 2), 4, "%02x", key[i]);
    }
    outputBuffer[64] = 0;
}

void ed25519_string_to_key(const char inputBuffer[65], uint8_t *key)
{
    char temp[3] = "";
    for(int i = 0; i < 32; i++) {
        temp[0] = inputBuffer[i * 2];
        temp[1] = inputBuffer[i * 2 + 1];
        key[i] = strtol(temp, 0, 16);
    }
}