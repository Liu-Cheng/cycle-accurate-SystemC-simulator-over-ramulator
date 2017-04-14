#include "MemWrapper.h"
#include "pe.h"

int sc_main(int argc, char *argv[]){

    sc_signal<BurstOp> burstReq;
    sc_signal<BurstOp> burstResp;
    
    int peClkCycle = 10;
    int memClkCycle = 5;

    MemWrapper memWrapper("memWrapper", memClkCycle, peClkCycle, argc, argv);
    memWrapper.burstReq(burstReq);
    memWrapper.burstResp(burstResp);

    pe peInst("peInst", 0, peClkCycle);
    peInst.burstReq(burstReq);
    peInst.burstResp(burstResp);

    sc_start();

    return 0;

}
