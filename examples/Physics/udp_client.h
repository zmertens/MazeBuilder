#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <SFML/Network.hpp>

/// @brief Pair of integers for coordinate data
struct IntegerPair {
    int first;
    int second;
    
    IntegerPair(int f = 0, int s = 0) : first(f), second(s) {}
};

/// @brief UDP client for peer-to-peer maze coordinate sharing
class udp_client {
public:
    /// @brief Connection modes
    enum class Mode {
        HOST,   // Acts as host/server
        CLIENT  // Connects to existing host
    };

    /// @brief Callback function for received data
    using DataCallback = std::function<void(const std::vector<IntegerPair>&, const sf::IpAddress&)>;

    /// @brief Constructor
    /// @param mode Connection mode (HOST or CLIENT)
    /// @param port Port to listen on (HOST) or connect to (CLIENT)
    /// @param host_address Host address (CLIENT mode only)
    explicit udp_client(Mode mode, unsigned short port = 5555, const std::string& host_address = "127.0.0.1");

    /// @brief Destructor
    ~udp_client();

    /// @brief Initialize the UDP client
    /// @return True if successful
    bool initialize();

    /// @brief Start the client (begins listening for incoming data)
    void start();

    /// @brief Stop the client
    void stop();

    /// @brief Send a pair of integers to connected peers
    /// @param pair The integer pair to send
    /// @return True if successful
    bool send_pair(const IntegerPair& pair);

    /// @brief Send multiple pairs of integers to connected peers
    /// @param pairs Vector of integer pairs to send
    /// @return True if successful
    bool send_pairs(const std::vector<IntegerPair>& pairs);

    /// @brief Set callback for received data
    /// @param callback Function to call when data is received
    void set_data_callback(DataCallback callback);

    /// @brief Get connection status
    /// @return True if connected/listening
    bool is_connected() const { return m_connected; }

    /// @brief Get current mode
    /// @return Current connection mode
    Mode get_mode() const { return m_mode; }

    /// @brief Get connected peer count (HOST mode only)
    /// @return Number of connected peers
    size_t get_peer_count() const;

    /// @brief Get lobby status information
    /// @return Status string
    std::string get_status() const;

private:
    /// @brief Network thread function
    void network_thread();

    /// @brief Handle incoming packets
    void handle_incoming_data();

    /// @brief Send broadcast to discover hosts (CLIENT mode)
    void send_discovery_broadcast();

    /// @brief Handle discovery requests (HOST mode)
    void handle_discovery_request(const sf::IpAddress& sender, unsigned short port);

    /// @brief Add a peer to the connected peers list
    void add_peer(const sf::IpAddress& address, unsigned short port);

    /// @brief Remove a peer from the connected peers list
    void remove_peer(const sf::IpAddress& address);

    /// @brief Peer information
    struct Peer {
        sf::IpAddress address;
        unsigned short port;
        sf::Clock last_seen;
        
        Peer(const sf::IpAddress& addr, unsigned short p) 
            : address(addr), port(p) {}
    };

private:
    Mode m_mode;
    unsigned short m_port;
    std::string m_host_address_str;
    sf::UdpSocket m_socket;
    
    std::atomic<bool> m_running;
    std::atomic<bool> m_connected;
    std::thread m_network_thread;
    
    std::vector<Peer> m_peers;
    mutable std::mutex m_peers_mutex;
    
    DataCallback m_data_callback;
    mutable std::mutex m_callback_mutex;
    
    // Protocol constants
    static const std::string DISCOVERY_MESSAGE;
    static const std::string DISCOVERY_RESPONSE;
    static const sf::Time PEER_TIMEOUT;
    static const sf::Time DISCOVERY_INTERVAL;
    
    sf::Clock m_discovery_clock;
};

#endif // UDP_CLIENT_H