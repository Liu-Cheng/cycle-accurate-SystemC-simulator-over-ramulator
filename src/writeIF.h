#ifndef __WRITE_IF_H__
#define __WRITE_IF_H__

#include "Request.h"
#include "common.h"

template<typename T> class writeIF : public sc_module {

    SC_HAS_PROCESS(writeIF);

    public:
        sc_in<bool> clk;
        sc_in<bool> wen;
        sc_in<long> addr;
        sc_in<int> length;

        sc_in<T> din;
        sc_in<bool> valid;
        sc_out<bool> complete;

        sc_out<long> writeBurstReq;
        sc_in<long> writeBurstResp;

        writeIF(sc_module_name _name, int _portIdx, int _peIdx, double _clkCycle){
            portIdx = _portIdx;
            peIdx = _peIdx;
            clkCycle = _clkCycle;

            SC_METHOD(issueWriteReq);
            sensitive << clk.pos();

            SC_METHOD(getWriteReq);
            sensitive << clk.pos();

            SC_METHOD(processWriteResp);
            sensitive << clk.pos();
        };

        void sigInit(){
            complete.write(false);
            writeBurstReq.write(-1);
        }

    private:
        int portIdx;
        int peIdx;
        double clkCycle;
        std::list<T> buffer;
        std::list<long> reqQueue;
        long createWriteBurstReq(
                ramulator::Request::Type type, 
                int pidx,
                long addr, 
                int length
                )
        {
            long burstIdx = GL::getBurstIdx();
            BurstOp* ptr = new BurstOp(type, peIdx, pidx, burstIdx, addr, length);
            ptr->updateReqVec();
            ptr->updateAddrVec();
            GL::bursts.push_back(ptr);
            return burstIdx;
        };

        void issueWriteReq(){
            if(reqQueue.empty() == false){
                long burstIdx = reqQueue.front();
                BurstOp* ptr = GL::bursts[burstIdx];
                if((int)(buffer.size() * sizeof(T)) >= ptr->length){
                    ptr->bufferToBurstReq<T>(buffer);
                    writeBurstReq.write(burstIdx);
                    reqQueue.pop_front();
                }
                else{
                    writeBurstReq.write(-1);
                }
            }
            else{
                writeBurstReq.write(-1);
            }

        };

        void getWriteReq(){
            if(valid.read()){
                buffer.push_back(din.read());
            }

            if(wen.read()){
                int burstIdx = createWriteBurstReq(
                        ramulator::Request::Type::WRITE, 
                        portIdx, 
                        addr.read(), 
                        length.read());
                reqQueue.push_back(burstIdx);
            }

        };

        void processWriteResp(){
            if(writeBurstResp.read() != -1){
                complete.write(true);
            }
            else{
                complete.write(false);
            }

        };
};

#endif
