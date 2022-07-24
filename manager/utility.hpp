#include "Hacl_Ed25519.h"

// ed25519 utils

void ed25519_sign(uint8_t* signature, uint8_t* secret, uint8_t* msg, uint32_t msg_len)
{
    Hacl_Ed25519_sign(signature, secret, msg, msg_len);
}

bool ed25519_verify(uint8_t* _public, uint8_t* msg, uint32_t msg_len, uint8_t* signature)
{
    return Hacl_Ed25519_verify(_public, msg, msg_len, signature);
}

void ed25519_sig_to_string(const uint8_t* sig, char outputBuffer[129])
{
    for (int i = 0; i < 64; i++) {
        snprintf(outputBuffer + (i * 2), 4, "%02x", sig[i]);
    }
    outputBuffer[128] = 0;
}

void ed25519_string_to_sig(const char inputBuffer[129], uint8_t* sig)
{
    char temp[3] = "";
    for (int i = 0; i < 64; i++) {
        temp[0] = inputBuffer[i * 2];
        temp[1] = inputBuffer[i * 2 + 1];
        sig[i] = strtol(temp, 0, 16);
    }
}

void ed25519_key_to_string(const uint8_t* key, char outputBuffer[65])
{
    for (int i = 0; i < 32; i++) {
        snprintf(outputBuffer + (i * 2), 4, "%02x", key[i]);
    }
    outputBuffer[64] = 0;
}

void ed25519_string_to_key(const char inputBuffer[65], uint8_t* key)
{
    char temp[3] = "";
    for (int i = 0; i < 32; i++) {
        temp[0] = inputBuffer[i * 2];
        temp[1] = inputBuffer[i * 2 + 1];
        key[i] = strtol(temp, 0, 16);
    }
}

#include "openssl/aes.h"
#include "openssl/sha.h"

static void sha256_string(const void* start, size_t size, char outputBuffer[65])
{
    unsigned char hash[SHA256_DIGEST_LENGTH];

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, start, size);
    SHA256_Final(hash, &sha256);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
}
