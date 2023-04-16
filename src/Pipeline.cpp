#include "Pipeline.h"

namespace dePhonica {
namespace Core {

Pipeline::Pipeline(unsigned sampleRate, const PipelineDescription& pipelineDescription)
    : firCorrector_(sampleRate, pipelineDescription.CorrectionEnvelope, pipelineDescription.CorrectionGain, 
        pipelineDescription.InitialSamplesBuffered)
    , pipelineReflection_(sampleRate, pipelineDescription.PeakMonitoringPeriodSeconds)        
    , preProcessor_(sampleRate, pipelineDescription.PreProcessing, pipelineReflection_)
    , masterProcessor_(sampleRate, pipelineDescription.MasterProcessing, pipelineReflection_)
{
    InitProcessings(sampleRate, pipelineDescription.SubBandProcessings);
}

void Pipeline::InitProcessings(unsigned sampleRate, const std::vector<PipelineBandDescription>& bandDescriptions)
{
    for (const auto& bandDescription : bandDescriptions)
    {
        bandProcessors_.push_back(std::make_unique<PipelineBandProcessor>(sampleRate, bandDescription, pipelineReflection_));
    }
}

void Pipeline::Push(const Buffers::SingleBuffer<PCMTYPE>& inputBuffer)
{
    pipelineReflection_.PushPeakLevel("input", inputBuffer);

    // Overall envelope correction
    firCorrector_.Push(inputBuffer);
    const auto& correctedBuffer = firCorrector_.Pop();

    preProcessor_.Push(correctedBuffer);
    const auto& preProcessedBuffer = preProcessor_.Pop();

    pipelineReflection_.PushPeakLevel("preprocessed", preProcessedBuffer);

    // Band processors
    intermediateBuffer_.Ensure(inputBuffer.DataLengthSamples());

    if (bandProcessors_.size() > 0)
    {
        int subBandIndex = 1;
        bool isFirstBuffer = true;

        for (auto& bandProcessor : bandProcessors_)
        {
            bandProcessor->Push(preProcessedBuffer);

            const auto& bandBuffer = bandProcessor->Pop();
            pipelineReflection_.PushPeakLevel("subband" + subBandIndex, bandBuffer);
            subBandIndex++;

            if (isFirstBuffer)
            {
                isFirstBuffer = false;
                intermediateBuffer_.Copy(bandBuffer);
            }
            else
            {
                intermediateBuffer_.Mix(bandBuffer);
            }
        }
    } else
    {
        intermediateBuffer_.Copy(preProcessedBuffer);
    }

    pipelineReflection_.PushPeakLevel("mixed", intermediateBuffer_);

    // Master correction
    masterProcessor_.Push(intermediateBuffer_);
    pipelineReflection_.PushPeakLevel("postprocessed", masterProcessor_.Pop());
}

} // namespace Core
} // namespace dePhonica
