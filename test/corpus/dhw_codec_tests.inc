#include "protocol/IoHomeDhwCodec.h"
TEST(protocol_dhw_codec_preserves_16bit_main_parameter_and_manual_visibility)
{
    uint16_t raw=0;uint8_t data[6]{};
    ASSERT_TRUE(IoHomeDhwCodec::encodeConfirmedTarget(54,200,true,raw));ASSERT_EQ(raw,0xA1E8);
    ASSERT_TRUE(IoHomeDhwCodec::executePayload(raw,0x63,data));ASSERT_MEM_EQ(data,"\x01\x63\xA1\xE8\x00\x00",6);
    ASSERT_TRUE(IoHomeDhwCodec::encodeConfirmedTarget(62,200,true,raw));ASSERT_EQ(raw,0xC800);
    ASSERT_TRUE(IoHomeDhwCodec::encodeConfirmedTarget(50,200,true,raw));
    for(float t:{20.f,49.f,51.f,63.f,NAN,INFINITY})ASSERT_TRUE(!IoHomeDhwCodec::encodeConfirmedTarget(t,200,true,raw));
    ASSERT_TRUE(!IoHomeDhwCodec::encodeConfirmedTarget(54,250,true,raw));ASSERT_TRUE(!IoHomeDhwCodec::encodeConfirmedTarget(54,200,false,raw));
    unsigned status=0;for(const auto &row:AlignmentCaptures::frames)if(row.sourceIndex==7){IoHomeFrame f;ASSERT_TRUE(deserializeFrameForTest(f,row.wire,row.length));
        ASSERT_TRUE(IoHomeDhwCodec::statusTarget(f,raw));float value=0;ASSERT_TRUE(IoHomeDhwCodec::decodeTarget(raw,value));ASSERT_TRUE(value==54.f||value==62.f||value==60.f);++status;}
    ASSERT_TRUE(status>=3);
    IoHomeDhwCodec::State state;state.targetValid=true;state.observeMode(0);ASSERT_TRUE(!state.visibleTarget());
    state.observeMode(1);ASSERT_TRUE(!state.visibleTarget());state.targetValid=true;ASSERT_TRUE(state.visibleTarget());
}
TEST(protocol_dhw_codec_is_big_endian_signed_and_energy_reset_is_not_rollover)
{
    const uint8_t negative[]={0xFF,0x38,0,0,0,0},invalid[]={0x7F,0xFF,0,0,0,0};int16_t t=0;
    ASSERT_TRUE(IoHomeDhwCodec::waterTemperature(negative,6,t));ASSERT_EQ(t,-200);ASSERT_TRUE(!IoHomeDhwCodec::waterTemperature(negative,2,t));ASSERT_TRUE(!IoHomeDhwCodec::waterTemperature(invalid,6,t));
    uint32_t wh=0;const uint8_t max[]={0xFF,0xFF,0xFF,0xFF};ASSERT_TRUE(IoHomeDhwCodec::energyWh(max,4,wh));ASSERT_EQ(wh,UINT32_MAX);
    IoHomeDhwCodec::State state;state.observeEnergy(wh);ASSERT_TRUE(!state.energyReset);ASSERT_TRUE(state.energyKWh()>4294967.29);
    state.observeEnergy(1);ASSERT_TRUE(state.energyReset);ASSERT_TRUE(state.energyKWh()==0.001);
    uint8_t data[6];for(uint8_t mode:{0,1,2})ASSERT_EQ(IoHomeDhwCodec::modeWrite(mode,data),5);ASSERT_EQ(IoHomeDhwCodec::modeWrite(3,data),0);
}
