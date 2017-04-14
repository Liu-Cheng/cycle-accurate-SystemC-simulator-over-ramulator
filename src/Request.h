#ifndef __REQUEST_H
#define __REQUEST_H

#include <vector>
#include <functional>
#include <iostream>

using namespace std;

namespace ramulator
{
    //-----------------------------
    // User defined member
    // ----------------------------

    struct UDF{
        long burstIdx;
        long reqIdx; 
        int peIdx;

        long departPeTime; 
        long arriveMemTime;
        long departMemTime;
        long arrivePeTime;
    };

    class Request
    {
        public:
            bool is_first_command;
            long addr;

            // long addr_row;
            // It specifies the id of the memory 
            // hierarchy such as channel id, rank id, 
            // bank id, row id, column id
            vector<int> addr_vec;

            // specify which core this request sent from, for virtual address translation
            int coreid;

            UDF udf;

            enum class Type
            {
                READ,
                WRITE,
                REFRESH,
                POWERDOWN,
                SELFREFRESH,
                EXTENSION,
                MAX
            } type;

            long arrive = -1;
            long depart;
            function<void(Request&)> callback; // call back with more info

            Request(long addr, Type type, int coreid = 0)
                : is_first_command(true), addr(addr), coreid(coreid), type(type),
                callback([](Request& req){}) {
                }

            Request(long addr, Type type, function<void(Request&)> callback, int coreid = 0)
                : is_first_command(true), addr(addr), coreid(coreid), type(type), callback(callback) { 
                }

            Request(vector<int>& addr_vec, Type type, function<void(Request&)> callback, int coreid = 0)
                : is_first_command(true), addr_vec(addr_vec), coreid(coreid), type(type), callback(callback) {
                }

            Request()
                : is_first_command(true), coreid(0) {
                }
    };


} /*namespace ramulator*/

#endif /*__REQUEST_H*/

