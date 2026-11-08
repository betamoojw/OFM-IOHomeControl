// Class S scheduling model of issue #209; no reconstructed RF capture.
TEST(controller_remote_settle_catches_unheard_group_member_and_debounces)
{
    ioHomeTestSetMillis(1000);const uint8_t key[16]={1};
    IoHomeController c;IoHomecontrol m;IoHomecontrolChannel mamma,pappa,other;
    initPaired2WControllerForTest(c,m,mamma,0x111111,0xEFB491,key);
    pappa.setNodeId(0xD4C8ED);pappa.setEncryptionKey(key);pappa.setPaired(true);m.testSetChannel(1,&pappa);
    ASSERT_TRUE(m.remoteMap().addRemote(0xBC40B3,"Mamma"));ASSERT_TRUE(m.remoteMap().linkDevice(0xBC40B3,0xEFB491));
    ASSERT_TRUE(m.remoteMap().addRemote(0x857067,"Pappa"));ASSERT_TRUE(m.remoteMap().linkDevice(0x857067,0xD4C8ED));
    ASSERT_TRUE(!c.configureRemoteSettling(0,1,500));ASSERT_TRUE(!c.configureRemoteSettling(0,3,499));
    other.setNodeId(0x333333);other.setEncryptionKey(key);other.setPaired(true);m.testSetChannel(2,&other);
    ASSERT_TRUE(m.remoteMap().linkDevice(0xBC40B3,0x333333));
    ASSERT_TRUE(c.configureRemoteSettling(0,3,1000));
    IoHomeFrame f;buildPassiveExecuteFrame(f,0xBC40B3,0x3F);f.set1WMode();f.dataLen=8;f.hasHmac=true;
    ASSERT_TRUE(queueControllerPassiveFrame(c,f));ASSERT_EQ(other.testScheduledStatusPollCount(),1);ASSERT_EQ(mamma.testScheduledStatusPollCount(),0);ASSERT_EQ(pappa.testScheduledStatusPollCount(),0);
    ioHomeTestAdvanceMillis(900);ASSERT_TRUE(queueControllerPassiveFrame(c,f));
    ioHomeTestAdvanceMillis(999);c.loop();ASSERT_EQ(mamma.testScheduledStatusPollCount(),0);
    ioHomeTestAdvanceMillis(1);c.loop();ASSERT_EQ(mamma.testScheduledStatusPollCount(),1);ASSERT_EQ(pappa.testScheduledStatusPollCount(),1);
    const auto epoch=pappa.mTestFollowUp.deadline;
    for(unsigned i=0;i<5;++i){ASSERT_TRUE(queueControllerPassiveFrame(c,f));ioHomeTestAdvanceMillis(1000);c.loop();}
    ASSERT_EQ(mamma.testScheduledStatusPollCount(),1);ASSERT_EQ(pappa.testScheduledStatusPollCount(),1);ASSERT_EQ(pappa.mTestFollowUp.deadline,epoch);
    ASSERT_EQ(c.radio().testTransmitCount(),0U);ASSERT_TRUE(!pappa.testHasPositionFeedback());
}
TEST(controller_remote_settle_identity_and_unmapped_source_cannot_schedule)
{
    for(unsigned variant=0;variant<5;++variant){ioHomeTestSetMillis(1000);const uint8_t key[16]={1},other[16]={2};
        IoHomeController c;IoHomecontrol m;IoHomecontrolChannel a,b;initPaired2WControllerForTest(c,m,a,0x111111,0xEFB491,key);
        b.setNodeId(0xD4C8ED);b.setEncryptionKey(key);b.setPaired(true);m.testSetChannel(1,&b);
        ASSERT_TRUE(m.remoteMap().addRemote(0xBC40B3,"Mamma"));ASSERT_TRUE(m.remoteMap().linkDevice(0xBC40B3,0xEFB491));
        ASSERT_TRUE(c.configureRemoteSettling(0,3,1000));IoHomeFrame f;buildPassiveExecuteFrame(f,variant==0?0x222222:0xBC40B3,0x3F);f.set1WMode();f.dataLen=8;f.hasHmac=true;
        ASSERT_TRUE(queueControllerPassiveFrame(c,f));
        if(variant==1)b.setEncryptionKey(other);if(variant==2)b.setIs1W(true);if(variant==3)b.setPaired(false);if(variant==4)c.configureRemoteSettling(0,0,0);
        ioHomeTestAdvanceMillis(1001);c.loop();ASSERT_EQ(a.testScheduledStatusPollCount(),0);ASSERT_EQ(b.testScheduledStatusPollCount(),0);
    }
}

// Class S timing model motivated by issue #209; these are not reconstructed captures.
TEST(controller_remote_settle_linked_union_waits_for_last_exchange_not_first_heard_remote)
{
    ioHomeTestSetMillis(1000);const uint8_t key[16]={1};IoHomeController c;IoHomecontrol m;IoHomecontrolChannel a,b;
    initPaired2WControllerForTest(c,m,a,0x111111,0xEFB491,key);ASSERT_TRUE(!c.configureLinkedRemoteSettling(1000));b.setNodeId(0xD4C8ED);b.setEncryptionKey(key);b.setPaired(true);m.testSetChannel(1,&b);
    ASSERT_TRUE(m.remoteMap().addRemote(0xBC40B3,"Mamma"));ASSERT_TRUE(m.remoteMap().linkDevice(0xBC40B3,0xEFB491));
    ASSERT_TRUE(m.remoteMap().addRemote(0x857067,"Pappa"));ASSERT_TRUE(m.remoteMap().linkDevice(0x857067,0xD4C8ED));
    ASSERT_TRUE(c.configureLinkedRemoteSettling(1000));IoHomeFrame heard;buildPassiveExecuteFrame(heard,0xBC40B3,0x3F);heard.set1WMode();heard.hasHmac=true;heard.dataLen=8;
    ASSERT_TRUE(queueControllerPassiveFrame(c,heard));ASSERT_TRUE(c.sendCommand(a.getNodeId(),key,IoHomeCommand::Private,3));IoHomeFrame tx,reply;ASSERT_TRUE(transmitQueuedControllerFrame(c,tx));
    ioHomeTestAdvanceMillis(100);const uint8_t status[]={1,0,0,0,0,0,0,0};buildSimpleResponseFrame(reply,0x111111,a.getNodeId(),IoHomeCommand::PrivateResponse,status,8);ASSERT_TRUE(queueControllerResponse(c,reply));
    const uint32_t ended=ioHomeTestMillis();ioHomeTestSetMillis(ended+900);c.loop();ASSERT_EQ(a.testScheduledStatusPollCount(),0);ASSERT_EQ(b.testScheduledStatusPollCount(),0);
    ioHomeTestSetMillis(ended+1000);c.loop();ASSERT_EQ(a.testScheduledStatusPollCount(),1);ASSERT_EQ(b.testScheduledStatusPollCount(),1);
}
TEST(controller_remote_settle_linked_union_revokes_on_mapping_change)
{
    const uint8_t key[16]={1};IoHomeController c;IoHomecontrol m;IoHomecontrolChannel a,b;
    initPaired2WControllerForTest(c,m,a,0x111111,0xEFB491,key);ASSERT_TRUE(!c.configureLinkedRemoteSettling(1000));b.setNodeId(0xD4C8ED);b.setEncryptionKey(key);b.setPaired(true);m.testSetChannel(1,&b);
    ASSERT_TRUE(m.remoteMap().addRemote(0xBC40B3,"remote"));ASSERT_TRUE(m.remoteMap().linkDevice(0xBC40B3,a.getNodeId()));ASSERT_TRUE(m.remoteMap().linkDevice(0xBC40B3,b.getNodeId()));ASSERT_TRUE(c.configureLinkedRemoteSettling(500));
    IoHomeFrame frame;buildPassiveExecuteFrame(frame,0xBC40B3,0x3F);frame.set1WMode();frame.hasHmac=true;frame.dataLen=8;ASSERT_TRUE(queueControllerPassiveFrame(c,frame));
    ASSERT_TRUE(m.remoteMap().unlinkDevice(0xBC40B3,b.getNodeId()));ioHomeTestAdvanceMillis(1000);c.loop();ASSERT_EQ(a.testScheduledStatusPollCount(),0);ASSERT_EQ(b.testScheduledStatusPollCount(),0);
}
