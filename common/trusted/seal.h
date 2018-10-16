#ifndef _SEAL_H_
#define _SEAL_H_

#include <string>

std::string sealEnclave( const std::string &src );
std::string sealSigner( const std::string &src );
std::string unseal( const std::string &src );
std::string getmrenclave();

#endif

