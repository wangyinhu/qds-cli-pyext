#include "SPacker.h"
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <errno.h>
#include <cstdio>

SPacker::SPacker()
{

}

SPacker::~SPacker()
{
}

int SPacker::newpacks(std::list<SPack*>* rtnlist, void* buf, int len)
{
	int rtnerror = 0;
	//finish pendding job first
	if(inbuffcutheadsize != 0){
		inbuff.cuthead(inbuffcutheadsize);
		inbuffcutheadsize = 0;
	}
	
	auto p = (char*)buf;
	auto l = len;
	auto rtn = ispackvalide(p, l);
	if((rtn == XRTN_LONGPACK)||(rtn == XRTN_OK)){
		inbuff.clear();
	}
	while(rtn == XRTN_LONGPACK){
		rtnlist->push_back((SPack*)p);
		l -= SPACKSIZE((SPack*)p);
		p += SPACKSIZE((SPack*)p);
		rtn = ispackvalide(p, l);
	}
	
	if(rtn == XRTN_OK){
		rtnlist->push_back((SPack*)p);
	}else if((rtn == XRTN_SHORTPIECE) || (rtn == XRTN_SHORTPACK) || !inbuff.empty()){
		//save pack to vector
		inbuff.append((char*)p, l);

		auto p = (char*)inbuff.data();
		auto l = inbuff.size();
		auto rtn = ispackvalide(p, l);

		while(rtn == XRTN_LONGPACK){
			rtnlist->push_back((SPack*)p);
			inbuffcutheadsize += SPACKSIZE((SPack*)p);
			l -= SPACKSIZE((SPack*)p);
			p += SPACKSIZE((SPack*)p);
			rtn = ispackvalide(p, l);
		}
		
		if(rtn == XRTN_OK){
			rtnlist->push_back((SPack*)p);
			inbuffcutheadsize += SPACKSIZE((SPack*)p);
		}else if((rtn == XRTN_SHORTPIECE) || (rtn == XRTN_SHORTPACK)){
			//still short,wait more data
		}else{ //drop pack : XRTN_VIRERROR, XRTN_CHECKSUMERROR, XRTN_ERRORPACK
			GLOGPRINTF0("inbuff dropped dpacker rtn=%d\n", rtn);
			GLOGPRINTHEX0(p, l);
			inbuff.clear();
			rtnerror = -10;
		} 		
			
	}else{ //drop pack : XRTN_VIRERROR, XRTN_CHECKSUMERROR, XRTN_ERRORPACK
		GLOGPRINTF0("new dropped dpacker rtn=%d\n", rtn);
		GLOGPRINTHEX0(p, l);
		rtnerror = -11;
	}
    if(inbuff.size()) printf("ibsz=%ld\n", inbuff.size());
	return rtnerror;
}



int SPacker::ispackvalide(void* buf, uint32_t len) const
{
	SPack* pack = (SPack*)buf;
	if(len < MINIMUMPACKLENTH){
		if(len == 0){
			return XRTN_EMPTYPACK;
		}else if(pack->SoF != STARTOFFRAME){
			return XRTN_SOFERROR;
		}else{
			return XRTN_SHORTPIECE;
		}
	}
	
	if(pack->SoF != STARTOFFRAME){
			return XRTN_SOFERROR;
	}else{
		if((pack->packver != 0) && (pack->packver < DPACKVERSION_TOP)){
			if(SPACKSIZE(pack) == len){
				if(pack->data[pack->datalen] == xor_checksum(0, pack->data, pack->datalen)){
					return XRTN_OK;
				}else{
					return XRTN_CHECKSUMERROR;
				}
			}else if(SPACKSIZE(pack) > len){
				return XRTN_SHORTPACK;
			}else if(SPACKSIZE(pack) < len){
				if(pack->data[pack->datalen] == xor_checksum(0, pack->data, pack->datalen)){
					return XRTN_LONGPACK;
				}else{
					return XRTN_ERRORPACK;
				}
				
			}else{
				//not reachable!
				return XRTN_OK;
			}
			
		}else{
			return XRTN_VIRERROR;
		}
	}
}


int SPacker::send(SPack* _packout)
{
	if(_packout != 0){
		_packout->data[_packout->datalen] = xor_checksum(0, _packout->data, _packout->datalen);
		auto count = writeBlocking(_packout, SPACKSIZE(_packout));
		if(count == -1){
			GLOGPRINTF2("In SPacker::send() write()  error=%s\n", strerror(errno));
			return -1;
		}else{
			return 0;
		}
		
	}else{
		return -1;
	}
}

int SPacker::SPackcpy(SPack* dest, SPack* source)
{
	memcpy(dest, source, SPACKSIZE(source));
	dest->data[dest->datalen] = 0;
	return 0;
}

int sbvector::cuthead(uint32_t _size)
{
	auto presize = size();
	
	if(_size > presize){
		resize(0);
		return 0;
	}else{
		auto leftsize = presize - _size;
		auto p = data();
		for(auto i = 0u; i < leftsize; i++){
			p[i] = p[i + _size];
		}
		resize(leftsize);
		return leftsize;
	}
	
}

int sbvector::append(const void* _msg, uint32_t _size)
{
	auto presize = size();
	resize(presize + _size);
	memcpy(data() + presize, _msg, _size);
	return size();
}
