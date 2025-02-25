#pragma once
#include "NetAddress.h"
#include "CorePch.h"

class NetAddress;
class Session;
using SessionRef = std::shared_ptr<Session>;
//using SessionFactory = std::function<SessionRef(asio::io_context&)>;
using SessionFactory = std::function<SessionRef(asio::io_context&)>;

enum class ServiceType : uint8_t
{
    Server,
    Client
};

/*-------------
    Service
--------------*/
class Service : public std::enable_shared_from_this<Service>
{
public:
    Service(ServiceType type, asio::io_context& ioc, const NetAddress& address,
        SessionFactory factory, int32_t maxSessionCount = 1);
    virtual ~Service();

    virtual bool Start() = 0;
    bool CanStart() const { return _sessionFactory != nullptr; }
    virtual void CloseService();

    void SetSessionFactory(SessionFactory factory) { _sessionFactory = factory; }

    void Broadcast(std::shared_ptr<class SendBuffer> sendBuffer);
    SessionRef CreateSession();
    void AddSession(SessionRef session);
    void ReleaseSession(SessionRef session);

    ServiceType GetServiceType() const { return _type; }
    const NetAddress& GetNetAddress() const { return _netAddress; }
    int32_t GetCurrentSessionCount() const { return _sessionCount; }
    int32_t GetMaxSessionCount() const { return _maxSessionCount; }
    asio::io_context& GetIOContext() { return _ioc; }

protected:
    asio::io_context& _ioc;
    ServiceType _type;
    NetAddress _netAddress;
    int32_t _maxSessionCount;
    int32_t _sessionCount = 0;
    SessionFactory _sessionFactory;
    std::recursive_mutex _lock;
    std::set<SessionRef> _sessions;
};

/*-----------------
    ClientService
------------------*/
class ClientService : public Service
{
public:
    ClientService(asio::io_context& ioc, const NetAddress& targetAddress,
        SessionFactory factory, int32_t maxSessionCount = 1);
    virtual ~ClientService() = default;

    virtual bool Start() override;
};

/*-----------------
    ServerService
------------------*/

class ServerService : public Service
{
public:
    ServerService(asio::io_context& ioc, const NetAddress& address,
        SessionFactory factory, int32_t maxSessionCount = 1);
    virtual ~ServerService();

    virtual bool Start() override;
    virtual void CloseService() override;

private:
    void StartAccept();
    std::unique_ptr<asio::ip::tcp::acceptor> _acceptor;
};


================================================================================
// Service.cpp file content
================================================================================

#include "pch.h"
#include "Service.h"
#include "Session.h"
#include "Listener.h"

#include "ThreadManager.h"

Service::Service(ServiceType type, asio::io_context& ioc, const NetAddress& address,
    SessionFactory factory, int32_t maxSessionCount)
    : _ioc(ioc)
    , _type(type)
    , _netAddress(address)
    , _sessionFactory(factory)
    , _maxSessionCount(maxSessionCount)
{
}

Service::~Service()
{
    CloseService();
}

void Service::CloseService()
{
    std::unique_lock<std::recursive_mutex> lock(_lock);
    for (const auto& session : _sessions)
        //session->Disconnect("Service Close");

    _sessions.clear();
}

void Service::Broadcast(std::shared_ptr<SendBuffer> sendBuffer)
{
    std::unique_lock<std::recursive_mutex> lock(_lock);
    for (const auto& session : _sessions)
        session->Send(sendBuffer);
}

SessionRef Service::CreateSession()
{
    SessionRef session = _sessionFactory(_ioc);
    session->SetService(shared_from_this());
    return session;
}

void Service::AddSession(SessionRef session)
{
    std::unique_lock<std::recursive_mutex> lock(_lock);
    _sessions.insert(session);
    _sessionCount++;
}

void Service::ReleaseSession(SessionRef session)
{
    std::unique_lock<std::recursive_mutex> lock(_lock);
    _sessions.erase(session);
    _sessionCount--;
}

/*-----------------
    ClientService
------------------*/
ClientService::ClientService(asio::io_context& ioc, const NetAddress& targetAddress,
    SessionFactory factory, int32_t maxSessionCount)
    : Service(ServiceType::Client, ioc, targetAddress, factory, maxSessionCount)
{
}

bool ClientService::Start()
{
    if (!CanStart())
        return false;

    for (int32_t i = 0; i < GetMaxSessionCount(); i++)
    {
        SessionRef session = CreateSession();
        if (!session->Connect())
            return false;
    }

    return true;
}

/*-----------------
    ServerService
------------------*/
ServerService::ServerService(asio::io_context& ioc, const NetAddress& address,
    SessionFactory factory, int32_t maxSessionCount)
    : Service(ServiceType::Server, ioc, address, factory, maxSessionCount)
{
}

ServerService::~ServerService()
{
    CloseService();
}

bool ServerService::Start()
{
    if (!CanStart())
        return false;

    std::error_code ec;
    auto endpoint = _netAddress.GetEndpoint();

    _acceptor = std::make_unique<asio::ip::tcp::acceptor>(_ioc, endpoint);
    if (_acceptor->is_open() == false)
        return false;

    StartAccept();
    return true;
}

void ServerService::CloseService()
{
    if (_acceptor)
        _acceptor->close();

    Service::CloseService();
}

void ServerService::StartAccept()
{
    // 최대 세션 수 체크
    if (GetCurrentSessionCount() >= GetMaxSessionCount())
    {
        std::cout << "Max sessions reached!" << std::endl;
        return;
    }

    SessionRef session = CreateSession();
    _acceptor->async_accept(
        session->GetSocket(),
        [this, session](const std::error_code& error)
        {
            if (!error)
            {
                if (GetCurrentSessionCount() < GetMaxSessionCount())
                {
                    session->ProcessConnect();
                    AddSession(session);
                }
                else
                {
                    // 세션 수가 초과되면 연결 거부
                    session->Disconnect("Max sessions");
                }
            }

            StartAccept(); // 다음 연결 대기
        }
    );
}