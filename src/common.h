#ifndef __COMMON_H__
#define __COMMON_H__

#include <list>
#include <sstream>
#include "Request.h"
#include "systemc.h"

std::ostream& operator<<(std::ostream &os, const ramulator::Request::Type &type);

// This macro is used to locate the code position.
#define HERE do {std::cout <<"File: " << __FILE__ << " Line: " << __LINE__ << std::endl;} while(0)

// ----------------------------------------------------------------------------
// The burst operation is decoded in the memory wrapper and 
// it provides a simple and easy-to-use interface to the processing elements.
// Although the data between the memory wrapper and the pe is supposed to 
// be transmitted word by word, now we will not send the burst data 
// untill the whole burst transmission from the ramulator is 
// detected while the real system may does the transmission gradually. 
// Basically the difference is where we are going to 
// buffer the partital data in the system. Since 
// buffering in memory controller makes the 
// whole system much more convenient and it is precise enough to simulate the 
// data transmission, we adopt it in the simulator.
// ----------------------------------------------------------------------------
struct BurstOp{

    public:
        bool valid;
        ramulator::Request::Type type;
        int portIdx;
        long burstIdx;
        int peIdx;
        long addr;
        int length;

        // Each burst operation consists of multiple basic memory requests/responses 
        // and the memory opid and address will be stored in the vector.
        std::vector<long> reqVec;
        std::vector<long> addrVec;

        long departPeTime;
        long arriveMemTime;
        long departMemTime;
        long arrivePeTime;

        void convertToReq(std::list<ramulator::Request> &reqQueue);

        template<typename T>
        void burstReqToBuffer(std::list<T> &buffer){
            char* p = (char*) malloc(sizeof(T));
            if(length%sizeof(T) != 0){
                HERE;
                std::cout << "The burst request length is not aligned to the buffer type.";
                std::cout << std::endl;
                exit(EXIT_FAILURE);
            }

            for(int i = 0; i < length;){
                for(size_t j = 0; j < sizeof(T); j++){
                    *(p+j) = data[i];
                    i++;
                }
                buffer.push_back(*((T*)p));
            }
            delete p;
        }


        // This fucntion copies the data from local buffer to the write burst request data section.
        template<typename T>
        void bufferToBurstReq(std::list<T> &buffer){
            T *p = (T*)malloc(sizeof(T));
            int size = length/sizeof(T);
            for(int i = 0; i < size; i++){
                *p = buffer.front();
                buffer.pop_front();
                for(int j = 0; j < (int)sizeof(T); j++){
                    data.push_back(*((char*)p+j));
                }
            }
        }

        template<typename T>
        T removeFrontData(){
            T t;
            T* p = &t;
            for(int i = 0; i < (int)sizeof(T); i++){
                auto it = data.begin();
                *((char*)p + i) = (*it);
                data.erase(it);
            }
            return t;
        }

        bool isDataAvail(){
            return !data.empty();
        };


        // Overloaded operators that are requred to support sc_in/out port
        void operator=(const BurstOp &op);
        bool operator==(const BurstOp &op) const; 
        friend void sc_trace(sc_trace_file *tf, const BurstOp &op, const std::string &name);
        friend std::ostream& operator<<(std::ostream &os, const BurstOp &op);

        int getReqNum() const;
        void updateReqVec();
        void updateAddrVec();

        void ramToReq(const std::vector<char> &ramData);
        void reqToRam(std::vector<char> &ramData);

        // Constructors
        BurstOp(ramulator::Request::Type _type, 
                int _peIdx, 
                int _portIdx,
                long _burstIdx, 
                long _addr, 
                int _length);

        BurstOp(bool _valid = false);

    private:
        // Attached data. 
        std::vector<char> data;

        long getAlignedAddr() const;
        int getOffset() const;
};

class GL{
    public:
        // Application parameters
        static int vecLen;
        static int bufferLen;
        static long vecMemAddr0;
        static long vecMemAddr1;
        static long vecMemAddr2;
        static long resultMemAddr;

        static int baseLen;
        static int burstLen; 
        static int burstAddrWidth;

        static int portNum;
        static int logon;

        // Gloabl container that stores all the bursts created in the bfs.
        // When the a burst is no longer used, the allocated memory will be 
        // released and the corresponding element in the vector will be set 
        // to be NULL.
        static std::vector<BurstOp*> bursts;

        // A long request will be split into base length of 
        // bursts such that the burst will not be overflow 
        // the buffer and block shorter bursts coming afterwards.
        static long getReqIdx();
        static long getBurstIdx();
        static void cfgBfsParam(const std::string &cfgFileName);

    private:
        static long reqIdx;
        static long burstIdx;
        static int getBurstAddrWidth();
};


#endif
