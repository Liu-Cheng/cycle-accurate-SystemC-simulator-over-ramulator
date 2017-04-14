#ifndef __PE_H__
#define __PE_H__

#include <list> 
#include <iostream>
#include <iterator>
#include <map>
#include "Request.h"
#include "common.h"
#include "systemc.h"

class pe : public sc_module{

    SC_HAS_PROCESS(pe);

    public:
        // Even though there may be multiple requests generated at the same time,
        // only one of the request goes to the ramulator eventually in each cycle.
        sc_out<BurstOp> burstReq;
        sc_in<BurstOp> burstResp;

        // pe parameter
        int peIdx;

        pe(sc_module_name _name, int _peIdx, int _peClkCycle);
        ~pe(){};

        // public member function
        void sendMemReq();
        void getMemResp();
        void issueReadReq();
        void issueWriteReq();
        void runtimeMonitor();
        void dumpResp();
        void setPeClkCycle(int _peClkCycle);
        void vecAdd();
        void statusMonitor();
        long createWriteBurstReq(
                ramulator::Request::Type type, 
                long addr, 
                int length, 
                int localAddr, 
                std::vector<int> &buffer);

        long createReadBurstReq(
                ramulator::Request::Type type, 
                long addr, 
                int length, 
                int localAddr);

        void setDataWidth(int width);

    private:
        int peClkCycle;
        int dataWidth;      // # of bytes can be transmitted to/from memory in each cycle
        std::vector<int> va; // Input of vector a
        std::vector<int> vb; // Input of vector b
        std::vector<int> vp; // result of the element-wise production of a and b

        bool allVaReqGenerated;
        bool allVaRespReceived;
        bool allVbReqGenerated;
        bool allVbRespReceived;
        bool allVpReqGenerated;
        bool allVpRespReceived;

        bool computingDone;
        bool vaReady;
        bool vbReady;
        bool vpReady;

        // The request queue is shared by all the different read/write ports
        std::list<BurstOp> burstReqQueue;
        std::list<BurstOp> burstRespQueue;

        // It keeps the status of the burst requests. If a burst with burstIdx is not found 
        // in the mapper, it doesn't exist. If it is found to be false, the request is generated 
        // but is not responsed yet. If it is set true, the request is responsed.
        // These data structure will remain valid until the end of the object life.
        std::map<long, bool> vaBurstReqStatus;
        std::map<long, bool> vbBurstReqStatus;
        std::map<long, bool> vpBurstReqStatus;

        std::list<long> vaReqQueue;
        std::list<long> vbReqQueue;
        std::list<long> vpReqQueue;

        bool isBurstReqQueueEmpty();
        bool isMemReqQueueEmpty();
        void init();
};

#endif
