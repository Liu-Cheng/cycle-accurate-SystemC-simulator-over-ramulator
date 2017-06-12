#ifndef __MEM_IF_H__
#define __MEM_IF_H__

#include "common.h"

template<typename T>
class readIF : public sc_module{

    SC_HAS_PROCESS(readIF);

    public:
        sc_in<bool> clk;
        sc_in<bool> ena;
        sc_in<long> addr;
        sc_in<int> length;

        // read interface signals
        sc_out<T> dout;
        sc_out<bool> valid;
        sc_out<bool> complete;

        sc_out<long> readBurstReq;
        sc_in<long> readBurstResp;

        readIF(sc_module_name _name, int _portIdx, int _peIdx, double _clkCycle){
            portIdx = _portIdx;
            peIdx = _peIdx;
            clkCycle = _clkCycle;

            SC_METHOD(issueReadReq);
            sensitive << clk.pos();

            SC_METHOD(processReadResp);
            sensitive << clk.pos();

            SC_METHOD(getReadResp);
            sensitive << clk.pos();
        };

        void sigInit(){
            dout.write(-1);
            valid.write(false);
            complete.write(false);
            readBurstReq.write(-1);
        }

    private:
        int portIdx;
        int peIdx;
        double clkCycle;
        std::list<long> respQueue;

        void issueReadReq(){
            if(ena.read()){
                long burstIdx = createReadBurstReq(
                        ramulator::Request::Type::READ, 
                        portIdx, 
                        addr.read(), 
                        length.read());
                readBurstReq.write(burstIdx);
            }
            else{
                readBurstReq.write(-1);
            }
        };

        void getReadResp(){
            int burstIdx = readBurstResp.read();
            if(burstIdx != -1){
                respQueue.push_back(burstIdx);
            }
        };

        void processReadResp(){
            if(respQueue.empty() == false){
                long burstIdx = respQueue.front();
                BurstOp* ptr = GL::bursts[burstIdx];
                if(ptr->isDataAvail()){ 
                    T t = ptr->removeFrontData<float>();
                    dout.write(t);
                    valid.write(true);

                    if(ptr->isDataAvail() == false){
                        respQueue.pop_front();
                        complete.write(true);
                    }
                    else{
                        complete.write(false);
                    }
                } 
                else{
                    HERE;
                    std::cout << "Unexpected scenario." << std::endl;
                    exit(EXIT_FAILURE);
                }
            }    
            else{
                dout.write(-1);
                valid.write(false);
                complete.write(false);
            }
        };

        long createReadBurstReq(
                ramulator::Request::Type type, 
                int pidx,
                long addr, 
                int length)
        {
            long burstIdx = GL::getBurstIdx();
            BurstOp* ptr = new BurstOp(type, peIdx, pidx, burstIdx, addr, length);
            ptr->updateReqVec();
            ptr->updateAddrVec();
            GL::bursts.push_back(ptr);
            return burstIdx;
        };
};

#endif
