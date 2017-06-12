#include "MemWrapper.h"

// The memory configuration is initially ported from ramulator.
// which are mostly parsed from input argument. We don't want to change it for now.
MemWrapper::MemWrapper(sc_module_name _name,
        double _memClkCycle,
        double _peClkCycle,
        int argc, 
        char* argv[]) 
    : sc_module(_name), configs(argv[1]){

    loadConfig(argc, argv);
    memClkCycle = _memClkCycle; 
    peClkCycle = _peClkCycle; 
    GL::burstLen = calBurstLen();
    burstReqQueues.resize(GL::portNum);
    burstRespQueues.resize(GL::portNum);
    ramInit();

    SC_THREAD(runMemSim);
    SC_THREAD(getBurstReq);
    SC_THREAD(sendBurstResp);
    SC_THREAD(statusMonitor);

}

void MemWrapper::sigInit(){
    resp0.write(-1);
    resp1.write(-1);
    resp2.write(-1);
}

// It reads request from pe and thus is synchronized to the pe's clock
void MemWrapper::getBurstReq(){
    while(true){
        auto reqProcess = [this](long burstIdx){
            if(burstIdx != -1){
                BurstOp* ptr = GL::bursts[burstIdx];
                burstReqQueues[ptr->portIdx].push_back(burstIdx);
                totalReqNum[burstIdx] = ptr->getReqNum();
                processedReqNum[burstIdx] = 0;
                ptr->convertToReq(reqQueue);
            }
        };
        
        reqProcess(req0.read());
        reqProcess(req1.read());
        reqProcess(req2.read());

        wait(peClkCycle, SC_NS);
    }
}

// The memory response will be sent in the same order with 
// its incoming order. Meanwhile, the response can only be 
// sent at the right timestamp.
void MemWrapper::sendBurstResp(){
    while(true){
        auto respProcess = [this](int pidx)->long{
            if(burstReqQueues[pidx].empty()){
                return -1;
            }

            int idx = burstReqQueues[pidx].front();
            if(std::find(burstRespQueues[pidx].begin(),
               burstRespQueues[pidx].end(), 
               idx) == burstRespQueues[pidx].end())
            {
                return -1;
            }
            else{
                BurstOp* ptr = GL::bursts[idx];
                long respReadyTime = ptr->departMemTime; 
                long currentTimeStamp = (long)(sc_time_stamp()/sc_time(1, SC_NS));
                if(respReadyTime <= currentTimeStamp){
                    if(ptr->type == ramulator::Request::Type::WRITE){
                        ptr->reqToRam(ramData);
                    }
                    else{
                        ptr->ramToReq(ramData);
                    }
                    burstReqQueues[pidx].remove(idx);
                    burstRespQueues[pidx].remove(idx);
                    return idx;
                }
            }
            return -1;
        };

        resp0.write(respProcess(0));
        resp1.write(respProcess(1));
        resp2.write(respProcess(2));

        wait(peClkCycle, SC_NS);
    }
}

int MemWrapper::calBurstLen(){

    int burstlen;
    if (standard == "DDR3") {
        DDR3* ddr3 = new DDR3(configs["org"], configs["speed"]);
        burstlen = ddr3->prefetch_size * ddr3->channel_width / 8;
    } else if (standard == "DDR4") {
        DDR4* ddr4 = new DDR4(configs["org"], configs["speed"]);
        burstlen = ddr4->prefetch_size * ddr4->channel_width / 8;
    } else if (standard == "SALP-MASA") {
        SALP* salp8 = new SALP(configs["org"], configs["speed"], "SALP-MASA", configs.get_subarrays());
        burstlen = salp8->prefetch_size * salp8->channel_width / 8;
    } else if (standard == "LPDDR3") {
        LPDDR3* lpddr3 = new LPDDR3(configs["org"], configs["speed"]);
        burstlen = lpddr3->prefetch_size * lpddr3->channel_width / 8;
    } else if (standard == "LPDDR4") {
        // total cap: 2GB, 1/2 of others
        LPDDR4* lpddr4 = new LPDDR4(configs["org"], configs["speed"]);
        burstlen = lpddr4->prefetch_size * lpddr4->channel_width / 8;
    } else if (standard == "GDDR5") {
        GDDR5* gddr5 = new GDDR5(configs["org"], configs["speed"]);
        burstlen = gddr5->prefetch_size * gddr5->channel_width / 8;
    } else if (standard == "HBM") {
        HBM* hbm = new HBM(configs["org"], configs["speed"]);
        burstlen = hbm->prefetch_size * hbm->channel_width / 8;
    } else if (standard == "WideIO") {
        // total cap: 1GB, 1/4 of others
        WideIO* wio = new WideIO(configs["org"], configs["speed"]);
        burstlen = wio->prefetch_size * wio->channel_width / 8;
    } else if (standard == "WideIO2") {
        // total cap: 2GB, 1/2 of others
        WideIO2* wio2 = new WideIO2(configs["org"], configs["speed"], configs.get_channels());
        wio2->channel_width *= 2;
        burstlen = wio2->prefetch_size * wio2->channel_width / 8;
    }
    // Various refresh mechanisms
    else if (standard == "DSARP") {
        DSARP* dsddr3_dsarp = new DSARP(configs["org"], configs["speed"], DSARP::Type::DSARP, configs.get_subarrays());
        burstlen = dsddr3_dsarp->prefetch_size * dsddr3_dsarp->channel_width / 8;
    } else if (standard == "ALDRAM") {
        ALDRAM* aldram = new ALDRAM(configs["org"], configs["speed"]);
        burstlen = aldram->prefetch_size * aldram->channel_width / 8;
    } else if (standard == "TLDRAM") {
        TLDRAM* tldram = new TLDRAM(configs["org"], configs["speed"], configs.get_subarrays());
        burstlen = tldram->prefetch_size * tldram->channel_width / 8;
    }
    else{
        HERE;
        std::cout << "Unknown memory standard." << std::endl;
        exit(EXIT_FAILURE);
    }

    return burstlen;
}

void MemWrapper::runMemSim(){

    if (standard == "DDR3") {
        DDR3* ddr3 = new DDR3(configs["org"], configs["speed"]);
        start_run(configs, ddr3, files);
    } else if (standard == "DDR4") {
        DDR4* ddr4 = new DDR4(configs["org"], configs["speed"]);
        start_run(configs, ddr4, files);
    } else if (standard == "SALP-MASA") {
        SALP* salp8 = new SALP(configs["org"], configs["speed"], "SALP-MASA", configs.get_subarrays());
        start_run(configs, salp8, files);
    } else if (standard == "LPDDR3") {
        LPDDR3* lpddr3 = new LPDDR3(configs["org"], configs["speed"]);
        start_run(configs, lpddr3, files);
    } else if (standard == "LPDDR4") {
        // total cap: 2GB, 1/2 of others
        LPDDR4* lpddr4 = new LPDDR4(configs["org"], configs["speed"]);
        start_run(configs, lpddr4, files);
    } else if (standard == "GDDR5") {
        GDDR5* gddr5 = new GDDR5(configs["org"], configs["speed"]);
        start_run(configs, gddr5, files);
    } else if (standard == "HBM") {
        HBM* hbm = new HBM(configs["org"], configs["speed"]);
        start_run(configs, hbm, files);
    } else if (standard == "WideIO") {
        // total cap: 1GB, 1/4 of others
        WideIO* wio = new WideIO(configs["org"], configs["speed"]);
        start_run(configs, wio, files);
    } else if (standard == "WideIO2") {
        // total cap: 2GB, 1/2 of others
        WideIO2* wio2 = new WideIO2(configs["org"], configs["speed"], configs.get_channels());
        wio2->channel_width *= 2;
        start_run(configs, wio2, files);
    }
    // Various refresh mechanisms
    else if (standard == "DSARP") {
        DSARP* dsddr3_dsarp = new DSARP(configs["org"], configs["speed"], DSARP::Type::DSARP, configs.get_subarrays());
        start_run(configs, dsddr3_dsarp, files);
    } else if (standard == "ALDRAM") {
        ALDRAM* aldram = new ALDRAM(configs["org"], configs["speed"]);
        start_run(configs, aldram, files);
    } else if (standard == "TLDRAM") {
        TLDRAM* tldram = new TLDRAM(configs["org"], configs["speed"], configs.get_subarrays());
        start_run(configs, tldram, files);
    }
    printf("Simulation done. Statistics written to %s\n", stats_out.c_str());

}

void MemWrapper::loadConfig(int argc, char* argv[]){
    if (argc < 2) {
        printf("Usage: %s <configs-file> --mode=cpu,dram,acc [--stats <filename>] <trace-filename1> <trace-filename2> Example: %s ramulator-configs.cfg --mode=cpu cpu.trace cpu.trace\n", argv[0], argv[0]);
    }

    standard = configs["standard"];
    assert(standard != "" || "DRAM standard should be specified.");

    const char *trace_type = strstr(argv[2], "=");
    trace_type++;
    if (strcmp(trace_type, "acc") == 0){
        configs.add("trace_type", "acc");
    } else {
        printf("invalid trace type: %s\n", trace_type);
        assert(false);
    }

    int trace_start = 3;
    Stats::statlist.output(standard+".stats");
    stats_out = standard + string(".stats");

    // When the accelerator is used, there is no need for trace files.
    if(argc >=3){
        for(int i = trace_start; i < argc; i++){
            files.push_back(argv[i]);
        }
    }

    configs.set_core_num(argc - trace_start);
}

template<typename T>
void MemWrapper::start_run(const Config& configs, T* spec, const vector<const char*>& files) {
    // initiate controller and memory
    int C = configs.get_channels(), R = configs.get_ranks();
    // Check and Set channel, rank number
    spec->set_channel_number(C);
    spec->set_rank_number(R);
    std::vector<Controller<T>*> ctrls;
    for (int c = 0 ; c < C ; c++) {
        DRAM<T>* channel = new DRAM<T>(spec, T::Level::Channel);
        channel->id = c;
        channel->regStats("");
        Controller<T>* ctrl = new Controller<T>(configs, channel);
        ctrls.push_back(ctrl);
    }
    Memory<T, Controller> memory(configs, ctrls);

    if (configs["trace_type"] == "acc") {
        run_acc(configs, memory);
    }
    else {
        std::cout << "Error: unexpected trace type." << std::endl;
        exit(EXIT_FAILURE);
    }
}

// The reqQueue can always accept requests from pe, 
// but the memory requests may not be processed by the 
// ramulator due to the internal queue limitation. This 
// infinite queue here is used to simplify the synchronization 
// between pe and ramulator. For example, they don't have to 
// constantly check the internal queue if it is ready to accept 
// new requests. At the same time, it keeps the original parallel 
// memory processing limitation of the DRAM model.
bool MemWrapper::getMemReq(Request &req){
    std::list<Request>::iterator it;
    if(reqQueue.empty() == false){
        Request tmp = reqQueue.front();
        shallowReqCopy(tmp, req);
        req.udf.arriveMemTime = (long)(sc_time_stamp()/sc_time(1, SC_NS));
        reqQueue.pop_front();
        return true;
    }
    else{
        return false;
    }
}

template<typename T>
void MemWrapper::run_acc(const Config& configs, Memory<T, Controller>& memory) {
    /* run simulation */
    bool stall = false;
    bool success = false;
    int reads = 0;
    int writes = 0;
    int clks = 0;
    Request::Type type = Request::Type::READ;
    map<int, int> latencies;

    // Callback function
    auto read_complete = [this, &latencies](Request& r){
        long latency = r.depart - r.arrive;
        latencies[latency]++;

        //update departMemTime
        r.udf.departMemTime = r.udf.arriveMemTime + memClkCycle * latency;
        int burstIdx = r.udf.burstIdx;

        processedReqNum[burstIdx]++;
        if(processedReqNum[burstIdx] == 1){
            GL::bursts[burstIdx]->arriveMemTime = r.udf.arriveMemTime;
        }
        if(processedReqNum[burstIdx] == totalReqNum[burstIdx]){
            GL::bursts[burstIdx]->departMemTime = r.udf.departMemTime;
            burstRespQueues[r.udf.portIdx].push_back(burstIdx);
        }
    };

    std::vector<int> addr_vec;
    Request req(addr_vec, type, read_complete);

    // Keep waiting for the memory request processing
    while (true){
        if (!stall){
            success = getMemReq(req);  
        }

        if (success){
            stall = !memory.send(req); 
            if (!stall){
                if (req.type == Request::Type::READ){ 
                    reads++;
                }
                else if (req.type == Request::Type::WRITE){ 
                    writes++;
                    long currentTimeStamp = (long)(sc_time_stamp()/sc_time(1, SC_NS));
                    req.udf.departMemTime = currentTimeStamp;
                    int burstIdx = req.udf.burstIdx;
                    processedReqNum[burstIdx]++;
                    if(processedReqNum[burstIdx] ==1){
                        GL::bursts[burstIdx]->arriveMemTime = req.udf.arriveMemTime;
                    }
                    if(processedReqNum[burstIdx] == totalReqNum[burstIdx]){
                        GL::bursts[burstIdx]->departMemTime = req.udf.departMemTime;
                        burstRespQueues[req.udf.portIdx].push_back(burstIdx);
                    }
                }
            }
        }

        wait(memClkCycle, SC_NS);
        memory.tick();
        clks ++;
        Stats::curTick++; // memory clock, global, for Statistics
    }
    // This a workaround for statistics set only initially lost in the end
    // memory.finish();
    // Stats::statlist.printall();
}


// We don't want to mess up the callback function while copying
void MemWrapper::shallowReqCopy(const Request &simpleReq, Request &req){
    req.type = simpleReq.type;
    req.coreid = simpleReq.coreid;
    req.addr = simpleReq.addr;
    req.udf.burstIdx = simpleReq.udf.burstIdx;
    req.udf.reqIdx = simpleReq.udf.reqIdx;
    req.udf.peIdx = simpleReq.udf.peIdx;
    req.udf.portIdx = simpleReq.udf.portIdx;
    req.udf.arriveMemTime = simpleReq.udf.arriveMemTime;
    req.udf.departMemTime = simpleReq.udf.departMemTime;
}

// Global variables are reset here while this part should be 
// moved to somewhere that is easy to be noticed.
void MemWrapper::ramInit(){
    ramData.resize(GL::vecLen * 4 * (int)sizeof(float)); 

    std::vector<float> vec0;
    std::vector<float> vec1;
    std::vector<float> vec2;
    std::vector<float> result;
    vec0.resize(GL::vecLen);
    vec1.resize(GL::vecLen);
    vec2.resize(GL::vecLen);
    result.resize(GL::vecLen);

    for(int i = 0; i < GL::vecLen; i++){
        vec0[i] = (rand()%100)/10.0;
        vec1[i] = (rand()%100)/10.0;
        vec2[i] = 0;
        result[i] = vec0[i] + vec1[i];
    }

    auto alignMyself = [](long addr)->long{
        int bw = 8;
        long mask = 0xFF;
        long alignedAddr = addr;
        if((addr & mask) != 0){
            alignedAddr = ((addr >> bw) + 1) << bw; 
        }
        return alignedAddr;
    };

    // Init memory
    long addr0 = GL::vecMemAddr0 = 0;
    long addr1 = addr0 + (long)sizeof(float) * GL::vecLen;
    GL::vecMemAddr1 = addr1 = alignMyself(addr1);
    long addr2 = addr1 + (long)sizeof(float) * GL::vecLen;
    GL::vecMemAddr2 = addr2 = alignMyself(addr2);
    long resultAddr = addr2 + (long)sizeof(float) * GL::vecLen;
    GL::resultMemAddr = resultAddr = alignMyself(resultAddr);

    // fill memory with the data
    auto fillMem = [this](const std::vector<float> &vec, long &addr){
        for(auto val : vec){
            updateSingleDataToRam<float>(addr, val);
            addr += (long)sizeof(float);
        }
    };

    fillMem(vec0, addr0);
    fillMem(vec1, addr1);
    fillMem(vec2, addr2);
    fillMem(result, resultAddr);
}


void MemWrapper::dumpArray(const std::string &fname, const long &baseAddr){
    std::ofstream fhandle(fname.c_str());
    if(!fhandle.is_open()){
        HERE;
        std::cout << "Failed to open " << fname << std::endl;
        exit(EXIT_FAILURE);
    }

    long addr = baseAddr;
    for(int i = 0; i < GL::vecLen; i++){
        fhandle << getSingleDataFromRam<float>(addr) << std::endl;
        addr += sizeof(float);
    }
}

void MemWrapper::statusMonitor(){
    while(true){
        if(computeDone.read()){
            dumpArray("result.txt", GL::vecMemAddr2);
            dumpArray("gold.txt", GL::resultMemAddr);
            sc_stop();
        }
        wait(peClkCycle, SC_NS);
    }
}
