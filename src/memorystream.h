#include <istream>
using std::istream;
#include <iostream>
using std::cout;
using std::endl;
#include <stdint.h>
#include <vector>
using std::vector;
#include <string>
using std::string;

#ifndef MEMORYSTREAM_H
#define MEMORYSTREAM_H

class MemoryStream
{
private:
    bool extdata = false;
public:
    vector<uint8_t>* data;
    size_t position;
    MemoryStream(vector<uint8_t>* ptr = nullptr);
    ~MemoryStream();
    vector<uint8_t> readBytes(size_t length);
    string readString();
    string readString(size_t len);
    template<typename T, typename S>
    S readString()
    {
        S str;
        T c = 0;
        do
        {
            c = readInt<T>();
            str += c;
        } while (c != 0);
        return str;
    }
    template<typename T>
    string readString()
    {
        string str;
        T c = 0;
        do
        {
            c = readInt<T>();
            str += (char)c;
        } while (c != 0);
        return str;
    }
    template<typename T, typename S>
    S readString(size_t len)
    {
        S str;
        for (size_t i = 0; i < len; i++)
        {
            str += readInt<T>();
        }
        return str;
    }
    template<typename T>
    string readString(size_t len)
    {
        string str;
        for (size_t i = 0; i < len; i++)
        {
            str += readInt<T>();
        }
        return str;
    }
    template<typename T>
    T readInt()
    {
        // T rtn = *((T*)&((*data)[position]));
        // position += sizeof(T);
        T rtn = 0;
        // Big Endian
        // for(int64_t size = sizeof(T)-1; size >= 0; size--)
        // {
        //     rtn += ((T)(*data)[position++]) << size * 0x8;
        // }
        // Little Endian
        for (int64_t i = 0; i < sizeof(T); i++)
        {
            rtn += ((T)(*data)[position++]) << i * 0x8;
        }
        return rtn;
    }
};

#endif