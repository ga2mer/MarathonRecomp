#pragma once

using ring_size_t = uint32_t;
class RingBuffer {
public:
    RingBuffer(uint8_t* buffer, size_t capacity)
    : buffer_(buffer), capacity_(static_cast<ring_size_t>(capacity)) {}

    uint8_t* Buffer() const { return buffer_; }
    ring_size_t Capacity() const { return capacity_; }
    bool Empty() const { return readOffset_ == writeOffset_; }

    ring_size_t ReadOffset() const { return readOffset_; }
    uintptr_t ReadPtr() const {
        return uintptr_t(buffer_) + static_cast<uintptr_t>(readOffset_);
    }

    // TODO: offset/capacity_ is probably always 1 when its over, just check and subtract instead
    void SetReadOffset(size_t offset) { readOffset_ = offset % capacity_; }

    ring_size_t WriteOffset() const { return writeOffset_; }

    uintptr_t WritePtr() const { return uintptr_t(buffer_) + writeOffset_; }

    void SetWriteOffset(size_t offset) {
        writeOffset_ = static_cast<ring_size_t>(offset) % capacity_;
    }

    ring_size_t WriteCount() const {
        if (readOffset_ == writeOffset_) {
            return capacity_;
        } else if (writeOffset_ < readOffset_) {
            return readOffset_ - writeOffset_;
        } else {
            return (capacity_ - writeOffset_) + readOffset_;
        }
    }

    void AdvanceRead(size_t count);
    void AdvanceWrite(size_t count);

    struct ReadRange {
        const uint8_t* first;

        const uint8_t* second;
        ring_size_t firstLength;
        ring_size_t secondLength;
    };

    ReadRange BeginRead(size_t count);
    void EndRead(ReadRange read_range);

    size_t Read(uint8_t* buffer, size_t count);
    template <typename T>
    size_t Read(T* buffer, size_t count) {
        return Read(reinterpret_cast<uint8_t*>(buffer), count);
    }

    template <typename T>
    T Read() {
        static_assert(std::is_fundamental<T>::value, "Immediate read only supports basic types!");

        T imm;
        size_t read = Read(reinterpret_cast<uint8_t*>(&imm), sizeof(T));
        assert(read == sizeof(T));
        return imm;
    }

    size_t Write(const uint8_t* buffer, size_t count);
    template <typename T>
    size_t Write(const T* buffer, size_t count) {
        return Write(reinterpret_cast<const uint8_t*>(buffer), count);
    }

    template <typename T>
    size_t Write(T& data) {
        return Write(reinterpret_cast<const uint8_t*>(&data), sizeof(T));
    }

private:
    uint8_t* buffer_ = nullptr;
    ring_size_t capacity_ = 0;
    ring_size_t readOffset_ = 0;
    ring_size_t writeOffset_ = 0;
};
