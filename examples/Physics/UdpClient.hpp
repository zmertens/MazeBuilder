#ifndef UDP_CLIENT_HPP
#define UDP_CLIENT_HPP

#include <memory>
#include <string>

/// @brief Pair of integers for coordinate data
struct IntegerPair
{
    int first;
    int second;

    IntegerPair(int f = 0, int s = 0) : first(f), second(s)
    {
    }
};

/// @brief UDP client for peer-to-peer maze coordinate sharing
class UdpClient
{
public:

private:
    struct UdpClientImpl;
    std::unique_ptr<UdpClientImpl> m_impl;
};

#endif // UDP_CLIENT_HPP
