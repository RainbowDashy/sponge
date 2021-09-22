#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    if (_map.count(next_hop_ip)) {
        auto dst = _map[next_hop_ip];
        EthernetFrame frame;
        frame.header().type = EthernetHeader::TYPE_IPv4;
        frame.header().src = _ethernet_address;
        frame.header().dst = dst;
        frame.payload() = dgram.serialize();
        _frames_out.push(frame);
    } else {
        auto it = _arp_req.find(next_hop_ip);
        if (it != _arp_req.end() && _clk - it->second <= 5000) {
            return;
        }

        _arp_req[next_hop_ip] = _clk;

        EthernetFrame frame;
        frame.header().type = EthernetHeader::TYPE_ARP;
        frame.header().src = _ethernet_address;
        frame.header().dst = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
        ARPMessage arp;
        arp.opcode = ARPMessage::OPCODE_REQUEST;
        arp.sender_ethernet_address = _ethernet_address;
        arp.sender_ip_address = _ip_address.ipv4_numeric();
        arp.target_ip_address = next_hop_ip;
        frame.payload() = arp.serialize();
        _frames_out.push(frame);
        _frames_wait[next_hop_ip].push(make_pair(dgram, next_hop));
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    if (frame.header().dst != _ethernet_address &&
        frame.header().dst != EthernetAddress{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}) {
        return {};
    }
    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram dgram;
        if (dgram.parse(frame.payload()) == ParseResult::NoError) {
            return dgram;
        }
    } else if (frame.header().type == EthernetHeader::TYPE_ARP) {
        ARPMessage arp;
        if (arp.parse(frame.payload()) == ParseResult::NoError) {
            if (!_map.count(arp.sender_ip_address)) {
                // new mapping
                _map[arp.sender_ip_address] = arp.sender_ethernet_address;
                _map_clk[arp.sender_ip_address] = _clk;
                auto &q = _frames_wait[arp.sender_ip_address];
                while (!q.empty()) {
                    auto p = q.front();
                    send_datagram(p.first, p.second);
                    q.pop();
                }
            }
            if (arp.opcode == ARPMessage::OPCODE_REQUEST && arp.target_ip_address == _ip_address.ipv4_numeric()) {
                EthernetFrame reply_frame;
                ARPMessage reply_arp;
                reply_arp.opcode = ARPMessage::OPCODE_REPLY;
                reply_arp.sender_ethernet_address = _ethernet_address;
                reply_arp.sender_ip_address = _ip_address.ipv4_numeric();
                reply_arp.target_ethernet_address = arp.sender_ethernet_address;
                reply_arp.target_ip_address = arp.sender_ip_address;
                reply_frame.header().type = EthernetHeader::TYPE_ARP;
                reply_frame.header().src = _ethernet_address;
                reply_frame.header().dst = frame.header().src;
                reply_frame.payload() = reply_arp.serialize();
                _frames_out.push(reply_frame);
            }
        }
    }
    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    _clk += ms_since_last_tick;
    for (auto it = _map_clk.begin(); it != _map_clk.end();) {
        if (_clk - it->second > 30000) {
            _map.erase(it->first);
            it = _map_clk.erase(it);
        } else {
            ++it;
        }
    }
}
