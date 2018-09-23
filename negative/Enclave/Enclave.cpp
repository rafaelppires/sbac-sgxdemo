#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include "Enclave.h"
#include "Enclave_t.h"  /* print_string */

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

int ecall_compute(int a, int b) {
    const char *hidden = "sbac-pad";
    (void) hidden;
    int res = a + b;
    if(res < 0) {
        printf("I do not like negative numbers\n");
    }
    return res;
}

