#ifndef BASE64_H
#define BASE64_H

#include <string>
#include <vector>


//#define SERVER_EDITION
#define CLIENT_EDITION


#ifdef SERVER_EDITION
void base64_decode(std::string &encoded_string, std::vector<unsigned char> &dest_code);
#endif

#ifdef CLIENT_EDITION
void base64_encode(std::vector<unsigned char> &v, std::string &str);
#endif


#endif
