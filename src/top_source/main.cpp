#include "common.h"
#include "config.h"
#include "../test/test.h"
#include "../component/fifo.h"
#include "../component/port.h"
#include "../component/memory.h"
#include "../component/regfile.h"
#include "../pipeline/fetch.h"
#include "../pipeline/fetch_decode.h"
#include "../pipeline/decode.h"
#include "../pipeline/decode_rename.h"
#include "../pipeline/rename.h"
#include "../pipeline/rename_readreg.h"
#include "../pipeline/readreg.h"
#include "../pipeline/readreg_issue.h"
#include "../pipeline/issue.h"
#include "../pipeline/execute/bru.h"

#include <cstring>

void run()
{
    component::fifo<pipeline::fetch_decode_pack_t> fetch_decode_fifo(16);
    component::fifo<pipeline::decode_rename_pack_t> decode_rename_fifo(16);
    pipeline::rename_readreg_pack_t default_rename_readreg_pack;
    memset(&default_rename_readreg_pack, 0, sizeof(default_rename_readreg_pack));
    component::port<pipeline::rename_readreg_pack_t> rename_readreg_port(default_rename_readreg_pack);
    pipeline::readreg_issue_pack_t default_readreg_issue_pack;
    memset(&default_readreg_issue_pack, 0, sizeof(default_readreg_issue_pack));
    component::port<pipeline::readreg_issue_pack_t> readreg_issue_port(default_readreg_issue_pack);

    component::memory memory(0x80000000, 64 * 0x1000);
    component::rat rat(PHY_REG_NUM, ARCH_REG_NUM);
    component::rob rob(16);
    component::regfile<pipeline::phy_regfile_item_t> phy_regfile(PHY_REG_NUM);

    pipeline::fetch fetch(&memory, &fetch_decode_fifo, 0x80000000);
    pipeline::decode decode(&fetch_decode_fifo, &decode_rename_fifo);
    pipeline::rename rename(&decode_rename_fifo, &rename_readreg_port, &rat, &rob);
    pipeline::readreg readreg(&rename_readreg_port, &readreg_issue_port, &phy_regfile);

    pipeline::issue_feedback_pack_t t_issue_feedback_pack;
    std::memset(&t_issue_feedback_pack, 0, sizeof(t_issue_feedback_pack));
    pipeline::execute::bru_feedback_pack t_bru_feedback_pack;
    std::memset(&t_bru_feedback_pack, 0, sizeof(t_bru_feedback_pack));

    rat.init_start();

    for(uint32_t i = 1;i < 32;i++)
    {
        rat.set_map(i, i);
    }

    rat.init_finish();

    while(1)
    {
        readreg.run(t_issue_feedback_pack);
        rename.run(t_issue_feedback_pack);
        decode.run();
        fetch.run(t_bru_feedback_pack);
        rat.sync();
        rob.sync();
        phy_regfile.sync();
    }
}

int main()
{
    test::test();
    run();
    return 0;
}