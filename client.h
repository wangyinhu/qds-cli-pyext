#ifndef CLIENT_H
#define CLIENT_H

#include "qdsport.h"

#define FINISHTOCONLINE  10001
#define FINISHTOCOFFLINE  10002
#define FINISHTOCDEFAULTK  10003
#define PY_EXT_EXCEPTION  10004


typedef int (*pack_process)(uint64_t _did, uint16_t _cid, uint8_t *data, size_t datalen);

class QdsClient : public qdsport {
    int noticeflag = 0;
    int noticewaittimer = 0;
public:
    QdsClient(uint64_t _did, uint16_t _cid, const uint8_t *aesk, int Qdsport, const char *host);

    virtual ~QdsClient();

    int sockfd;

public:
    void ServerChange(uint8_t *host, int port) override;

    void connWDF(void) override;

    int mypackdata(uint8_t *data, int len) override;

    int notmypack(SPack *pack) override;

    void upevent_cb(void) override;

    int writeBlocking(const void *data, size_t len) override;

    int run(pack_process func);

    int SendTheData(const DeviceUID_Type &The_did, CLusterID_Type The_cid, const uint8_t *The_data, size_t The_datalen);

    DeviceUID_Type Thedid;
    CLusterID_Type Thecid;
    const uint8_t *Thedata;
    size_t Thedatalen;
    int timeouter = 0;
    pack_process pack_process_func = nullptr;
};

#endif // CLIENT_H
