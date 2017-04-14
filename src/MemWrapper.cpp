#include "MemWrapper.h"

// The memory configuration is initially ported from ramulator.
// which are mostly parsed from input argument. We don't want to change it for now.
MemWrapper::MemWrapper(sc_module_name _name,
        int _memClkCycle,
        int _peClkCycle,
        int argc, 
        char* argv[]) 
    : sc_module(_name), configs(argv[1]){

    loadConfig(argc, argv);
    memClkCycle = _memClkCycle; 
    peClkCycle = _peClkCycle; 
    GL::burstLen = calBurstLen();
    ramInit();
    
    SC_THREAD(runMemSim);
    SC_THREAD(getBurstReq);
    SC_THREAD(sendBurstResp);
    SC_THREAD(respMonitor);

}

// This function is used to update ram content.
// As the ramulator doesn't returns the write response to request, 
// it is not easy to decide when to update write request based on the 
// basic read/write request. To solve this problem, we update ram content 
// based on the burst request order. We will not update write burst 
// request until there is a following read request coming up. In this case, 
// we can be sure the ram follows the sequential consistence despite the 
// parallel memory processing.
void MemWrapper::updateBurstToRam(long watchedBurstIdx){
    for(auto it = burstReqQueue.begin(); it != burstReqQueue.end(); it++){
        // All the write burst requests that arrive after the watched burst request
        // will not affect the read processing.
        if(it->burstIdx == watchedBurstIdx){
            break;
        }
        else{
            if(it->type == ramulator::Request::Type::WRITE){
                it->reqToRam(ramData);
                writebackHistory[it->burstIdx] = true;
            }
        }
    }
}

// This process basically detects the respQueue every memClkCycle.
// When a burstRequest has all its requests processed, we will declare 
// the end of the burstRequest processing. And a burst Response will be 
// genenrated and enqueued into the burstRespQueue.
void MemWrapper::respMonitor(){
    while(true){
        for(auto bit = burstReqQueue.begin(); bit != burstReqQueue.end(); bit++){

            // Skip the burst requests that have been responsed.
            if(burstStatus[bit->burstIdx]) continue;

            // Check if all the splitted requests get responsed.
            bool burstRespReady = true;
            if(burstStatus[bit->burstIdx] == false){
                for(auto rit = bit->reqVec.begin(); rit != bit->reqVec.end(); rit++){
                    if(reqStatus[*rit] == false){
                        burstRespReady = false;
                        break;
                    }
                }
            }

            if(burstRespReady){
                burstStatus[bit->burstIdx] = true;
                BurstOp resp = *bit;
                resp.setArriveMemTime(getMinArriveTime(bit->reqVec));
                resp.setDepartMemTime(getMaxDepartTime(bit->reqVec));
                burstRespQueue.push_back(resp);
                cleanRespQueue(bit->reqVec);
            }
        }

        wait(memClkCycle, SC_NS);
    }
}

// Analyze the arrive memory time of all basic requests of a single burst operation.
// To save the function passsing cost, we have only the involved reqIdx vectoe passed.
long MemWrapper::getMinArriveTime(const std::vector<long> &reqVec){
    std::vector<long> arriveTime;
    for(auto rit = reqVec.begin(); rit != reqVec.end(); rit++){
        for(auto it = respQueue.begin(); it != respQueue.end(); it++){
            if(it->udf.reqIdx == *rit){
                arriveTime.push_back(it->udf.arriveMemTime);
                break;
            }
        }
    }

    auto mit = std::min_element(arriveTime.begin(), arriveTime.end());
    return *mit; 
        
}

long MemWrapper::getMaxDepartTime(const std::vector<long> &reqVec){
    std::vector<long> departTime;
    for(auto rit = reqVec.begin(); rit != reqVec.end(); rit++){
        for(auto it = respQueue.begin(); it != respQueue.end(); it++){
            if(it->udf.reqIdx == *rit){
                departTime.push_back(it->udf.departMemTime);
                break;
            }
        }
    }

    auto it = std::max_element(departTime.begin(), departTime.end());
    return *it;
}

// As we don't want to have the resp/req queues grow with time and don't want to 
// implement complex and accurate clean strategy, thus we just clean 
// the old memory requests based on the timestamps.  
void MemWrapper::cleanProcessedRequests(){}

// It reads request from pe and thus is synchronized to the pe's clock
void MemWrapper::getBurstReq(){
    int counter = 0;
    while(true){
        BurstOp op = burstReq.read();
        if(op.valid){
            burstReqQueue.push_back(op);
            burstStatus[op.burstIdx] = false;
            if(op.type == ramulator::Request::Type::WRITE){
                writebackHistory[op.burstIdx] = false;
            }

            op.convertToReq(reqQueue);

            // update request status
            for(auto it = op.reqVec.begin(); it != op.reqVec.end(); it++){
                reqStatus[*it] = false;
            }

            if(op.type == ramulator::Request::Type::READ){
                counter++;
                std::cout << counter << " burst requests are received." << std::endl;
            }
        }
        wait(peClkCycle, SC_NS);
    }
}

// Traverse the response request and write the response to 
// pe based on the response time. Basically, we need to synchronize 
// pe and memory operations.
void MemWrapper::sendBurstResp(){
    while(true){
        long currentTimeStamp = (long)(sc_time_stamp()/sc_time(1, SC_NS));
        if(burstRespQueue.empty()){
            BurstOp op(false);
            burstResp.write(op);
        }
        else {
            // Although multiple memory response may be sent, only a single 
            // one can be sent to a pe each time. Basically, it is possible 
            // that memory responses may not be able to sent out and 
            // additional queuing time is required. 
            auto it = burstRespQueue.begin();
            while(it != burstRespQueue.end()){
                long respReadyTime = it->getDepartMemTime(); 
                if(respReadyTime <= currentTimeStamp){
                    if(it->type == ramulator::Request::Type::READ){
                        // Update all the write burst requests that 
                        // arrives earlier than this request to ramData
                        updateBurstToRam(it->burstIdx); 
                        it->ramToReq(ramData);
                    }
                    burstResp.write(*it);
                    burstRespQueue.erase(it++);
                    break;
                }
                else{
                    it++;
                }
            }
        }

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
    if(!reqQueue.empty()){
        it = reqQueue.begin();
        shallowReqCopy(*it, req);
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
    bool end = false;
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
        respQueue.push_back(r);
        reqStatus[r.udf.reqIdx] = true;
        std::cout << "req: " << r.udf.reqIdx << " is returnned at " << sc_time_stamp() << std::endl;
        std::cout << "req: " << r.udf.reqIdx << " " << r.arrive << " " << r.depart << std::endl;
    };

    std::vector<int> addr_vec;
    Request req(addr_vec, type, read_complete);

    // Keep waiting for the memory request processing
    while (true){
        if (!stall){
            end = !getMemReq(req); 
        }

        if (!end){
            stall = !memory.send(req);
            if (!stall){
                if (req.type == Request::Type::READ){ 
                    std::cout << "req: " << req.udf.reqIdx << " is issued at " << sc_time_stamp() << std::endl;
                    reads++;
                }
                // At this time, we can already assume that the write operation is done.
                else if (req.type == Request::Type::WRITE){ 
                    writes++;
                    long currentTimeStamp = (long)(sc_time_stamp()/sc_time(1, SC_NS));
                    req.udf.departMemTime = currentTimeStamp;
                    respQueue.push_back(req);
                    reqStatus[req.udf.reqIdx] = true;
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
    req.udf.departPeTime = simpleReq.udf.departPeTime;
    req.udf.arriveMemTime = simpleReq.udf.arriveMemTime;
    req.udf.departMemTime = simpleReq.udf.departMemTime;
    req.udf.arrivePeTime = simpleReq.udf.arrivePeTime;
}

void MemWrapper::ramInit(){

    ramData.resize(1024*1024);

    long addrVa = GL::vaMemAddr;
    long addrVb = GL::vbMemAddr;
    long addrVp = GL::vpMemAddr;
    int val = 0;

    for(int i = 0; i < 512; i++){
        updateSingleDataToRam<int>(addrVa, val);
        updateSingleDataToRam<int>(addrVb, val);
        updateSingleDataToRam<int>(addrVp, 0);
        val++;
        addrVa += (long)sizeof(int);
        addrVb += (long)sizeof(int);
        addrVp += (long)sizeof(int);
    }

}

// Update ram on a specified addr with specified data type.
template <typename T>
void MemWrapper::updateSingleDataToRam(long addr, T t){
    T* p = (T*)malloc(sizeof(T));
    *p = t;

    for(int i = 0; i < (int)sizeof(T); i++){
        ramData[addr+i] = *((char*)p + i);
    }

    delete p;
}


// Clean the resp that has been combined to a burst response from the queue.
void MemWrapper::cleanRespQueue(const std::vector<long> &reqVec){
    for(auto rit = reqVec.begin(); rit != reqVec.end(); rit++){
        for(auto it = respQueue.begin(); it != respQueue.end(); it++){
            if(it->udf.reqIdx == *rit){
                respQueue.erase(it);
                break;
            }
        }
    }
}
