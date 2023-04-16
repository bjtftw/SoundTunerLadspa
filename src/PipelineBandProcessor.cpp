#include "PipelineBandProcessor.h"

namespace dePhonica {
namespace Core {

PipelineBandProcessor::PipelineBandProcessor(unsigned sampleRate,
                                             const PipelineBandDescription& bandDescription,
                                             PipelineReflection& pipelineReflection)
    : isInverted_(bandDescription.IsInverted)
    , autoGainInstance_(sampleRate, bandDescription.AutoGain, pipelineReflection)
{
    InitFilters(sampleRate, bandDescription.IirFilters);
    InitCompressors(sampleRate, bandDescription.Compressors);
}

void PipelineBandProcessor::InitFilters(unsigned sampleRate, const std::vector<Iir::IirFilterDescription>& filterDescriptions)
{
    for (const auto& filterDescription : filterDescriptions)
    {
        iirFilters_.push_back(std::make_unique<Iir::IirFilter>(sampleRate, filterDescription));
    }
}

void PipelineBandProcessor::InitCompressors(unsigned sampleRate, const std::vector<Dynamics::CompressorDescription>& compressorDescriptions)
{
    for (const auto& compressorDescription : compressorDescriptions)
    {
        compressorInstances_.push_back(std::make_unique<Dynamics::Compressor>(sampleRate, compressorDescription));
    }
}

void PipelineBandProcessor::Push(const Buffers::SingleBuffer<PCMTYPE>& inputBuffer)
{
    outputBuffer_.Copy(inputBuffer);

    for (auto& iirFilter : iirFilters_)
    {
        iirFilter->Apply(outputBuffer_);
    }

    for (auto& compressor : compressorInstances_)
    {
        compressor->Apply(outputBuffer_);
    }

    autoGainInstance_.Apply(outputBuffer_);

    if (isInverted_)
    {
        outputBuffer_.Invert();
    }
}

} // namespace Core
} // namespace dePhonica
