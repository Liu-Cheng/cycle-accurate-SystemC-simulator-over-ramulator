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
    SC_THREAD(issueReadReq);
    SC_THREAD(issueWriteReq);
    SC_THREAD(vecAdd);
    SC_THREAD(statusMonitor);
}

void pe::statusMonitor(){
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
        }

        if(allVbReqGenerated){
            allVbRespReceived = true;
            for(auto it = vbBurstReqStatus.begin(); it != vbBurstReqStatus.end(); it++){
                if(it->second == false){
                    allVbRespReceived = false;
                    break;
                }
            }
        }

        if(allVpReqGenerated){
            allVpRespReceived = true;
            for(auto it = vpBurstReqStatus.begin(); it != vpBurstReqStatus.end(); it++){
                if(it->second == false){
                    allVpRespReceived = false;
                }
            }
        }

        if(allVaRespReceived && allVbRespReceived && allVpRespReceived){
            std::cout << "Simulation ends at " << sc_time_stamp() << std::endl;
            std::cout << "The resulting vp : " << std::endl;
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
        if(vpAddr == GL::vpLen){
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
        std::vector<int> &buffer)
{
    long burstIdx = GL::getBurstIdx();
    BurstOp op(type, burstIdx, peIdx, addr, length, localAddr);
    op.updateReqVec();
    op.updateAddrVec();
    op.bufferToBurstReq(buffer, localAddr);
    wait(peClkCycle * op.getReqNum() * GL::burstLen/dataWidth, SC_NS);
    burstReqQueue.push_back(op);

    return burstIdx;
}

long pe::createReadBurstReq(
        ramulator::Request::Type type, 
        long addr, 
        int length, 
        int localAddr)
{
    long burstIdx = GL::getBurstIdx();
    BurstOp op(type, burstIdx, peIdx, addr, length, localAddr);
    op.updateReqVec();
    op.updateAddrVec();
    burstReqQueue.push_back(op);
    
    return burstIdx;
}


void pe::issueWriteReq(){
    long _vpMemAddr = GL::vpMemAddr;
    int vpBufferAddr = 0;
    long burstIdx;
    while(true){
        if(vpBufferAddr == GL::vpLen){
            allVpReqGenerated = true;
        }

        if(computingDone == true && vpBufferAddr < GL::vpLen){
            ramulator::Request::Type type = ramulator::Request::Type::WRITE;
            burstIdx = createWriteBurstReq(type, _vpMemAddr, 64, vpBufferAddr, vp);
            _vpMemAddr += 64;
            vpBufferAddr += 64/sizeof(int);
            vpBurstReqStatus[burstIdx] = false;
        }
        else{
            wait(peClkCycle, SC_NS);
        }
    }
}

// Generate 1024-byte read memory request from both addrVa and addrVb.
// This process may also be invoked on certain user defined condition.
void pe::issueReadReq(){
    // These are the initial memory address of va, vb and vp.
    long _vaMemAddr = GL::vaMemAddr;
    int vaBufferAddr = 0;
    long _vbMemAddr = GL::vbMemAddr;
    int vbBufferAddr = 0;
    long burstIdx;
    ramulator::Request::Type type = ramulator::Request::Type::READ;

    // Create 128 byte read requests 
    int len = 64;
    for(int i = 0; i < GL::vaLen * (int)(sizeof(int)); i = i + len){
        burstIdx = createReadBurstReq(type, _vaMemAddr, len, vaBufferAddr);
        _vaMemAddr += len;
        vaBufferAddr += len/sizeof(int);
        vaReqQueue.push_back(burstIdx);
        vaBurstReqStatus[burstIdx] = false;

        burstIdx = createReadBurstReq(type, _vbMemAddr, len, vbBufferAddr);
        _vbMemAddr += len;
        vbBufferAddr += len/sizeof(int);
        vbReqQueue.push_back(burstIdx);
        vbBurstReqStatus[burstIdx] = false;
        wait(peClkCycle, SC_NS);
    }

    allVaReqGenerated = true;
    allVbReqGenerated = true;
 
    HERE;
    std::cout << "All the read burst memory operations have been generated at ";
    std::cout << sc_time_stamp() << std::endl;

}

/*=-----------------------------------------------------------
 * The MemWrapper will have an infinite request queue 
 * that can always accomodate all the requests from 
 * PEs. Then the queue will gradually send the requests to the
 * ramulator which has the admission control.
 *=---------------------------------------------------------*/
void pe::sendMemReq(){ 
    while(true){
        if(!burstReqQueue.empty()){
            BurstOp op = burstReqQueue.front();
            long departTime = (long) (sc_time_stamp()/sc_time(1, SC_NS));
            op.setDepartPeTime(departTime);
            burstReq.write(op);
            burstReqQueue.pop_front();
            std::cout << "burst " << op.burstIdx << " is sent to ramulator. " << std::endl;

            // Update burst request status
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
            burstRespQueue.push_back(op);

            // Update resp status
            if(vaBurstReqStatus.find(op.burstIdx) != vaBurstReqStatus.end()){
                vaBurstReqStatus[op.burstIdx] = true;

                // copy data back to buffer for read request
                op.burstReqToBuffer(va, op.localAddr);
            }

            else if(vbBurstReqStatus.find(op.burstIdx) != vbBurstReqStatus.end()){
                vbBurstReqStatus[op.burstIdx] = true;
                op.burstReqToBuffer(vb, op.localAddr);
            }

            else if(vpBurstReqStatus.find(op.burstIdx) != vpBurstReqStatus.end()){
                vpBurstReqStatus[op.burstIdx] = true;

                // doing nothing for write response.
            }

            else {
                HERE;
                std::cout << "There is no such burst id in this pe." << std::endl;
                exit(EXIT_FAILURE);
            }
        } 

        wait(peClkCycle, SC_NS);
    }
}

void pe::setPeClkCycle(int _peClkCycle){
    peClkCycle = _peClkCycle;
}

void pe::dumpResp(){
    for(auto it = burstRespQueue.begin(); it != burstRespQueue.end(); it++){
        std::cout << (*it) << std::endl;
    }
}

void pe::setDataWidth(int width){
    dataWidth = width; 
}
