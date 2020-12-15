#include "client.h"
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <openssl/sha.h>

#define BUFFERSIZE 5000

#define MAXNOTICEWAITTIME 5


#define SENDQDSTIMEOUT		3


QdsClient::QdsClient(const uint64_t _did, const uint16_t _cid, const uint8_t *aesk, const int Qdsport, const char *host):
	qdsport(_did, _cid, aesk)
{
	/* Create a socket point */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}
	struct timeval tv;
	tv.tv_sec = 1;  /* 1 Secs Timeout */
	tv.tv_usec = 0;  // Not init'ing this can cause strange errors
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));

	server = gethostbyname(host);

	if (server == NULL) {
		perror("ERROR, gethostbyname");
		exit(1);
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(Qdsport);

	/* Now connect to the server */
	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		exit(1);
	}

	sendwhb();
}

QdsClient::~QdsClient()
{
	close(sockfd);
}

void QdsClient::ServerChange(uint8_t* host, int port)
{
}

void QdsClient::connWDF(void)
{
}

int QdsClient::mypackdata(uint8_t* data, int len)
{
	if(strncmp((char*)data, "OK set notice port", sizeof("OK set notice port")) == 0) {
		//GLOGPRINTF0("OK set notice port\n");
		noticeflag = 1;
	}
	return 0;
}

int QdsClient::notmypack(SPack* pack)
{
    if(pack_process_func){
        return pack_process_func(pack->did, pack->cid, pack->data, pack->datalen);
    } else {
        if(pack->did == Thedid && pack->cid == 0){
            if(0 == strcmp("OK sent", (char*)pack->data)){
                return FINISHTOCONLINE;
            }else if(0 == strcmp("error 5:default key", (char*)pack->data)){
                return FINISHTOCDEFAULTK;
            }else{
                return FINISHTOCOFFLINE;
            }
        }else{
            return 0;
        }
    }
}


int QdsClient::SendTheData(const DeviceUID_Type &The_did, CLusterID_Type The_cid, const uint8_t *The_data, size_t The_datalen)
{
	if(sockfd > 0){
		Thedid = The_did;
		Thecid = The_cid;
		Thedata = The_data;
		Thedatalen = The_datalen;
		return 0;
	}else{
		return -1;
	}
}


void QdsClient::upevent_cb(void) {
    if (pack_process_func) {
        appsend("notice", 7);
    } else {
        packsend(Thedid, Thecid, Thedata, Thedatalen);
    }

}

int QdsClient::writeBlocking(const void* data, size_t len)
{
	return write(sockfd, data, SPACKSIZE(data));
}

int QdsClient::run(pack_process func)
{
	uint8_t buffer[BUFFERSIZE];
    pack_process_func = func;
	while(true) {
		auto n = read(sockfd, buffer, BUFFERSIZE);
		if(n > 0) {
			std::list<SPack*> packlist;
			auto rtn = newpacks(&packlist, buffer, n);
			if(packlist.size() >= 5){
                printf("n=%ld\tpksz=%ld", n, packlist.size());
            }
			if(rtn == 0) {
				for(auto &pack : packlist) {
					auto he = packhandle(pack);
					if(he == FINISHTOCONLINE or he == FINISHTOCOFFLINE or he == FINISHTOCDEFAULTK or he == PY_EXT_EXCEPTION){
						return he;
					}else if(0 != he) {
						GLOGPRINTF0("packhandle fail! rtn=%d did=0x%016lX, cid=0x%04X\n", he, pack->did, pack->cid);
						close(sockfd);
						return -1;
					}
				}
			} else {
				GLOGPRINTF0("newpacks fail! rtn=%d\n", rtn);
				close(sockfd);
				return -2;
			}
		} else {
			if(errno == EAGAIN) {
				if(pack_process_func == nullptr and timeouter++ > SENDQDSTIMEOUT){
					GLOGPRINTF0("timeout\n");
					return -5;
				}
				periodcall(1);
				if(noticewaittimer++ >= MAXNOTICEWAITTIME){
					noticewaittimer = MAXNOTICEWAITTIME;
					if(noticeflag == 0){
						GLOGPRINTF0("notice set fail!\n");
						return -4;
					}
				}
				//appsend((const uint8_t*)"hello server", 13);
			} else {
				GLOGPRINTF0("qds socket read error=%s\n", strerror(errno));
				return -3;
			}
		}
	}
}

