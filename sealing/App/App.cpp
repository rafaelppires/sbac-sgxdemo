#include <init_encl.h>
#include <fstream>
#include <vector>

#include "App.h"
#include "Enclave_u.h"
#include "sgx_urts.h"

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

/* OCall functions */
int ocall_print_string(const char *str) {
    /* Proxy/Bridge will check the length and null-terminate
     * the input string to prevent buffer overflow.
     */
    return printf("\033[34,1mEnclave says\033[0m: %s", str);
}

/* Application entry */
int SGX_CDECL main(int argc, char *argv[]) {
    const char *fname = "sealed.dat";

    /* Initialize the enclave */
    if (initialize_enclave(ENCLAVE_FILENAME, &global_eid) < 0) {
        printf("Enter a character before exit ...\n");
        getchar();
        return -1;
    }

    if (argc > 1) {
        char buff[1000];
        int ret;
        if (SGX_SUCCESS ==
            ecall_seal_secret(global_eid, &ret, argv[1], buff, sizeof(buff))) {
            std::ofstream out(fname);
            if (ret > 0) out << std::string(buff, ret);
        } else
            printf("ecall_seal_secret: error\n");
    } else {
        std::ifstream in(fname, std::ios::binary | std::ios::ate);
        if (in.good()) {
            std::streamsize file_size = in.tellg();
            in.seekg(0, std::ios::beg);
            std::vector<char> buff(file_size);
            if (in.read(buff.data(), file_size)) {
                if (SGX_SUCCESS !=
                    ecall_unseal(global_eid, buff.data(), file_size))
                    fprintf(stderr, "ecall_unseal: error\n");
            }
        } else {
            fprintf(stderr,"'%s': file not found or denied access\n", fname);
        }
    }

    /* Destroy the enclave */
    sgx_destroy_enclave(global_eid);

    return 0;
}
