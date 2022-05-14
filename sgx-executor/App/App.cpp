#include <stdio.h>
#include <map>
#include <unordered_map>

using namespace std;

#include "../Enclave1/Enclave1_u.h"
#include "../Enclave2/Enclave2_u.h"
#include "../Enclave3/Enclave3_u.h"

#include "sgx_eid.h"
#include "sgx_urts.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

std::map<sgx_enclave_id_t, uint32_t>g_enclave_id_map;

sgx_enclave_id_t e1_enclave_id = 0;
sgx_enclave_id_t e2_enclave_id = 0;
sgx_enclave_id_t e3_enclave_id = 0;

#define ENCLAVE1_PATH "libenclave1.so"
#define ENCLAVE2_PATH "libenclave2.so"
#define ENCLAVE3_PATH "libenclave3.so"

uint32_t load_enclaves(void)
{
    uint32_t enclave_temp_no;
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;

    enclave_temp_no = 0;

    ret = sgx_create_enclave(ENCLAVE1_PATH, SGX_DEBUG_FLAG, NULL, NULL, &e1_enclave_id, NULL);
    if (ret != SGX_SUCCESS) {
                return ret;
    }

    enclave_temp_no++;
    g_enclave_id_map.insert(std::pair<sgx_enclave_id_t, uint32_t>(e1_enclave_id, enclave_temp_no));

    ret = sgx_create_enclave(ENCLAVE2_PATH, SGX_DEBUG_FLAG, NULL, NULL, &e2_enclave_id, NULL);
    if (ret != SGX_SUCCESS) {
                return ret;
    }

    enclave_temp_no++;
    g_enclave_id_map.insert(std::pair<sgx_enclave_id_t, uint32_t>(e2_enclave_id, enclave_temp_no));

    ret = sgx_create_enclave(ENCLAVE3_PATH, SGX_DEBUG_FLAG, NULL, NULL, &e3_enclave_id, NULL);
    if (ret != SGX_SUCCESS) {
                return ret;
    }

    enclave_temp_no++;
    g_enclave_id_map.insert(std::pair<sgx_enclave_id_t, uint32_t>(e3_enclave_id, enclave_temp_no));

    printf("\nEnclave1 - EnclaveID %" PRIx64, e1_enclave_id);
    printf("\nEnclave2 - EnclaveID %" PRIx64, e2_enclave_id);
    printf("\nEnclave3 - EnclaveID %" PRIx64, e3_enclave_id);
    printf("\n");

    return SGX_SUCCESS;
}

void drop_enclave(void)
{
    sgx_destroy_enclave(e1_enclave_id);
    sgx_destroy_enclave(e2_enclave_id);
    sgx_destroy_enclave(e3_enclave_id);
}

void ocall_print_string(const char *str)
{
    printf("%s", str);
}

////////////////////////////////////////////////////////////////////////////////

#include "ZeroTrace/ZT_ORAM.hpp"
#include "ZeroTrace/ZT_Api.hpp"
#include "ZT_Globals.hpp"

#include "common.hpp"

static
void ZeroTrace_Init()
{
    uint32_t D;

    // init ls_oram for digest
    LocalStorage * ls_oram = new LocalStorage();
    #define DIGEST_BLOCK_NUM (1024 * 16)
    #define DIGEST_LENGTH (512)
    D = (uint32_t) ceil(log((double) DIGEST_BLOCK_NUM / 2) / log((double) 2));
    ls_oram -> setParams(DIGEST_BLOCK_NUM, D, 2, 10, DIGEST_LENGTH + ADDITIONAL_METADATA_SIZE, true, 128 + ADDITIONAL_METADATA_SIZE, 1);
    ls_CORAM.insert(std::make_pair(0, ls_oram));

    // init ls_oram for index
    LocalStorage * ls_oram2 = new LocalStorage();
    #define INDEX_BLOCK_NUM (1024 * 8)
    #define INDEX_BLOCK_SIZE (512)
    D = (uint32_t) ceil(log((double) INDEX_BLOCK_NUM / 2) / log((double) 2));
    ls_oram2 -> setParams(INDEX_BLOCK_NUM, D, 2, 10, INDEX_BLOCK_SIZE + ADDITIONAL_METADATA_SIZE, true, 128 + ADDITIONAL_METADATA_SIZE, 1);
    ls_CORAM.insert(std::make_pair(1, ls_oram2));
}

int main(int argc, char * argv[])
{
    ZeroTrace_Init();

    if (load_enclaves() != SGX_SUCCESS)
    {
        printf("\nLoad Enclave Failure");
    }

    if (argc < 2) {
        printf("usage: %s [job type]\n", argv[0]);
        return 1;
    }

    // job dispatch: desearch pipeline
    thread th;
    if (0 == strcmp(argv[1], "crawler")) {
        th = thread(crawler_loop);
        printf("[%d] crawler_loop created\n", getpid());
    } else if (0 == strcmp(argv[1], "indexer")) {
        th = thread(indexer_loop);
        printf("[%d] indexer_loop created\n", getpid());
    } else if (0 == strcmp(argv[1], "reducer")) {
        th = thread(reducer_loop);
        printf("[%d] reducer_loop created\n", getpid());
    } else if (0 == strcmp(argv[1], "querier")) {
        th = thread(querier_loop, argv[2], stoi(argv[3], nullptr, 10));
        printf("[%d] querier_loop created\n", getpid());
    }
    th.join();

    drop_enclave();

    return 0;
}
