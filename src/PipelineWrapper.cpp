#include "PipelineWrapper.h"

namespace dePhonica {
namespace Core {

PipelineWrapper::PipelineWrapper(unsigned int sampleRate, const std::string& configFileJson)
    : pipelineDescription_(PipelineDescription::FromFile(configFileJson))
    , pipeline_(sampleRate, pipelineDescription_)
    , isInitBuffer_(true)
{
}

void PipelineWrapper::Process(size_t samplesCount)
{
    inputBuffer_.Copy(portDataLocation[(int)PipelineWrapperPorts::Input], samplesCount);

    /*
    const auto& dataConst = inputBuffer_.BufferDataConst();
    for (size_t n = 0; n < samplesCount; n++)
    {
        if (dataConst[n] != portDataLocation[(int)PipelineWrapperPorts::Input][n])
        {
            printf("Not equal data: %f and %f\n",
                dataConst[n], portDataLocation[(int)PipelineWrapperPorts::Input][n]);
        }
    }*/

    pipeline_.Push(inputBuffer_);

    auto& resultBuffer = pipeline_.Pop();
    auto resultData = resultBuffer.BufferDataConst().data();
    auto resultSamplesCount = resultBuffer.DataLengthSamples();

    auto outputDataLocation = portDataLocation[(int)PipelineWrapperPorts::Output];

    if (resultSamplesCount < samplesCount)
    {
        memset(outputDataLocation, 0, samplesCount * sizeof(PCMTYPE));

        auto diffSamples = samplesCount - resultSamplesCount;

        if (isInitBuffer_)
        {
            memcpy(&outputDataLocation[diffSamples], resultData, resultSamplesCount * sizeof(PCMTYPE));
        }
        else
        {
            memcpy(outputDataLocation, resultData, resultSamplesCount * sizeof(PCMTYPE));
        }
    }
    else
    {
        memcpy(outputDataLocation, resultData, resultSamplesCount * sizeof(PCMTYPE));
    }

    if (isInitBuffer_ && resultSamplesCount > 0)
    {
        isInitBuffer_ = false;
    }
}

} // namespace Core
} // namespace dePhonica
