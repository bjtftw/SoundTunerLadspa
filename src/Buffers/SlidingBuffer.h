#pragma once

#include <iostream>
#include <vector>

namespace dePhonica {
namespace Buffers {

static size_t InitialSlidingBufferSize = 65536 * 2;

template<typename T>
class SlidingBuffer
{
private:
    std::vector<T> storeBuffer_;
    size_t positionLow_, positionHigh_, dataStored_;

    bool isCanGrow_;

    void UpsizeBuffer(int newSize = -1)
    {
        size_t newBufferSize = newSize > 0 ? newSize : storeBuffer_.size() * 2;

        std::vector<T> newBuffer(newBufferSize);

        size_t amountOfData = DataLengthSamples();

        if (amountOfData > newBufferSize)
        {
            Purge(amountOfData - newBufferSize);
            amountOfData = newBufferSize;
        }

        Pop(newBuffer, 0, amountOfData);

        storeBuffer_.assign(newBuffer.begin(), newBuffer.end());

        positionLow_ = 0;
        positionHigh_ = amountOfData;
        dataStored_ = amountOfData;
    }

    void PopInternal(std::vector<T>& buffer, int offset, int length, bool purge)
    {
        size_t copyLength = length;
        size_t remainingLength = 0;

        if (copyLength > DataLengthSamples())
        {
            copyLength = DataLengthSamples();
            remainingLength = length - copyLength;
        }

        size_t storageTop = storeBuffer_.size();

        if (storageTop - positionLow_ >= copyLength)
        {
            if (purge == false)
            {
                std::copy(storeBuffer_.begin() + positionLow_, storeBuffer_.begin() + positionLow_ + copyLength, buffer.begin() + offset);
            }

            positionLow_ += copyLength;
        }
        else
        {
            size_t firstChunkLength = storageTop - positionLow_;

            if (purge == false)
            {
                std::copy(storeBuffer_.begin() + positionLow_,
                          storeBuffer_.begin() + positionLow_ + firstChunkLength,
                          buffer.begin() + offset);
            }

            size_t secondChunkLength = copyLength - firstChunkLength;
            if (purge == false)
            {
                std::copy(storeBuffer_.begin(), storeBuffer_.begin() + secondChunkLength, buffer.begin() + offset + firstChunkLength);
            }

            positionLow_ = secondChunkLength;
        }

        if (remainingLength > 0 && purge == false)
        {
            for (size_t n = 0, targetIndex = offset + copyLength; n < remainingLength; n++, targetIndex++)
            {
                buffer[targetIndex] = T();
            }
        }

        dataStored_ -= copyLength;
    }

public:
    SlidingBuffer(size_t bufferSize = InitialSlidingBufferSize, bool isCanGrow = false)
        : storeBuffer_(bufferSize)
        , isCanGrow_(isCanGrow)
    {
        Flush();
    }

    void Flush()
    {
        positionLow_ = positionHigh_ = 0;
        dataStored_ = 0;
    }

    void Push(const std::vector<T>& buffer, int offset, size_t length)
    {
        if (length > storeBuffer_.size())
        {
            if (isCanGrow_)
            {
                UpsizeBuffer(length);
            }
            else
            {
                std::cerr << "Sliding buffer push overflow";
                return;
            }
        }

        size_t storeTop = storeBuffer_.size();

        if (length > FreeSpace())
        {
            if (isCanGrow_ == false)
            {
                // Move low pointer up to free enough space for incoming data
                size_t moveLength = length - FreeSpace() + 1;

                positionLow_ += moveLength;
                if (positionLow_ >= storeTop)
                    positionLow_ -= storeTop;
            }
            else
            {
                UpsizeBuffer();
                storeTop = storeBuffer_.size();
            }
        }

        if (storeTop - positionHigh_ >= length)
        {
            std::copy(buffer.cbegin() + offset, buffer.cbegin() + offset + length, storeBuffer_.begin() + positionHigh_);
            positionHigh_ += length;
        }
        else
        {
            size_t firstChunkLength = storeTop - positionHigh_;
            std::copy(buffer.cbegin() + offset, buffer.cbegin() + offset + firstChunkLength, storeBuffer_.begin() + positionHigh_);

            size_t secondChunkLength = length - firstChunkLength;
            std::copy(buffer.cbegin() + offset + firstChunkLength,
                      buffer.cbegin() + offset + firstChunkLength + secondChunkLength,
                      storeBuffer_.begin());

            positionHigh_ = secondChunkLength;
        }

        dataStored_ += length;
    }

    void Pop(std::vector<T>& buffer, int offset, int length) { PopInternal(buffer, offset, length, false); }

    void Purge(int samplesCount)
    {
        std::vector<T> nullVector;
        PopInternal(nullVector, 0, samplesCount, true);
    }

    size_t FreeSpace() const { return storeBuffer_.size() - dataStored_; }

    size_t DataLengthSamples() const { return dataStored_; }
};

} // namespace Buffers
} // namespace dePhonica
