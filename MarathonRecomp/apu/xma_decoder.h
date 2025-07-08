#pragma once

#include <utils/bit_stream.h>
#include <utils/ring_buffer.h>
#include <kernel/function.h>
#include <kernel/heap.h>
#include <condition_variable>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/opt.h>
    #include <libswresample/swresample.h>
}

#define MARATHON_RECOMP_DEBUG_XMA_DECODER

#ifdef MARATHON_RECOMP_DEBUG_XMA_DECODER
#define debug_printf(...) printf(__VA_ARGS__)
#else
#define debug_printf(...)
#endif

struct XMAPLAYBACKINIT {
    be<uint32_t> sampleRate;
    be<uint32_t> outputBufferSize;
    uint8_t channelCount;
    uint8_t subframes;
};

constexpr uint32_t kBytesPerPacket = 2048;
constexpr uint32_t kBytesPerPacketHeader = 4;
constexpr uint32_t kBytesPerPacketData = kBytesPerPacket - kBytesPerPacketHeader;
constexpr uint32_t kBytesPerSample = 2;
constexpr uint32_t kSamplesPerFrame = 512;
constexpr uint32_t kBytesPerFrameChannel = kSamplesPerFrame * kBytesPerSample;

constexpr uint32_t kOutputBytesPerBlock = 256;
constexpr uint32_t kBitsPerPacketHeader = 32;
constexpr uint32_t kBitsPerPacket = kBytesPerPacket * 8;
constexpr uint32_t kMaxFrameLength = 0x7FFF;
constexpr uint32_t kBitsPerFrameHeader = 15;
constexpr uint32_t kMaxFrameSizeinBits = 0x4000 - kBitsPerPacketHeader;

class XmaPlayback {
public:
    uint32_t sampleRate;
    uint32_t outputBufferSize;
    uint32_t channelCount;
    uint32_t subframes;
    uint32_t outputBuffer;
    uint32_t currentDataSize;

    uint32_t partialBytesRead = 0;
    uint32_t streamPosition = 0;

    bool bAllowedToDecode = false;

    // ffmpeg
    AVCodecContext *codec_ctx = nullptr;
    const AVCodec *codec = nullptr;
    AVPacket *av_packet_ = nullptr;
    AVFrame *av_frame_ = nullptr;

    // xenia
    std::array<uint8_t, kBytesPerPacketData * 2> inputBuffer;
    std::array<uint8_t, 1 + 4096> xmaFrame;
    std::array<uint8_t, kBytesPerFrameChannel * 2> rawFrame;
    uint32_t outputBufferBlockCount = 0;
    uint32_t outputBufferReadOffset = 0;
    uint32_t outputBufferWriteOffset = 0;
    int32_t remainingSubframeBlocksInOutputBuffer = 0;
    uint8_t currentFrameRemainingSubframes = 0;
    uint32_t inputBufferReadOffset = 32;

    uint32_t numSubframesToSkip = 0;

    RingBuffer outputRb;
    uint32_t inputBuffer1 = 0;
    uint32_t inputBuffer2 = 0;
    size_t inputBuffer1Size = 0;
    size_t inputBuffer2Size = 0;
    uint32_t validInputBuffer = 0;
    uint32_t inputBuffer1Valid = 0;
    uint32_t inputBuffer2Valid = 0;
    uint32_t currentBuffer = 0;

    uint32_t outputBufferValid = 1;

    uint8_t numLoops = 0;
    uint8_t loopSubframeEnd = 0;
    uint8_t loopSubframeSkip = 0;
    uint32_t loopStartOffset = 0;
    uint32_t loopEndOffset = 0;

    std::thread decoderThread;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<bool> isLocked { false };
    std::atomic<bool> isRunning { true };

    XmaPlayback(uint32_t sampleRate, uint32_t outputBufferSize, uint32_t channelCount, uint32_t subframes)
    : sampleRate(sampleRate), outputBufferSize(outputBufferSize), channelCount(channelCount),
    subframes(subframes), outputRb(nullptr, 0) {
        outputBufferBlockCount = (((channelCount * outputBufferSize) << 15) & 0x7C00000) >> 22;
        outputBuffer = g_memory.MapVirtual(g_userHeap.AllocPhysical((size_t)0x2000, 0));

        codec = avcodec_find_decoder(AV_CODEC_ID_XMAFRAMES);
        if (!codec) {
            throw std::runtime_error("Decoder not found");
        }

        codec_ctx = avcodec_alloc_context3(codec);
        if (!codec_ctx) {
            throw std::runtime_error("Failed to allocate codec context");
        }

        codec_ctx->sample_rate = sampleRate;
        codec_ctx->ch_layout.nb_channels = channelCount;

        av_frame_ = av_frame_alloc();
        if (!av_frame_) {
            throw std::runtime_error("Couldn't allocate frame");
        }

        if (int err = avcodec_open2(codec_ctx, codec, nullptr); err < 0) {
            throw std::runtime_error("Failed to open codec");
        }
        av_packet_ = av_packet_alloc();
    }

    void Decode();

    const uint32_t GetInputBufferAddress(uint8_t bufferIndex) const {
        return bufferIndex == 0 ? inputBuffer1 : inputBuffer2;
    }

    const uint32_t GetCurrentInputBufferAddress() const {
        return GetInputBufferAddress(currentBuffer);
    }

    const uint32_t GetInputBufferPacketCount(uint8_t bufferIndex) const {
        return bufferIndex == 0 ? inputBuffer1Size : inputBuffer2Size;
    }

    const uint32_t GetCurrentInputBufferPacketCount() const {
        return GetInputBufferPacketCount(currentBuffer);
    }

    uint8_t *GetCurrentInputBuffer() {
        return (uint8_t *)g_memory.Translate(GetCurrentInputBufferAddress());
    }

    bool IsInputBufferValid(uint8_t bufferIndex) const {
        return bufferIndex == 0 ? inputBuffer1Valid : inputBuffer2Valid;
    }

    bool IsCurrentInputBufferValid() const {
        return IsInputBufferValid(currentBuffer);
    }

    bool IsAnyInputBufferValid() const {
        return inputBuffer1Valid || inputBuffer2Valid;
    }

    ~XmaPlayback() {
        {
          std::lock_guard lock(mutex);
          isRunning = false;
          cv.notify_one();
        }

        if (decoderThread.joinable()) {
            decoderThread.join();
        }
    }
};

uint32_t XMAPlaybackGetFrameOffsetFromPacketHeader(uint32_t header) {
    uint32_t result = 0;
    if (header != 0x7FFF)
        return ((header >> 11) & 0x7FFF) + 32;
    return result;
}

uint32_t XMAGetOutputBufferBlockCount(XmaPlayback *playback) {
    return (((playback->channelCount * playback->outputBufferSize) << 15) &
            0x7C00000) >> 22;
}

int16_t GetPacketNumber(size_t size, size_t bitOffset) {
    if (bitOffset < kBitsPerPacketHeader) {
        return -1;
    }

    if (bitOffset >= (size << 3)) {
        return -1;
    }

    size_t byteOffset = bitOffset >> 3;
    size_t packetNumber = byteOffset / kBytesPerPacket;

    return (int16_t)packetNumber;
}

static uint32_t GetPacketFrameOffset(const uint8_t *packet) {
    uint32_t val = (uint16_t)(((packet[0] & 0x3) << 13) | (packet[1] << 5) |
                            (packet[2] >> 3));
    return val + 32;
}

struct kPacketInfo {
    uint8_t frameCount;
    uint8_t currentFrame;
    uint32_t currentFrameSize;

    const bool isLastFrameInPacket() const {
        return currentFrame == frameCount - 1;
    }
};

kPacketInfo GetPacketInfo(uint8_t *packet, uint32_t frameOffset) {
    kPacketInfo packetInfo = {};

    const uint32_t firstFrameOffset = GetPacketFrameOffset(packet);
    BitStream stream(packet, kBitsPerPacket);
    stream.SetOffset(firstFrameOffset);

    // Handling of splitted frame
    if (frameOffset < firstFrameOffset) {
        packetInfo.currentFrame = 0;
        packetInfo.currentFrameSize = firstFrameOffset - frameOffset;
    }

    while (true) {
        if (stream.BitsRemaining() < kBitsPerFrameHeader) {
            break;
        }

        const uint64_t frameSize = stream.Peek(kBitsPerFrameHeader);
        if (frameSize == kMaxFrameLength) {
            break;
        }

        if (stream.offset_bits() == frameOffset) {
            packetInfo.currentFrame = packetInfo.frameCount;
            packetInfo.currentFrameSize = (uint32_t)frameSize;
        }

        packetInfo.frameCount++;

        if (frameSize > stream.BitsRemaining()) {
            // Last frame.
            break;
        }

        stream.Advance(frameSize - 1);

        // Read the trailing bit to see if frames follow
        if (stream.Read(1) == 0) {
            break;
        }
    }

    return packetInfo;
}

uint8_t *GetNextPacket(XmaPlayback *playback, uint32_t nextPacketIndex, uint32_t currentInputPacketCount) {
    if (nextPacketIndex < currentInputPacketCount) {
        uint8_t *currentInputBuffer = (uint8_t *)g_memory.Translate(playback->GetCurrentInputBufferAddress());
        return currentInputBuffer + nextPacketIndex * kBytesPerPacket;
    }

    const uint8_t nextBufferIndex = playback->currentBuffer ^ 1;

    if (!playback->IsInputBufferValid(nextBufferIndex)) {
        return nullptr;
    }

    const uint32_t nextBufferAddress = playback->GetInputBufferAddress(nextBufferIndex);

    if (!nextBufferAddress) {
        // This should never occur but there is always a chance
        debug_printf("XmaContext: Buffer is marked as valid, but doesn't have valid pointer!\n");
        return nullptr;
    }

    return (uint8_t *)g_memory.Translate(nextBufferAddress);
}

uint8_t GetPacketSkipCount(const uint8_t *packet) { return packet[3]; }

uint32_t GetAmountOfBitsToRead(const uint32_t remainingStreamBits,
                               const uint32_t frameSize) {
    return std::min(remainingStreamBits, frameSize);
}

template <typename T> T clamp_float(T value, T minValue, T maxValue) {
    float clampedToMin = std::isgreater(value, minValue) ? value : minValue;
    return std::isless(clampedToMin, maxValue) ? clampedToMin : maxValue;
}

const uint32_t GetNextPacketReadOffset(uint8_t *buffer,
                                       uint32_t nextPacketIndex,
                                       uint32_t currentInputPacketCount) {
    if (nextPacketIndex >= currentInputPacketCount) {
        return kBitsPerPacketHeader;
    }

    uint8_t *nextPacket = buffer + (nextPacketIndex * kBytesPerPacket);
    const uint32_t packetFrameOffset = GetPacketFrameOffset(nextPacket);

    if (packetFrameOffset > kMaxFrameSizeinBits) {
        const uint32_t offset = GetNextPacketReadOffset(
            buffer, nextPacketIndex + 1, currentInputPacketCount);
        return offset;
    }

    const uint32_t newInputBufferOffset = (nextPacketIndex * kBitsPerPacket) + packetFrameOffset;

    debug_printf("XmaContext: newInputBufferOffset: %d packetFrameOffset: %d packet: %d/%d\n",
        newInputBufferOffset, packetFrameOffset, nextPacketIndex,
        currentInputPacketCount);
    return newInputBufferOffset;
}

void SwapInputBuffer(XmaPlayback *playback) {
    debug_printf("Swap Input Buffer\n");
    // No more frames.
    if (playback->currentBuffer == 0) {
        playback->inputBuffer1Valid = 0;
    } else {
        playback->inputBuffer2Valid = 0;
    }
    playback->currentBuffer ^= 1;
    playback->inputBufferReadOffset = kBitsPerPacketHeader;
}

void UpdateLoopStatus(XmaPlayback *playback) {
    if (playback->numLoops == 0) {
        return;
    }

    const uint32_t loop_start = std::max(kBitsPerPacketHeader, playback->loopStartOffset);
    const uint32_t loop_end = std::max(kBitsPerPacketHeader, playback->loopEndOffset);

    debug_printf("XmaContext: Looped Data: %d < %d (Start: %d) Remaining: %d\n",
        playback->inputBufferReadOffset, playback->loopEndOffset,
        playback->loopStartOffset, playback->numLoops);

    if (playback->inputBufferReadOffset != loop_end) {
        return;
    }

    playback->inputBufferReadOffset = loop_start;

    if (playback->numLoops != 255) {
        playback->numLoops--;
    }
}

void Consume(XmaPlayback *playback) {
    debug_printf("playback->currentFrameRemainingSubframes: %d\n", playback->currentFrameRemainingSubframes);
    if (!playback->currentFrameRemainingSubframes) {
        return;
    }

    const int8_t subframesToWrite =
        std::min((int8_t)playback->currentFrameRemainingSubframes,
            (int8_t)playback->subframes);

    const int8_t rawFrameReadOffset =
        ((kBytesPerFrameChannel / kOutputBytesPerBlock) << 1) - // is stereo
        playback->currentFrameRemainingSubframes;
    //  + data->subframe_skip_count;

    playback->outputRb.Write(playback->rawFrame.data() +
        (kOutputBytesPerBlock * rawFrameReadOffset),
        subframesToWrite * kOutputBytesPerBlock);\

    debug_printf("kOutputBytesPerBlock * rawFrameReadOffset %d\n", kOutputBytesPerBlock * rawFrameReadOffset);
    debug_printf("subframesToWrite * kOutputBytesPerBlock %d\n", subframesToWrite * kOutputBytesPerBlock);
    debug_printf("writeCount %d\n", playback->outputRb.WriteCount());

    playback->remainingSubframeBlocksInOutputBuffer -= subframesToWrite;
    playback->currentFrameRemainingSubframes -= subframesToWrite;

    debug_printf("Consume: %d - %d - %d - %d - %d\n",
        playback->remainingSubframeBlocksInOutputBuffer,
        playback->outputBufferWriteOffset,
        playback->outputBufferReadOffset,
        playback->outputRb.WriteOffset(),
        playback->currentFrameRemainingSubframes);
}

void DecoderThreadFunc(XmaPlayback *playback) {
    while (playback->isRunning) {
        std::unique_lock lock(playback->mutex);

        playback->cv.wait(lock, [&] {
            return (!playback->isRunning || (playback->outputBufferValid == 1 && playback->IsAnyInputBufferValid()))
                && !playback->isLocked.load();
        });

        if (!playback->outputBufferValid)
            continue;

        // debug_printf("allowed to decode: %d\n", playback->bAllowedToDecode);

        if (!playback->isRunning)
            break;

        lock.unlock();

        const int32_t minimumSubframeDecodeCount = (playback->subframes * 2) - 1;

        size_t outputCapacity = playback->outputBufferBlockCount * kOutputBytesPerBlock;

        const uint32_t outputReadOffset = playback->outputBufferReadOffset * kOutputBytesPerBlock;
        const uint32_t outputWriteOffset = playback->outputBufferWriteOffset * kOutputBytesPerBlock;

        playback->outputRb = RingBuffer((uint8_t *)g_memory.Translate(playback->outputBuffer), outputCapacity);
        playback->outputRb.SetReadOffset(outputReadOffset);
        playback->outputRb.SetWriteOffset(outputWriteOffset);
        playback->remainingSubframeBlocksInOutputBuffer = (int32_t)playback->outputRb.WriteCount() / kOutputBytesPerBlock;

        if (minimumSubframeDecodeCount > playback->remainingSubframeBlocksInOutputBuffer) {
            debug_printf("No space for subframe decoding %d/%d!\n",
                minimumSubframeDecodeCount,
                playback->remainingSubframeBlocksInOutputBuffer);

            playback->bAllowedToDecode = false;
            lock.lock();
            continue;
        }

        while (playback->remainingSubframeBlocksInOutputBuffer >= minimumSubframeDecodeCount) {
            debug_printf("while: %d >= %d\n",
                playback->remainingSubframeBlocksInOutputBuffer,
                minimumSubframeDecodeCount);

            playback->Decode();
            Consume(playback);

            if (!playback->IsAnyInputBufferValid()) {
                break;
            }
        }

        playback->outputBufferWriteOffset = playback->outputRb.WriteOffset() / kOutputBytesPerBlock;
        debug_printf("playback->outputBufferWriteOffset: %d\n", playback->outputBufferWriteOffset);

        playback->bAllowedToDecode = false;

        if (playback->outputRb.Empty()) {
            playback->outputBufferValid = 0;
        }

        lock.lock();
    }
}

uint32_t XMAPlaybackCreate(uint32_t streams, XMAPLAYBACKINIT *init,
                           uint32_t flags, be<uint32_t> *outPlayback) {
    debug_printf("XMAPlaybackCreate: %x %x %x %x\n", streams, init, flags, outPlayback);
    debug_printf("sampleRate: %d\n", init->sampleRate.get());
    debug_printf("outputBufferSize: %x\n", init->outputBufferSize.get());
    debug_printf("channelCount: %x\n", init->channelCount);
    debug_printf("subframes: %x\n", init->subframes);

    const auto xmaPlayback = g_userHeap.AllocPhysical<XmaPlayback>(
        init->sampleRate.get(), init->outputBufferSize.get(), init->channelCount,
        init->subframes);

    xmaPlayback->decoderThread = std::thread(DecoderThreadFunc, xmaPlayback);
    *outPlayback = g_memory.MapVirtual(xmaPlayback);
    return 0;
}

uint32_t XMAPlaybackRequestModifyLock(XmaPlayback *playback) {
    debug_printf("XMAPlaybackRequestModifyLock\n");

    std::lock_guard lock(playback->mutex);
    playback->isLocked = true;
    return 0;
}

uint32_t XMAPlaybackWaitUntilModifyLockObtained(XmaPlayback *playback) {
    debug_printf("XMAPlaybackWaitUntilModifyLockObtained\n");

    std::unique_lock lock(playback->mutex);
    playback->cv.wait(lock, [&playback] { return playback->isLocked.load(); });
    return 0;
}

uint32_t XMAPlaybackQueryReadyForMoreData(XmaPlayback *playback, uint32_t stream) {
    debug_printf("XMAPlaybackQueryReadyForMoreData %x\n", stream);

    return playback->inputBuffer1Valid == 0 ||
           playback->inputBuffer2Valid == 0;
}

uint32_t XMAPlaybackIsIdle(XmaPlayback *playback, uint32_t stream) {
    debug_printf("XMAPlaybackIsIdle %x %d\n", stream,
        playback->inputBuffer1Valid == 0 &&
        playback->inputBuffer2Valid == 0);

    return playback->inputBuffer1Valid == 0 &&
           playback->inputBuffer2Valid == 0;
}

uint32_t XMAPlaybackQueryContextsAllocated(XmaPlayback *playback) {
    debug_printf("XMAPlaybackQueryContextsAllocated %x\n", playback);

    return playback != nullptr;
}

uint32_t XMAPlaybackResumePlayback(XmaPlayback *playback) {
    debug_printf("XMAPlaybackResumePlayback %x\n", playback);
    std::lock_guard lock(playback->mutex);
    playback->isLocked = false;
    playback->cv.notify_one();
    debug_printf("XMAPlaybackResumePlayback resumed\n");
    return 0;
}

uint32_t XMAPlaybackQueryInputDataPending(XmaPlayback *playback, uint32_t stream, uint32_t data) {
    debug_printf("XMAPlaybackQueryInputDataPending %x %x\n", playback, data);

    if (playback->inputBuffer1Valid && playback->inputBuffer1 == data) {
        return 1;
    }
    if (playback->inputBuffer2Valid && playback->inputBuffer2 == data) {
        return 1;
    }
    return 0;
}

uint32_t XMAPlaybackGetErrorBits(XmaPlayback *playback, uint32_t stream) {
    debug_printf("XMAPlaybackGetErrorBits %x\n", playback);
    return 0;
}

uint32_t XMAPlaybackSubmitData(XmaPlayback *playback, uint32_t stream, uint32_t data, uint32_t dataSize) {
    debug_printf("XMAPlaybackSubmitData %x %x %x %x\n", playback, stream, data, dataSize);

    if (!playback->isLocked)
        return 1;

    std::lock_guard lock(playback->mutex);
    uint32_t packetCount = dataSize >> 11;

    debug_printf("is valid1: %d\n", playback->inputBuffer1Valid);
    uint32_t validBuffer = playback->inputBuffer1Valid | (playback->inputBuffer2Valid << 1);
    if (playback->inputBuffer1Valid == 1) {
        if (playback->inputBuffer2Valid == 1) {
            return 0x80070005;
        }

        playback->inputBuffer2 = data;
        playback->inputBuffer2Size = packetCount & 0xFFF;
        playback->inputBuffer2Valid = 1;

        debug_printf("input2 written\n");
    } else {
        playback->inputBuffer1 = data;
        playback->inputBuffer1Size = packetCount & 0xFFF;
        playback->inputBuffer1Valid = 1;

        debug_printf("input1 written\n");
    }

    if (!validBuffer) {
        uint8_t *currentInputBuffer = (uint8_t *)g_memory.Translate(data);
        uint32_t frameOffset = XMAPlaybackGetFrameOffsetFromPacketHeader(*currentInputBuffer);
        if (frameOffset) {
            playback->inputBufferReadOffset = frameOffset & 0x3FFFFFF;
        }
    }

    playback->bAllowedToDecode = true;
    playback->cv.notify_one();
    return 0;
}

uint32_t XMAPlaybackQueryAvailableData(XmaPlayback *playback, uint32_t stream) {
    debug_printf("XMAPlaybackQueryAvailableData %x %x\n", playback, stream);

    if (!playback->isLocked || (playback->inputBuffer1Valid == 0 &&
                                playback->inputBuffer2Valid == 0))
        return 0;

    uint32_t partialBytesRead = playback->partialBytesRead;
    uint32_t writeBufferReadOffset = playback->outputBufferReadOffset & 0x1F;
    uint32_t writeOffset = playback->outputBufferWriteOffset;
    uint32_t writeSize = playback->outputBufferBlockCount & 0x1F;

    uint32_t availableBytes = 0;
    uint32_t isWriteValid = playback->outputBufferValid;

    if (partialBytesRead) {
        availableBytes = 256 - partialBytesRead;
        writeBufferReadOffset++;
        isWriteValid = 1;
    }

    uint32_t available_blocks = 0;
    if (writeOffset <= writeBufferReadOffset) {
        if (writeOffset < writeBufferReadOffset || !isWriteValid) {
        available_blocks = writeSize - writeBufferReadOffset;
        }
    } else {
        available_blocks = writeOffset - writeBufferReadOffset;
    }

    uint32_t totalBytes = (available_blocks << 8) + availableBytes;
    uint32_t bytesPerSample = 1 + 1; // TODO: find all channels and refactor

    debug_printf("samples accessed: %d\n", totalBytes >> bytesPerSample);

    return totalBytes >> bytesPerSample;
}


uint32_t XMAPlaybackAccessDecodedData(XmaPlayback *playback, uint32_t stream, uint32_t **data) {
    debug_printf("XMAPlaybackAccessDecodedData %x %x %x\n", playback, stream, data);

    if (!playback->isLocked)
        return 0;

    uint32_t partialBytesRead = playback->partialBytesRead;
    debug_printf("data: %x %x\n", data, *data);
    uint32_t addr = playback->outputBuffer + (playback->outputBufferReadOffset << 8 & 0x1F00) + partialBytesRead;

    *data = (uint32_t *)__builtin_bswap32(addr);
    debug_printf("data2: %x %x\n", data, *data);

    uint32_t writeBufferReadOffset = playback->outputBufferReadOffset & 0x1F;
    uint32_t writeOffset = playback->outputBufferWriteOffset;
    uint32_t writeSize = playback->outputBufferBlockCount & 0x1F;

    uint32_t availableBytes = 0;
    uint32_t isWriteValid = playback->outputBufferValid;
    if (partialBytesRead) {
        availableBytes = 256 - partialBytesRead;
        writeBufferReadOffset++;
        isWriteValid = 1;
    }

    uint32_t availableBlocks = 0;
    if (writeOffset <= writeBufferReadOffset) {
        if (writeOffset < writeBufferReadOffset || !isWriteValid) {
            availableBlocks = writeSize - writeBufferReadOffset;
        }
    } else {
        availableBlocks = writeOffset - writeBufferReadOffset;
    }

    uint32_t totalBytes = (availableBlocks << 8) + availableBytes;
    uint32_t bytesPerSample = 1 + 1; // TODO: find all channels and refactor
    debug_printf("samples accessed: %d\n", totalBytes >> bytesPerSample);
    return totalBytes >> bytesPerSample;
}

uint32_t XMAPlaybackConsumeDecodedData(XmaPlayback *playback, uint32_t stream, uint32_t maxSamples, uint32_t **data) {
    debug_printf("XMAPlaybackConsumeDecodedData %x %x %x %x\n", playback, stream, maxSamples, data);

    if (!playback->isLocked)
        return 0;

    uint32_t totalBytes = 0;
    uint32_t partialBytesRead = playback->partialBytesRead;
    uint32_t addr = playback->outputBuffer + (playback->outputBufferReadOffset << 8 & 0x1F00) + partialBytesRead;

    *data = (uint32_t *)__builtin_bswap32(addr);
    uint32_t bytesPerSample = 1 + 1;
    uint32_t bytesDesired = maxSamples << bytesPerSample;
    if (partialBytesRead) {
        if (bytesDesired < 256 - partialBytesRead) {
            totalBytes = bytesDesired;
            playback->partialBytesRead += bytesDesired;
            bytesDesired = 0;
        } else {
            totalBytes = 256 - partialBytesRead;
            bytesDesired -= 256 - partialBytesRead;
            playback->partialBytesRead = 0;

            uint32_t writeIndex = (playback->outputBufferReadOffset + 1) & 0x1F;
            if (writeIndex >= (playback->outputBufferBlockCount & 0x1F))
                writeIndex = 0;
            playback->outputBufferReadOffset = writeIndex;
            playback->outputBufferValid = 1;
        }
    }

    uint32_t writeIndex = playback->outputBufferReadOffset;
    uint32_t blocksToProcess = bytesDesired >> 8;
    uint32_t availableBlocks = 0;

    uint32_t writeSize = playback->outputBufferBlockCount;
    if (writeSize <= writeIndex) {
        if (writeSize < writeIndex || !playback->outputBufferValid)
            availableBlocks = (playback->outputBufferBlockCount & 0x1F) - writeIndex;
    } else {
        availableBlocks = writeSize - writeIndex;
    }

    if (blocksToProcess) {
        if (blocksToProcess > availableBlocks)
            blocksToProcess = availableBlocks;

        totalBytes += blocksToProcess << 8;
        availableBlocks -= blocksToProcess;
        writeIndex = (writeIndex + blocksToProcess) & 0x1F;
        if (writeIndex >= (playback->outputBufferBlockCount & 0x1F))
            writeIndex = 0;

        playback->outputBufferReadOffset = writeIndex;
        playback->outputBufferValid = 1;
    }

    uint32_t remainingBytes = bytesDesired & 0xFF;
    if (remainingBytes && availableBlocks) {
        totalBytes += remainingBytes;
        playback->partialBytesRead = remainingBytes;
    }

    // playback->bAllowedToDecode = true;
    // playback->cv.notify_one();

    uint32_t samplesConsumed = totalBytes >> bytesPerSample;
    playback->streamPosition += samplesConsumed;
    debug_printf("samples desired and consumed: %d %d\n", maxSamples, samplesConsumed);
    return samplesConsumed;
}

uint32_t XMAPlaybackQueryModifyLockObtained(XmaPlayback *playback) {
    debug_printf("XMAPlaybackQueryModifyLockObtained %x\n", playback);
    return playback->isLocked.load();
}

uint32_t XMAPlaybackDestroy(XmaPlayback *playback) {
    debug_printf("XMAPlaybackDestroy %x\n", playback);
    return 0;
}

uint32_t XMAPlaybackFlushData(XmaPlayback *playback) {
    debug_printf("XMAPlaybackFlushData %x\n", playback);
    // __builtin_debugtrap();
    return 0;
}

struct XMAPLAYBACKLOOP {
    be<uint32_t> loopStartOffset;
    be<uint32_t> loopEndOffset;
    uint8_t loopSubframeEnd;
    uint8_t loopSubframeSkip;
    uint8_t numLoops;
    uint8_t reserved;
};

uint32_t XmaPlaybackSetLoop(XmaPlayback *playback, uint32_t streamIndex, XMAPLAYBACKLOOP *loop) {
    debug_printf("XmaPlaybackSetLoop %x\n", playback);
    debug_printf("loopStartOffset: %x\n", loop->loopStartOffset.get());
    debug_printf("loopEndOffset: %x\n", loop->loopEndOffset.get());
    debug_printf("loopSubframeEnd: %x\n", loop->loopSubframeEnd);
    debug_printf("loopSubframeSkip: %x\n", loop->loopSubframeSkip);
    debug_printf("numLoops: %x\n", loop->numLoops);

    playback->numLoops = loop->numLoops;
    playback->loopSubframeEnd = loop->loopSubframeEnd;
    playback->loopSubframeSkip = loop->loopSubframeSkip;
    playback->loopStartOffset = loop->loopStartOffset.get() & 0x3FFFFFF;
    playback->loopEndOffset = loop->loopEndOffset.get() & 0x3FFFFFF;
    // __builtin_debugtrap();
    return 0;
}

uint32_t XMAPlaybackGetRemainingLoopCount(XmaPlayback *playback) {
    debug_printf("XMAPlaybackGetRemainingLoopCount %x\n", playback);
    __builtin_debugtrap();
    return 0;
}

uint32_t XMAPlaybackGetStreamPosition(XmaPlayback *playback) {
    debug_printf("XMAPlaybackGetStreamPosition %x\n", playback);
    return playback->streamPosition;
}

uint32_t XMAPlaybackSetDecodePosition(XmaPlayback *playback, uint32_t streamIndex,
                                      uint32_t bitOffset, uint32_t subframe) {
    debug_printf("XMAPlaybackSetDecodePosition %x %d %d\n", playback, bitOffset, subframe);
    playback->inputBufferReadOffset = bitOffset & 0x3FFFFFF;
    playback->numSubframesToSkip = subframe & 0x7;
    return 0;
}

uint32_t XMAPlaybackRewindDecodePosition(XmaPlayback *playback, uint32_t streamIndex, uint32_t numSamples) {
    debug_printf("XMAPlaybackRewindDecodePosition %x %d %d\n", playback, streamIndex, numSamples);
    uint32_t shift = 7 - (1 != 0);
    uint32_t adjustedSamples = numSamples >> shift;
    uint32_t writeSize = playback->outputBufferBlockCount & 0x1F;

    uint32_t new_offset;
    if (adjustedSamples >= writeSize) {
        new_offset = playback->inputBufferReadOffset & 0x3FFFFFF;
        playback->outputBufferValid = 1;
        playback->outputBufferWriteOffset = new_offset >> 27;
        return 0;
    }

    new_offset = (playback->outputBufferWriteOffset - adjustedSamples + writeSize) & 0x1F;
    if (new_offset >= writeSize) {
        new_offset -= writeSize;
    }

    playback->outputBufferValid = 1;
    playback->outputBufferWriteOffset = new_offset;

    return 1;
}

uint32_t XMAPlaybackQueryCurrentPosition(XmaPlayback *playback) {
    debug_printf("XMAPlaybackQueryCurrentPosition %x\n", playback);
    __builtin_debugtrap();
    return 0;
}
