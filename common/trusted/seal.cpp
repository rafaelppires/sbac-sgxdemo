#include "seal.h"
#include <sgx_tseal.h>

static int seal_common( uint16_t policy,
                        const uint8_t *src, size_t srclen, void **sealed ) {
    std::string mactxt;
    size_t sz;
    if( (sz = (size_t)sgx_calc_sealed_data_size((uint32_t)mactxt.size(),(uint32_t)srclen)) == 0xFFFFFFFF )
        return -1;

    *sealed = malloc( sz ); // warning: caller frees it
    sgx_attributes_t attr;
    attr.flags = 0xFF0000000000000BULL;
    attr.xfrm  = 0;
    if( sgx_seal_data_ex( policy, attr, 0xF0000000, 0, NULL, (uint32_t)srclen, src,
                            (uint32_t)sz, (sgx_sealed_data_t*)*sealed ) != SGX_SUCCESS ) {
        free( *sealed );
        *sealed = 0;
        return -2;
    }
    return (int)sz;
}

int seal_signer ( const uint8_t *src, size_t srclen, void **sealed ) {
    return seal_common( SGX_KEYPOLICY_MRSIGNER, src, srclen, sealed );

}

int seal_enclave( const uint8_t *src, size_t srclen, void **sealed ) {
    return seal_common( SGX_KEYPOLICY_MRENCLAVE, src, srclen, sealed );
}

int unseal( const uint8_t *src, void **unsealed,
            void **mactxt, size_t *txt_len ) {
    void *mactxt_aux = 0;
    uint32_t mactxt_len =sgx_get_add_mac_txt_len((const sgx_sealed_data_t*)src),
            decryp_len = sgx_get_encrypt_txt_len((const sgx_sealed_data_t*)src);

    *unsealed = malloc( decryp_len );
    if( txt_len == 0 ) mactxt = 0;
    if( mactxt )
        *mactxt = malloc( mactxt_len );
    else {
        mactxt = &mactxt_aux;
        mactxt_len = 0;
    }

    sgx_status_t uret;
    if((uret= sgx_unseal_data( (const sgx_sealed_data_t*)src,
                         (uint8_t*)*mactxt, &mactxt_len,
                         (uint8_t*)*unsealed, &decryp_len )) != SGX_SUCCESS ) {
        free( *unsealed );
        free( *mactxt );
        *unsealed = *mactxt = 0;
        throw (int) uret;
        return -1;
    }

    *unsealed = realloc( *unsealed, decryp_len );
    if( *mactxt ) {
        *mactxt   = realloc( *mactxt, mactxt_len );
        *txt_len  = mactxt_len;
    }
    return decryp_len;
}

std::string sealSigner( const std::string &src ) {
    std::string ret;
    void *sealed;
    size_t sz;
    if( (sz = seal_signer( (const uint8_t*)src.c_str(), src.size(), &sealed ))
                                                                         > 0 ) {
        ret = std::string( (char*)sealed, sz );
        free( sealed );
    }
    return ret;
}

std::string sealEnclave( const std::string &src ) {
    std::string ret;
    void *sealed;
    size_t sz;
    if( (sz = seal_enclave( (const uint8_t*)src.c_str(), src.size(), &sealed ))
                                                                         > 0 ) {
        ret = std::string( (char*)sealed, sz );
        free( sealed );
    }
    return ret;
}

std::string unseal( const std::string &src ) {
    std::string ret;
    void *unsealed;
    int len = ::unseal( (const uint8_t*)src.c_str(), &unsealed, 0, 0 );
    if( len > 0 ) {
        ret = std::string( (const char*)unsealed, len );
        free( unsealed );
    }
    return ret;
}

