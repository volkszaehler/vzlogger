#ifndef __hmac_h_
#define __hmac_h_

#include <stddef.h>

namespace vz {
void hmac_sha1(char *digest, const unsigned char *data, size_t dataLen,
			   const unsigned char *secretKey, size_t secretLen);

} // namespace vz

#endif
