#include "tcp_connection.hh"

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    _time_since_last_segment_received = 0;
    if (seg.header().rst) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        // kill the connection
        return;
    }


    _receiver.segment_received(seg);

    if (seg.length_in_sequence_space() > 0) {
        if ((_sender.next_seqno_absolute() == 0) || (_sender.stream_in().eof() && _sender.next_seqno_absolute() < _sender.stream_in().bytes_written() + 2)) {
            // SYN or FIN
            _sender.fill_window();
        } else {
            _sender.send_empty_segment();
        }
    }
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }

    if (inbound_stream().eof() && !_sender.stream_in().eof()) {
        _linger_after_streams_finish = false;
    }

    if (!_linger_after_streams_finish && _receiver.stream_out().eof() && _sender.stream_in().eof() &&
        _sender.fin_acked()) {
            // end_input_stream();
    }

    _send_all();
}

void TCPConnection::_send_all() {
    while (!_sender.segments_out().empty()) {
        auto seg = move(_sender.segments_out().front());
        _sender.segments_out().pop();

        auto ackno = _receiver.ackno();
        if (ackno.has_value()) {
            seg.header().ackno = ackno.value();
            seg.header().ack = true;
        }
        seg.header().win = min(static_cast<size_t>(0xffff), _receiver.window_size());
        segments_out().push(move(seg));
    }
}

void TCPConnection::_send_reset() {
    _sender.send_empty_segment(true);
    _send_all();
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
}

bool TCPConnection::active() const {
    if (_receiver.stream_out().error() && _sender.stream_in().error())
        return false;
    if (!_linger_after_streams_finish && _receiver.stream_out().eof() && _sender.stream_in().eof() &&
        _sender.fin_acked())
        return false;

    return true;
}

size_t TCPConnection::write(const string &data) {
    auto n = _sender.stream_in().write(data);
    _sender.fill_window();
    _send_all();
    if (inbound_stream().eof() && !_sender.stream_in().eof()) {
        _linger_after_streams_finish = false;
    }
    return n;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _time_since_last_segment_received += ms_since_last_tick;

    if (_sender.consecutive_retransmissions() >= _cfg.MAX_RETX_ATTEMPTS) {
        _send_reset();
        return;
    }
    if (inbound_stream().eof() && _sender.stream_in().eof() && _sender.fin_acked() &&
        time_since_last_segment_received() >= 10 * _cfg.rt_timeout) {
        _linger_after_streams_finish = false;
        end_input_stream();
        return;
    }
    _sender.tick(ms_since_last_tick);
    _send_all();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    _send_all();
}

void TCPConnection::connect() {
    _sender.fill_window();
    _send_all();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            _send_reset();
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
