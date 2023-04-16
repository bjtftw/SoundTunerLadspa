#pragma once

#include "Buffers/SingleBuffer.h"
#include "Configuration.h"
#include "Gain/AutoGainDescription.h"
#include "Reflection/PipelineReflection.h"

namespace dePhonica {
namespace Gain {

class AutoGain
{
private:
    unsigned sampleRate_;
    const AutoGainDescription& autoGainDescription_;
    Core::PipelineReflection& pipelineReflection_;

    float gainIncreaseThreshold_, gainReduceThreshold_;

    int gainStepIndex_;
    int samplesFromLastGainIncrease_, samplesPerGainIncrease_;

    void ApplyGain(Buffers::SingleBuffer<PCMTYPE>& inputBuffer, double initialGain, double finalGain);

public:
    AutoGain(unsigned sampleRate, const AutoGainDescription& autoGainDescription, Core::PipelineReflection& pipelineReflection);

    void Apply(Buffers::SingleBuffer<PCMTYPE>& inputBuffer);

    void Flush() 
    {
        gainStepIndex_ = 0;
    }
};

} // namespace Gain
} // namespace dePhonica
