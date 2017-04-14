#ifndef __MEM_WRAPPER_H__
#define __MEM_WRAPPER_H__

#include "Processor.h"
#include "Config.h"
#include "Controller.h"
#include "SpeedyController.h"
#include "Memory.h"
#include "DRAM.h"
#include "Statistics.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdlib.h>
#include <functional>
#include <map>
#include <iterator>
#include <list>
#include <vector>
#include <algorithm>
#include "pe.h"
#include "common.h"

/* Standards */
#include "Gem5Wrapper.h"
#include "DDR3.h"
#include "DDR4.h"
#include "DSARP.h"
#include "GDDR5.h"
#include "LPDDR3.h"
#include "LPDDR4.h"
#include "WideIO.h"
#include "WideIO2.h"
#include "HBM.h"
#include "SALP.h"
#include "ALDRAM.h"
#include "TLDRAM.h"

class MemWrapper : public sc_module{

    SC_HAS_PROCESS(MemWrapper);

    public:
        // Memory related configurations
        std::string name;
        std::string standard;
        Config configs;
        string stats_out;
        std::vector<const char*> files;

        // Signals from/to pes. They will be processed following the peClk.
        sc_in <BurstOp> burstReq;
        sc_out <BurstOp> burstResp;

        // The queue stores all the burst request transactions 
        // and it will not be removed untill the end of the program.
        // In addition, as the requests are stored in order, 
        // it is also the basis of the data memory content management.  
        std::list<BurstOp> burstReqQueue;

        // It stores the response obtained from the ramulator and it will 
        // be gradually removed when the response is sent out.
        std::list<BurstOp> burstRespQueue;

        // It stores all the requests to be sent to the ramulator
        // It will gradually be removed when it is processed.
        std::list<Request> reqQueue; 

        // It stores all the responses returned from the ramulator as well as 
        // the write response generated accordingly. The response will be removed 
        // when it is combined to a burst response and sent back to pe. 
        std::list<Request> respQueue;
        
        MemWrapper(sc_module_name _name, 
                int _memClkCycle, 
                int _peClkCycle,
                int argc, 
                char* argv[]);

        template<typename T>
        void run_acc(const Config& configs, Memory<T, Controller>& memory);

        template<typename T>
        void start_run(const Config& configs, T* spec, const vector<const char*>& files);
        void getBurstReq();
        void runMemSim();
        bool getMemReq(Request &req);
        void sendBurstResp();
        void memReqMonitor();
        void respMonitor();
        void updateBurstToRam(long watchedBurstIdx);
        ~MemWrapper(){};

    private:
        int memSize;               // # of bytes
        std::vector<char> ramData; // byte level memory data management.

        // It stores the status of all the basic memory requests.
        // If a reqIdx is not found, it means the request doesn't exist.
        // If a regIdx is found and boolean value is false, it means the 
        // request is under processing.
        // If a reqIdx is found and boolean value is true, it means the request gets 
        // responsed.
        std::map<long, bool> reqStatus;

        // burstStatus represnts similar information with that of reqStatus.
        std::map<long, bool> burstStatus;

        // Basically we keep a record of the write request updating history.
        // If the write request has its content written to ramData, it will be set 
        // true. If the write response is under processing, it will be set as false;
        // It it is not found here, it means there is no such write request yet.
        std::map<long, bool> writebackHistory;

        int memClkCycle;
        int peClkCycle;

        void loadConfig(int argc, char* argv[]);
        int calBurstLen();
        long getMaxDepartTime(const std::vector<long> &reqVec);
        long getMinArriveTime(const std::vector<long> &reqVec);
        void cleanProcessedRequests();
        void shallowReqCopy(const Request &simpleReq, Request &req);
        void ramInit();

        // Update ram on a specified addr with specified data type.
        template <typename T>
        void updateSingleDataToRam(long addr, T t);
        void cleanRespQueue(const std::vector<long> &reqVec);
};

#endif 
