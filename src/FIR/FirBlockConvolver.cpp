#include "FirBlockConvolver.h"

#include "KernelConverter.h"

namespace dePhonica {
namespace Fir {

FirBlockConvolver::FirBlockConvolver(const FirKernelSource& kernelSource)
    : taps_(kernelSource.GetTaps())
    , chunkSize_(taps_)
    , fftSize_(KernelConverter::GetDesiredSizeOfFft(taps_, chunkSize_))
    , fftEngine_(fftSize_)
    , storingBuffer_(fftSize_)
    , processingBuffer_(fftSize_)
    , isFirstRun_(true)
{
    Flush();

    if (kernelSource.GetPoints().size() < 1)
    {
        return;
    }

    auto kernelImpulse = KernelConverter::ComplexKernelToImpulseResponse(kernelSource.ToComplexKernel());

    std::vector<PCMTYPE> paddedImpulse(fftSize_);
    std::copy(kernelImpulse.begin(), kernelImpulse.end(), paddedImpulse.begin());

    auto complexKernel = KernelConverter::ImpulseResponseToComplexKernel(paddedImpulse);

    fftEngine_.SetConvolutionKernel(complexKernel);
}

void FirBlockConvolver::Flush()
{
    isFirstRun_ = true;

    std::fill(storingBuffer_.begin(), storingBuffer_.end(), 0);
    std::fill(processingBuffer_.begin(), processingBuffer_.end(), 0);
}

size_t FirBlockConvolver::Convolve(const std::vector<PCMTYPE>& inputBuffer, std::vector<PCMTYPE>& outputBuffer, float gain)
{
    std::copy(processingBuffer_.begin() + chunkSize_, processingBuffer_.end(), processingBuffer_.begin());
    std::copy(inputBuffer.begin(), inputBuffer.end(), processingBuffer_.begin() + chunkSize_);

    fftEngine_.ExecuteConvolution(processingBuffer_, storingBuffer_);

    size_t filteredLength = isFirstRun_ ? chunkSize_ / 2 : chunkSize_;
    size_t sourcePointer = taps_ - 1 + (isFirstRun_ ? chunkSize_ / 2 : 0);

    for (size_t targetPointer = 0; targetPointer < filteredLength; targetPointer++, sourcePointer++)
    {
        outputBuffer[targetPointer] = storingBuffer_[sourcePointer] * gain;
    }

    isFirstRun_ = false;
    return filteredLength;
}

} // namespace Fir
} // namespace dePhonica
