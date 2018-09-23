#include <init_encl.h>
#include <unistd.h>

#include "sgx_urts.h"
#include "App.h"
#include "Enclave_u.h"

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
int SGX_CDECL main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    printf("Pid: %d\n", getpid());

    /* Initialize the enclave */
    if(initialize_enclave(ENCLAVE_FILENAME, &global_eid) < 0){
        printf("Enter a character before exit ...\n");
        getchar();
        return -1; 
    }
 
    int ret;
    ecall_compute( global_eid, &ret, 4, -5 );
    printf("Result: %d\n", ret);

    printf("Enter a character before exit ...\n");
    getchar();

    /* Destroy the enclave */
    sgx_destroy_enclave(global_eid);
    
    return 0;
}

