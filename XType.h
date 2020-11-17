
#ifndef idAF681A87_DDD2_4E6F_8167A0F763C34645
#define idAF681A87_DDD2_4E6F_8167A0F763C34645

#include <netdb.h>



size_t hex2raw(uint8_t* raw, const char* hex, size_t hexsize = __UINT64_MAX__);
void raw2hex(char *hex, const uint8_t *raw, size_t rawsize);

uint8_t xor_checksum(uint8_t xorvalue, const uint8_t *msg_ptr, uint32_t len);


#define GLOGPRINTF0(arg...)
#define GLOGPRINTHEX0(arg...)
#define GLOGPRINTF2(arg...)

#endif // idAF681A87_DDD2_4E6F_8167A0F763C34645
//mode_t