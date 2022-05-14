#include "sgx_eid.h"
#include "Enclave2_t.h"

#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */
#include <string>

using namespace std;

void printf(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}
