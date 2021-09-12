#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _resender.bytes_in_flight(); }

void TCPSender::fill_window() {
    bool again = false;
    TCPSegment seg;
    seg.header().seqno = next_seqno();

    if (next_seqno_absolute() == 0) {
        // CLOSED
        // send SYN
        seg.header().syn = true;
        goto send;
    }
    if (next_seqno_absolute() > bytes_in_flight() && !stream_in().eof()) {
        auto window_size = _window_size == 0 ? 1 : _window_size;
        if (_next_seqno >= _last_ackno + window_size) {
            return;
        }
        if (_stream.buffer_empty())
            return;
        // remaining sequence
        auto window = _last_ackno + window_size - _next_seqno;
        // payload length
        uint16_t n = min(TCPConfig::MAX_PAYLOAD_SIZE, window);
        string payload = _stream.read(n);
        if (stream_in().eof() && window - payload.size() > 0) {
            seg.header().fin = true;
        }
        seg.payload() = move(payload);
        if (window > seg.length_in_sequence_space())
            again = true;
        goto send;
    }
    if (stream_in().eof() && next_seqno_absolute() < stream_in().bytes_written() + 2) {
        auto window_size = _window_size == 0 ? 1 : _window_size;
        if (_next_seqno >= _last_ackno + window_size) {
            return;
        }
        seg.header().fin = true;
        goto send;
    }
    if (stream_in().eof() && next_seqno_absolute() == stream_in().bytes_written() + 2) {
        if (bytes_in_flight() > 0) {
            // FIN SENT
        }
        if (bytes_in_flight() == 0) {
            // FIN ACKED
        }
    }
    return;
send:
    _segments_out.push(seg);
    _resender.push(seg, next_seqno_absolute());
    _resender.start_timer();

    _next_seqno += seg.length_in_sequence_space();
    if (again)
        fill_window();
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t ackno_absolute = unwrap(ackno, _isn, _stream.bytes_read());

    if (ackno_absolute < _last_ackno)
        return;
    if (ackno_absolute == _last_ackno) {
        _window_size = window_size;
        return;
    }

    _retransmission_timeout = _initial_retransmission_timeout;
    _consecutive_retransmissions = 0;

    _last_ackno = ackno_absolute;
    _window_size = window_size;

    fill_window();
    _resender.ack_received(ackno_absolute);
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _resender.time_pass(ms_since_last_tick);
    if (_resender.timer() >= _retransmission_timeout) {
        if (!_resender.empty()) {
            _segments_out.push(_resender.front().second);
        } else {
            _resender.stop_timer();
            return;
        }
        if (_window_size) {
            _consecutive_retransmissions += 1;
            _retransmission_timeout *= 2;
        }
        _resender.reset_timer();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
}

void TCPResender::push(const TCPSegment &seg, const uint64_t seqno_absolute) {
    _segments_flying.push_back(make_pair(seqno_absolute, seg));
    _bytes_in_flight += seg.length_in_sequence_space();
}

uint64_t TCPResender::ack_received(const uint64_t ackno_absolute) {
    while (!_segments_flying.empty()) {
        const auto &[seqno_absolute, seg] = _segments_flying.front();
        if (seqno_absolute + seg.length_in_sequence_space() <= ackno_absolute) {
            _bytes_in_flight -= seg.length_in_sequence_space();
            _segments_flying.pop_front();
        } else {
            break;
        }
    }
    if (_segments_flying.empty())
        stop_timer();
    else
        reset_timer();
    return 0;
}

uint64_t TCPResender::bytes_in_flight() const { return _bytes_in_flight; }
