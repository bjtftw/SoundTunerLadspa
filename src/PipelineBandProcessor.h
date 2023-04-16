#pragma once

#include <vector>
#include <memory>

#include "PipelineDescription.h"
#include "IIR/IirFilter.h"
#include "Dynamics/Compressor.h"
#include "Buffers/SingleBuffer.h"
#include "Gain/AutoGain.h"

#include "Configuration.h"

namespace dePhonica {
namespace Core {

class PipelineBandProcessor
{
private:
    bool isInverted_;

    std::vector<std::unique_ptr<Iir::IirFilter>> iirFilters_;
    std::vector<std::unique_ptr<Dynamics::Compressor>> compressorInstances_;

    Gain::AutoGain autoGainInstance_;

    Buffers::SingleBuffer<PCMTYPE> outputBuffer_;

    void InitFilters(unsigned sampleRate, const std::vector<Iir::IirFilterDescription>& filterDescriptions);
    void InitCompressors(unsigned sampleRate, const std::vector<Dynamics::CompressorDescription>& compressorDescriptions);

public:
    PipelineBandProcessor(unsigned sampleRate, const PipelineBandDescription& bandDescription, PipelineReflection& pipelineReflection);
    void Push(const Buffers::SingleBuffer<PCMTYPE>& inputBuffer);

    const Buffers::SingleBuffer<PCMTYPE>& Pop() const
    {
        return outputBuffer_;
    }

    void Flush()
    {
        for (auto& iirFilter : iirFilters_)
        {
            iirFilter->Flush();
        }

        for (auto& compressor : compressorInstances_)
        {
            compressor->Flush();
        }
    }
};

} // namespace Core
} // namespace dePhonica
