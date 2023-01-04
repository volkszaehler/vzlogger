
#include <cstdio>
#include <cstring>
#if defined(WITH_OPENSSL)
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#else
#include <gnutls/crypto.h>
#include <gnutls/gnutls.h>
#endif

namespace vz {

#if defined(WITH_OPENSSL)

void hmac_sha1(char *digest, const unsigned char *data, size_t dataLen,
			   const unsigned char *secretKey, size_t secretLen) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	HMAC_CTX hmacContext;

	HMAC_Init(&hmacContext, secretKey, secretLen), EVP_sha1());
	HMAC_Update(&hmacContext, data, dataLen);
#elif OPENSSL_VERSION_NUMBER < 0x30000000L
	HMAC_CTX *hmacContext = HMAC_CTX_new();

	HMAC_Init_ex(hmacContext, secretKey, secretLen, EVP_sha1(), NULL);
	HMAC_Update(hmacContext, data, dataLen);
#else
	EVP_MD_CTX *evpContext = EVP_MD_CTX_new();
	EVP_PKEY *pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_HMAC, NULL, secretKey, secretLen);
	EVP_DigestSignInit(evpContext, NULL, EVP_sha1(), NULL, pkey);
	EVP_PKEY_free(pkey);
	EVP_DigestSignUpdate(evpContext, data, dataLen);
#endif

	unsigned char out[EVP_MAX_MD_SIZE];

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	unsigned int len = EVP_MAX_MD_SIZE;
	HMAC_Final(&hmacContext, out, &len);
#elif OPENSSL_VERSION_NUMBER < 0x30000000L
	unsigned int len = EVP_MAX_MD_SIZE;
	HMAC_Final(hmacContext, out, &len);
#else
	size_t len = EVP_MAX_MD_SIZE;
	EVP_DigestSignFinal(evpContext, out, &len);
#endif

	char ret[2 * EVP_MAX_MD_SIZE + 1];
	memset(ret, 0, sizeof(ret));

	for (size_t i = 0; i < len; i++) {
		char s[4];
		snprintf(s, 3, "%02x",
				 out[i]); // format string was "%02x:" but size was only 3 so last : was not printed
		strncat(ret, s, 2 * len);
		// strncat(ret, s, sizeof(ret));
	}
	snprintf(digest, 255 /*sizeof(digest)*/, "X-Digest: %s", ret);

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	// Nothing to free
#elif OPENSSL_VERSION_NUMBER < 0x30000000L
	HMAC_CTX_free(hmacContext);
#else
	EVP_MD_CTX_free(evpContext);
#endif
}

#else

void hmac_sha1(char *digest, const unsigned char *data, size_t dataLen,
			   const unsigned char *secretKey, size_t secretLen) {
	// compile time digest size for HMAC-SHA1
	const unsigned int len = 20;
	unsigned char out[len];
	gnutls_hmac_fast(GNUTLS_MAC_SHA1, secretKey, secretLen, data, dataLen, out);

	size_t ret_len = 2 * len + 1;
	char ret[ret_len];
	const gnutls_datum_t d_out = {out, len};
	gnutls_hex_encode(&d_out, ret, &ret_len);

	snprintf(digest, 255 /*sizeof(digest)*/, "X-Digest: %s", ret);
}

#endif // WITH_OPENSSL

} // namespace vz
