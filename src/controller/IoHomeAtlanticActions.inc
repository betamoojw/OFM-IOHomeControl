// Typed actions reuse the bounded existing register job; no universal 0C61 endpoint.
bool IoHomeController::qualifiedHeatingName(IoHomecontrolChannel *ch,IoHomeAdelis::HeatingModel model)const {
    if(!ch||ch->selectedHeatingModel()!=model)return false;
    for(auto *p=mHeatingNames.get();p;p=p->next.get())if(p->channel==channelIndexFor(ch))
        return p->model==model&&p->gateway==mOwnNodeId&&!std::memcmp(p->systemKey,mSystemKey,16)&&
            sampleIdentityMatches(p->channel,p->node,p->key,p->revision)&&uint32_t(millis()-p->receivedMs)<300000U;
    return false;
}
void IoHomeController::recordHeatingModelName(IoHomecontrolChannel *ch,const uint8_t *data,uint8_t length,uint32_t rx) {
    if(!ch||!IoHomeAdelis::identity(ch->getProtocolIdentity(),ch->getNodeId()))return;
    HeatingNameProof *p=mHeatingNames.get();while(p&&p->channel!=channelIndexFor(ch))p=p->next.get();
    if(!p){std::unique_ptr<HeatingNameProof> entry(new(std::nothrow) HeatingNameProof);if(!entry)return;
        entry->next=std::move(mHeatingNames);mHeatingNames=std::move(entry);p=mHeatingNames.get();}
    p->channel=channelIndexFor(ch);p->node=ch->getNodeId();p->gateway=mOwnNodeId;p->revision=ch->productContextRevision();p->receivedMs=rx;
    std::memcpy(p->key,ch->getEncryptionKey(),16);std::memcpy(p->systemKey,mSystemKey,16);p->model=IoHomeAdelis::HeatingModel::Unknown;
    for(auto model:{IoHomeAdelis::HeatingModel::Agilia,IoHomeAdelis::HeatingModel::Adelis})if(IoHomeAdelis::modelName(model,data,length))p->model=model;
}

bool IoHomeController::startAdelisAction(IoHomecontrolChannel *ch,uint16_t reg,uint8_t value) {
    uint8_t payload[6];if(!ch||ch->selectedHeatingModel()!=IoHomeAdelis::HeatingModel::Adelis||!IoHomeAdelis::write(reg,value,payload))return false;
    if(!startAtlanticRegisterRead(ch,1,0x19,1,IoHomeAtlanticRead::Model::Raw))return false;
    auto &s=*mRegisterRead;s.action=IoHomeAtlanticRead::Action::AdelisWrite;s.model=IoHomeAtlanticRead::Model::AdelisClassic;
    s.count=3;s.writeIndex=1;s.desired=value;s.samples[1].reg=s.samples[2].reg=reg;return true;
}
bool IoHomeController::startDhwMode(IoHomecontrolChannel *ch,uint8_t mode) {
    if(!ch||mode>2||!IoHomeDhwCodec::identity(ch->getProtocolIdentity(),ch->getNodeId())||!startAtlanticRegisterRead(ch,0,6,1,IoHomeAtlanticRead::Model::Calypso))return false;
    auto &s=*mRegisterRead;s.action=IoHomeAtlanticRead::Action::DhwMode;s.count=4;s.writeIndex=1;s.desired=mode;
    s.samples[1].reg=s.samples[2].reg=6;s.samples[3].reg=0xFFFF;return true;
}
bool IoHomeController::startDhwTarget(IoHomecontrolChannel *ch,float celsius) {
    uint16_t target=0;if(!ch||!IoHomeDhwCodec::encodeConfirmedTarget(celsius,200,true,target)||
       !IoHomeDhwCodec::identity(ch->getProtocolIdentity(),ch->getNodeId())||!startAtlanticRegisterRead(ch,0,8,1,IoHomeAtlanticRead::Model::Calypso))return false;
    auto &s=*mRegisterRead;s.action=IoHomeAtlanticRead::Action::DhwTarget;s.count=5;s.writeIndex=2;s.targetRaw=target;
    s.samples[1].reg=6;s.samples[2].reg=0xFFFF;s.samples[3].reg=6;s.samples[4].reg=0xFFFF;return true;
}
bool IoHomeController::decodeAtlanticJobReply(const IoHomeQueueEntry &entry,IoHomeAtlanticRead::Sample &out)const {
    if(!mRegisterRead)return false;const auto &s=*mRegisterRead;const uint16_t reg=s.samples[s.completed].reg;
    if(entry.command==IoHomeCommand::Private||entry.command==IoHomeCommand::Execute){
        uint16_t target;if(!IoHomeDhwCodec::statusTarget(mRxFrame,target))return false;
        out={};out.reg=reg;out.result=IoHomeAtlanticRead::Result::Value;out.length=mRxFrame.dataLen;std::memcpy(out.data,mRxFrame.data,out.length);return true;
    }
    if(s.completed==s.writeIndex){
        if(mRxFrame.commandId!=IoHomeCommand::WritePrivateResponse||mRxFrame.dataLen!=entry.heatingLength||std::memcmp(mRxFrame.data,entry.heatingData,entry.heatingLength))return false;
        out={};out.reg=reg;out.result=IoHomeAtlanticRead::Result::Value;out.length=mRxFrame.dataLen-4;std::memcpy(out.data,mRxFrame.data+4,out.length);return true;
    }
    return mRxFrame.commandId==IoHomeCommand::WritePrivateResponse&&IoHomeAtlanticRead::reply(reg,mRxFrame.data,mRxFrame.dataLen,out);
}
#ifdef TEST_NATIVE
// Precondition for existing AGILIA regressions: a prior synthetic qualified name read.
void IoHomeController::testSeedAgiliaName(IoHomecontrolChannel *ch) {
    const uint8_t name[]={'A','G','I','L','I','A',' ','I','O'};recordHeatingModelName(ch,name,sizeof(name),millis());
}
#endif
