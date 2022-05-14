#pragma once

#include "sgx_trts.h"
#include "sgx_report.h"
#include "sgx_utils.h"

#include <sgx_tcrypto.h>
#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include "config.hpp"

// C++ STL
#include <set>
#include <vector>
#include <unordered_map>
#include <algorithm>

using namespace std;

#include <cstdint>
#include <chrono>
#include <string>
#include <thread>
#include <cstring>

#include "Utility.hpp"

// json for encode/decode
#include "json.hpp"
using json::JSON;

//////////////////////////////////////////////////////////////////////////

// search engine library

#include "sphinx.h"

// utility functions

static
ISphTokenizer * CreateTokenizer()
{
    CSphString sError;

    CSphTokenizerSettings tSettings;
    tSettings.m_iMinWordLen = 1;
    tSettings.m_iNgramLen = 1;
    tSettings.m_sNgramChars = "U+4E00..U+9FBF, U+3400..U+4DBF, U+20000..U+2A6DF,"
        "U+F900..U+FAFF, U+2F800..U+2FA1F, U+2E80..U+2EFF, U+2F00..U+2FDF,"
        "U+3100..U+312F, U+31A0..U+31BF, U+3040..U+309F, U+30A0..U+30FF,"
        "U+31F0..U+31FF, U+AC00..U+D7AF, U+1100..U+11FF, U+3130..U+318F,"
        "U+A000..U+A48F, U+A490..U+A4CF";

    ISphTokenizer * pTokenizer = sphCreateUTF8NgramTokenizer ();
    pTokenizer->AddPlainChar ( '*' );
    pTokenizer->AddPlainChar ( '?' );
    pTokenizer->AddPlainChar ( '%' );
    pTokenizer->AddPlainChar ( '=' );
    pTokenizer->AddSpecials ( "()|-!@~\"/^$<=" );
    pTokenizer->SetCaseFolding ( "-, 0..9, A..Z->a..z, _, a..z, U+80..U+FF", sError );
    pTokenizer->EnableSentenceIndexing ( sError );

    return pTokenizer;
}

//////////////////////////////////////////////////////////////////////////

// remove beginning and ending \" in a JSON string
static inline string trim_string(string original_string)
{
    return original_string.substr(1, original_string.length()-2);
}

// Longest Common Subsequence
template <typename InIt>
InIt intersection(InIt &v1, InIt &v2)
{
    InIt v3;

    set_intersection(v1.begin(), v1.end(), v2.begin(), v2.end(),
        std::inserter(v3, std::begin(v3)));
    return v3;
}

//////////////////////////////////////////////////////////////////////////

// Variable Length Byte (VLB) encoding
// store int variable in as much bytes as actually needed to represent it
static inline void ZipDword ( vector < BYTE > * pOut, DWORD uValue )
{
    do
    {
        BYTE bOut = (BYTE)( uValue & 0x7f );
        uValue >>= 7;
        if ( uValue )
            bOut |= 0x80;
        pOut->push_back ( bOut );
    } while ( uValue );
}

// Variable Length Byte (VLB) decoding
static inline const BYTE * UnzipDword ( DWORD * pValue, const BYTE * pIn )
{
    DWORD uValue = 0;
    BYTE bIn;
    int iOff = 0;

    do
    {
        bIn = *pIn++;
        uValue += ( DWORD ( bIn & 0x7f ) ) << iOff;
        iOff += 7;
    } while ( bIn & 0x80 );

    *pValue = uValue;
    return pIn;
}

typedef struct _VLB_coder
{
    vector<BYTE>    m_pHits;
    DWORD           m_uLastHit;
    DWORD           m_iLeft;
    DWORD           m_uLast;

    const BYTE *    m_pCur;
    bool            m_bInit;

    _VLB_coder ( )
    {
        m_uLastHit = m_iLeft = m_uLast = 0;

        m_bInit = false;
        m_pCur = NULL;
    }

    _VLB_coder ( struct _VLB_coder const & src )
    {
        // copy from src
        m_iLeft = src.m_iLeft;
        m_uLast = src.m_uLast;
        m_uLastHit = src.m_uLastHit;

        // deep copy here
        m_pHits = src.m_pHits;
    }

    void Zip ( DWORD uValue )
    {
        // NOTE: deduplicate here
        if (0 == uValue - m_uLastHit)
            return;

        ZipDword ( &m_pHits, uValue - m_uLastHit );
        m_uLastHit = uValue;

        m_iLeft++;

        if (!m_bInit) {
            m_pCur = &m_pHits[0];
            m_bInit = true;
        }
    }

    DWORD Unzip ()
    {
        if ( !m_iLeft )
            return 0;

        if (!m_bInit) {
            m_pCur = &m_pHits[0];
            m_bInit = true;
        }

        DWORD uValue;
        m_pCur = UnzipDword ( &uValue, m_pCur );
        m_uLast += uValue;
        m_iLeft--;

        return m_uLast;
    }

    DWORD Length () const
    {
        return m_iLeft;
    }

    void Tighten ()
    {
        m_pHits.resize(m_pHits.size());
    }
} VLB_coder;

static
void Dump_VLB(VLB_coder &src, vector<DWORD> &dest)
{
    dest.clear();
    VLB_coder tmp(src);
    for ( DWORD uValue = tmp.Unzip(); uValue; uValue = tmp.Unzip() )
        dest.push_back(uValue);
}

static
void Load_VLB(vector<DWORD> &src, VLB_coder &dest)
{
    for ( const auto &uValue : src )
        dest.Zip(uValue);
}

static
void ConstantTime_Dump_VLB(const DWORD &wordID, unordered_map<DWORD, _VLB_coder> &table, vector<DWORD> &dest)
{
    dest.clear();

    VLB_coder src;
    // linear scan over the map
    for (auto const &i : table) {
        if (wordID == i.first)
            src = i.second;
    }
    VLB_coder tmp(src);
    for ( DWORD uValue = tmp.Unzip(); uValue; uValue = tmp.Unzip() )
        dest.push_back(uValue);
}

// NOTE: plz remember to release the memory
static
unsigned char *marshall_VLB_to_buffer(const VLB_coder &src, int &size)
{
    int total_size = sizeof(DWORD) * 4;
    total_size += src.m_pHits.size();

    unsigned char *ret = (unsigned char *)malloc(total_size);
    memcpy(ret, &src.m_pHits[0], src.m_pHits.size());
    
    DWORD *p1 = (DWORD *)(ret + src.m_pHits.size());
    *p1 = src.m_uLastHit; p1++;
    *p1 = src.m_iLeft; p1++;
    *p1 = src.m_uLast; p1++;
    
    *p1 = src.m_bInit; p1++;
    
    size = total_size;

    return ret;
}

// NOTE: plz also release the memory
static
void unmarshall_buffer_VLB(const unsigned char *buffer, int size, VLB_coder &src)
{
    int vec_size = size - sizeof(DWORD) * 4;
    src.m_pHits.resize(vec_size);
    
    // fill up "vector<BYTE> m_pHits"
    memcpy(&src.m_pHits[0], buffer, vec_size);
    
    DWORD *p1 = (DWORD *)(buffer + src.m_pHits.size());
    src.m_uLastHit = *p1; p1++;
    src.m_iLeft = *p1; p1++;
    src.m_uLast = *p1; p1++;

    src.m_bInit = *p1; p1++;
}

#if 0
    // unit test: VLB and delta encoding tests
    VLB_coder test;
    for (int i = 14; i < 20; ++i)
        test.Zip(i);
    printf("%d\n", test.Length());
    VLB_coder test2(test);
    for ( DWORD uValue = test.Unzip(); uValue; uValue = test.Unzip() )
        printf("get %d\n", uValue);
    for ( DWORD uValue = test2.Unzip(); uValue; uValue = test2.Unzip() )
        printf("get2 %d\n", uValue);
#endif

//////////////////////////////////////////////////////////////////////////

void ed25519_sign(uint8_t *signature, uint8_t *secret, uint8_t *msg, uint32_t msg_len);
bool ed25519_verify(uint8_t *_public, uint8_t *msg, uint32_t msg_len, uint8_t *signature);

void ed25519_sig_to_string(const uint8_t *sig, char outputBuffer[129]);
void ed25519_string_to_sig(const char inputBuffer[129], uint8_t *sig);
void ed25519_key_to_string(const uint8_t *key, char outputBuffer[65]);
void ed25519_string_to_key(const char inputBuffer[65], uint8_t *key);

typedef uint8_t sha256_t[65];
typedef uint8_t signature_t[129];

typedef struct {
    size_t nr_in; // number of inputs
    sha256_t bin; // who computes the inputs
    sha256_t out; // will be the key for this witness on Kanban
} witness_t;

#if 0
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
#endif
void sgx_sha256_string(const uint8_t *hash, char outputBuffer[65]);

typedef struct _witness_context {
    string tag; // init

    vector <string> ins;
    string out, bin;

    int stage; // [init, input, output, func, final] in case of revert
} witness_context;

void witness_clear(witness_context &context);
void witness_init(witness_context &context, const string &tag);
void witness_input(witness_context &context, const void *start, size_t size);
void witness_output(witness_context &context, const void *start, size_t size);
void witness_func(witness_context &context, const void *start, size_t size);
void witness_finalize(witness_context &context, string & output_witness);

////////////////////////////////////////////////////////////////////////////////

// debug only

static void dump_digest(const vector <string> digest_tbl)
{
    for (auto &d : digest_tbl)
        printf("%s\n", d.data());
}

static void dump_index(const unordered_map <DWORD, vector <DWORD> > index_tbl)
{
    // dump index
    for (auto i = index_tbl.begin(); i != index_tbl.end(); ++i) {
        printf("wordID %lu ", i->first);
        printf("docID : ");

        vector<DWORD> docID_list = i->second;
        for (auto const &doc : docID_list) {
            printf("%lu ", doc);
        }
        printf("\n");
    }
}
