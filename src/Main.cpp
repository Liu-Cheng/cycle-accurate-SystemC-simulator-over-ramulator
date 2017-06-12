#include "MemWrapper.h"
#include "pe.h"

int sc_main(int argc, char *argv[]){

    sc_set_time_resolution(1, SC_NS);

    // Only burstIdx is transferred.
    sc_signal<long> readReq0;
    sc_signal<long> readReq1;
    sc_signal<long> writeReq;
    sc_signal<long> readResp0;
    sc_signal<long> readResp1;
    sc_signal<long> writeResp;

    sc_signal<bool> readEna0;
    sc_signal<bool> readEna1;
    sc_signal<bool> writeEna;
    sc_signal<long> readAddr0;
    sc_signal<long> readAddr1;
    sc_signal<long> writeAddr;
    sc_signal<int> readLength0;
    sc_signal<int> readLength1;
    sc_signal<int> writeLength;

    sc_signal<bool> readComplete0;
    sc_signal<bool> readComplete1;
    sc_signal<bool> writeComplete;
    sc_signal<bool> computeDone;

    sc_signal<float> readData0;
    sc_signal<float> readData1;
    sc_signal<float> writeData;
    sc_signal<bool> readValid0;
    sc_signal<bool> readValid1;
    sc_signal<bool> writeValid;

    double peClkCycle = 10;
    double memClkCycle = 2;
    sc_clock peClk("peClk", peClkCycle, SC_NS, 0.5);

    GL::cfgBfsParam("./config.txt");
    MemWrapper memWrapper("memWrapper", memClkCycle, peClkCycle, argc, argv);
    memWrapper.req0(readReq0);
    memWrapper.req1(readReq1);
    memWrapper.req2(writeReq);
    memWrapper.resp0(readResp0);
    memWrapper.resp1(readResp1);
    memWrapper.resp2(writeResp);
    memWrapper.computeDone(computeDone);
    memWrapper.sigInit();

    pe vecadd("vecadd", 0, peClkCycle);
    vecadd.peClk(peClk);
    vecadd.readDin0(readData0);
    vecadd.readValid0(readValid0);
    vecadd.readEna0(readEna0);
    vecadd.readAddr0(readAddr0);
    vecadd.readLength0(readLength0);
    vecadd.readDin1(readData1);
    vecadd.readValid1(readValid1);
    vecadd.readEna1(readEna1);
    vecadd.readAddr1(readAddr1);
    vecadd.readLength1(readLength1);
    vecadd.writeDout(writeData);
    vecadd.writeValid(writeValid);
    vecadd.writeEna(writeEna);
    vecadd.writeAddr(writeAddr);
    vecadd.writeLength(writeLength);
    vecadd.writeComplete(writeComplete);
    vecadd.computeDone(computeDone);
    vecadd.sigInit();

    readIF<float> readVec0("readVec0", 0, 0, peClkCycle);
    readVec0.clk(peClk);
    readVec0.ena(readEna0);
    readVec0.addr(readAddr0);
    readVec0.length(readLength0);
    readVec0.dout(readData0);
    readVec0.valid(readValid0);
    readVec0.complete(readComplete0);
    readVec0.readBurstReq(readReq0);
    readVec0.readBurstResp(readResp0);
    readVec0.sigInit();

    readIF<float> readVec1("readVec1", 1, 0, peClkCycle);
    readVec1.clk(peClk);
    readVec1.ena(readEna1);
    readVec1.addr(readAddr1);
    readVec1.length(readLength1);
    readVec1.dout(readData1);
    readVec1.valid(readValid1);
    readVec1.complete(readComplete1);
    readVec1.readBurstReq(readReq1);
    readVec1.readBurstResp(readResp1);
    readVec1.sigInit();

    writeIF<float> writeVec("writeVec", 2, 0, peClkCycle);
    writeVec.clk(peClk);
    writeVec.wen(writeEna);
    writeVec.addr(writeAddr);
    writeVec.length(writeLength);
    writeVec.din(writeData);
    writeVec.valid(writeValid);
    writeVec.complete(writeComplete);
    writeVec.writeBurstReq(writeReq);
    writeVec.writeBurstResp(writeResp);
    writeVec.sigInit();

    sc_start();

    return 0;

}
