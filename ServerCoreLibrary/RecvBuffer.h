#pragma once

class RecvBuffer
{
    enum { BUFFER_COUNT = 10 };

public:
    RecvBuffer(int32_t bufferSize);
    ~RecvBuffer() = default;

    void            Clean();
    bool            OnRead(int32_t numOfBytes);
    bool            OnWrite(int32_t numOfBytes);

    BYTE* ReadPos() { return &_buffer[_readPos]; }
    BYTE* WritePos() { return &_buffer[_writePos]; }
    int32_t         DataSize() const { return _writePos - _readPos; }
    int32_t         FreeSize() const { return _capacity - _writePos; }

private:
    int32_t         _capacity = 0;
    int32_t         _bufferSize = 0;
    int32_t         _readPos = 0;
    int32_t         _writePos = 0;
    std::vector<BYTE> _buffer;
};

================================================================================
// RecvBuffer.cpp file content
================================================================================

#include "pch.h"
#include "RecvBuffer.h"

RecvBuffer::RecvBuffer(int32_t bufferSize) : _bufferSize(bufferSize)
{
    _capacity = bufferSize * BUFFER_COUNT;
    _buffer.resize(_capacity);
}

void RecvBuffer::Clean()
{
    int32_t dataSize = DataSize();
    if (dataSize == 0)
    {
        _readPos = _writePos = 0;
    }
    else
    {
        if (FreeSize() < _bufferSize)
        {
            ::memcpy(&_buffer[0], &_buffer[_readPos], dataSize);
            _readPos = 0;
            _writePos = dataSize;
        }
    }
}

bool RecvBuffer::OnRead(int32_t numOfBytes)
{
    if (numOfBytes > DataSize())
        return false;

    _readPos += numOfBytes;
    return true;
}

bool RecvBuffer::OnWrite(int32_t numOfBytes)
{
    if (numOfBytes > FreeSize())
        return false;

    _writePos += numOfBytes;
    return true;
}