#include "pe.h"

void pe::init(){

    allVaReqGenerated = false;
    allVbReqGenerated = false;
    allVpReqGenerated = false;
    allVaRespReceived = false;
    allVbRespReceived = false;
    allVpRespReceived = false;

    computingDone = false;

    // The capacity of the three buffer is set to be 1024-word.
    va.resize(GL::vaBufferDepth);
    vb.resize(GL::vbBufferDepth);
    vp.resize(GL::vpBufferDepth);

    dataWidth = 4; // 4 bytes
}

// In order to process the memory request, 
// the address sent to the memory must be aligned to burstlen.
pe::pe(sc_module_name _name, 
        int _peIdx, 
        int _peClkCycle
        ) :sc_module(_name) 
{
    peIdx = _peIdx;
    peClkCycle = _peClkCycle;
    init();

    // Memory send and receive processes
    SC_THREAD(sendMemReq);
    SC_THREAD(getMemResp);
    SC_THREAD(issueVaReadReq);
    SC_THREAD(issueVbReadReq);
    SC_THREAD(issueVpWriteReq);
    SC_THREAD(vecAdd);
    SC_THREAD(statusMonitor);
    SC_THREAD(vaRespProcess);
    SC_THREAD(vbRespProcess);
    SC_THREAD(vpRespProcess);
}

// The burst resp must be accepted immediately each cycle as it is 
// sent by the memory without any flow control. However, it takes time 
// to process the response and the processing time is determined by the 
// length of the burst transmission.
void pe::vbRespProcess(){
    while(true){
        if(!vbBurstRespQueue.empty()){
            BurstOp op = vbBurstRespQueue.front();
            wait((op.length/dataWidth) * peClkCycle, SC_NS);
            vbBurstReqStatus[op.burstIdx] = true;
            op.burstReqToBuffer(vb, op.localAddr);
            vbBurstRespQueue.pop_front();
        }
        else{
            wait(peClkCycle, SC_NS);
        }
    }
}

void pe::vaRespProcess(){
    while(true){
        if(!vaBurstRespQueue.empty()){
            BurstOp op = vaBurstRespQueue.front();
            wait((op.length/dataWidth) * peClkCycle, SC_NS);
            vaBurstReqStatus[op.burstIdx] = true;
            op.burstReqToBuffer(va, op.localAddr);
            vaBurstRespQueue.pop_front();
        }
        else{
            wait(peClkCycle, SC_NS);
        }
    }
}

// Write response
void pe::vpRespProcess(){
    while(true){
        if(!vpBurstRespQueue.empty()){
            BurstOp op = vpBurstRespQueue.front();
            vpBurstReqStatus[op.burstIdx] = true;
            vpBurstRespQueue.pop_front();
        }

        wait(peClkCycle, SC_NS);
    }
}

void pe::statusMonitor(){

    // They are used to avoid repeated printing
    bool vaflag = true;
    bool vbflag = true;
    bool vpflag = true;
    while(true){
        // In hardware we may use counter instead of traversing the status container.
        // With the counter, we can do it in a single cycle. Thus we have the 
        // traverse done in a single cycle to simulate the functionality here.
        if(allVaReqGenerated){
            allVaRespReceived = true;
            for(auto it = vaBurstReqStatus.begin(); it != vaBurstReqStatus.end(); it++){
                if (it->second == false){
                    allVaRespReceived = false;
                    break;
                }
            }
            if(vaflag && allVaRespReceived){
                vaflag = false;
                std::cout << "All the va resp have been received at " << sc_time_stamp() << std::endl; 
            }
        }

        if(allVbReqGenerated){
            allVbRespReceived = true;
            for(auto it = vbBurstReqStatus.begin(); it != vbBurstReqStatus.end(); it++){
                if(it->second == false){
                    allVbRespReceived = false;
                    break;
                }
            }
            if(vbflag && allVbRespReceived){
                vbflag = false;
                std::cout << "All the vb resp have been received at " << sc_time_stamp() << std::endl;
            }
        }

        if(allVpReqGenerated){
            allVpRespReceived = true;
            for(auto it = vpBurstReqStatus.begin(); it != vpBurstReqStatus.end(); it++){
                if(it->second == false){
                    allVpRespReceived = false;
                }
            }
            if(vpflag && allVpRespReceived){
                vpflag = false;
                std::cout << "All the vp resp have been received at " << sc_time_stamp() << std::endl;
            }
        }

        if(allVaRespReceived && allVbRespReceived && allVpRespReceived){
            std::cout << "Simulation ends at " << sc_time_stamp() << std::endl;
            std::cout << "The first 20 elements of the resulting vp : " << std::endl;
            for(int i = 0; i < 20; i++){
                std::cout << vp[i] << " ";
            } 
            std::cout << std::endl;
            sc_stop();
        }

        wait(peClkCycle, SC_NS);
    }
}

// Dot production when both two input vectors are loaded from memory.
// It may either run with a streaming mode or pre-fetching mode.
void pe::vecAdd(){

    bool readStage = false;
    bool mulStage = false;
    bool writeStage = false;

    int vaAddr = 0;
    int vbAddr = 0;
    int vpAddr = 0;

    int rega;
    int regb;
    int regp;

    while(true){
        // Computing done detection
        if(vpAddr == GL::vpLen && computingDone == false){
            std::cout << "Computing is done at " << sc_time_stamp() << std::endl;
            computingDone = true;
        }

        // Writing stage
        writeStage = mulStage;
        if(writeStage){
            vp[vpAddr] = regp;
            vpAddr++;
        }

        // Multiplication stage
        mulStage = readStage;
        if(mulStage){
            regp = rega + regb;
        }

        // Reading pipeline stage
        if(allVaRespReceived && allVbRespReceived && vaAddr < GL::vaLen && vbAddr < GL::vbLen){
            rega = va[vaAddr];
            regb = vb[vbAddr];
            vaAddr++;
            vbAddr++;
            readStage = true;
        }
        else{
            readStage = false;
        }

        wait(peClkCycle, SC_NS);
    }
}

// As it takes time to write data to memory, even though the write requests 
// can always be accommodated. Thus we add additional delay here to simulate 
// the transmission cost here.
long pe::createWriteBurstReq(
        ramulator::Request::Type type, 
        long addr, 
        int length, 
        int localAddr, 
        std::vector<int> &buffer,
        PortType ptype)
{
    long burstIdx = GL::getBurstIdx();
    BurstOp op(type, burstIdx, peIdx, addr, length, localAddr);
    op.updateReqVec();
    op.updateAddrVec();

    // It takes a few cycles to read data from buffer
    wait(peClkCycle * op.length / dataWidth, SC_NS);
    op.bufferToBurstReq(buffer, localAddr);

    // It takes a number of cycles to transmit data over the bus to memory.
    // However, we assume it is only limited by the memory bandwidth and thus 
    // we rely the ramulator to contrain the memory write access.
    // wait(peClkCycle * op.getReqNum() * GL::burstLen/dataWidth, SC_NS);
    if(ptype == VP){
        vpBurstReqQueue.push_back(op);
    }
    else{
        HERE;
        std::cout << "Unknown write port." << std::endl;
        exit(EXIT_FAILURE);
    }

    return burstIdx;
}

long pe::createReadBurstReq(
        ramulator::Request::Type type, 
        long addr, 
        int length, 
        int localAddr, 
        PortType ptype)
{
    long burstIdx = GL::getBurstIdx();
    BurstOp op(type, burstIdx, peIdx, addr, length, localAddr);
    op.updateReqVec();
    op.updateAddrVec();

    wait(peClkCycle, SC_NS);
    if(ptype == VA){
        vaBurstReqQueue.push_back(op);
    }
    else if(ptype == VB){
        vbBurstReqQueue.push_back(op);
    }
    else{
        HERE;
        std::cout << "Unknown port type." << std::endl;
        exit(EXIT_FAILURE);
    }
    
    return burstIdx;
}


void pe::issueVpWriteReq(){
    long vpMemAddr = GL::vpMemAddr;
    int vpBufferAddr = 0;
    long burstIdx;
    int len = 64;
    int maxLen = GL::vpLen * sizeof(int);
    int currentLen = 0;
    while(true){
        if(vpBufferAddr == GL::vpLen && allVpReqGenerated == false){
            std::cout << "All the vp requests have been generated at " << sc_time_stamp() << std::endl;
            allVpReqGenerated = true;
        }

        if(computingDone == true && vpBufferAddr < GL::vpLen){
            if(currentLen + len > maxLen){
                len = maxLen - currentLen;
            }
            currentLen += len;
            ramulator::Request::Type type = ramulator::Request::Type::WRITE;
            burstIdx = createWriteBurstReq(type, vpMemAddr, len, vpBufferAddr, vp, VP);
            vpMemAddr += len;
            vpBufferAddr += len/sizeof(int);
            vpBurstReqStatus[burstIdx] = false;
        }
        else{
            wait(peClkCycle, SC_NS);
        }
    }
}

// Generate 1024-byte read memory request from addrVa
void pe::issueVaReadReq(){
    long vaMemAddr = GL::vaMemAddr;
    int vaBufferAddr = 0;
    long burstIdx;
    ramulator::Request::Type type = ramulator::Request::Type::READ;

    // Create 128 byte read requests 
    int len = 64;
    int maxLen = GL::vaLen * (int)(sizeof(int));
    int currentLen = 0;
    while(currentLen < maxLen){
        if(currentLen + len > maxLen){
            len = maxLen - currentLen;
        }
        burstIdx = createReadBurstReq(type, vaMemAddr, len, vaBufferAddr, VA);
        vaMemAddr += len;
        vaBufferAddr += len/sizeof(int);
        vaReqQueue.push_back(burstIdx);
        vaBurstReqStatus[burstIdx] = false;
        currentLen += len;
    }
    allVaReqGenerated = true;
 
    std::cout << "All the va read burst memory operations have been generated at ";
    std::cout << sc_time_stamp() << std::endl;
}

// We may add other read condition
void pe::issueVbReadReq(){
    long vbMemAddr = GL::vbMemAddr;
    int vbBufferAddr = 0;
    long burstIdx;
    ramulator::Request::Type type = ramulator::Request::Type::READ;

    // Create 128 byte read requests 
    int len = 64;
    int maxLen = GL::vaLen * (int)(sizeof(int));
    int currentLen = 0;
    while(currentLen < maxLen){
        if(currentLen + len > maxLen){
            len = maxLen - currentLen;
        }
        burstIdx = createReadBurstReq(type, vbMemAddr, len, vbBufferAddr, VB); 
        vbMemAddr += len;
        vbBufferAddr += len/sizeof(int);
        vbReqQueue.push_back(burstIdx);
        vbBurstReqStatus[burstIdx] = false;
        currentLen += len;
    }

    allVbReqGenerated = true;
 
    std::cout << "All the vb read burst memory operations have been generated at ";
    std::cout << sc_time_stamp() << std::endl;

}

// It decides which of the requests will win for sending the request to memory.
// Round robin strategy is used here. Note that the arbiter will only be invoked 
// when there is unempty queue available.
PortType pe::burstReqArbiter(PortType winner){
    if(winner == VA){
        if(!vbBurstReqQueue.empty()){
            return VB;
        }
        else if(!vpBurstReqQueue.empty()){
            return VP;
        }
        else{
            return VA;
        }
    }
    else if(winner == VB){
        if(!vpBurstReqQueue.empty()){
            return VP;
        }
        else if(!vaBurstReqQueue.empty()){
            return VA;
        }
        else{
            return VB;
        }
    }
    else if(winner == VP){
        if(!vaBurstReqQueue.empty()){
            return VA;
        }
        else if(!vbBurstReqQueue.empty()){
            return VB;
        }
        else{
            return VP;
        }
    }
    else{
        HERE;
        std::cout << "Unknown winner port type." << std::endl;
        exit(EXIT_FAILURE);
    }
}
/*=-----------------------------------------------------------
 * The MemWrapper will have an infinite request queue 
 * that can always accomodate all the requests from 
 * PEs. Then the queue will gradually send the requests to the
 * ramulator which has the admission control.
 *=---------------------------------------------------------*/
void pe::sendMemReq(){ 
    PortType winner = VA;
    while(true){
        if(!vaBurstReqQueue.empty() || !vbBurstReqQueue.empty() || !vpBurstReqQueue.empty()){
            PortType ptype = burstReqArbiter(winner);
            winner = ptype;

            BurstOp op; 
            if(ptype == VA){
                op = vaBurstReqQueue.front();
            }
            else if(ptype == VB){
                op = vbBurstReqQueue.front();
            }
            else if(ptype == VP){
                op = vpBurstReqQueue.front();
            }
            else{
                HERE;
                std::cout << "Unknown port type." << std::endl;
                exit(EXIT_FAILURE);
            }

            long departTime = (long) (sc_time_stamp()/sc_time(1, SC_NS));
            op.setDepartPeTime(departTime);
            burstReq.write(op);

            if(ptype == VA){
                vaBurstReqQueue.pop_front();
            }
            else if(ptype == VB){
                vbBurstReqQueue.pop_front();
            }
            else if(ptype == VP){
                vpBurstReqQueue.pop_front();
            }
            else{
                HERE;
                std::cout << "Unknown port type." << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else{
            BurstOp op(false);
            burstReq.write(op);
        }

        wait(peClkCycle, SC_NS);
    }
}

void pe::getMemResp(){
    while(true){
        BurstOp op = burstResp.read();
        if(op.valid){
            // update arrivePeTime stamp when a valid memory response is captured.
            long arriveTime = (long)(sc_time_stamp()/sc_time(1, SC_NS));
            op.setArrivePeTime(arriveTime);

            if(vaBurstReqStatus.find(op.burstIdx) != vaBurstReqStatus.end()){
                vaBurstRespQueue.push_back(op);
            }
            else if(vbBurstReqStatus.find(op.burstIdx) != vbBurstReqStatus.end()){
                vbBurstRespQueue.push_back(op);
            }
            else if(vpBurstReqStatus.find(op.burstIdx) != vpBurstReqStatus.end()){
                vpBurstRespQueue.push_back(op);
            }
            else{
                HERE;
                std::cout << "Unknown burst response. " << std::endl;
                exit(EXIT_FAILURE);
            }
        } 

        wait(peClkCycle, SC_NS);
    }
}

void pe::setPeClkCycle(int _peClkCycle){
    peClkCycle = _peClkCycle;
}

void pe::setDataWidth(int width){
    dataWidth = width; 
}
