#pragma once
class SendBufferChunk;

/*----------------
    SendBuffer
-----------------*/
class SendBuffer
{
public:
    SendBuffer(std::shared_ptr<SendBufferChunk> owner, BYTE* buffer, uint32_t allocSize);
    ~SendBuffer() = default;

    BYTE* Buffer() { return _buffer; }
    uint32_t        AllocSize() const { return _allocSize; }
    uint32_t        WriteSize() const { return _writeSize; }
    void            Close(uint32_t writeSize);

private:
    BYTE* _buffer;
    uint32_t        _allocSize = 0;
    uint32_t        _writeSize = 0;
    std::shared_ptr<SendBufferChunk> _owner;
};

/*--------------------
    SendBufferChunk
--------------------*/
class SendBufferChunk : public std::enable_shared_from_this<SendBufferChunk>
{
    enum { SEND_BUFFER_CHUNK_SIZE = 6000 };

public:
    SendBufferChunk();
    ~SendBufferChunk() = default;

    void                        Reset();
    std::shared_ptr<SendBuffer> Open(uint32_t allocSize);
    void                        Close(uint32_t writeSize);

    bool                        IsOpen() const { return _open; }
    BYTE* Buffer() { return &_buffer[_usedSize]; }
    uint32_t                    FreeSize() const { return static_cast<uint32_t>(_buffer.size()) - _usedSize; }

private:
    std::vector<BYTE>          _buffer;
    bool                       _open = false;
    uint32_t                   _usedSize = 0;
};

/*---------------------
    SendBufferManager
----------------------*/
class SendBufferManager
{
public:
    std::shared_ptr<SendBuffer> Open(uint32_t size);

private:
    std::shared_ptr<SendBufferChunk> Pop();
    void                        Push(std::shared_ptr<SendBufferChunk> buffer);

    static void                 PushGlobal(SendBufferChunk* buffer);

private:
    std::mutex                  _lock;
    std::vector<std::shared_ptr<SendBufferChunk>> _sendBufferChunks;
};

================================================================================
// SendBuffer.cpp file content
================================================================================

#include "pch.h"
#include "SendBuffer.h"

SendBuffer::SendBuffer(std::shared_ptr<SendBufferChunk> owner, BYTE* buffer, uint32_t allocSize)
    : _owner(owner), _buffer(buffer), _allocSize(allocSize)
{
}

void SendBuffer::Close(uint32_t writeSize)
{
    assert(_allocSize >= writeSize);
    _writeSize = writeSize;
    _owner->Close(writeSize);
}

/*--------------------
    SendBufferChunk
--------------------*/
SendBufferChunk::SendBufferChunk()
{
    _buffer.resize(SEND_BUFFER_CHUNK_SIZE);
}

void SendBufferChunk::Reset()
{
    _open = false;
    _usedSize = 0;
}

std::shared_ptr<SendBuffer> SendBufferChunk::Open(uint32_t allocSize)
{
    assert(allocSize <= SEND_BUFFER_CHUNK_SIZE);
    assert(_open == false);

    if (allocSize > FreeSize())
        return nullptr;

    _open = true;
    return std::make_shared<SendBuffer>(shared_from_this(), Buffer(), allocSize);
}

void SendBufferChunk::Close(uint32_t writeSize)
{
    assert(_open == true);
    _open = false;
    _usedSize += writeSize;
}

/*---------------------
    SendBufferManager
----------------------*/
thread_local std::shared_ptr<SendBufferChunk> LSendBufferChunk;

std::shared_ptr<SendBuffer> SendBufferManager::Open(uint32_t size)
{
    if (LSendBufferChunk == nullptr)
    {
        LSendBufferChunk = Pop();
        LSendBufferChunk->Reset();
    }

    assert(LSendBufferChunk->IsOpen() == false);

    if (LSendBufferChunk->FreeSize() < size)
    {
        LSendBufferChunk = Pop();
        LSendBufferChunk->Reset();
    }

    return LSendBufferChunk->Open(size);
}

std::shared_ptr<SendBufferChunk> SendBufferManager::Pop()
{
    std::lock_guard<std::mutex> lock(_lock);

    if (!_sendBufferChunks.empty())
    {
        std::shared_ptr<SendBufferChunk> sendBufferChunk = _sendBufferChunks.back();
        _sendBufferChunks.pop_back();
        return sendBufferChunk;
    }

    return std::make_shared<SendBufferChunk>();
}

void SendBufferManager::Push(std::shared_ptr<SendBufferChunk> buffer)
{
    std::lock_guard<std::mutex> lock(_lock);
    _sendBufferChunks.push_back(buffer);
}

void SendBufferManager::PushGlobal(SendBufferChunk* buffer)
{
    GSendBufferManager->Push(std::shared_ptr<SendBufferChunk>(buffer, PushGlobal));
}