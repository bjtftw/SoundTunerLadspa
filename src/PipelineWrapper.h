#pragma once

#include <string>

#include "Ladspa/src/ladspa.h"
#include "Pipeline.h"

namespace dePhonica {
namespace Core {

enum class PipelineWrapperPorts
{
    Input = 0,
    Output
};
const int PipelineWrapperPortsCount = 2;

class PipelineWrapper
{
private:
    PipelineDescription pipelineDescription_;
    Pipeline pipeline_;

    LADSPA_Data* portDataLocation[PipelineWrapperPortsCount];

    Buffers::SingleBuffer<PCMTYPE> inputBuffer_;

    bool isInitBuffer_;

public:
    PipelineWrapper(unsigned int sampleRate, const std::string& configFileJson);

    void SetPortDataLocation(PipelineWrapperPorts port, LADSPA_Data* dataLocation) { portDataLocation[(int)port] = dataLocation; }

    void Flush()
    {
        isInitBuffer_ = true;
        pipeline_.Flush();
    }

    void Process(size_t samplesCount);
};

} // namespace Core
} // namespace dePhonica