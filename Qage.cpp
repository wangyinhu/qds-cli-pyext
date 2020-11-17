#include "Qage.h"


Qage::Qage(uint32_t _deathAge, uint32_t _eventsRateTh):
    dead(false),
	myage(0),
	deathAge(_deathAge),
    events(0),
	eventsRateTh(_eventsRateTh)
{

}

Qage::~Qage()
{
}

void Qage::setAgeTh(uint32_t _deathAge, uint32_t _eventsRateTh)
{
	myage = 0;
	deathAge = _deathAge;
    events = 0;
	eventsRateTh = _eventsRateTh;
	
}

void Qage::refresh(void)
{
    myage = 0;
    events++;
}

bool Qage::isOnline(void) const
{
    return !dead;
}

int Qage::checker(const uint32_t _elapsed)
{
	int rtn;
	myage += _elapsed;
    if(myage >= deathAge){
		myage = deathAge;
        if(!dead){
            rtn = OFFLINEEVENT;      //emit offline message
			dead = true;
        }else{
            rtn = OFFLINESTATUS;      // message
		}
    }else{
		if(events > (eventsRateTh * _elapsed)){
			rtn = DEVICEEVENTFAULT;      //device fault: request too offten
		}else{
			rtn = DEVICEOK;
		}
		events = 0;
    }
    return rtn;
}
