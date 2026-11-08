#include "protocol/IoHomeAdelis.h"
TEST(protocol_adelis_model_identity_is_not_the_shared_radiator_type)
{
    using M=IoHomeAdelis::HeatingModel;
    ASSERT_TRUE(IoHomeAdelis::modelName(M::Adelis,reinterpret_cast<const uint8_t*>("ADELIS 2 DIGIT"),14));
    ASSERT_TRUE(!IoHomeAdelis::modelName(M::Agilia,reinterpret_cast<const uint8_t*>("ADELIS 2 DIGIT"),14));
    ASSERT_TRUE(!IoHomeAdelis::modelName(M::Adelis,reinterpret_cast<const uint8_t*>("AGILIA IO"),9));
    for(uint16_t reg:{0x100,0x103,0x106,0x110,0x131,0x137}){uint8_t data[6];ASSERT_EQ(IoHomeAdelis::write(reg,1,data),0);}
    for(uint8_t mode:{0,2,3}){uint8_t data[6];ASSERT_EQ(IoHomeAdelis::write(0x136,mode,data),5);const uint8_t expected[]={0x0C,0x61,1,0x36,mode};ASSERT_MEM_EQ(data,expected,5);}
    uint8_t data[6];ASSERT_EQ(IoHomeAdelis::write(0x136,1,data),0);
}
TEST(protocol_adelis_original_modes_and_boost_use_little_endian_register_family)
{
    unsigned modes=0,remaining=0;
    for(const auto &row:AlignmentCaptures::frames){if(row.sourceIndex<2||row.sourceIndex>5)continue;IoHomeFrame f;ASSERT_TRUE(deserializeFrameForTest(f,row.wire,row.length));
        if(f.commandId!=IoHomeCommand::WritePrivateResponse)continue;
        const uint16_t reg=uint16_t(f.data[2])<<8|f.data[3];const auto v=IoHomeAdelis::decode(reg,f.data+4,f.dataLen-4);
        if(reg==0x136){ASSERT_TRUE(v.valid);ASSERT_TRUE(v.scaled==0||v.scaled==2||v.scaled==3);++modes;}
        if(row.sourceIndex==3&&reg==0x13A){ASSERT_TRUE(v.valid&&v.candidate);ASSERT_TRUE(v.scaled==55||v.scaled==60);++remaining;}
    }
    ASSERT_TRUE(modes>=3&&remaining>=2);
    const uint8_t negative[]={0x30,0xF8},sentinel[]={0xFF,0xFF};
    ASSERT_EQ(IoHomeAdelis::decode(0x115,negative,2).scaled,-2000);ASSERT_TRUE(!IoHomeAdelis::decode(0x115,sentinel,2).valid);
    ASSERT_TRUE(!IoHomeAdelis::decode(0x137,negative,2).valid);
}
