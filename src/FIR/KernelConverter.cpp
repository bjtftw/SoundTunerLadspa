#include "KernelConverter.h"

#include "MathDefines.h"

#include "FftEngineFftw.h"
#include "WindowFunctions.h"

namespace dePhonica {
namespace Fir {

std::vector<std::complex<PCMTYPE>> KernelConverter::EnvelopeToComplexKernel(const std::vector<EnvelopePoint>& envelope)
{
    if (envelope.size() < 1)
    {
        return std::vector<std::complex<PCMTYPE>>();
    }

    auto size = (envelope.size() - 1) * 2;

    auto referenceKernel = GetReferenceKernel(size);

    for (size_t n = 0; n < referenceKernel.size(); n++)
    {
        auto gain = std::abs(referenceKernel[n]);
        auto phase = std::arg(referenceKernel[n]);

        gain *= envelope[n].Gain;
        phase += envelope[n].Phase;

        referenceKernel[n] = std::complex<PCMTYPE>(gain * std::cos(phase), gain * std::sin(phase));
    }

    return referenceKernel;
}

std::vector<EnvelopePoint> KernelConverter::ComplexKernelToEnvelope(const std::vector<std::complex<PCMTYPE>>& complexKernel)
{
    if (IsDebug && (complexKernel.size() - 1) % 2 != 0)
    {
        std::cerr << "Unable to assembly complex kernel from envelopes - invalid size of input array (must be odd)" << std::endl;
        return std::vector<EnvelopePoint>();
    }

    auto resultEnvelope = std::vector<EnvelopePoint>(complexKernel.size());

    auto size = (complexKernel.size() - 1) * 2;
    auto referenceKernel = GetReferenceKernel(size);

    for (size_t n = 0; n < complexKernel.size(); n++)
    {
        auto referenceGain = std::abs(referenceKernel[n]);
        auto referencePhase = std::arg(referenceKernel[n]);

        auto gain = std::abs(complexKernel[n]);
        auto phase = std::arg(complexKernel[n]);

        phase = phase - referencePhase;

        if (phase > M_PIf32)
            phase -= 2 * M_PIf32;
        if (phase < M_PIf32)
            phase += 2 * M_PIf32;

        resultEnvelope[n].Gain = gain / referenceGain;
        resultEnvelope[n].Phase = phase;
    }

    return resultEnvelope;
}

std::vector<PCMTYPE> KernelConverter::ComplexKernelToImpulseResponse(const std::vector<std::complex<PCMTYPE>>& complexKernel)
{
    if (complexKernel.size() < 1)
    {
        return std::vector<PCMTYPE>();
    }

    auto size = (complexKernel.size() - 1) * 2;

    FftEngine fftEngine(size);

    std::vector<PCMTYPE> impulseResponse(size);

    fftEngine.ExecuteC2R(complexKernel, impulseResponse);

    WindowFunctions windowFunction(WindowFunctionTypes::Blackman, size);
    windowFunction.Apply(impulseResponse, 1.0f / size);

    return impulseResponse;
}

std::vector<std::complex<PCMTYPE>> KernelConverter::ImpulseResponseToComplexKernel(const std::vector<PCMTYPE>& impulseResponse)
{
    if (IsDebug && impulseResponse.size() % 2 != 0)
    {
        std::cerr << "Unable to convert impulse response to complex kernel - invalid size of input array (must be even)";
    }

    FftEngine fftEngine(impulseResponse.size());

    std::vector<std::complex<PCMTYPE>> complexKernel(fftEngine.GetComplexSize());

    fftEngine.ExecuteR2C(impulseResponse, complexKernel);

    return complexKernel;
}

std::vector<std::complex<PCMTYPE>> KernelConverter::GetReferenceKernel(size_t size)
{
    std::vector<PCMTYPE> referenceImpulse(size);
    referenceImpulse[size / 2] = 1.0;

    return ImpulseResponseToComplexKernel(referenceImpulse);
}

std::vector<PCMTYPE> KernelConverter::ResizeDoubles(const std::vector<PCMTYPE>& source, size_t targetPointsCount)
{
    std::vector<PCMTYPE> target(targetPointsCount);

    if (targetPointsCount == source.size())
    {
        target = source;
        return target;
    }

    float scale = float(targetPointsCount) / source.size();

    if (scale > 1)
    {
        // Interpolate
        float sourcePosition = 0;
        float sourceStep = float(source.size() - 1) / (targetPointsCount - 1);
        auto sourceLength = source.size();

        for (size_t n = 0; n < targetPointsCount; n++)
        {
            unsigned intIndex = static_cast<unsigned>(sourcePosition);
            float fracIndex = sourcePosition - intIndex;

            auto intIndexNext = intIndex + 1;
            if (intIndexNext >= sourceLength)
            {
                intIndexNext = intIndex;
            }

            auto currentValue = source[intIndex];
            auto nextValue = source[intIndexNext];

            target[n] = currentValue * (1 - fracIndex) + nextValue * fracIndex;

            sourcePosition += sourceStep;
        }
    }
    else
    {
        // 1. Upsample
        auto upsampleLength = targetPointsCount * 2;

        while (upsampleLength < source.size())
        {
            upsampleLength *= 2;
        }

        auto upsamples = ResizeDoubles(source, upsampleLength);

        // 2. Decimate
        auto m = upsamples.size() / targetPointsCount;

        float offset = 0.0;
        float offsetStep = float(m) / targetPointsCount;

        for (size_t i = 0; i < targetPointsCount; i++)
        {
            target[i] = upsamples[static_cast<size_t>(i * m + offset)];
            offset += offsetStep;
        }

        target[target.size() - 1] = source[source.size() - 1];
    }

    return target;
}

std::vector<std::complex<PCMTYPE>> KernelConverter::ResizeComplexKernel(const std::vector<std::complex<PCMTYPE>>& sourceKernel,
                                                                        size_t targetSize)
{
    auto sourceEnvelope = ComplexKernelToEnvelope(sourceKernel);

    std::vector<PCMTYPE> sourceGains(sourceEnvelope.size()), sourcePhases(sourceEnvelope.size());

    for (size_t n = 0; n < sourceEnvelope.size(); n++)
    {
        sourceGains[n] = sourceEnvelope[n].Gain;
        sourcePhases[n] = sourceEnvelope[n].Phase;
    }

    auto targetGains = ResizeDoubles(sourceGains, targetSize);
    auto targetPhases = ResizeDoubles(sourcePhases, targetSize);

    std::vector<EnvelopePoint> targetEnvelope(targetSize);

    for (size_t n = 0; n < targetSize; n++)
    {
        targetEnvelope[n].Gain = targetGains[n];
        targetEnvelope[n].Phase = targetPhases[n];
    }

    return EnvelopeToComplexKernel(targetEnvelope);
}

size_t KernelConverter::GetDesiredSizeOfFft(size_t kernelSize, size_t chunkSize)
{
    if (IsDebug && chunkSize > kernelSize)
    {
        std::cerr << "Unable to execute convolution for chunk which is longer than convolution kernel";
    }

    int minimalFftSize = kernelSize + chunkSize - 1;

    int fftSize = 1;

    while (fftSize < minimalFftSize)
    {
        fftSize *= 2;
    }

    return fftSize;
}

} // namespace Fir
} // namespace dePhonica
