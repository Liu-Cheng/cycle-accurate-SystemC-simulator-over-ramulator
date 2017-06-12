#include "pe.h"

// Constructor
pe::pe(
        sc_module_name _name, 
        int _peIdx, 
        int _peClkCycle
        ) :sc_module(_name) 
{
    peIdx = _peIdx;
    peClkCycle = _peClkCycle;
    writeReqNum = 0;
    writeReqCounter = 0;
    allWriteReqSent = false;

    init();

    SC_METHOD(readVec0);
    sensitive << peClk.pos();

    SC_METHOD(readVec1);
    sensitive << peClk.pos();

    SC_METHOD(writeVec);
    sensitive << peClk.pos();

    SC_METHOD(vecAdd);
    sensitive << peClk.pos();

    SC_METHOD(statusMonitor);
    sensitive << peClk.pos();
}

// Output signals are initialized to avoid wrong signal 
// based processing.
void pe::sigInit(){
    readEna0.write(false);
    readAddr0.write(-1);
    readLength0.write(-1);

    readEna1.write(false);
    readAddr1.write(-1);
    readLength1.write(-1);

    writeDout.write(-1);
    writeValid.write(false);

    writeEna.write(false);
    writeAddr.write(-1);
    writeLength.write(-1);

    computeDone.write(false);
}

void pe::init(){
    for(int i = 0; i < 3; i++){
        std::list<float> queue;
        queues.push_back(queue);
        queues.push_back(queue);
    }

    availBufferVec0 = GL::bufferLen;
    availBufferVec1 = GL::bufferLen;
    readCounter0 = 0;
    readCounter1 = 0;
    writeCounter = 0;
    localCounter = 0;
}

void pe::readVec0(){
    int resNum = GL::vecLen - readCounter0;
    if(resNum > 0){
        int actualNum = GL::baseLen/(int)sizeof(float);
        if(resNum < actualNum){
            actualNum = resNum;
        }

        if(actualNum <= availBufferVec0){
            long addr = GL::vecMemAddr0 + readCounter0 * (int)sizeof(float);
            readEna0.write(true);
            readAddr0.write(addr);
            readLength0.write(actualNum * (int)sizeof(float));

            readCounter0 += actualNum;
            availBufferVec0 -= actualNum;
        }
        else{
            readEna0.write(false);
            readAddr0.write(-1);
            readLength0.write(-1);
        }
    }
    else{
        readEna0.write(false);
        readAddr0.write(-1);
        readLength0.write(-1);
    }

    if(readValid0.read()){
        queues[0].push_back(readDin0.read());
    }
}

void pe::readVec1(){
    int resNum = GL::vecLen - readCounter1;
    if(resNum > 0){
        int actualNum = GL::baseLen/(int)sizeof(float);
        if(resNum < actualNum){
            actualNum = resNum;
        }

        if(actualNum <= availBufferVec1){
            long addr = GL::vecMemAddr1 + readCounter1 * (int)sizeof(float);
            readEna1.write(true);
            readAddr1.write(addr);
            readLength1.write(actualNum * (int)sizeof(float));

            readCounter1 += actualNum;
            availBufferVec1 -= actualNum;
        }
        else{
            readEna1.write(false);
            readAddr1.write(-1);
            readLength1.write(-1);
        }
    }
    else{
        readEna1.write(false);
        readAddr1.write(-1);
        readLength1.write(-1);
    }

    if(readValid1.read()){
        queues[1].push_back(readDin1.read());
    }
}

void pe::vecAdd(){
   if(queues[0].empty() == false && queues[1].empty() == false){
       float d0 = queues[0].front();
       float d1 = queues[1].front();
       float d2 = d0 + d1;
       queues[2].push_back(d2);
       queues[0].pop_front();
       queues[1].pop_front();
       availBufferVec0 += 1;
       availBufferVec1 += 1;
   } 
}

void pe::writeVec(){
    if(queues[2].empty() == false){
        writeDout.write(queues[2].front());
        writeValid.write(true);
        queues[2].pop_front();
        localCounter++;
    }
    else{
        writeDout.write(-1);
        writeValid.write(false);
    }

    long addr = GL::vecMemAddr2 + writeCounter * (int)sizeof(float);
    int baseSize = GL::baseLen/(int)sizeof(float);
    if(localCounter == GL::vecLen - writeCounter && 
       localCounter > 0 && localCounter <= baseSize){
        writeEna.write(true);
        writeAddr.write(addr);
        writeLength.write(localCounter * sizeof(float));
        writeCounter = GL::vecLen;
        localCounter = 0;
        writeReqNum++;
        allWriteReqSent = true;
    }
    else if(localCounter == baseSize){
        writeEna.write(true);
        writeAddr.write(addr);
        writeLength.write(GL::baseLen);
        writeCounter += baseSize;
        localCounter = 0;
        writeReqNum++;
    }
    else{
        writeEna.write(false);
        writeAddr.write(-1);
        writeLength.write(-1);
    }
}

void pe::statusMonitor(){
    if(writeComplete.read()){
        writeReqCounter ++;
    }

    if(writeReqCounter == writeReqNum && allWriteReqSent == true){
        std::cout << "End of the vector addition." << std::endl;
        computeDone.write(true);
    }
}
