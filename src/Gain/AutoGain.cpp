#include "AutoGain.h"
#include "LogConversions.h"

namespace dePhonica {
namespace Gain {

AutoGain::AutoGain(unsigned sampleRate, const AutoGainDescription& autoGainDescription, Core::PipelineReflection& pipelineReflection)
    : sampleRate_(sampleRate)
    , autoGainDescription_(autoGainDescription)
    , pipelineReflection_(pipelineReflection)
    , gainIncreaseThreshold_(Math::LogConversions::DecibelsToValue(autoGainDescription.GainIncreaseThresholdDb))
    , gainReduceThreshold_(Math::LogConversions::DecibelsToValue(autoGainDescription.GainReduceThresholdDb))
    , gainStepIndex_(0)
    , samplesFromLastGainIncrease_(0)
    , samplesPerGainIncrease_(autoGainDescription.GainIncreasePeriodMs * sampleRate / 1000)
{
}

void AutoGain::Apply(Buffers::SingleBuffer<PCMTYPE>& inputBuffer)
{
    if (autoGainDescription_.IsBypassed)
    {
        return;
    }

    double initialGain = Math::LogConversions::DecibelsToValue(autoGainDescription_.GainStepValueDb * gainStepIndex_);

    if (autoGainDescription_.IsMaster)
    {
        auto measuredValue = pipelineReflection_.GetPeakLevel(autoGainDescription_.Binding);

        //printf("Peak value: %f for binding %s\n", measuredValue, autoGainDescription_.Binding.c_str());

        if (measuredValue > gainReduceThreshold_ && gainStepIndex_ < autoGainDescription_.MaxGainSteps)
        {
            gainStepIndex_++;
            pipelineReflection_.FlushPeakLevel(autoGainDescription_.Binding);
            pipelineReflection_.SetVariable(autoGainDescription_.GainStepVariableName, gainStepIndex_);

            samplesFromLastGainIncrease_ = 0;

            //printf("Gain down, step: %d\n", gainStepIndex_);
        }
        else if (measuredValue < gainIncreaseThreshold_ && gainStepIndex_ > 0 && samplesFromLastGainIncrease_ >= samplesPerGainIncrease_)
        {
            gainStepIndex_--;
            pipelineReflection_.FlushPeakLevel(autoGainDescription_.Binding);
            pipelineReflection_.SetVariable(autoGainDescription_.GainStepVariableName, gainStepIndex_);

            samplesFromLastGainIncrease_ = 0;

            //printf("Gain up, step: %d\n", gainStepIndex_);
        }
    }
    else
    {
        gainStepIndex_ = pipelineReflection_.GetVariable(autoGainDescription_.GainStepVariableName);
    }

    samplesFromLastGainIncrease_ += inputBuffer.DataLengthSamples();

    double finalGain = Math::LogConversions::DecibelsToValue(autoGainDescription_.GainStepValueDb * gainStepIndex_);

    ApplyGain(inputBuffer, initialGain, finalGain);
}

void AutoGain::ApplyGain(Buffers::SingleBuffer<PCMTYPE>& inputBuffer, double initialGain, double finalGain)
{
    auto& dataSamples = inputBuffer.BufferData();
    size_t samplesCount = inputBuffer.DataLengthSamples();

    if (std::abs(finalGain - initialGain) < 0.0001)
    {
        // constant gain
        for (size_t n = 0; n < samplesCount; n++)
        {
            dataSamples[n] *= initialGain;
        }
    }
    else
    {
        // fading
        double currentGain = initialGain;
        double gainStep = (finalGain - initialGain) / samplesCount;

        for (size_t n = 0; n < samplesCount; n++, currentGain += gainStep)
        {
            dataSamples[n] *= static_cast<float>(currentGain);
        }
    }
}

} // namespace Gain
} // namespace dePhonica
