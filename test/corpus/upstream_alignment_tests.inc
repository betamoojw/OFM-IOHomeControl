#include "upstream_alignment_frames.h"
TEST(protocol_upstream_alignment_complete_functional_replies_roundtrip)
{
    unsigned sun=0,battery=0,registers=0,status=0;
    for(const auto &row:AlignmentCaptures::frames){IoHomeFrame frame;ASSERT_TRUE(deserializeFrameForTest(frame,row.wire,row.length));
        uint8_t wire[32]{};ASSERT_EQ(frame.serialize2W(wire,sizeof(wire)),row.length);ASSERT_MEM_EQ(wire,row.wire,row.length);
        ASSERT_TRUE(!(frame.ctrlByte0&IOHC_CTRL0_MODE_1W));
        if(frame.commandId==IoHomeCommand::StatusUpdate){if(frame.data[3]==5)++sun;else if(frame.data[3]==1)++battery;}
        if(frame.commandId==IoHomeCommand::WritePrivateResponse)++registers;
        if(frame.commandId==IoHomeCommand::PrivateResponse)++status;
    }
    ASSERT_TRUE(sun>=2&&battery&&registers&&status);
}
