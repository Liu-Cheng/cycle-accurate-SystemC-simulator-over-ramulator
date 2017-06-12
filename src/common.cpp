#include "common.h"

int GL::vecLen = 0;
int GL::portNum = 3;
std::vector<BurstOp*> GL::bursts;
int GL::bufferLen = 4096;
long GL::vecMemAddr0 = 0;
long GL::vecMemAddr1 = 0;
long GL::vecMemAddr2 = 0;
long GL::resultMemAddr = 0;

long GL::reqIdx = -1;
long GL::burstIdx = -1;
int GL::burstLen = 64;
int GL::baseLen = 1024; 
int GL::burstAddrWidth = GL::getBurstAddrWidth();

int GL::logon = 0;

void GL::cfgBfsParam(const std::string &cfgFileName){

    std::ifstream fhandle(cfgFileName.c_str());
    if(!fhandle.is_open()){
        HERE;
        std::cout << "Failed to open " << cfgFileName << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string graphName;
    std::string graphDir;
    while(!fhandle.eof()){
        std::string cfgKey;
        fhandle >> cfgKey;
        if(cfgKey == "vecLen"){
            fhandle >> vecLen;
        }
        else if(cfgKey == "bufferLen"){
            fhandle >> bufferLen;
        }
        else if(cfgKey == "logon"){
            fhandle >> logon;
        }
        else if(cfgKey == "portNum"){
            fhandle >> portNum;
        }
    }

    fhandle.close();

}

long GL::getBurstIdx(){
    burstIdx++;
    return burstIdx;
}

long GL::getReqIdx(){
    reqIdx++;
    return reqIdx;
}

// Calculate the # of bits when mapping a basic memory burst to the address.
int GL::getBurstAddrWidth(){

    int width = 1;
    int val = 1;
    for(int i = 0; i <= 10; i++){
        if(val == burstLen){
            return width;
        }
        else{
            width++;
            val = val * 2;
        }
    }

    HERE;
    std::cout << "burstLen is not power(2,n)." << std::endl;
    exit(EXIT_FAILURE);

}


// Overload ostream of type for convenient output
std::ostream& operator<< (std::ostream &os, const ramulator::Request::Type &type){

    switch (type){
        case ramulator::Request::Type::READ:
            os << "READ";
            break;
        case ramulator::Request::Type::WRITE:
            os << "WRITE";
            break;
        case ramulator::Request::Type::REFRESH:
            os << "REFRESH";
            break;
        case ramulator::Request::Type::POWERDOWN:
            os << "POWERDOWN";
            break;
        case ramulator::Request::Type::SELFREFRESH:
            os << "SELFREFRESH";
            break;
        case ramulator::Request::Type::EXTENSION:
            os << "EXTENSION";
            break;
        case ramulator::Request::Type::MAX:
            os << "MAX";
            break;
        default:
            os << "Unknown Type";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream &os, const BurstOp &op){

    os << "valid: " << op.valid << " ";
    os << "type: " << op.type << " ";
    os << "portIdx: " << op.portIdx << " ";
    os << "burstIdx: " << op.burstIdx << " ";
    os << "peIdx: " << op.peIdx << " ";
    os << "addr: " << op.addr << " ";
    os << "length: " << op.length << " ";

    os << "departPeTime: " << op.departPeTime << " ";
    os << "arriveMemTime: " << op.arriveMemTime << " ";
    os << "departMemTime: " << op.departMemTime << " ";
    os << "arrivePeTime: " << op.arrivePeTime << " ";

    return os;
}

BurstOp::BurstOp(
        ramulator::Request::Type _type, 
        int _peIdx, 
        int _portIdx,
        long _burstIdx, 
        long _addr, 
        int _length)
{
    valid = true;
    type = _type;
    portIdx = _portIdx;
    peIdx = _peIdx;
    burstIdx = _burstIdx;
    addr = _addr;
    length = _length;
    departPeTime = 0;
    arriveMemTime = 0;
    departMemTime = 0;
    arrivePeTime = 0;
}

BurstOp::BurstOp(bool _valid){
    valid = _valid;
    type = ramulator::Request::Type::READ;
    portIdx = 0;
    peIdx = 0;
    addr = 0;
    burstIdx = 0;
    length = 0;
    departPeTime = 0;
    arriveMemTime = 0;
    departMemTime = 0;
    arrivePeTime = 0;
}

void BurstOp::operator=(const BurstOp &op){            

    valid = op.valid;
    type = op.type;
    portIdx = op.portIdx;
    burstIdx = op.burstIdx;
    peIdx = op.peIdx;
    addr = op.addr;
    length = op.length;
    reqVec = op.reqVec;
    addrVec = op.addrVec;
    data = op.data;

    departPeTime = op.departPeTime;
    arriveMemTime = op.arriveMemTime;
    departMemTime = op.departMemTime;
    arrivePeTime = op.arrivePeTime;

}

// We may ignore the vector data and it should be fine.
bool BurstOp::operator==(const BurstOp &op) const{

    bool equal = true;;
    equal &= (valid == op.valid);
    equal &= (type == op.type);
    equal &= (portIdx == op.portIdx);
    equal &= (burstIdx == op.burstIdx);
    equal &= (peIdx == op.peIdx);
    equal &= (addr == op.addr);
    equal &= (length == op.length);

    equal &= (departPeTime == op.departPeTime);
    equal &= (arriveMemTime == op.arriveMemTime);
    equal &= (departMemTime == op.departMemTime);
    equal &= (arrivePeTime == op.arrivePeTime);

    return equal;

}

void sc_trace(sc_trace_file *tf, const BurstOp &op, const std::string &name){

    std::ostringstream oss;
    oss << op.type;
    sc_trace(tf, op.valid, name+".valid");
    sc_trace(tf, oss.str().c_str(), name+".type");
    sc_trace(tf, op.portIdx, name+".portIdx");
    sc_trace(tf, op.burstIdx, name+".burstIdx");
    sc_trace(tf, op.peIdx, name+".peIdx");
    sc_trace(tf, op.addr, name+".addr");
    sc_trace(tf, op.length, name+".length");
    sc_trace(tf, op.departPeTime, name+".departPeTime");
    sc_trace(tf, op.arriveMemTime, name+".arriveMemTime");
    sc_trace(tf, op.departMemTime, name+".departMemTime");
    sc_trace(tf, op.arrivePeTime, name+".arrivePeTime");

}

// Convert the burst operations to basic memory requests
// and put the basic requests in a queue.
void BurstOp::convertToReq(std::list<ramulator::Request> &reqQueue){
    int reqNum = getReqNum();
    for(int i = 0; i < reqNum; i++){
        ramulator::Request req;
        req.type = type;
        req.addr = addrVec[i];
        req.udf.burstIdx = burstIdx;
        req.udf.reqIdx = reqVec[i];
        req.udf.peIdx = peIdx;
        req.udf.portIdx = portIdx;
        req.udf.arriveMemTime = arriveMemTime;
        req.udf.departMemTime = departMemTime;
        reqQueue.push_back(req);
    }
}

// Align address to the memory burst
long BurstOp::getAlignedAddr() const {
    long alignedAddr = (addr >> GL::burstAddrWidth) << GL::burstAddrWidth;
    return alignedAddr;
}

// Calculate the offset to the aligned addr;
int BurstOp::getOffset() const {
    long offset = addr - getAlignedAddr(); 
    return (int) offset;
}

int BurstOp::getReqNum() const {

    int reqNum;
    long offset = getOffset();
    if(offset == 0){
        if(length%GL::burstLen == 0){
            reqNum = length/GL::burstLen;
        }
        else{
            reqNum = length/GL::burstLen + 1;
        }
    }
    else{
        reqNum = 1;
        int residueLen = GL::burstLen - offset;
        if(length > residueLen){
            residueLen = length - residueLen;
            if(residueLen%GL::burstLen == 0){
                reqNum = 1 + residueLen/GL::burstLen;
            }
            else{
                reqNum = 1 + residueLen/GL::burstLen + 1;
            }
        }
    }

    return reqNum;

}

void BurstOp::updateReqVec() {

    int reqNum = getReqNum();
    for(int i = 0; i < reqNum; i++){
        reqVec.push_back(GL::getReqIdx());
    }

}

void BurstOp::updateAddrVec() {

    int reqNum = getReqNum();
    int offset = getOffset();
    long reqAddr; 

    if(offset == 0){
        reqAddr = addr;
        for(int i = 0; i < reqNum; i++){
            addrVec.push_back(reqAddr);
            reqAddr += GL::burstLen;
        }
    }
    else{
        reqAddr = getAlignedAddr();
        addrVec.push_back(reqAddr);
        int reqLen = GL::burstLen - offset;
        while(reqLen < length){
            reqAddr += GL::burstLen;
            addrVec.push_back(reqAddr);
            if(reqLen + GL::burstLen <= length){
                reqLen += GL::burstLen;
            }
            else{
                reqLen = length;
            }
        }
    }
}

void BurstOp::ramToReq(const std::vector<char> &ramData){
    for(int i = 0; i < length; i++){
        data.push_back(ramData[addr+i]);
    }
}

void BurstOp::reqToRam(std::vector<char> &ramData){ 
    for(int i = 0; i < length; i++){
        ramData[addr+i] = data[i];
    }
}

