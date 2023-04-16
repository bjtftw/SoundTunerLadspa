#pragma once

#include <vector>

#include "Configuration.h"

#include "FftEngineFftw.h"
#include "FirKernelSource.h"

namespace dePhonica {
namespace Fir {

class FirBlockConvolver
{
private:
    size_t taps_, chunkSize_, fftSize_;

    FftEngine fftEngine_;

    std::vector<PCMTYPE> storingBuffer_, processingBuffer_;

    bool isFirstRun_;

public:
    explicit FirBlockConvolver(const FirKernelSource& kernelSource);

    void Flush();
    size_t Convolve(const std::vector<PCMTYPE>& inputBuffer, std::vector<PCMTYPE>& outputBuffer, float gain);

    size_t ChunkSize() const
    {
        return chunkSize_;
    }
};

} // namespace Fir
} // namespace dePhonica
