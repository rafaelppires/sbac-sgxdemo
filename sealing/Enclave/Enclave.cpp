#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */
#include <string>

#include <sgx_trts.h>
#include "Enclave.h"
#include "Enclave_t.h"  /* print_string */
#include <seal.h>

/* 
 * printf: 
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
int printf(const char *fmt, ...) {
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);

    int ret;
    ocall_print_string(&ret,buf);
    return ret;
}

std::string hexmrenclave() {
    const char *hex = "0123456789abcdef";
    std::string mre_bin = getmrenclave(), ret;
    for(int i = 0; i < mre_bin.size(); ++i) {
        char c = mre_bin[i];
        ret += hex[(c & 0xf0)>>4];
        ret += hex[c & 0xf];
    }
    return ret;
}

int ecall_seal_secret(const char *input, char *output, size_t out_size) {
    unsigned int rnd;
    printf("MRENCLAVE: %s\n", hexmrenclave().c_str());
    sgx_read_rand((unsigned char*)&rnd,sizeof(rnd));
    std::string secret = std::string(input) + " " + std::to_string(rnd % 100);
    std::string sealed = sealEnclave( secret );
    //std::string sealed = sealSigner( secret );
    int ret = (int)std::min(out_size,sealed.size());
    memcpy(output,sealed.c_str(),ret);
    return ret;
}

void ecall_unseal(const char *buff, size_t in_size) {
    printf("MRENCLAVE: %s\n", hexmrenclave().c_str());
    try {
        std::string unsealed = unseal( std::string(buff,in_size) );
        printf("'%s'\n", unsealed.c_str());
    } catch(int e) {
        if (e == SGX_ERROR_MAC_MISMATCH) printf("MAC mismatch!\n");
        else printf("Unsealing error %d\n.", e);
    }
}

