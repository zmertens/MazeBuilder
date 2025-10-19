#include "udp_client.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <optional>

// Protocol constants
const std::string udp_client::DISCOVERY_MESSAGE = "MAZE_DISCOVERY";
const std::string udp_client::DISCOVERY_RESPONSE = "MAZE_HOST_HERE";
const sf::Time udp_client::PEER_TIMEOUT = sf::seconds(30.0f);
const sf::Time udp_client::DISCOVERY_INTERVAL = sf::seconds(5.0f);

udp_client::udp_client(Mode mode, unsigned short port, const std::string& host_address)
    : m_mode(mode)
    , m_port(port)
    , m_host_address_str(host_address)
    , m_running(false)
    , m_connected(false)
{
}

udp_client::~udp_client() {
    stop();
}

bool udp_client::initialize() {
    if (m_mode == Mode::HOST) {
        // Bind socket to listen for incoming connections
        if (m_socket.bind(m_port) != sf::Socket::Status::Done) {
            std::cerr << "Error: Failed to bind UDP socket to port " << m_port << std::endl;
            return false;
        }
        std::cout << "UDP Host: Listening on port " << m_port << std::endl;
    } else {
        // Client mode - bind to any available port
        if (m_socket.bind(sf::Socket::AnyPort) != sf::Socket::Status::Done) {
            std::cerr << "Error: Failed to bind UDP socket for client" << std::endl;
            return false;
        }
        std::cout << "UDP Client: Bound to local port " << m_socket.getLocalPort() << std::endl;
    }
    
    m_socket.setBlocking(false);
    return true;
}

void udp_client::start() {
    if (m_running) {
        return;
    }
    
    m_running = true;
    m_network_thread = std::thread(&udp_client::network_thread, this);
    
    if (m_mode == Mode::HOST) {
        m_connected = true;
        std::cout << "UDP Host: Started and ready for connections" << std::endl;
    } else {
        std::cout << "UDP Client: Started, discovering hosts..." << std::endl;
        send_discovery_broadcast();
    }
}

void udp_client::stop() {
    if (!m_running) {
        return;
    }
    
    m_running = false;
    m_connected = false;
    
    if (m_network_thread.joinable()) {
        m_network_thread.join();
    }
    
    m_socket.unbind();
    
    std::lock_guard<std::mutex> lock(m_peers_mutex);
    m_peers.clear();
    
    std::cout << "UDP Client: Stopped" << std::endl;
}

void udp_client::network_thread() {
    while (m_running) {
        handle_incoming_data();
        
        // Client mode: periodically send discovery broadcasts
        if (m_mode == Mode::CLIENT && !m_connected && 
            m_discovery_clock.getElapsedTime() >= DISCOVERY_INTERVAL) {
            send_discovery_broadcast();
            m_discovery_clock.restart();
        }
        
        // Remove timed-out peers
        std::lock_guard<std::mutex> lock(m_peers_mutex);
        m_peers.erase(
            std::remove_if(m_peers.begin(), m_peers.end(),
                [](const Peer& peer) {
                    return peer.last_seen.getElapsedTime() > PEER_TIMEOUT;
                }),
            m_peers.end()
        );
        
        sf::sleep(sf::milliseconds(16)); // ~60 FPS
    }
}

void udp_client::handle_incoming_data() {
    sf::Packet packet;
    std::optional<sf::IpAddress> sender_ip;
    unsigned short sender_port;
    
    // Check for incoming packets
    sf::Socket::Status status = m_socket.receive(packet, sender_ip, sender_port);
    
    if (status == sf::Socket::Status::Done && sender_ip.has_value()) {
        std::string message_type;
        packet >> message_type;
        
        sf::IpAddress actual_sender = sender_ip.value();
        
        if (message_type == DISCOVERY_MESSAGE && m_mode == Mode::HOST) {
            // Host received discovery request from client
            std::cout << "UDP Host: Received discovery from " << actual_sender.toString() << ":" << sender_port << std::endl;
            handle_discovery_request(actual_sender, sender_port);
            
        } else if (message_type == DISCOVERY_RESPONSE && m_mode == Mode::CLIENT) {
            // Client received discovery response from host
            std::cout << "UDP Client: Found host at " << actual_sender.toString() << ":" << sender_port << std::endl;
            add_peer(actual_sender, sender_port);
            m_connected = true;
            
        } else if (message_type == "DATA") {
            // Received coordinate data
            std::uint32_t pair_count;
            packet >> pair_count;
            
            std::vector<IntegerPair> pairs;
            pairs.reserve(pair_count);
            
            for (std::uint32_t i = 0; i < pair_count; ++i) {
                std::int32_t x, y;
                packet >> x >> y;
                pairs.emplace_back(x, y);
            }
            
            std::cout << "UDP: Received " << pairs.size() << " coordinate pairs from " 
                      << actual_sender.toString() << ":" << sender_port << std::endl;
            
            // Update peer activity
            add_peer(actual_sender, sender_port);
            
            // Call data callback if set
            std::lock_guard<std::mutex> lock(m_callback_mutex);
            if (m_data_callback) {
                m_data_callback(pairs, actual_sender);
            }
            
        } else {
            std::cout << "UDP: Received unknown message type: " << message_type 
                      << " from " << actual_sender.toString() << ":" << sender_port << std::endl;
        }
    } else if (status == sf::Socket::Status::Error) {
        std::cerr << "UDP: Error receiving data" << std::endl;
    }
    // Note: sf::Socket::Status::NotReady is expected for non-blocking sockets
}

void udp_client::send_discovery_broadcast() {
    sf::Packet packet;
    packet << DISCOVERY_MESSAGE;
    
    sf::IpAddress target_ip = sf::IpAddress::LocalHost;
    if (m_mode == Mode::CLIENT && !m_host_address_str.empty()) {
        // Client mode: send to specific host
        auto resolved_ip_opt = sf::IpAddress::resolve(m_host_address_str);
        if (resolved_ip_opt.has_value()) {
            target_ip = resolved_ip_opt.value();
        } else {
            std::cerr << "UDP Client: Could not resolve host address: " << m_host_address_str << std::endl;
            return;
        }
    }
    
    if (m_socket.send(packet, target_ip, m_port) != sf::Socket::Status::Done) {
        std::cerr << "UDP Client: Failed to send discovery broadcast to " << target_ip.toString() << ":" << m_port << std::endl;
    } else {
        std::cout << "UDP Client: Sent discovery broadcast to " << target_ip.toString() << ":" << m_port << std::endl;
    }
}

void udp_client::handle_discovery_request(const sf::IpAddress& sender, unsigned short port) {
    sf::Packet response;
    response << DISCOVERY_RESPONSE;
    
    if (m_socket.send(response, sender, port) == sf::Socket::Status::Done) {
        std::cout << "UDP Host: Sent discovery response to " << sender << ":" << port << std::endl;
        add_peer(sender, port);
    }
}

void udp_client::add_peer(const sf::IpAddress& address, unsigned short port) {
    std::lock_guard<std::mutex> lock(m_peers_mutex);
    
    // Check if peer already exists
    auto it = std::find_if(m_peers.begin(), m_peers.end(),
        [&address](const Peer& peer) {
            return peer.address == address;
        });
    
    if (it == m_peers.end()) {
        m_peers.emplace_back(address, port);
        std::cout << "UDP: Added peer " << address << ":" << port 
                  << " (Total peers: " << m_peers.size() << ")" << std::endl;
    } else {
        // Update existing peer info
        it->port = port;
        it->last_seen.restart();
    }
}

void udp_client::remove_peer(const sf::IpAddress& address) {
    std::lock_guard<std::mutex> lock(m_peers_mutex);
    
    auto it = std::remove_if(m_peers.begin(), m_peers.end(),
        [&address](const Peer& peer) {
            return peer.address == address;
        });
    
    if (it != m_peers.end()) {
        std::cout << "UDP: Removed peer " << address << std::endl;
        m_peers.erase(it, m_peers.end());
    }
}

bool udp_client::send_pair(const IntegerPair& pair) {
    return send_pairs({pair});
}

bool udp_client::send_pairs(const std::vector<IntegerPair>& pairs) {
    if (!m_connected || pairs.empty()) {
        return false;
    }
    
    sf::Packet packet;
    packet << std::string("DATA");
    packet << static_cast<std::uint32_t>(pairs.size());
    
    for (const auto& pair : pairs) {
        packet << static_cast<std::int32_t>(pair.first) << static_cast<std::int32_t>(pair.second);
    }
    
    std::lock_guard<std::mutex> lock(m_peers_mutex);
    bool success = true;
    
    for (const auto& peer : m_peers) {
        if (m_socket.send(packet, peer.address, peer.port) != sf::Socket::Status::Done) {
            std::cerr << "UDP: Failed to send data to " << peer.address << ":" << peer.port << std::endl;
            success = false;
        }
    }
    
    if (success && !m_peers.empty()) {
        std::cout << "UDP: Sent " << pairs.size() << " pairs to " << m_peers.size() << " peers" << std::endl;
    }
    
    return success;
}

void udp_client::set_data_callback(DataCallback callback) {
    std::lock_guard<std::mutex> lock(m_callback_mutex);
    m_data_callback = callback;
}

size_t udp_client::get_peer_count() const {
    std::lock_guard<std::mutex> lock(m_peers_mutex);
    return m_peers.size();
}

std::string udp_client::get_status() const {
    std::ostringstream oss;
    
    if (m_mode == Mode::HOST) {
        oss << "HOST on port " << m_port;
    } else {
        oss << "CLIENT targeting " << m_host_address_str << ":" << m_port;
    }
    
    oss << " | Status: " << (m_connected ? "Connected" : "Disconnected");
    oss << " | Peers: " << get_peer_count();
    
    return oss.str();
}