static void qualifySolarSensorForTest(IoHomecontrolChannel &ch)
{
    IoHomeProtocolIdentity id;id.valid=true;id.ioAddress=ch.getNodeId();id.profile=2;id.manufacturerId=2;id.nodeClass=IoHomeNodeClass::Actuator;
    ch.onProtocolIdentity(ch.getNodeId(),id);
    const uint8_t signature[]={'5','1','6','3','3','4','1','E','0','2'};
    ch.onPostPairEnrichmentResponse(IoHomeCommand::GetGeneralInfo1Response,signature,sizeof(signature));
}
TEST(protocol_solar_sensor_originals_decode_without_units_or_battery_invention)
{
    unsigned sun=0,battery=0;
    for(const auto &row:AlignmentCaptures::frames){if(row.sourceIndex>1)continue;IoHomeFrame frame;ASSERT_TRUE(deserializeFrameForTest(frame,row.wire,row.length));
        IoHomeSensorSubscription::Sample sample;const uint16_t event=frame.data[3]==5?0x2005:0x2001;
        ASSERT_TRUE(IoHomeSensorSubscription::decode(event,frame,sample,1,10,20));ASSERT_TRUE(sample.valid);
        if(event==0x2005){ASSERT_TRUE(sample.raw==183||sample.raw==744);++sun;}else{ASSERT_EQ(sample.raw,100);++battery;}
    }
    ASSERT_TRUE(sun>=2&&battery);
    IoHomeFrame frame;frame.init();frame.commandId=IoHomeCommand::StatusUpdate;frame.dataLen=6;
    const uint8_t body[]={5,0x60,0x20,1,1,0};memcpy(frame.data,body,6);uint8_t out=0xFF;
    ASSERT_TRUE(IoHomeSensorSubscription::battery(frame,out));ASSERT_EQ(out,0);
    for(unsigned value=101;value<256;++value){frame.data[5]=value;ASSERT_TRUE(!IoHomeSensorSubscription::battery(frame,out));}
    frame.data[5]=100;frame.data[4]=2;ASSERT_TRUE(!IoHomeSensorSubscription::battery(frame,out));
}
TEST(controller_solar_sensor_direct_reply_needs_own_outbound_proof_and_exact_context)
{
    const uint8_t key[16]={1},nonce[]={1,2,3,4,5,6},other[16]={2};
    for(unsigned variant=0;variant<12;++variant){IoHomeController c;IoHomecontrol m;IoHomecontrolChannel ch;
        initPaired2WControllerForTest(c,m,ch,0xA1B2C3,0x24635F,key);qualifySolarSensorForTest(ch);
        ASSERT_TRUE(c.configureSolarSensor(&ch,true));ASSERT_TRUE(c.requestSolarSensor(&ch,0x2005));IoHomeFrame tx,challenge,reply;
        ASSERT_TRUE(transmitQueuedControllerFrame(c,tx));ASSERT_EQ(tx.commandId,IoHomeCommand::SubscribeEvent);ASSERT_EQ(tx.dataLen,3);ASSERT_MEM_EQ(tx.data,"\x01\x20\x05",3);
        const auto &row=AlignmentCaptures::frames[1];ASSERT_TRUE(deserializeFrameForTest(reply,row.wire,row.length));
        // Choose a sun reply regardless of source-file order, with its original routing intact.
        for(const auto &r:AlignmentCaptures::frames)if(r.sourceIndex==1){ASSERT_TRUE(deserializeFrameForTest(reply,r.wire,r.length));break;}
        if(variant!=1){buildSimpleResponseFrame(challenge,0xA1B2C3,0x24635F,IoHomeCommand::ChallengeRequest,nonce,6);ASSERT_TRUE(queueControllerResponse(c,challenge));}
        if(variant==2)reply.setSrcNode(0x112233);if(variant==3)reply.setDestNode(0x112233);if(variant==4)reply.data[3]=1;
        if(variant==5)c.testSetRxScanFrequency(IOHC_FREQ_3);if(variant==6)ch.setEncryptionKey(other);
        if(variant==7)ch.invalidateProductContext();if(variant==8)ioHomeTestAdvanceMillis(16000);
        if(variant==9)c.setOwnNodeId(0x112233);if(variant==10)ch.setPaired(false);if(variant==11)reply.ctrlByte0|=IOHC_CTRL0_MODE_1W;
        const auto before=c.radio().testTransmitCount();ASSERT_TRUE(queueControllerResponse(c,reply));
        const auto *state=c.solarSensor(0);
        if(variant==0){ASSERT_TRUE(state&&state->sun.valid);ASSERT_TRUE(state->sun.raw==183||state->sun.raw==744);ASSERT_EQ(state->sun.receivedMs,ioHomeTestMillis());
            const auto proofTime=state->sun.verifiedMs;uint8_t wire[32];const auto length=serializeFrameForTest(reply,wire,sizeof(wire));
            c.testSetRxScanFrequency(IOHC_FREQ_2);c.radio().testQueueReceivedPacket(wire,length);c.loop();ASSERT_EQ(c.solarSensor(0)->sun.verifiedMs,proofTime);}
        else ASSERT_TRUE(!state||!state->sun.valid);
        ASSERT_EQ(c.radio().testTransmitCount(),before);ASSERT_TRUE(!ch.lastDeviceEvent().valid&&!ch.lastLocalSensorSample().valid&&!ch.testHasPositionFeedback()&&!ch.testHasStatusUpdate());
    }
}
TEST(controller_solar_sensor_requires_explicit_model_opt_in_and_no_generic_subscription)
{
    const uint8_t key[16]={1};IoHomeController c;IoHomecontrol m;IoHomecontrolChannel ch;
    initPaired2WControllerForTest(c,m,ch,0xA1B2C3,0x24635F,key);
    ASSERT_TRUE(!c.requestSolarSensor(&ch,0x2005));ASSERT_TRUE(!c.configureSolarSensor(&ch,true));
    qualifySolarSensorForTest(ch);ASSERT_TRUE(!c.requestSolarSensor(&ch,0x2005));ASSERT_TRUE(c.configureSolarSensor(&ch,true));
    ASSERT_TRUE(!c.requestSolarSensor(&ch,0x100A));ASSERT_TRUE(!c.sendCommand(ch.getNodeId(),key,IoHomeCommand::SubscribeEvent,1));
    ch.onStatusUpdate(true);ASSERT_TRUE(!c.requestSolarSensor(&ch,0x2005));ch.onStatusUpdate(false);
    ASSERT_TRUE(c.configureSolarSensor(&ch,false));ASSERT_TRUE(!c.requestSolarSensor(&ch,0x2001));ASSERT_EQ(c.radio().testTransmitCount(),0U);
}

TEST(protocol_solar_sensor_poll_is_bounded_staggered_and_retry_is_delayed)
{
    IoHomeSensorPoll p;ASSERT_TRUE(!p.configure(0,3,899,1,0));ASSERT_TRUE(p.configure(UINT32_MAX-1000,3,900,1,0));
    const uint32_t due=p.dueMs;ASSERT_EQ(p.due(due-1),0);ASSERT_EQ(p.due(due),0x2005);
    for(unsigned cycle=0;cycle<3;++cycle){
        const uint32_t now=p.dueMs;ASSERT_EQ(p.due(now),0x2005);p.waiting=true;p.finish(now,0x2005,false);
        ASSERT_EQ(p.due(now+59999),0);ASSERT_EQ(p.due(now+60000),0x2005);p.waiting=true;p.finish(now+60000,0x2005,false);
        ASSERT_EQ(p.misses[0],cycle+1);ASSERT_EQ(p.due(p.dueMs),0x2001);p.waiting=true;p.finish(p.dueMs,0x2001,true);
        ASSERT_EQ(p.misses[1],0);ASSERT_TRUE(uint32_t(p.dueMs-(now+61000))>=900000U);
    }
    ASSERT_TRUE(p.configure(0,0,0,1,0));ASSERT_EQ(p.due(UINT32_MAX),0);
}
TEST(controller_solar_sensor_poll_is_explicit_and_reuses_existing_exchange_queue)
{
    const uint8_t key[16]={1};IoHomeController c;IoHomecontrol m;IoHomecontrolChannel ch;
    initPaired2WControllerForTest(c,m,ch,0xA1B2C3,0x24635F,key);qualifySolarSensorForTest(ch);
    ASSERT_TRUE(!c.configureSolarPoll(&ch,3,900));ASSERT_TRUE(c.configureSolarSensor(&ch,true));
    ASSERT_TRUE(c.configureSolarPoll(&ch,3,900));const auto due=c.solarSensor(0)->poll.dueMs;
    ioHomeTestSetMillis(due-100);c.loop();ASSERT_EQ(c.radio().testTransmitCount(),0U);
    ch.onStatusUpdate(true);ioHomeTestSetMillis(due);c.loop();
    ASSERT_EQ(c.radio().testTransmitCount(),0U);
    ch.onStatusUpdate(false);IoHomeFrame tx;ASSERT_TRUE(transmitQueuedControllerFrame(c,tx));ASSERT_EQ(tx.commandId,IoHomeCommand::SubscribeEvent);
    ASSERT_TRUE(c.solarSensor(0)->pending);ASSERT_TRUE(!c.requestSolarSensor(&ch,0x2001));
}
