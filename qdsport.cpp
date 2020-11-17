#include "qdsport.h"
#include <immintrin.h>
#include <hiredis/hiredis.h>
#include <string.h>
#include <ctime>

qdsport::qdsport(uint64_t _did, uint16_t _cid, const uint8_t *aesk):
did(_did),
cid(_cid)
{
    authstat = AUTH_START;
    memset(_port.headermem, 0, 16);
    setaesk(aesk);
}

int qdsport::setaesk(const uint8_t *aesk)
{
	AES_set_encrypt_key(aesk, 128, &ek);
	AES_set_decrypt_key(aesk, 128, &dk);
	return 0;
}

int qdsport::appsend(const void *data, uint8_t len)
{
    if(authstat == AUTH_PASS){
        return packsend(cid, data, len);
    }else{
        return ERROR_UNAUTH;
    }
}


int qdsport::packsend(CLusterID_Type _cid, const void *data, uint8_t len)
{
    int rtn;
    SPack pack;
    pack.SoF = STARTOFFRAME;
    pack.packver = DPACKVERSION_3;
    pack.did = did;
    pack.cid = _cid;
    if(_cid == 0x0000){
        pack.datalen = len;
        memcpy(pack.data, data, len);
    }else{
        rtn = dataencrypt((uint8_t*)data, len, pack.data);
        if(rtn > 0){
            pack.datalen = rtn;
        }else{
            return ERROR_TOOMUCH;
        }
    }
    rtn = send(&pack);
    if(rtn < 0){
        return ERROR_IOFAIL;
    }else{
        return 0;
    }
}


int qdsport::packsend(uint64_t _did, CLusterID_Type _cid, const void *data, uint8_t len)
{
    int rtn;
    SPack pack;
    pack.SoF = STARTOFFRAME;
    pack.packver = DPACKVERSION_3;
    pack.did = _did;
    pack.cid = _cid;
    if(_cid == 0x0000){
        pack.datalen = len;
        memcpy(pack.data, data, len);
    }else{
        rtn = dataencrypt((uint8_t*)data, len, pack.data);
        if(rtn > 0){
            pack.datalen = rtn;
        }else{
            return ERROR_TOOMUCH;
        }
    }
    rtn = send(&pack);
    if(rtn < 0){
        return ERROR_IOFAIL;
    }else{
        return 0;
    }
}

#define HEARTBEATRATE_S  40		//seconds

void getrandnum(uint64_t * rand)
{
    _rdrand64_step((unsigned long long*)rand);
}

int qdsport::periodcall(const uint8_t _elapsedS)
{
    holdtime += _elapsedS;
    if(holdtime >= HEARTBEATRATE_S){
        holdtime = 0;
        //GLOGPRINTF0("$:heartbeat sent\n");
        return heartbeatsend();
    }else{
        return ERROR_OK;
    }
}


int qdsport::auth23(uint8_t* _datain)
{
    union16b memblock;

	AES_decrypt(_datain, memblock.plain, &dk);
    if(memblock.first == 1235){
        memblock.first = memblock.second;
        getrandnum(&memblock.second);
        randnum = memblock.second;
		AES_encrypt(memblock.plain, _datain, &ek);
        return 0;
    }else{
        return -1;
    }
}

int qdsport::auth45(uint8_t* _datain)
{
    union16b memblock;
//    AES128_ECB_decrypt(_datain, RoundKey);
//    memcpy(memblock.plain, _datain, 16);
	AES_decrypt(_datain, memblock.plain, &dk);
    if(memblock.first == randnum){
        settime((time_t)memblock.second);
        return 0;
    }else{
        return -1;
    }
}

int qdsport::heartbeatsend(void)
{
    if(authstat == AUTH_PASS){
        uint8_t data[9] = {
            0x05,0,0,0,0,0,0,0,0
        };
        return lnksend(data, 9);
    }else{
        return ERROR_UNAUTH;
    }
}

#define PORTSTRINGLENGTH 6

int qdsport::serverswitch(uint8_t *p, uint8_t len)
{
    uint8_t i;
    for(i = 0;i < len; i++){
        if(p[i] == ':'){
            p[i] = 0;
            break;
        }
    }
    if(i >= len - 2){
        return ERROR_DOMAIN;
    }else{
        char numberstring[PORTSTRINGLENGTH];
        uint8_t j;
        uint16_t port;
        for(j = 0,i++;(i < len)&&(j < PORTSTRINGLENGTH - 1);i++,j++){
            if((p[i] >= '0')&&(p[i] <= '9')){
                numberstring[j] = p[i];
            }else{
                return ERROR_DOMAIN;
            }
        }
        numberstring[j] = 0;
        port = atoi(numberstring);
        if(port != 0){
            ServerChange(p, port);
        }
    }
    return 0;
}


void qdsport::settime(time_t _time)
{
   
}

void qdsport::gettime(time_t *_time)
{
    *_time = time(0);
}

int qdsport::dataencrypt(const uint8_t *textdata, const uint8_t textsize, uint8_t *chpherdata)
{
    if(textsize < 0xE0){
        uint8_t div = 1 + (textsize / 16);
        uint8_t outsize = (div + 1) * 16;
        gettime(&_port.packtime);
        _port.outINC++;
		AES_encrypt(_port.headermem, chpherdata, &ek);

        uint8_t aessum[16];
        uint8_t j;
        for(j = 0u; j < div; j++){
            uint8_t k;
            for(k = 0u; k < 16; k++){
                uint8_t index = j*16 + k;
                if(index < textsize){
                    aessum[k] = textdata[index] ^ chpherdata[index];
                }else if(index == textsize){
                    aessum[k] = chpherdata[index];
                }else{
                    aessum[k] = chpherdata[index] ^ 0xff;
                }
            }
			AES_encrypt(aessum, chpherdata + (j + 1)*16, &ek);
        }
        return outsize;
    } else {
        return -1;
    }
}


int qdsport::packhandle(SPack* pack)
{
    if(pack->did == did){
        if(pack->cid == 0x0000){
            switch(pack->cmd){
                case(LNKCMD_AUTH2):{
                    if(pack->datalen >= 16 + 1){
                        if(0 == auth23(pack->data + 1)){
                            pack->cmd = LNKCMD_AUTH3;
                            return lnksend(pack->data, 16 + 1);
                        }else{
                            return ERROR_AUTH23;
                        }
                    }else{
                        return ERROR_DATALEN;
                    }
                }break;
                case(LNKCMD_AUTH4):{
                    if(pack->datalen >= 16 + 1){
                        if(0 == auth45(pack->data + 1)){
                            authstat = AUTH_PASS;
                            upevent_cb();
                            return heartbeatsend();
                        }else{
                            return ERROR_AUTH45;
                        }
                    }else{
                        return ERROR_DATALEN;
                    }
                }break;
                case(LNKCMD_HEARTBEAT5):{
                    if(pack->datalen >= 8 + 1){
                        if(authstat == AUTH_PASS){
                            time_t servertime;
                            memcpy(&servertime, pack->data + 1, 8);
                            settime(servertime);
                            connWDF();
                            return ERROR_OK;
                        }else{
                            return ERROR_UNAUTH;
                        }
                    }else{
                        return ERROR_DATALEN;
                    }
                }break;
                case(LNKCMD_SERVERSWITCH6):{
                    if(pack->datalen >= 4 + 1){
                        if(authstat == AUTH_PASS){
                            pack->data[pack->datalen] = 0;
                            int len = datadecrypt(pack->data + 1, pack->datalen - 1);
                            if(len > 0){
                                if(serverswitch(pack->data + 1, len) != 0){
                                    pack->cmd |= 0x80;
                                }
                                return lnksend(pack->data, 1);
                            }else{
                                return ERROR_DECRYPT;
                            }
                        }else{
                            return ERROR_UNAUTH;
                        }
                    }else{
                        return ERROR_DATALEN;
                    }
                }break;
                case(LNKCMD_SETAESK7):{
                    if(pack->datalen >= 32 + 1){
                        if(authstat == AUTH_PASS){
                            pack->data[pack->datalen] = 0;
                            int len = datadecrypt(pack->data + 1, pack->datalen - 1);
                            if(len >= 16){
                                if(setaesk(pack->arg) != 0){
                                    pack->cmd |= 0x80;
                                }
                                return lnksend(pack->data, 1);
                            }else{
                                return ERROR_DECRYPT;
                            }
                        }else{
                            return ERROR_UNAUTH;
                        }
                    }else{
                        return ERROR_DATALEN;
                    }
                }break;
                default:{
                    if(authstat == AUTH_PASS){
                        pack->cmd |= 0x80;
                        return lnksend(pack->data, pack->datalen);
                    }else{
                        return ERROR_UNAUTH;
                    }
                }break;
            }
        }else if(pack->cid == cid){
            if(authstat == AUTH_PASS){
                int len = datadecrypt(pack->data, pack->datalen);
                if(len > 0){
                    if(0 == mypackdata(pack->data, len)){
                        return ERROR_OK;
                    }else{
                        return ERROR_CMDRUN;
                    }
                }else{
                    return ERROR_DECRYPT;
                }
            }else{
                return ERROR_UNAUTH;
            }
        }else{
            return ERROR_WRONGCLASS;
        }
    }else if(authstat == AUTH_PASS){
        int len = datadecrypt(pack->data, pack->datalen);
        if(len > 0){
            pack->datalen = len;
            return notmypack(pack);
        }else{
            return ERROR_DECRYPT;
        }
    }else{
		return ERROR_UNAUTH;
	}
}


int qdsport::lnksend(const uint8_t *data, uint8_t len)
{
    return packsend(0x0000, data, len);
}

int qdsport::sendwhb(void)
{
    uint8_t data = 0x01;
    return lnksend(&data, 1);
}

int qdsport::datadecrypt(uint8_t *_data, uint8_t _inlen)
{
    uint8_t div = _inlen / 16u;
    uint8_t mod = _inlen % 16u;
    if(mod == 0 && div > 1){
        uint8_t plaintext[16];
		AES_decrypt(_data, plaintext, &dk);
        time_t timedif;
        time_t now;
        int32_t tmpINC;
        memcpy(&timedif, plaintext, sizeof(timedif));
        gettime(&now);
        timedif -= now;
        memcpy(&tmpINC, plaintext + 8 + 4, sizeof(tmpINC));

        if((tmpINC > _port.inINC) && (20 > timedif) && (-20 < timedif)){
            _port.inINC = tmpINC;

            uint8_t i;
            for(i = 1u; i < div; i++){
				AES_decrypt(_data + i*16, plaintext, &dk);
                uint8_t j;
                for(j = 0u; j < 16; j++){
                    _data[(i-1)*16 + j] ^= plaintext[j];
                }
            }

            for(_inlen -= 16 + 1; _inlen > 0; _inlen--){
                if(_data[_inlen] == 0){
                    break;
                }else if(_data[_inlen] != 0xff){
					GLOGPRINTF0("datadecrypt: pad error\n");
                    return -1;
                }
            }
            return _inlen;
        }else{
            GLOGPRINTF0("datadecrypt: time or INC varify fail! "
			"myinINC=%d, inINC=%d, timedif=%ld, did=0x%016lX, cid=0x%04X\n"
			, _port.inINC, tmpINC, timedif, did, cid);
            return -1;
        }
    }else{
		GLOGPRINTF0("datadecrypt: block error\n");
        return -1;
    }
}

