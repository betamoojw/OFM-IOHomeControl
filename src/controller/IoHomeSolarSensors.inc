// RS100 direct replies never borrow the unsolicited-event inbound trust flag.
IoHomeController::SolarSensorNode *IoHomeController::findSolarSensor(uint8_t channel)const {
    for(auto *n=mSolarSensors.get();n;n=n->next.get())if(n->state.channel==channel)return n;return nullptr;
}
bool IoHomeController::solarIdentityMatches(const IoHomeSensorSubscription::State &s)const {
    const auto *ch=mModule&&s.channel<16?mModule->getChannel(s.channel):nullptr;
    return s.enabled&&ch&&s.gateway==mOwnNodeId&&!std::memcmp(s.systemKey,mSystemKey,16)&&
        sampleIdentityMatches(s.channel,s.node,s.key,s.revision)&&
        IoHomeSensorSubscription::rs100(ch->getProtocolIdentity(),ch->getProductIdentityEvidence(),s.node);
}
const IoHomeSensorSubscription::State *IoHomeController::solarSensor(uint8_t channel)const {
    auto *n=findSolarSensor(channel);return n&&solarIdentityMatches(n->state)?&n->state:nullptr;
}
bool IoHomeController::configureSolarSensor(IoHomecontrolChannel *channel,bool enabled) {
    const uint8_t c=channelIndexFor(channel);if(!channel||c>=16)return false;
    auto *n=findSolarSensor(c);
    if(!enabled){if(n){n->state={};n->state.channel=c;}return true;}
    if(!idleForManagedOperation()||!mModule||!mModule->managementRequestsAllowed()||!channel->isPaired()||channel->is1W()||
       !channel->productContextRevision()||!IoHomeSensorSubscription::rs100(channel->getProtocolIdentity(),channel->getProductIdentityEvidence(),channel->getNodeId()))return false;
    if(!n){std::unique_ptr<SolarSensorNode> added(new(std::nothrow) SolarSensorNode);if(!added)return false;
        added->next=std::move(mSolarSensors);mSolarSensors=std::move(added);n=mSolarSensors.get();}
    n->state={};auto &s=n->state;s.channel=c;s.enabled=true;s.node=channel->getNodeId();s.gateway=mOwnNodeId;s.revision=channel->productContextRevision();
    std::memcpy(s.key,channel->getEncryptionKey(),16);std::memcpy(s.systemKey,mSystemKey,16);return true;
}
bool IoHomeController::solarEntryMatches(const IoHomeQueueEntry &entry)const {
    const auto *s=solarSensor(entry.sourceChannelIndex);
    return s&&mModule&&!mModule->getChannel(entry.sourceChannelIndex)->isMoving()&&s->pending&&entry.sensorSubscriptionToken&&entry.sensorSubscriptionToken==s->token&&s->node==entry.destNodeId&&
        entry.command==IoHomeCommand::SubscribeEvent&&entry.productContextRevision==s->revision&&entry.managementRead&&
        !std::memcmp(entry.managementKey,s->key,16)&&uint32_t(millis()-s->startedMs)<15000U;
}
bool IoHomeController::requestSolarSensor(IoHomecontrolChannel *channel,uint16_t event) {return queueSolarSensor(channel,event,false);}
bool IoHomeController::queueSolarSensor(IoHomecontrolChannel *channel,uint16_t event,bool retryAllowance) {
    const uint8_t c=channelIndexFor(channel);auto *n=findSolarSensor(c);uint8_t payload[3];
    if(!n||!solarIdentityMatches(n->state)||!IoHomeSensorSubscription::build(event,payload)||mNextSolarToken==UINT32_MAX||
       !idleForManagedOperation()||!mModule||!mModule->managementRequestsAllowed()||backgroundPollPauseRemaining()||
       !diagnosticRfAdmissionAllowed()||channel->isMoving())return false;
    auto &s=n->state;const uint8_t index=event==0x2001;const uint32_t now=millis();
    if(s.pending||(s.started[index]&&uint32_t(now-s.lastStartMs[index])<900000U&&
       !(retryAllowance&&s.poll.retry&&s.poll.due(now)==event)))return false;
    s.token=++mNextSolarToken;s.event=event;s.startedMs=now;s.pending=true;
    IoHomeQueueEntry e{};e.destNodeId=s.node;e.encKey=s.key;e.command=IoHomeCommand::SubscribeEvent;e.sourceChannelIndex=c;
    e.productContextRevision=s.revision;e.managementRead=true;std::memcpy(e.managementKey,s.key,16);
    e.sensorSubscriptionToken=s.token;e.twoWayTxFreqIdx=frequencyIndexForHz(kNormal2WTxFreqHz);e.maxAttempts=1;e.background=true;e.active=true;
    if(!queuePush(e)){s.pending=false;return false;}
    s.outcome=1;s.started[index]=true;s.lastStartMs[index]=now;return true;
}
void IoHomeController::serviceSolarSensors() {
    for(auto *n=mSolarSensors.get();n;n=n->next.get()){
        auto &s=n->state;
        if(s.enabled&&!solarIdentityMatches(s)){const auto c=s.channel;s={};s.channel=c;s.outcome=5;continue;}
        if(s.pending&&uint32_t(millis()-s.startedMs)>=15000U){s.pending=false;s.outcome=6;
            if(s.poll.waiting)s.poll.finish(millis(),s.event,false);}
        if(s.enabled&&!s.pending){const uint16_t event=s.poll.due(millis());
            if(event&&queueSolarSensor(mModule->getChannel(s.channel),event,true))s.poll.waiting=true;
        }
        if(s.poll.misses[0]>=3)s.sun.valid=false;if(s.poll.misses[1]>=3)s.battery.valid=false;
        for(auto *sample:{&s.sun,&s.battery})if(sample->valid&&uint32_t(millis()-sample->verifiedMs)>=1800000U)sample->valid=false;
    }
}
void IoHomeController::finishSolarSensor(const IoHomeQueueEntry &entry,IoHomeCommandExchangeResult result) {
    auto *n=findSolarSensor(entry.sourceChannelIndex);if(!n||n->state.token!=entry.sensorSubscriptionToken)return;
    auto &s=n->state;
    if(!solarIdentityMatches(s)){s.enabled=false;s.pending=false;s.sun.valid=s.battery.valid=false;s.outcome=5;return;}
    if(!s.pending)return;
    if(!solarEntryMatches(entry)){s.pending=false;s.outcome=3;if(s.poll.waiting)s.poll.finish(millis(),s.event,false);return;}
    if((result==IoHomeCommandExchangeResult::MediaAccessFailed||mLastTxDutyDenied)&&!entry.hadPeerResponse){
        s.pending=false;s.outcome=7;s.started[s.event==0x2001]=false;if(s.poll.waiting){s.poll.waiting=false;s.poll.dueMs=millis()+60000U;}return;
    }
    bool accepted=false;
    if(result==IoHomeCommandExchangeResult::Completed&&mSawChallenge&&mAuthResponseSent&&mWaitingFinalResponse&&
       mLastResponseFreqIdx==entry.twoWayTxFreqIdx)
        accepted=IoHomeSensorSubscription::decode(s.event,mRxFrame,s.event==0x2005?s.sun:s.battery,mLastResponseFreqIdx,mLastRxReadMs,millis());
    if(accepted){s.lastReplyLength=mRxFrame.dataLen;std::memcpy(s.lastReply,mRxFrame.data,mRxFrame.dataLen);}
    if(s.poll.waiting)s.poll.finish(millis(),s.event,accepted);
    s.pending=false;s.outcome=accepted?2:result==IoHomeCommandExchangeResult::ExplicitlyRejected?4:3;
}
void IoHomeController::printSolarSensor(uint8_t channel)const {
    const auto *s=solarSensor(channel);if(!s){logInfoP("Solar sensor disabled or identity changed ch=%u",unsigned(channel+1));return;}
    for(uint8_t i=0;i<2;++i){const auto &v=i?s->battery:s->sun;const bool fresh=v.valid&&uint32_t(millis()-v.verifiedMs)<1800000U;
        logInfoP("Solar sensor ch=%u event=%04X valid=%u raw=%u aux=%02X unit=%s received_ms=%lu verified_ms=%lu age_ms=%lu frequency=%u proof=outbound_challenge_correlated pending=%u outcome=%u",
            unsigned(channel+1),i?0x2001:0x2005,fresh,v.raw,v.auxiliary,i?"percent_scale_only_100_captured":"UNKNOWN",
            (unsigned long)v.receivedMs,(unsigned long)v.verifiedMs,(unsigned long)uint32_t(millis()-v.verifiedMs),v.frequency,s->pending,s->outcome);
    }
}

bool IoHomeController::configureSolarPoll(IoHomecontrolChannel *channel,uint8_t mask,uint32_t seconds) {
    auto *n=findSolarSensor(channelIndexFor(channel));
    return n&&solarIdentityMatches(n->state)&&!n->state.pending&&
        n->state.poll.configure(millis(),mask,seconds,n->state.node^millis(),n->state.channel);
}
