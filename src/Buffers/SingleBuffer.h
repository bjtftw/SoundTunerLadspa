#pragma once

#include <limits>
#include <vector>
#include <iostream>

#include "Configuration.h"

namespace dePhonica {
namespace Buffers {

static float DefaultBufferSampleRate = 44100;

template<typename T>
class SingleBuffer
{
private:
    std::vector<T> bufferData_;

    int channelsCount_;
    float sampleRate_;
    size_t dataLengthSamples_;

public:
    SingleBuffer(size_t samplesPerBuffer = 0)
        : channelsCount_(1)
        , sampleRate_(DefaultBufferSampleRate)
        , dataLengthSamples_(0)
    {
        if (samplesPerBuffer > 0)
        {
            Ensure(samplesPerBuffer);
        }
    }

    const std::vector<T>& BufferDataConst() const
    {
        if (IsDebug && bufferData_.size() < DataLengthSamples())
        {
            std::cout << "C: Something went wrong - data length samples in buffer is " << DataLengthSamples() << ", but allocated size is "
                      << bufferData_.size() << std::endl;
            std::cout << "Buffer address: " << (size_t) this << std::endl;
        }

        return bufferData_;
    }

    std::vector<T>& BufferData()
    {
        if (IsDebug && bufferData_.size() < DataLengthSamples())
        {
            std::cout << "R: Something went wrong - data length samples in buffer is " << DataLengthSamples() << ", but allocated size is "
                      << bufferData_.size() << std::endl;
            std::cout << "Buffer address: " << (size_t) this << std::endl;
        }

        return bufferData_;
    }

    void Ensure(size_t desiredSize)
    {
        if (bufferData_.size() < desiredSize)
        {
            try
            {
                bufferData_.resize(desiredSize);
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
        }
    }

    float SampleRate() const { return sampleRate_; }

    void SampleRate(float sampleRate) { sampleRate_ = sampleRate; }

    int Channels() const { return channelsCount_; }

    void Channels(int channelsCount) { channelsCount_ = channelsCount; }

    size_t DataLengthSamples() const { return dataLengthSamples_; }

    void DataLengthSamples(size_t dataLengthSamples) { dataLengthSamples_ = dataLengthSamples; }

    float DataLengthSeconds() const
    {
        if (SampleRate() < std::numeric_limits<float>::min() || Channels() < 1)
        {
            return 0;
        }

        return static_cast<float>(DataLengthSamples() / SampleRate() / Channels());
    }

    void Copy(const SingleBuffer<T>& sourceBuffer)
    {
        Ensure(sourceBuffer.DataLengthSamples());

        const auto& sourceData = sourceBuffer.BufferDataConst();

        std::copy(sourceData.cbegin(), sourceData.cbegin() + sourceBuffer.DataLengthSamples(), BufferData().begin());

        Channels(sourceBuffer.Channels());
        SampleRate(sourceBuffer.SampleRate());
        DataLengthSamples(sourceBuffer.DataLengthSamples());
    }

    void Copy(const T* sourceData, int samplesCount, float sampleRate = DefaultBufferSampleRate, int channelsCount = 1)
    {
        bufferData_.clear();

        std::copy(sourceData, sourceData + samplesCount, std::back_inserter(bufferData_));

        Channels(channelsCount);
        SampleRate(sampleRate);
        DataLengthSamples(samplesCount);
    }

    void Mix(const SingleBuffer<T>& sourceBuffer)
    {
        size_t samplesToMix = std::min(DataLengthSamples(), sourceBuffer.DataLengthSamples());

        const auto& sourceData = sourceBuffer.BufferDataConst();
        auto& targetData = BufferData();

        for (size_t n = 0; n < samplesToMix; n++)
        {
            targetData[n] += sourceData[n];
        }
    }

    void Invert()
    {
        auto& bufferSamples = BufferData();

        for (size_t n = 0; n < DataLengthSamples(); n++)
        {
            bufferSamples[n] = -bufferSamples[n];
        }
    }

    void Amplify(float gain)
    {
        auto& samplesToAdjust = BufferData();
        size_t samplesCount = DataLengthSamples();

        for (size_t n = 0; n < samplesCount; n++)
        {
            samplesToAdjust[n] *= gain;
        }
    }
};

} // namespace Buffers
} // namespace dePhonica
