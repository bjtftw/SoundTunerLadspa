#pragma once

#include <vector>
#include <memory>

#include "PipelineDescription.h"
#include "FIR/FirCorrector.h"
#include "PipelineBandProcessor.h"
#include "Buffers/SingleBuffer.h"
#include "Reflection/PipelineReflection.h"

#include "Configuration.h"

namespace dePhonica {
namespace Core {

class Pipeline
{
private:
    Fir::FirCorrector firCorrector_;

    PipelineReflection pipelineReflection_;
    
    PipelineBandProcessor preProcessor_;
    std::vector<std::unique_ptr<PipelineBandProcessor>> bandProcessors_;
    PipelineBandProcessor masterProcessor_;

    Buffers::SingleBuffer<PCMTYPE> intermediateBuffer_;

    void InitProcessings(unsigned sampleRate, const std::vector<PipelineBandDescription>& bandDescriptions);

public:
    Pipeline(unsigned sampleRate, const PipelineDescription& pipelineDescription);
    void Push(const Buffers::SingleBuffer<PCMTYPE>& inputBuffer);

    const Buffers::SingleBuffer<PCMTYPE>& Pop() const
    {
        return masterProcessor_.Pop();
    }

    void Flush()
    {
        firCorrector_.Flush();

        for (auto &bandProcessor : bandProcessors_)
        {
            bandProcessor->Flush();
        }

        masterProcessor_.Flush();
    }
};

} // namespace Core
} // namespace dePhonica
