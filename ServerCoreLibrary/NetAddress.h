#pragma once

class NetAddress {
public:
    // Constructors
    NetAddress() = default;
    NetAddress(const std::string& ip, uint16_t port);
    NetAddress(const asio::ip::tcp::endpoint& endpoint);

    // Getters
    const asio::ip::tcp::endpoint& GetEndpoint() const { return _endpoint; }
    std::string GetIPAddress() const;
    uint16_t GetPort() const;

    // Utility methods
    static NetAddress FromEndpoint(const asio::ip::tcp::endpoint& endpoint);
    static NetAddress Any(uint16_t port);

    // Operators
    bool operator==(const NetAddress& other) const;
    bool operator!=(const NetAddress& other) const;

private:
    asio::ip::tcp::endpoint _endpoint;
};

================================================================================
// NetAddress.cpp 파일 내용
================================================================================

#include "pch.h"
#include "NetAddress.h"

NetAddress::NetAddress(const std::string& ip, uint16_t port)
    : _endpoint(asio::ip::make_address(ip), port)
{
}

NetAddress::NetAddress(const asio::ip::tcp::endpoint& endpoint)
    : _endpoint(endpoint)
{
}

std::string NetAddress::GetIPAddress() const
{
    return _endpoint.address().to_string();
}

uint16_t NetAddress::GetPort() const
{
    return _endpoint.port();
}

NetAddress NetAddress::FromEndpoint(const asio::ip::tcp::endpoint& endpoint)
{
    return NetAddress(endpoint);
}

NetAddress NetAddress::Any(uint16_t port)
{
    return NetAddress(asio::ip::tcp::endpoint(
        asio::ip::tcp::v4(),
        port
    ));
}

bool NetAddress::operator==(const NetAddress& other) const
{
    return _endpoint == other._endpoint;
}

bool NetAddress::operator!=(const NetAddress& other) const
{
    return !(*this == other);
}