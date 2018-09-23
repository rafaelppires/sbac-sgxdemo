#ifndef _INIT_ENCLAVE_H_
#define _INIT_ENCLAVE_H_

#include <sgx_eid.h> 

int initialize_enclave(const char*,sgx_enclave_id_t *global_eid);

#endif

