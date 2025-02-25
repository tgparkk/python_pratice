#pragma once
#include <asio.hpp>
#include "RecvBuffer.h"
#include "SendBuffer.h"
#include "NetAddress.h"
#include "AsioEvent.h"

using asio::ip::tcp;

class RecvBuffer;
class Service;
class SendBuffer;
using SendBufferRef = std::shared_ptr<SendBuffer>;
class AsioEvent;

class Session : public std::enable_shared_from_this<Session>
{
    friend class Service;
    friend class ServerService;

    enum
    {
        BUFFER_SIZE = 0x10000, // 64KB
    };

public:
    Session(asio::io_context& ioc);
    virtual ~Session();

    /* External Interface */
    void                Start();
    void                Send(std::shared_ptr<SendBuffer> sendBuffer);
    bool                Connect();
    void                Disconnect(const char* cause);

    void                SetService(std::shared_ptr<Service> service) { _service = service; }
    std::shared_ptr<Service> GetService() { return _service.lock(); }

    /* Info */
    void                SetNetAddress(NetAddress address) { _netAddress = address; }
    NetAddress          GetAddress() { return _netAddress; }
    asio::ip::tcp::socket& GetSocket() { return _socket; }
    bool                IsConnected() { return _connected; }
    std::shared_ptr<Session> GetSessionRef() { return std::static_pointer_cast<Session>(shared_from_this()); }

private:
    void Dispatch(EventType type, size_t bytes);

protected:
    /* ������ �ڵ忡�� ������ */
    virtual void        OnConnected() {}
    virtual int32_t     OnRecv(BYTE* buffer, int32_t len) { return len; }
    virtual void        OnSend(int32_t len) {}
    virtual void        OnDisconnected() {}

private:
    /* Network Core */
    //void                RegisterConnect();
    //void                RegisterDisconnect();
    void                RegisterRecv();
    void                RegisterSend();

    void                ProcessConnect();
    void                ProcessDisconnect();
    void                ProcessRecv(size_t bytesTransferred);
    void                ProcessSend(size_t bytesTransferred);

    void                HandleError(const std::error_code& error);

private:
    asio::ip::tcp::socket      _socket;
    NetAddress                 _netAddress;
    std::atomic<bool>          _connected = false;

    std::weak_ptr<Service>     _service;
    RecvBuffer                 _recvBuffer;

    std::mutex                 _sendLock;
    std::queue<std::shared_ptr<SendBuffer>> _sendQueue;
    std::atomic<bool>          _sendRegistered = false;
};

/*-----------------
    PacketSession
------------------*/
struct PacketHeader
{
    uint16_t size;
    uint16_t id;
};

class PacketSession : public Session
{
public:
    PacketSession(asio::io_context& ioc) : Session(ioc) {}
    virtual ~PacketSession() {}

    std::shared_ptr<PacketSession> GetPacketSessionRef()
    {
        return std::static_pointer_cast<PacketSession>(shared_from_this());
    }

protected:
    virtual int32_t OnRecv(BYTE* buffer, int32_t len) sealed;
    virtual void OnRecvPacket(BYTE* buffer, int32_t len) abstract;
};

================================================================================
// Session.cpp file content
================================================================================

#include "pch.h"
#include "Session.h"
#include "Service.h"
#include "SocketUtils.h"
#include <iostream>

Session::Session(asio::io_context& ioc)
    : _socket(ioc)
    , _recvBuffer(BUFFER_SIZE)
{
}

Session::~Session()
{
    Disconnect("Destructor");
}

void Session::Start()
{
    RegisterRecv();
}

void Session::Send(std::shared_ptr<SendBuffer> sendBuffer)
{
    if (!IsConnected())
        return;

    bool registerSend = false;
    {
        std::lock_guard<std::mutex> lock(_sendLock);
        _sendQueue.push(sendBuffer);
        // ���� ���� ���� send �۾��� ���� ���� ���ο� send �۾��� ���
        if (_sendRegistered.exchange(true) == false)
            registerSend = true;
    }

    if (registerSend)
        RegisterSend();
}

bool Session::Connect()
{
    if (IsConnected())
        return false;

    if (auto service = GetService())
    {
        const NetAddress& address = service->GetNetAddress();
        _socket.async_connect(
            address.GetEndpoint(),
            [this](const std::error_code& error)
            {
                if (!error)
                {
                    ProcessConnect();
                }
                else
                {
                    HandleError(error);
                }
            });
        return true;
    }
    return false;
}

void Session::Disconnect(const char* cause)
{
    if (_connected.exchange(false) == false)
        return;

    std::error_code ec;
    _socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    _socket.close(ec);

    OnDisconnected();
    if (auto service = GetService())
        service->ReleaseSession(GetSessionRef());
}

void Session::Dispatch(EventType type, size_t bytes)
{
    switch (type) {
    case EventType::Recv:
        ProcessRecv(bytes);
        break;
    case EventType::Send:
        ProcessSend(bytes);
        break;
    }
}

void Session::RegisterRecv()
{
    if (!IsConnected())
        return;

    BYTE* buffer = _recvBuffer.WritePos();
    int32_t len = _recvBuffer.FreeSize();

    GetSocket().async_read_some(
        asio::buffer(buffer, len),
        [this](const std::error_code& error, size_t bytesTransferred)
        {
            if (!error)
            {
                if (_recvBuffer.OnWrite(bytesTransferred))
                {
                    int32_t dataSize = _recvBuffer.DataSize();
                    int32_t processLen = OnRecv(_recvBuffer.ReadPos(), dataSize);
                    if (processLen < 0 || dataSize < processLen || !_recvBuffer.OnRead(processLen))
                    {
                        Disconnect("Read Overflow");
                        return;
                    }

                    _recvBuffer.Clean();
                    RegisterRecv();  // ���� ���� ���
                }
            }
            else
            {
                Disconnect("RegisterRecv Error");
            }
        });
}

void Session::RegisterSend()
{
    if (!IsConnected())
        return;

    // ���� ������ �غ�
    std::vector<asio::const_buffer> sendBuffers;
    std::vector<std::shared_ptr<SendBuffer>> pendingBuffers;
    {
        std::lock_guard<std::mutex> lock(_sendLock);
        while (!_sendQueue.empty()) {
            auto buffer = _sendQueue.front();
            _sendQueue.pop();

            pendingBuffers.push_back(buffer);  // ���� ������ ���� ����
            sendBuffers.push_back(
                asio::buffer(buffer->Buffer(), buffer->WriteSize())
            );
        }
    }

    // ���� �����Ͱ� ���ٸ� ����
    if (sendBuffers.empty()) {
        _sendRegistered.store(false);
        return;
    }

    // �񵿱� ���� �۾� ���
    auto self = shared_from_this();  // ���� ����
    _socket.async_write_some(
        sendBuffers,
        [this, self, pendingBuffers](const std::error_code& error, size_t bytesTransferred) {
            if (!error) {
                Dispatch(EventType::Send, bytesTransferred);
            }
            else {
                HandleError(error);
            }
        }
    );
}

void Session::ProcessConnect()
{
    _connected.store(true);

    // ���� ���
    GetService()->AddSession(GetSessionRef());

    // ������ �ڵ忡�� ������
    OnConnected();

    // ���� ���
    RegisterRecv();
}

void Session::ProcessDisconnect()
{


    OnDisconnected(); // ������ �ڵ忡�� ������
    GetService()->ReleaseSession(GetSessionRef());
}

void Session::ProcessRecv(size_t bytesTransferred)
{
}

void Session::ProcessSend(size_t bytesTransferred)
{
    if (!IsConnected())
        return;

    if (bytesTransferred == 0) {
        Disconnect("Send bytesTransferred is 0");
        return;
    }

    // ������ �ڵ忡�� ������
    OnSend(bytesTransferred);

    std::lock_guard<std::mutex> lock(_sendLock);
    if (_sendQueue.empty())
        _sendRegistered.store(false);
    else
        RegisterSend();
}

void Session::HandleError(const std::error_code& error)
{
    if (error == asio::error::operation_aborted ||
        error == asio::error::connection_reset ||
        error == asio::error::connection_aborted)
    {
        Disconnect("Error");
    }
    else
    {
        // Log error
    }
}

/* PacketSession Implementation */
int32_t PacketSession::OnRecv(BYTE* buffer, int32_t len)
{
    int32_t processLen = 0;

    while (true)
    {
        int32_t dataSize = len - processLen;
        if (dataSize < sizeof(PacketHeader))
            break;

        PacketHeader* header = reinterpret_cast<PacketHeader*>(&buffer[processLen]);
        if (dataSize < header->size)
            break;

        OnRecvPacket(&buffer[processLen], header->size);
        processLen += header->size;
    }

    return processLen;
}