#ifndef RSA_H
#define RSA_H


#include <string>
#include <vector>

#define RSA_MAX 100

//#define SERVER_EDITION
#define CLIENT_EDITION

#ifdef SERVER_EDITION
void rsa_init();
void rsa_gen_key(int RSA_Public_Key_d[RSA_MAX], int RSA_Public_Key_n[RSA_MAX]);
void rsa_get_public_key_str(std::string &public_key);
void rsa_decrypt(std::vector<unsigned char> &src, std::vector<unsigned char> &dest,int *d, int *n);

void buffer_to_vector(unsigned char *buffer, size_t buffer_len, std::vector<unsigned char> &v);
void vector_to_string(std::vector<unsigned char> &v, std::string &str);
#endif

#ifdef CLIENT_EDITION
void rsa_public_key_str_to_bin(std::string &public_key, int e[RSA_MAX], int n[RSA_MAX]);
void rsa_encrypt(std::vector<unsigned char> &src, std::vector<unsigned char> &dest, int *e, int *n);

void string_to_vector(std::string &str, std::vector<unsigned char> &v);
void vector_to_string(std::vector<unsigned char> &v, std::string &str);
#endif


#endif /* !RSA_H */
