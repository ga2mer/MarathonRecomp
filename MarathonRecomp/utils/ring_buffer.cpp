/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "ring_buffer.h"

void RingBuffer::AdvanceRead(size_t _count) {
  ring_size_t count = static_cast<ring_size_t>(_count);
  if (readOffset_ + count < capacity_) {
    readOffset_ += count;
  } else {
    ring_size_t left_half = capacity_ - readOffset_;
    ring_size_t right_half = count - left_half;
    readOffset_ = right_half;
  }
}

void RingBuffer::AdvanceWrite(size_t _count) {
  ring_size_t count = static_cast<ring_size_t>(_count);

  if (writeOffset_ + count < capacity_) {
    writeOffset_ += count;
  } else {
    ring_size_t left_half = capacity_ - writeOffset_;
    ring_size_t right_half = count - left_half;
    writeOffset_ = right_half;
  }
}

RingBuffer::ReadRange RingBuffer::BeginRead(size_t _count) {
  ring_size_t count =
      std::min<ring_size_t>(static_cast<ring_size_t>(_count), capacity_);
  if (!!(count)) [[likely]] {
    if (readOffset_ + count < capacity_) {
      return {buffer_ + readOffset_, nullptr, count, 0};
    } else {
      ring_size_t left_half = capacity_ - readOffset_;
      ring_size_t right_half = count - left_half;
      return {buffer_ + readOffset_, buffer_, left_half, right_half};
    }
  } else {
    return {0};
  }
}

void RingBuffer::EndRead(ReadRange read_range) {
  if (read_range.second) {
    readOffset_ = read_range.secondLength;
  } else {
    readOffset_ += read_range.firstLength;
  }
}

size_t RingBuffer::Read(uint8_t *buffer, size_t _count) {
  ring_size_t count = static_cast<ring_size_t>(_count);
  count = std::min(count, capacity_);
  if (!count) {
    return 0;
  }

  // Sanity check: Make sure we don't read over the write offset.
  if (readOffset_ < writeOffset_) {
    assert(read_offset_ + count <= write_offset_);
  } else if (readOffset_ + count >= capacity_) {
    __attribute__((unused)) ring_size_t left_half = capacity_ - readOffset_;

    assert(count - left_half <= write_offset_);
  }

  if (readOffset_ + count < capacity_) {
    std::memcpy(buffer, buffer_ + readOffset_, count);
    readOffset_ += count;
  } else {
    ring_size_t left_half = capacity_ - readOffset_;
    ring_size_t right_half = count - left_half;
    std::memcpy(buffer, buffer_ + readOffset_, left_half);
    std::memcpy(buffer + left_half, buffer_, right_half);
    readOffset_ = right_half;
  }

  return count;
}

size_t RingBuffer::Write(const uint8_t *buffer, size_t _count) {
  ring_size_t count = static_cast<ring_size_t>(_count);
  count = std::min(count, capacity_);
  if (!count) {
    return 0;
  }

  // Sanity check: Make sure we don't write over the read offset.
  if (writeOffset_ < readOffset_) {
    assert(write_offset_ + count <= read_offset_);
  } else if (writeOffset_ + count >= capacity_) {
    __attribute__((unused)) size_t left_half = capacity_ - writeOffset_;
    assert(count - left_half <= read_offset_);
  }

  if (writeOffset_ + count < capacity_) {
    std::memcpy(buffer_ + writeOffset_, buffer, count);
    writeOffset_ += count;
  } else {
    ring_size_t left_half = capacity_ - writeOffset_;
    ring_size_t right_half = count - left_half;
    std::memcpy(buffer_ + writeOffset_, buffer, left_half);
    std::memcpy(buffer_, buffer + left_half, right_half);
    writeOffset_ = right_half;
  }

  return count;
}