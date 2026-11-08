#pragma once
#include "IoHomeLocalSensorEvent.h"
#include "IoHomeSensorPoll.h"
#include <cstring>

// https://github.com/laberning/home_io_control/commit/a0d103591f3d340975a40745b894fbfbb85f9237
namespace IoHomeSensorSubscription {
inline bool build(uint16_t event,uint8_t (&data)[3]) {
    if(event!=0x2005&&event!=0x2001)return false;
    data[0]=1;data[1]=event>>8;data[2]=event;return true;
}
inline bool battery(const IoHomeFrame &frame,uint8_t &out) {
    if((frame.ctrlByte0&IOHC_CTRL0_MODE_1W)||frame.commandId!=IoHomeCommand::StatusUpdate||frame.dataLen!=6||
       frame.data[2]!=0x20||frame.data[3]!=1||frame.data[4]!=1||frame.data[5]>100)return false;
    out=frame.data[5];return true;
}
inline bool rs100(const IoHomeProtocolIdentity &id,const IoHomeProductIdentityEvidence &e,uint32_t node) {
    constexpr uint8_t gi1[]={'5','1','6','3','3','4','1','E','0','2'};
    return id.valid&&id.ioAddress==node&&id.manufacturerId==2&&id.profile==2&&id.subProfile==0&&
        e.generalInfo1Len>=sizeof(gi1)&&!std::memcmp(e.generalInfo1,gi1,sizeof(gi1));
}
struct Sample {
    uint32_t receivedMs=0,verifiedMs=0;uint16_t raw=0;uint8_t auxiliary=0,frequency=0xFF;bool valid=false;
};
struct State {
    uint32_t node=0,gateway=0,revision=0,token=0,startedMs=0,lastStartMs[2]{};
    uint8_t key[16]{},systemKey[16]{},channel=0xFF;uint16_t event=0;
    bool enabled=false,pending=false,started[2]{};
    IoHomeSensorPoll poll;
    uint8_t outcome=0,lastReplyLength=0,lastReply[10]{};Sample sun,battery;
};
inline bool decode(uint16_t event,const IoHomeFrame &frame,Sample &sample,uint8_t frequency,uint32_t rx,uint32_t verified) {
    Sample result;result.frequency=frequency;result.receivedMs=rx;result.verifiedMs=verified;
    if(event==0x2005){IoHomeLocalSensorEvent decoded;if(!IoHomeLocalSensorEvent::decode(frame,decoded))return false;result.raw=decoded.raw;result.auxiliary=decoded.auxiliary;}
    else if(event==0x2001){uint8_t value;if(!battery(frame,value))return false;result.raw=value;}
    else return false;
    result.valid=true;sample=result;return true;
}
}
