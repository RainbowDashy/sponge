#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : _capacity(capacity), _size(0), _total_read(0), _total_write(0), _write_end(false), _error(false) {}

size_t ByteStream::write(const string &data) {
    auto n = data.size();
    if (n > _capacity - _size) {
        n = _capacity - _size;
    }
    _size += n;
    _total_write += n;
    _buffer.append(BufferList(move(data.substr(0, n))));
    return n;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const { return _buffer.concatenate().substr(0, min(_size, len)); }

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    const auto n = min(len, _size);
    _total_read += n;
    _size -= n;
    _buffer.remove_prefix(n);
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    auto res = peek_output(len);
    pop_output(len);
    return res;
}

void ByteStream::end_input() { _write_end = true; }

bool ByteStream::input_ended() const { return _write_end; }

size_t ByteStream::buffer_size() const { return _size; }

bool ByteStream::buffer_empty() const { return _size == 0; }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _total_write; }

size_t ByteStream::bytes_read() const { return _total_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - _size; }
