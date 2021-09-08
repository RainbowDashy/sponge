#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), _buffer(), _buffer_size(0), _eof(-1) {}

void StreamReassembler::_try_assemble() {
    auto it = _buffer.begin();
    while (it != _buffer.end()) {
        if (it->first == _output.bytes_written()) {
            _output.write(it->second);
            it = _buffer.erase(it);
        } else {
            break;
        }
    }
    if (_eof == _output.bytes_written())
        _output.end_input();
}

void StreamReassembler::_merge_buffer() {
    if (_buffer.size() <= 1)
        return;
    for (auto it = _buffer.begin(); it != _buffer.end();) {
        auto nit = next(it);
        if (nit == _buffer.end()) {
            break;
        }
        if (it->first + it->second.size() > nit->first) {
            auto overlap = it->first + it->second.size() - nit->first;
            if (nit->second.size() > overlap) {
                it->second = it->second + nit->second.substr(overlap);
            }
            _buffer.erase(nit);
            nit = it;
        }
        it = nit;
    }
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (eof) {
        _eof = index + data.size();
    }
    auto expected_index = _output.bytes_written();
    const auto last_index = index + data.size();
    if (last_index <= expected_index) {
        _try_assemble();
        return;
    }
    if (index > _output.bytes_read() + _capacity) {
        _try_assemble();
        return;
    }
    // l pos in stream
    // sl pos in data string
    // len data length
    size_t l = index;
    size_t sl = 0, len = data.size();
    if (index < expected_index) {
        sl = expected_index - index;
        l = expected_index;
        len -= sl;
    }
    if (last_index > _output.bytes_read() + _capacity) {
        len -= last_index - (_output.bytes_read() + _capacity);
    }
    auto it = _buffer.find(l);
    if (it != _buffer.end()) {
        if (len > it->second.size())
            it->second = data.substr(sl, len);
    } else {
        _buffer.emplace(l, data.substr(sl, len));
    }
    _merge_buffer();
    _try_assemble();
}

size_t StreamReassembler::unassembled_bytes() const {
    size_t res = 0;
    for (auto const &it : _buffer) {
        res += it.second.size();
    }
    return res;
}

bool StreamReassembler::empty() const { return _buffer.empty(); }
