#ifndef __COMMON_H__
#define __COMMON_H__

#include <list>
#include <sstream>
#include "Request.h"
#include "systemc.h"

std::ostream& operator<< (std::ostream &os, const ramulator::Request::Type &type);

class GL{
    public:
        // Application parameters
        static int vaLen;
        static int vbLen;
        static int vpLen;

        // Initial va, vb, and vp address. 
        // Suppose they stay in a continuous address space.
        static long vaMemAddr;
        static long vbMemAddr;
        static long vpMemAddr;

        // Processing element setup
        static int vaBufferDepth;
        static int vbBufferDepth;
        static int vpBufferDepth;

        // It will be reset based on memory configuration
        static int burstLen; 
        static int burstAddrWidth;
        static long getReqIdx();
        static long getBurstIdx();

    private:
        static long reqIdx;
        static long burstIdx;
        static int getBurstAddrWidth();
};

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
        long burstIdx;
        int peIdx;
        long addr;
        int length;

        // corresponding local buffer address
        int localAddr; 

        // Each burst operation consists of multiple basic memory requests/responses 
        // and the memory opid and address will be stored in the vector.
        std::vector<long> reqVec;
        std::vector<long> addrVec;

        void convertToReq(std::list<ramulator::Request> &reqQueue);
        void burstReqToBuffer(std::vector<int> &buffer, int localAddr);
        void bufferToBurstReq(std::vector<int> &buffer, int localAddr);

        // Overloaded operators that are requred to support sc_in/out port
        void operator=(const BurstOp &op);
        bool operator==(const BurstOp &op) const; 
        friend void sc_trace(sc_trace_file *tf, const BurstOp &op, const std::string &name);
        friend std::ostream& operator<<(std::ostream &os, const BurstOp &op);

        void setDepartPeTime(long departTime);
        void setArrivePeTime(long arriveTime);
        void setDepartMemTime(long departTime);
        void setArriveMemTime(long arriveTime);

        long getDepartPeTime() const;
        long getArrivePeTime() const;
        long getDepartMemTime() const;
        long getArriveMemTime() const;
        int getReqNum() const;

        void updateReqVec();
        void updateAddrVec();

        void ramToReq(const std::vector<char> &ramData);
        void reqToRam(std::vector<char> &ramData);

        // Constructors
        BurstOp(ramulator::Request::Type _type, 
                long _burstIdx, 
                int _peIdx, 
                long _addr, 
                int _length,
                int localAddr);

        BurstOp(bool _valid = false);

        ~BurstOp();

    private:
        long departPeTime;
        long arriveMemTime;
        long departMemTime;
        long arrivePeTime;

        // Attached data. 
        std::vector<char> data;

        long getAlignedAddr() const;
        int getOffset() const;
};

// This macro is used to locate the code position.
#define HERE do {std::cout <<"File: " << __FILE__ << " Line: " << __LINE__ << std::endl;} while(0)

#endif
