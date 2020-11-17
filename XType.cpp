#include "XType.h"
#include <errno.h>
#include <sys/timerfd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>




int make_socket_non_blocking(const int _fd)
{
    int flags, s;

    flags = fcntl (_fd, F_GETFL, 0);
    if (flags == -1) {
        GLOGPRINTF0 ("fcntl error while F_GETFL. fd=%d\n", _fd);
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl (_fd, F_SETFL, flags);
    if (s == -1) {
        GLOGPRINTF0 ("fcntl error while F_SETFL. fd=%d\n", _fd);
        return -1;
    }

    return 0;
}

/**
 * @brief 
 * @param raw
 * @param hex
 * @param hexsize default value hexsize = UINT64_MAX must be given in prototype
 * @return 
 */
size_t hex2raw(uint8_t* raw, const char* hex, size_t hexsize) 
{
	size_t i;
	char t;
	for(i = 0; i < hexsize; i++){
		if(hex[i] >= 'a' && hex[i] <= 'f'){
			t= hex[i] - 'a' + 10;
		}else if(hex[i] >= 'A' && hex[i] <= 'F'){
			t= hex[i] - 'A' + 10;
		}else if(hex[i] >= '0' && hex[i] <= '9'){
			t= hex[i] - '0';
		}else{
			break;
		}
		if(i & 0x01){
			raw[i >> 1] += t;
		}else{
			raw[i >> 1] = t << 4;
		}
	}
	raw[i >> 1] = 0;
	return i >> 1;
}

void raw2hex(char *hex, const uint8_t *raw, size_t rawsize) {
    for (size_t i = 0; i < rawsize; i++) {
        hex[i << 1U] = raw[i] >> 4U;
        hex[(i << 1U) + 1] = raw[i] & 0x0fU;
    }

    auto hexsize = 2 * rawsize;
    for (size_t i = 0; i < hexsize; i++) {
        if (hex[i] >= 10) {
            hex[i] += 'a' - 10;
        } else {
            hex[i] += '0';
        }
    }
    hex[hexsize] = 0;
}

uint8_t xor_checksum(uint8_t xorvalue, const uint8_t *msg_ptr, uint32_t len)
{
	for(; len >0; len--, msg_ptr++) {
		xorvalue = xorvalue ^ *msg_ptr;
	}
	return (xorvalue);
}



/**
	
 * Ansi C "itoa" based on Kernighan & Ritchie's "Ansi C":
	
 */
	
static void strreverse(char* begin, char* end)
{
	
	char aux;
	
	while(end>begin)
	
		aux=*end, *end--=*begin, *begin++=aux;
	
}
