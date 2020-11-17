#ifndef SPACKER_H
#define SPACKER_H


#include "Qage.h"
#include <vector>
#include <list>

using DStartOfFrame_Type = uint8_t;
using DPackVersion_Type = uint8_t;
using DeviceUID_Type = uint64_t;
using CLusterID_Type = uint16_t;
using DDataLen_Type = uint8_t;
using Session_Type = uint8_t;
using DChecksum_Type = uint8_t;

#define				STARTOFFRAME		(DStartOfFrame_Type)0xFE


static constexpr unsigned int SPACKHEADSIZE	=	sizeof(DStartOfFrame_Type) + 
												sizeof(DPackVersion_Type) + 
												sizeof(DeviceUID_Type) + 
												sizeof(CLusterID_Type) + 
												sizeof(DDataLen_Type);
										
static constexpr unsigned int SPACKSHELLSIZE	=	SPACKHEADSIZE + sizeof(DChecksum_Type);
										
static constexpr unsigned int MINIMUMPACKLENTH  = 	SPACKSHELLSIZE + sizeof(Session_Type);


enum : DPackVersion_Type{
	DPACKVERSION_1 = 0x01,
	DPACKVERSION_2 = 0x02,
	DPACKVERSION_3 = 0x03,
	DPACKVERSION_TOP,
};

class sbvector : public std::vector<char>
{
public:	
	int append(const void *_msg, uint32_t _size);
	int cuthead(uint32_t _size);
};

struct SPack{
	DStartOfFrame_Type	SoF;
	DPackVersion_Type 	packver;
	DeviceUID_Type		did;
	CLusterID_Type		cid;
	DDataLen_Type		datalen;
	union {
		uint8_t data[255 + 1];
		struct {
			Session_Type cmd;
			uint8_t arg[1];
		} __attribute__((packed));
	} __attribute__((packed));
} __attribute__((packed));

#define SPACKSIZE(pack)         (((SPack*)(pack))->datalen + SPACKSHELLSIZE)

class SPacker
{
public:
    SPacker();
	virtual ~SPacker();
	virtual int						newpacks(std::list<SPack*>* rtnlist, void* buf, int len);
	virtual int 					send(SPack* _packout);
	static int SPackcpy(SPack *dest, SPack *source);
    virtual int writeBlocking(const void *data, size_t len) = 0;
private:
	//return type
	enum : uint32_t {
		XRTN_OK = 0,
		XRTN_LONGPACK,
		XRTN_CHECKSUMERROR,
		XRTN_SHORTPACK,
		XRTN_SHORTPIECE,
		XRTN_VIRERROR,
		XRTN_ERRORPACK,
		XRTN_EMPTYPACK,
		XRTN_SOFERROR,
	};
	
	sbvector inbuff;
	int inbuffcutheadsize;
	int ispackvalide(void* buf, uint32_t len) const;
};

#endif // SPACKER_H
