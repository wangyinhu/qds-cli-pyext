#ifndef QDSPORT_H
#define QDSPORT_H

#include "SPacker.h"
#include <openssl/aes.h>

int testsetaesk(const DeviceUID_Type &uid);

class qdsport : public SPacker
{
public:
    qdsport(uint64_t _did, uint16_t _cid, const uint8_t *aesk);
    virtual void upevent_cb(void) = 0;
    virtual void connWDF(void) = 0;
    virtual void ServerChange(uint8_t* host, int port) = 0;
    virtual int notmypack(SPack* pack) = 0;
    virtual int mypackdata(uint8_t *data, int len) = 0;

    enum {
        ERROR_OK = 0,
        ERROR_TOOMUCH,
        ERROR_UNAUTH,
        ERROR_AUTH23,
        ERROR_AUTH45,
        ERROR_CMDRUN,
        ERROR_DECRYPT,
        ERROR_WRONGCLASS,
        ERROR_WRONGDID,
        ERROR_IOFAIL,
        ERROR_DATALEN,
        ERROR_DOMAIN,
        ERROR_TOP
    }ERROR_TYPE;

    enum{
        LNKCMD_INVALID		 = 0x00,
        LNKCMD_AUTH1		 = 0x01,
        LNKCMD_AUTH2		 = 0x02,
        LNKCMD_AUTH3		 = 0x03,
        LNKCMD_AUTH4		 = 0x04,
        LNKCMD_HEARTBEAT5	 = 0x05,
        LNKCMD_SERVERSWITCH6 = 0x06,
        LNKCMD_SETAESK7		 = 0x07,
        LNKCMD_TOP,
    };

protected:

    typedef int64_t time_t;

    void settime(time_t time);
    void gettime(time_t *time);
    enum {
        AUTH_START = 0,
        AUTH_PASS,
        AUTH_TOP,
    } authstat = AUTH_START;

    uint64_t did;
    uint16_t cid;
    struct {
        union{
            uint8_t headermem[16];
            struct {
                int64_t packtime;
                union{
                    uint64_t randnum;
                    struct{
                        int32_t outINC;
                        int32_t inINC;
                    };
                };
            };
        };
    } _port;

    int appsend(const void *data, uint8_t len);
    int lnksend(const uint8_t *data, uint8_t len);
    int packhandle(SPack* pack);
    int sendwhb(void);
    int ispackvalide(SPack* pack, uint16_t len);
    int setaesk(const uint8_t *aesk);
    int serverswitch(uint8_t *p, uint8_t len);
    int dataencrypt(const uint8_t *textdata, const uint8_t textsize, uint8_t *chpherdata);
    int datadecrypt(uint8_t *_data, uint8_t _inlen);
    int heartbeatsend(void);
    int auth45(uint8_t* _datain);
    int auth23(uint8_t* _datain);
    int packsend(uint64_t _did, CLusterID_Type _cid, const void *data, uint8_t len);
    int packsend(CLusterID_Type _cid, const void *data, uint8_t len);
    int periodcall(const uint8_t _elapsedS);
	AES_KEY ek;
	AES_KEY dk;
    uint8_t RoundKey[176];
    uint64_t randnum;
    uint16_t holdtime = 0;

    typedef union {
        uint8_t plain[16];
        struct {
            uint64_t first;
            uint64_t second;
        };
    }union16b;
};

#endif // QDSPORT_H
