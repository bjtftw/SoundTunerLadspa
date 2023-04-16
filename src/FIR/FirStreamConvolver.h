#pragma once

#include "Buffers/SlidingBuffer.h"
#include "Configuration.h"
#include "FirBlockConvolver.h"
#include "FirKernelSource.h"

namespace dePhonica {
namespace Fir {

class FirStreamConvolver
{
private:
    Buffers::SlidingBuffer<PCMTYPE> collectBuffer_, resultBuffer_;
    FirBlockConvolver blockConvolver_;

    std::vector<PCMTYPE> inputProcessingBuffer_, outputProcessingBuffer_;

    bool isPreBuffering_;
    size_t initialSamplesBuffered_;

public:
    FirStreamConvolver(const FirKernelSource& kernelSource, size_t initialSamplesBuffered = 0);

    void Flush()
    {
        collectBuffer_.Flush();
        resultBuffer_.Flush();
        blockConvolver_.Flush();
    }

    size_t Convolve(const std::vector<PCMTYPE>& inputBuffer, std::vector<PCMTYPE>& outputBuffer, size_t samplesCount, float gain);
};

} // namespace Fir
} // namespace dePhonica
