#include "common.hpp"

#include "Hacl_Ed25519.h"

// hard coded Curve 25519 public key
uint8_t pkey[32] = {
    0xd7, 0x5a, 0x98, 0x01, 0x82, 0xb1, 0x0a, 0xb7,
    0xd5, 0x4b, 0xfe, 0xd3, 0xc9, 0x64, 0x07, 0x3a,
    0x0e, 0xe1, 0x72, 0xf3, 0xda, 0xa6, 0x23, 0x25,
    0xaf, 0x02, 0x1a, 0x68, 0xf7, 0x07, 0x51, 0x1a,
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