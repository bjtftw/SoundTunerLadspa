#include "FirCorrector.h"
#include "KernelConverter.h"

namespace dePhonica {
namespace Fir {

FirCorrector::FirCorrector(unsigned sampleRate, const std::vector<EnvelopePoint>& filterEnvelope, float gain, size_t initialSamplesBuffered)
    : streamConvolver_(FirKernelSource(sampleRate, filterEnvelope), initialSamplesBuffered)
    , gain_(gain)
    , isDisabled_(filterEnvelope.size() < 1)
{
}

void FirCorrector::Push(const Buffers::SingleBuffer<PCMTYPE>& inputBuffer)
{
    if (isDisabled_)
    {
        outputBuffer_.Copy(inputBuffer);
        outputBuffer_.Amplify(gain_);
        return;
    }

    outputBuffer_.Ensure(inputBuffer.DataLengthSamples());

    outputBuffer_.DataLengthSamples(
        streamConvolver_.Convolve(inputBuffer.BufferDataConst(), outputBuffer_.BufferData(), inputBuffer.DataLengthSamples(), gain_));
}

} // namespace Fir
} // namespace dePhonica
