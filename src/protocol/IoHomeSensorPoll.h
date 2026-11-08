#pragma once
#include <cstdint>

// One schedule per explicit sensor permission, not a second RF/exchange engine.
struct IoHomeSensorPoll {
    uint32_t intervalMs=0,dueMs=0,random=1;uint8_t mask=0,remaining=0,misses[2]{};
    bool retry=false,waiting=false;
    uint32_t interval() {
        random^=random<<13;random^=random>>17;random^=random<<5;
        const uint32_t spread=intervalMs/10;
        const uint32_t value=intervalMs-spread+random%(spread*2+1);
        return value<900000U?900000U:value;
    }
    bool configure(uint32_t now,uint8_t enabled,uint32_t seconds,uint32_t seed,uint8_t channel) {
        if(enabled>3||(enabled&&(seconds<900||seconds>86400)))return false;
        *this={};if(!enabled)return true;mask=enabled;intervalMs=seconds*1000U;random=seed?seed:1;
        dueMs=now+interval()+uint32_t(channel+1)*1000U;return true;
    }
    uint16_t due(uint32_t now) {
        if(!mask||waiting||int32_t(now-dueMs)<0)return 0;
        if(!remaining)remaining=mask;
        return remaining&1?0x2005:0x2001;
    }
    void finish(uint32_t now,uint16_t event,bool success) {
        waiting=false;const uint8_t i=event==0x2001,bit=1U<<i;
        if(success)misses[i]=0;
        else if(!retry){retry=true;dueMs=now+60000U;return;}
        else if(misses[i]<255)++misses[i];
        retry=false;remaining&=~bit;
        dueMs=now+(remaining?1000U:interval());
    }
};
