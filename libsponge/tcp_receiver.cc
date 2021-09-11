#include "tcp_receiver.hh"

#include <iostream>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    bool first = false;
    if (!_isn.has_value() && seg.header().syn) {
        _isn = seg.header().seqno;
        first = true;
    }
    if (!_isn.has_value())
        return;
    uint64_t index = unwrap(seg.header().seqno + first, _isn.value(), _reassembler.stream_out().bytes_written());
    _reassembler.push_substring(seg.payload().copy(), index - 1, seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_isn.has_value()) {
        auto n = _checkpoint() + 1;
        if (_reassembler.stream_out().input_ended())
            n += 1;
        return wrap(n, _isn.value());
    }
    return {};
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
