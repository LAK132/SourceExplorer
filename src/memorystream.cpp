#include "memorystream.h"

MemoryStream::MemoryStream(vector<uint8_t>* ptr)
{
    if (ptr == nullptr)
    {
        data = new vector<uint8_t>();
        extdata = false;
    }
    else
    {
        data = ptr;
        extdata = true;
    }
}


MemoryStream::MemoryStream(vector<uint8_t>& mem)
{
    data = new vector<uint8_t>(mem);
    extdata = false;
}

MemoryStream::~MemoryStream()
{
    if (!extdata)
    {
        delete data;
    }
}

vector<uint8_t> MemoryStream::readBytes(size_t size)
{
    auto it = data->begin() + position;
    vector<uint8_t> rtn(it, it+size);
    position += size;
    return rtn;
}

string MemoryStream::readString()
{
    string str;
    char c = 0;
    do
    {
        c = readInt<char>();
        str += c;
    } while (c != 0);
    return str;
}

string MemoryStream::readString(size_t len)
{
    string str;
    for (size_t i = 0; i < len; i++)
    {
        str += readInt<char>();
    }
    return str;
}