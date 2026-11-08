TEST(protocol_discovery_target_requires_exact_class_suffix_and_supported_profile)
{
    ASSERT_EQ(ioHomeDiscoveryTargetType(0x00FFFB),1023);
    ASSERT_EQ(ioHomeDiscoveryTargetType(0x0002FB),11);
    for(uint32_t dst:{0x00003B,0x00003F,0x01FFFB,0x00FFFC,0x00067B,0x100FFFB})
        ASSERT_EQ(ioHomeDiscoveryTargetType(dst),0);
}
TEST(controller_key_extract_typed_discovery_is_request_local)
{
    ioHomeTestSetMillis(1000);ioHomeTestSetMicros(1000000);
    IoHomeController c;IoHomecontrol m;initKeyExtractControllerForTest(c,m,0x112233);
    ASSERT_TRUE(c.startKeyExtraction());
    auto query=[&](uint32_t dst,uint16_t profile) {
        IoHomeFrame req,resp;buildGatewayDiscoverRequest(req,0xD46FF6);req.setDestNode(dst);
        ASSERT_TRUE(queueGatewayRequestAndLoop(c,req,resp));
        ASSERT_EQ(resp.commandId,IoHomeCommand::DiscoverResponse);
        ASSERT_EQ(uint16_t(resp.data[0])<<2|(resp.data[1]>>6),profile);
        ASSERT_EQ(resp.data[1]&0x3F,0);ASSERT_EQ(resp.data[5],2);
        ASSERT_EQ(resp.getDestNodeId(),0xD46FF6U);
        // Drain the remaining broadcast copies before the next request.
        for(unsigned i=0;i<10;++i){ioHomeTestAdvanceMillis(100);c.loop();}
    };
    query(0x00FFFB,1023);query(0x00003B,2);query(0x00067B,2);
    c.stopKeyExtraction();ASSERT_TRUE(c.startKeyExtraction());query(0x00003B,2);
}

TEST(controller_typed_discovery_report_does_not_prove_key_recovery)
{
    IoHomeController c;IoHomecontrol m;initKeyExtractControllerForTest(c,m,0x112233);
    uint8_t original[16];memcpy(original,c.getSystemKey(),16);
    IoHomeFrame req,resp;buildGatewayDiscoverRequest(req,0xF96718);req.setDestNode(0x00FFFB);
    ASSERT_TRUE(!queueGatewayRequestAndLoop(c,req,resp));ASSERT_EQ(c.radio().testTransmitCount(),0U);
    ASSERT_TRUE(c.startKeyExtraction());ASSERT_TRUE(queueGatewayRequestAndLoop(c,req,resp));
    ASSERT_EQ(uint16_t(resp.data[0])<<2|(resp.data[1]>>6),1023);ASSERT_EQ(resp.data[5],2);
    ASSERT_EQ(c.keyExtractStatus(),IoHomeController::KeyExtractStatus::Armed);
    ASSERT_MEM_EQ(c.getSystemKey(),original,16); // no successful proof or imported key from a29 answer
    c.stopKeyExtraction();ASSERT_TRUE(c.startKeyExtraction());
    req.setDestNode(0x00003B);ASSERT_TRUE(queueGatewayRequestAndLoop(c,req,resp));
    ASSERT_EQ(uint16_t(resp.data[0])<<2|(resp.data[1]>>6),2);
}

// Class S: issue 215 supplies a discovery/waiting scenario, not authenticated RF proof.
TEST(controller_typed_discovery_freebox_alternate_destination_never_binds_key)
{
    ioHomeTestSetMillis(1000);ioHomeTestSetMicros(1000000);
    IoHomeController c;IoHomecontrol m;initKeyExtractControllerForTest(c,m,0x112233);
    uint8_t original[16];memcpy(original,c.getSystemKey(),16);
    ASSERT_EQ(getAddressClass(0x00003D),IoHomeAddressClass::BroadcastDeviceType);
    ASSERT_EQ(ioHomeDiscoveryTargetType(0x00003D),0);
    IoHomeFrame req,resp;buildGatewayDiscoverRequest(req,0x445566);req.setDestNode(0x00003D);
    ASSERT_TRUE(!queueGatewayRequestAndLoop(c,req,resp));
    ASSERT_TRUE(c.startKeyExtraction());
    for(unsigned wait:{0U,5000U,90000U}){
        ioHomeTestAdvanceMillis(wait);c.loop();
        ASSERT_TRUE(queueGatewayRequestAndLoop(c,req,resp));
        ASSERT_EQ(resp.commandId,IoHomeCommand::DiscoverResponse);
        ASSERT_EQ(uint16_t(resp.data[0])<<2|(resp.data[1]>>6),2);
        ASSERT_EQ(c.keyExtractCandidateHubNodeId(),0x445566U);
        for(unsigned i=0;i<10;++i){ioHomeTestAdvanceMillis(100);c.loop();}
        ASSERT_EQ(c.keyExtractHubNodeId(),0U);
        ASSERT_EQ(c.keyExtractStatus(),IoHomeController::KeyExtractStatus::Armed);
        ASSERT_MEM_EQ(c.getSystemKey(),original,16);
    }
}
