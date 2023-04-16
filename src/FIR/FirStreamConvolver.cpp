#include "FirStreamConvolver.h"

namespace dePhonica {
namespace Fir {

static size_t StreamConvolverInitBufferSize = 65536 * 2;

FirStreamConvolver::FirStreamConvolver(const FirKernelSource& kernelSource, size_t initialSamplesBuffered)
    : collectBuffer_(StreamConvolverInitBufferSize, true)
    , resultBuffer_(StreamConvolverInitBufferSize, true)
    , blockConvolver_(kernelSource)
    , inputProcessingBuffer_(blockConvolver_.ChunkSize())
    , outputProcessingBuffer_(blockConvolver_.ChunkSize())
    , isPreBuffering_(true)
    , initialSamplesBuffered_(initialSamplesBuffered)
{
}

size_t FirStreamConvolver::Convolve(const std::vector<PCMTYPE>& inputBuffer,
                                    std::vector<PCMTYPE>& outputBuffer,
                                    size_t samplesCount,
                                    float gain)
{
    collectBuffer_.Push(inputBuffer, 0, samplesCount);

    size_t expectedSize = blockConvolver_.ChunkSize();
    if (isPreBuffering_)
    {
        expectedSize += blockConvolver_.ChunkSize() / 2 + samplesCount * 2;

        if (initialSamplesBuffered_ > 0)
        {
            expectedSize = initialSamplesBuffered_;
        }
    }

    while (collectBuffer_.DataLengthSamples() >= expectedSize)
    {
        isPreBuffering_ = false;

        collectBuffer_.Pop(inputProcessingBuffer_, 0, inputProcessingBuffer_.size());

        size_t convolvedSamples = blockConvolver_.Convolve(inputProcessingBuffer_, outputProcessingBuffer_, gain);

        resultBuffer_.Push(outputProcessingBuffer_, 0, convolvedSamples);
    }

    size_t resultLength = std::min(resultBuffer_.DataLengthSamples(), samplesCount);

    if (resultLength > 0)
    {
        resultBuffer_.Pop(outputBuffer, 0, resultLength);
    }

    return resultLength;
}

} // namespace Fir
} // namespace dePhonica
