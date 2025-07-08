#include "xma_decoder.h"

void XmaPlayback::Decode() {
    if (!IsAnyInputBufferValid()) {
        return;
    }

    if (currentFrameRemainingSubframes > 0) {
        return;
    }

    uint8_t *currentInputBuffer = GetCurrentInputBuffer();

    inputBuffer.fill(0);

    UpdateLoopStatus(this); // TODO

    debug_printf("Write Count: %d, Capacity: %d - Subframes: %d\n",
         outputRb.WriteCount(),
         remainingSubframeBlocksInOutputBuffer,
         subframes);

    const uint32_t currentInputSize = GetCurrentInputBufferPacketCount() * kBytesPerPacket;

    debug_printf("Input Size 1: %d\n", inputBuffer1Size);
    debug_printf("currentInputSize: %d\n", currentInputSize);

    uint32_t currentInputPacketCount = currentInputSize / kBytesPerPacket;

    debug_printf("currentInputPacketCount: %d\n", currentInputPacketCount);
    debug_printf("playback->inputBufferReadOffset: %d\n", inputBufferReadOffset);

    int16_t packetIndex = GetPacketNumber(currentInputSize, inputBufferReadOffset);

    uint8_t *packet = currentInputBuffer + (packetIndex * kBytesPerPacket);

    if (packetIndex == -1) {
        debug_printf("Invalid packet index. Input read offset: %d\n", inputBufferReadOffset);
        return;
    }

    const uint32_t frameOffset = GetPacketFrameOffset(packet);
    if (inputBufferReadOffset < frameOffset) {
        inputBufferReadOffset = frameOffset;
    }

    uint32_t relativeOffset = inputBufferReadOffset % kBitsPerPacket;
    const kPacketInfo packetInfo = GetPacketInfo(packet, relativeOffset);
    const uint32_t packetToSkip = GetPacketSkipCount(packet) + 1;
    const uint32_t nextPacketIndex = packetIndex + packetToSkip;

    BitStream stream = BitStream(currentInputBuffer, (packetIndex + 1) * kBitsPerPacket);
    stream.SetOffset(inputBufferReadOffset);

    const uint64_t bitsToCopy = GetAmountOfBitsToRead(
          (uint32_t)stream.BitsRemaining(), packetInfo.currentFrameSize);

    if (bitsToCopy == 0) {
        debug_printf("There is no bits to copy!\n");
        SwapInputBuffer(this);
        return;
    }

    if (packetInfo.isLastFrameInPacket()) {
        debug_printf("packetInfo.isLastFrameInPacket\n");
        // Frame is a splitted frame
        if (stream.BitsRemaining() < packetInfo.currentFrameSize) {
            const uint8_t *nextPacket = GetNextPacket(this, nextPacketIndex, currentInputPacketCount);

            if (!nextPacket) {
                // Error path
                // Decoder probably should return error here
                // Not sure what error code should be returned
                // data->error_status = 4;
                __builtin_debugtrap();
                return;
            }

            // Copy next packet to buffer
            std::memcpy(inputBuffer.data() + kBytesPerPacketData, nextPacket + kBytesPerPacketHeader,
                kBytesPerPacketData);
        }
    }

    std::memcpy(inputBuffer.data(), packet + kBytesPerPacketHeader, kBytesPerPacketData);

    stream = BitStream(inputBuffer.data(), (kBitsPerPacket - kBitsPerPacketHeader) * 2);
    stream.SetOffset(relativeOffset - kBitsPerPacketHeader);

    xmaFrame.fill(0);

    debug_printf("Reading Frame %d/%d (size: %d) From Packet %d/%d\n",
         (int32_t)packetInfo.currentFrame, packetInfo.frameCount,
         packetInfo.currentFrameSize, packetIndex,
         currentInputPacketCount);

    const uint32_t paddingStart = static_cast<uint8_t>(stream.Copy(xmaFrame.data() + 1,
        packetInfo.currentFrameSize));

    rawFrame.fill(0);

    av_packet_->data = xmaFrame.data();
    av_packet_->size = static_cast<int>(
      1 + ((paddingStart + packetInfo.currentFrameSize) / 8) +
      (((paddingStart + packetInfo.currentFrameSize) % 8) ? 1 : 0));

    auto paddingEnd = av_packet_->size * 8 - (8 + paddingStart + packetInfo.currentFrameSize);

    xmaFrame[0] = ((paddingStart & 7) << 5) | ((paddingEnd & 7) << 2);

    auto ret = avcodec_send_packet(codec_ctx, av_packet_);
    if (ret < 0) {
        debug_printf("Error sending packet for decoding: %s\n", av_err2str(ret));
    }

    ret = avcodec_receive_frame(codec_ctx, av_frame_);
    if (ret < 0) {
        debug_printf("Error receiving frame from decoder: %s\n", av_err2str(ret));
    }

    constexpr float scale = (1 << 15) - 1;
    auto out = reinterpret_cast<int16_t *>(rawFrame.data());
    auto samples = reinterpret_cast<const uint8_t **>(&av_frame_->data);

    debug_printf("decoded samples: %d\n", av_frame_->nb_samples);

    uint32_t o = 0;
    if (av_frame_->nb_samples != 0) {
        for (uint32_t i = 0; i < kSamplesPerFrame; i++) {
            for (uint32_t j = 0; j <= uint32_t(true); j++) {
                // Select the appropriate array based on the current channel.
                auto in = reinterpret_cast<const float *>(samples[j]);

                // Raw samples sometimes aren't within [-1, 1]
                float scaledSample = clamp_float(in[i], -1.0f, 1.0f) * scale;

                // Convert the sample and output it in big endian.
                auto sample = static_cast<int16_t>(scaledSample);
                out[o++] = ByteSwap(sample);
            }
        }
    }

    currentFrameRemainingSubframes = 4 << 1;

    if (!packetInfo.isLastFrameInPacket()) {
        const uint32_t nextFrameOffset = (inputBufferReadOffset + bitsToCopy) % kBitsPerPacket;

        debug_printf("Index: %d/%d - Next frame offset: %d\n",
           (int32_t)packetInfo.currentFrame, packetInfo.frameCount,
           nextFrameOffset);

        inputBufferReadOffset = (packetIndex * kBitsPerPacket) + nextFrameOffset;
        return;
    }

    uint32_t nextInputOffset = GetNextPacketReadOffset(currentInputBuffer, nextPacketIndex, currentInputPacketCount);

    if (nextInputOffset == kBitsPerPacketHeader) {
        SwapInputBuffer(this);
        // We're at the start of next buffer
        // If this packet has any frames, decoder should go to first frame in
        // packet. If it doesn't have any frames then it should immediately go to
        // the next packet.

        if (IsAnyInputBufferValid()) {
            nextInputOffset = GetPacketFrameOffset((uint8_t *)g_memory.Translate(GetCurrentInputBufferAddress()));

            if (nextInputOffset > kMaxFrameSizeinBits) {
                debug_printf("XmaContext: Next buffer contains no frames in packet! Frame offset: %d\n",
                    nextInputOffset);
                SwapInputBuffer(this);
                return;
            }

            debug_printf("XmaContext: Next buffer first frame starts at: %d\n", nextInputOffset);
        } else {
            // HACK
            SwapInputBuffer(this);
        }
    }

    inputBufferReadOffset = nextInputOffset;
}

GUEST_FUNCTION_HOOK(sub_8255C090, XMAPlaybackCreate);
GUEST_FUNCTION_HOOK(sub_8255CC48, XMAPlaybackRequestModifyLock);
GUEST_FUNCTION_HOOK(sub_8255CCC8, XMAPlaybackWaitUntilModifyLockObtained);
GUEST_FUNCTION_HOOK(sub_8255C4D0, XMAPlaybackQueryReadyForMoreData);
GUEST_FUNCTION_HOOK(sub_8255C520, XMAPlaybackIsIdle);
GUEST_FUNCTION_HOOK(sub_8255C388, XMAPlaybackQueryContextsAllocated);
GUEST_FUNCTION_HOOK(sub_8255CF10, XMAPlaybackResumePlayback);
GUEST_FUNCTION_HOOK(sub_8255C470, XMAPlaybackQueryInputDataPending);
GUEST_FUNCTION_HOOK(sub_8255C9A0, XMAPlaybackGetErrorBits);
GUEST_FUNCTION_HOOK(sub_8255C398, XMAPlaybackSubmitData);
GUEST_FUNCTION_HOOK(sub_8255C578, XMAPlaybackQueryAvailableData);
GUEST_FUNCTION_HOOK(sub_8255C7A8, XMAPlaybackAccessDecodedData);
GUEST_FUNCTION_HOOK(sub_8255C5F0, XMAPlaybackConsumeDecodedData);
GUEST_FUNCTION_HOOK(sub_8255CD90, XMAPlaybackQueryModifyLockObtained);
GUEST_FUNCTION_HOOK(sub_8255C8D8, XMAPlaybackFlushData);
GUEST_FUNCTION_HOOK(sub_8255C9D8, XmaPlaybackSetLoop);
GUEST_FUNCTION_HOOK(sub_8255CA50, XMAPlaybackGetRemainingLoopCount);
GUEST_FUNCTION_HOOK(sub_8255CA90, XMAPlaybackGetStreamPosition);
GUEST_FUNCTION_HOOK(sub_8255CB20, XMAPlaybackSetDecodePosition);
GUEST_FUNCTION_HOOK(sub_8255C850, XMAPlaybackRewindDecodePosition);
GUEST_FUNCTION_HOOK(sub_8255CAB0, XMAPlaybackQueryCurrentPosition);
GUEST_FUNCTION_HOOK(sub_8255C2C0, XMAPlaybackDestroy);
