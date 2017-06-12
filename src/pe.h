#ifndef __PE_H__
#define __PE_H__

#include <list> 
#include <iostream>
#include <iterator>
#include <map>
#include "Request.h"
#include "common.h"
#include "systemc.h"
#include "readIF.h"
#include "writeIF.h"

class pe : public sc_module{

    SC_HAS_PROCESS(pe);

    public:
        sc_in<bool> peClk;

        // signals for read memory
        sc_in<float> readDin0;
        sc_in<bool> readValid0;

        sc_out<bool> readEna0;
        sc_out<long> readAddr0;
        sc_out<int> readLength0;

        sc_in<float> readDin1;
        sc_in<bool> readValid1;

        sc_out<bool> readEna1;
        sc_out<long> readAddr1;
        sc_out<int> readLength1;

        // signals for write memory
        sc_out<float> writeDout;
        sc_out<bool> writeValid;

        sc_out<bool> writeEna;
        sc_out<long> writeAddr;
        sc_out<int> writeLength;

        sc_in<bool> writeComplete;
        sc_out<bool> computeDone;

        int peIdx;

        pe(sc_module_name _name, int _peIdx, int _peClkCycle);
        ~pe(){};

        void sigInit();

    private:
        std::vector<std::list<float>> queues;
        int peClkCycle;

        // Registers for sending
        int resLenVec0;
        int availBufferVec0;
        int resLenVec1;
        int availBufferVec1;
        int resLenVec;

        int readCounter0;
        int readCounter1;
        int writeCounter;
        int localCounter;
        int writeReqCounter;
        int writeReqNum;
        bool allWriteReqSent;

        void init();
        void readVec0();
        void readVec1();
        void writeVec();
        void vecAdd();
        void statusMonitor();
};

#endif
