static bool completeAtlanticStep(IoHomeController &c,const uint8_t *payload,uint8_t length,IoHomeCommand replyCommand=IoHomeCommand::WritePrivateResponse)
{
    IoHomeFrame challenge,reply;const uint8_t nonce[]={1,2,3,4,5,6};
    buildSimpleResponseFrame(challenge,0xC0FFEE,0xBD684B,IoHomeCommand::ChallengeRequest,nonce,6);
    if(!queueControllerResponse(c,challenge))return false;
    buildSimpleResponseFrame(reply,0xC0FFEE,0xBD684B,replyCommand,payload,length);return queueControllerResponse(c,reply);
}
static void initTypedAtlantic(IoHomeController &c,IoHomecontrol &m,IoHomecontrolChannel &ch,bool dhw)
{
    const uint8_t key[16]={1};initPaired2WControllerForTest(c,m,ch,0xC0FFEE,0xBD684B,key);
    IoHomeProtocolIdentity id;id.valid=true;id.ioAddress=ch.getNodeId();id.manufacturerId=0x0C;id.profile=dhw?51:52;id.subProfile=dhw?0:1;id.nodeClass=IoHomeNodeClass::Actuator;
    ch.onProtocolIdentity(ch.getNodeId(),id);if(!dhw)ch.setHeatingModelForTest(IoHomeAdelis::HeatingModel::Adelis);
}
TEST(controller_atlantic_action_adelis_requires_selected_model_and_live_name_before_write)
{
    for(bool correct:{false,true}){IoHomeController c;IoHomecontrol m;IoHomecontrolChannel ch;initTypedAtlantic(c,m,ch,false);
        ASSERT_TRUE(c.startAdelisAction(&ch,0x136,3));IoHomeFrame tx;ASSERT_TRUE(transmitQueuedControllerFrame(c,tx));ASSERT_MEM_EQ(tx.data,"\x0C\x60\x01\x19",4);
        uint8_t name[18]={0x0C,0x60,1,0x19};memcpy(name+4,correct?"ADELIS 2 DIGIT":"WRONG MODEL   ",14);
        ASSERT_TRUE(completeAtlanticStep(c,name,18));ioHomeTestAdvanceMillis(1000);
        if(!correct){for(unsigned i=0;i<4;++i)c.loop();ASSERT_TRUE(!c.atlanticRegisterRead().active);ASSERT_TRUE(!c.atlanticRegisterRead().readbackVerified);continue;}
        ASSERT_TRUE(transmitQueuedControllerFrame(c,tx));ASSERT_MEM_EQ(tx.data,"\x0C\x61\x01\x36\x03",5);
        const uint8_t ack[]={0x0C,0x61,1,0x36,3};ASSERT_TRUE(completeAtlanticStep(c,ack,5));ASSERT_TRUE(!c.atlanticRegisterRead().readbackVerified);
        ioHomeTestAdvanceMillis(1000);ASSERT_TRUE(transmitQueuedControllerFrame(c,tx));ASSERT_MEM_EQ(tx.data,"\x0C\x60\x01\x36",4);
        const uint8_t readback[]={0x0C,0x60,1,0x36,3};ASSERT_TRUE(completeAtlanticStep(c,readback,5));ASSERT_TRUE(c.atlanticRegisterRead().readbackVerified);
        ASSERT_TRUE(!ch.testHasPositionFeedback()&&!ch.testHasStatusUpdate());
    }
    IoHomeController c;IoHomecontrol m;IoHomecontrolChannel ch;initTypedAtlantic(c,m,ch,false);
    ch.setHeatingModelForTest(IoHomeAdelis::HeatingModel::Agilia);ASSERT_TRUE(!c.startAdelisAction(&ch,0x13E,1));
    ASSERT_TRUE(!c.requestHeatingRegister(&ch,0x100,true,1));
}
TEST(controller_atlantic_action_dhw_target_preserves_full_width_and_uses_readback)
{
    IoHomeController c;IoHomecontrol m;IoHomecontrolChannel ch;initTypedAtlantic(c,m,ch,true);
    ASSERT_TRUE(!c.startDhwTarget(&ch,51));ASSERT_TRUE(c.startDhwTarget(&ch,54));IoHomeFrame tx;
    ASSERT_TRUE(transmitQueuedControllerFrame(c,tx));ASSERT_MEM_EQ(tx.data,"\x0C\x60\x00\x08",4);
    const uint8_t volume[]={0x0C,0x60,0,8,0,0xC8};ASSERT_TRUE(completeAtlanticStep(c,volume,6));ioHomeTestAdvanceMillis(1000);
    ASSERT_TRUE(transmitQueuedControllerFrame(c,tx));const uint8_t mode[]={0x0C,0x60,0,6,1};ASSERT_TRUE(completeAtlanticStep(c,mode,5));ioHomeTestAdvanceMillis(1000);
    ASSERT_TRUE(transmitQueuedControllerFrame(c,tx));ASSERT_EQ(tx.commandId,IoHomeCommand::Execute);ASSERT_EQ(tx.dataLen,6);ASSERT_EQ(tx.data[2],0xA1);ASSERT_EQ(tx.data[3],0xE8);
    const uint8_t status[]={0x2D,0,0xA1,0xE8,0,0};ASSERT_TRUE(completeAtlanticStep(c,status,6,IoHomeCommand::PrivateResponse));ASSERT_TRUE(!c.atlanticRegisterRead().readbackVerified);
    ioHomeTestAdvanceMillis(1000);ASSERT_TRUE(transmitQueuedControllerFrame(c,tx));ASSERT_TRUE(completeAtlanticStep(c,mode,5));ioHomeTestAdvanceMillis(1000);
    ASSERT_TRUE(transmitQueuedControllerFrame(c,tx));ASSERT_EQ(tx.commandId,IoHomeCommand::Private);ASSERT_TRUE(completeAtlanticStep(c,status,6,IoHomeCommand::PrivateResponse));
    ASSERT_TRUE(c.atlanticRegisterRead().readbackVerified&&c.atlanticRegisterRead().dhw.visibleTarget());ASSERT_TRUE(!ch.testHasPositionFeedback());
}
TEST(controller_atlantic_action_cannot_advance_on_foreign_stale_or_changed_identity)
{
    const uint8_t nonce[]={1,2,3,4,5,6},other[16]={2};
    for(unsigned variant=0;variant<9;++variant){IoHomeController c;IoHomecontrol m;IoHomecontrolChannel ch;initTypedAtlantic(c,m,ch,false);
        ASSERT_TRUE(c.startAdelisAction(&ch,0x13E,1));IoHomeFrame tx,challenge,reply;ASSERT_TRUE(transmitQueuedControllerFrame(c,tx));
        const uint8_t name[]={0x0C,0x60,1,0x19,'A','D','E','L','I','S',' ','2',' ','D','I','G','I','T'};
        if(variant!=6){buildSimpleResponseFrame(challenge,0xC0FFEE,0xBD684B,IoHomeCommand::ChallengeRequest,nonce,6);ASSERT_TRUE(queueControllerResponse(c,challenge));}
        buildSimpleResponseFrame(reply,0xC0FFEE,0xBD684B,IoHomeCommand::WritePrivateResponse,name,sizeof(name));
        if(variant==0)ch.setEncryptionKey(other);if(variant==1)ch.setHeatingModelForTest(IoHomeAdelis::HeatingModel::Agilia);
        if(variant==2)c.setOwnNodeId(0x111111);if(variant==3)c.setSystemKey(other);if(variant==4)c.testSetRxScanFrequency(IOHC_FREQ_3);
        if(variant==5)reply.setSrcNode(0x111111);if(variant==7){c.loop();ioHomeTestAdvanceMillis(16000);}if(variant==8)c.cancelAtlanticRegisterRead(c.atlanticRegisterRead().token);
        ASSERT_TRUE(queueControllerResponse(c,reply));ioHomeTestAdvanceMillis(1000);for(unsigned i=0;i<4;++i)c.loop();
        ASSERT_TRUE(!c.atlanticRegisterRead().readbackVerified);ASSERT_TRUE(lastTransmittedFrameForTest(c,tx));
        ASSERT_TRUE(tx.commandId!=IoHomeCommand::WritePrivate||tx.dataLen<2||tx.data[1]!=0x61);
        ASSERT_TRUE(!ch.testHasPositionFeedback());
    }
}
TEST(controller_atlantic_action_dhw_auto_or_other_tank_cannot_send_target)
{
    for(bool automatic:{false,true}){IoHomeController c;IoHomecontrol m;IoHomecontrolChannel ch;initTypedAtlantic(c,m,ch,true);
        ASSERT_TRUE(c.startDhwTarget(&ch,54));IoHomeFrame tx;ASSERT_TRUE(transmitQueuedControllerFrame(c,tx));
        const uint8_t volume[]={0x0C,0x60,0,8,0,uint8_t(automatic?200:250)};ASSERT_TRUE(completeAtlanticStep(c,volume,6));ioHomeTestAdvanceMillis(1000);
        ASSERT_TRUE(transmitQueuedControllerFrame(c,tx));const uint8_t mode[]={0x0C,0x60,0,6,uint8_t(automatic?0:1)};ASSERT_TRUE(completeAtlanticStep(c,mode,5));
        ioHomeTestAdvanceMillis(1000);for(unsigned i=0;i<4;++i)c.loop();ASSERT_TRUE(!c.atlanticRegisterRead().active&&!c.atlanticRegisterRead().readbackVerified);
        ASSERT_TRUE(lastTransmittedFrameForTest(c,tx));ASSERT_TRUE(tx.commandId!=IoHomeCommand::Execute);ASSERT_TRUE(!ch.testHasPositionFeedback());
    }
}

TEST(controller_atlantic_action_poll_is_opt_in_typed_and_identity_bound)
{
    for(bool dhw:{false,true}){IoHomeController c;IoHomecontrol m;IoHomecontrolChannel ch;initTypedAtlantic(c,m,ch,dhw);
        ASSERT_TRUE(!c.configureAtlanticPoll(&ch,dhw?299:59));ASSERT_TRUE(c.configureAtlanticPoll(&ch,dhw?300:60));
        c.loop();ASSERT_EQ(c.radio().testTransmitCount(),0U);ioHomeTestAdvanceMillis((dhw?300:60)*1000+1001);
        IoHomeFrame tx;ASSERT_TRUE(transmitQueuedControllerFrame(c,tx));ASSERT_EQ(tx.commandId,IoHomeCommand::WritePrivate);ASSERT_EQ(tx.data[1],0x60);
        ASSERT_EQ(c.atlanticRegisterRead().count,dhw?5:4);ASSERT_EQ(c.atlanticRegisterRead().action,IoHomeAtlanticRead::Action::Read);
        const uint8_t other[16]={2};ch.setEncryptionKey(other);for(unsigned i=0;i<4;++i)c.loop();
        ASSERT_TRUE(!c.atlanticRegisterRead().readbackVerified);
    }
}
