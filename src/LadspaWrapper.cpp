#include <iostream>
#include <vector>
#include <string.h>

#include "Ladspa/src/ladspa.h"
#include "PipelineWrapper.h"

using namespace dePhonica::Core;

#define PORT_INPUT ((int)PipelineWrapperPorts::Input)
#define PORT_OUTPUT ((int)PipelineWrapperPorts::Output)

static LADSPA_Handle instantiatePipeline(const LADSPA_Descriptor* ladspaDescriptor, unsigned long sampleRate)
{
    auto configFileJson = static_cast<const char*>(ladspaDescriptor->ImplementationData);

    return new PipelineWrapper(sampleRate, configFileJson);
}

static void connectPortToPipeline(LADSPA_Handle instance, unsigned long port, LADSPA_Data* dataLocation)
{
    if (port < PipelineWrapperPortsCount)
    {
        auto pipelineWrapper = static_cast<PipelineWrapper*>(instance);
        pipelineWrapper->SetPortDataLocation((PipelineWrapperPorts)port, dataLocation);
    }
}

static void activatePipeline(LADSPA_Handle instance)
{
    auto pipelineWrapper = static_cast<PipelineWrapper*>(instance);
    pipelineWrapper->Flush();
}

static void runPipeline(LADSPA_Handle instance, unsigned long samplesCount)
{
    auto pipelineWrapper = static_cast<PipelineWrapper*>(instance);
    pipelineWrapper->Process(samplesCount);
}

static void cleanupPipeline(LADSPA_Handle instance)
{
    auto pipelineWrapper = static_cast<PipelineWrapper*>(instance);
    delete pipelineWrapper;
}

/****************************************************************************/

class LadspaWrapper
{
private:
    std::vector<LADSPA_Descriptor*> ladspaDescriptors_;

    static inline char* LocalStrdup(const char* input)
    {
        char* output = new char[strlen(input) + 1];
        strcpy(output, input);
        return output;
    }

    void ConstructPorts(LADSPA_Descriptor* ladspaDescriptor)
    {
        ladspaDescriptor->PortCount = PipelineWrapperPortsCount;

        auto ladspaPortDescriptors = new LADSPA_PortDescriptor[PipelineWrapperPortsCount];
        ladspaPortDescriptors[PORT_INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
        ladspaPortDescriptors[PORT_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

        ladspaDescriptor->PortDescriptors = ladspaPortDescriptors;

        // Port names
        auto portNames = new char*[PipelineWrapperPortsCount];
        portNames[PORT_INPUT] = LocalStrdup("Input audio stream");
        portNames[PORT_OUTPUT] = LocalStrdup("Output audio stream");

        ladspaDescriptor->PortNames = portNames;

        // Port range hint
        auto portRanges = new LADSPA_PortRangeHint[PipelineWrapperPortsCount];
        portRanges[PORT_INPUT].HintDescriptor = 0;
        portRanges[PORT_OUTPUT].HintDescriptor = 0;

        ladspaDescriptor->PortRangeHints = portRanges;
    }

    void ConstructDescriptors(const char* label, const char* configFileJson)
    {
        auto ladspaDescriptor = new LADSPA_Descriptor();
        if (ladspaDescriptor == NULL)
        {
            std::cerr << "Unable to allocate new LADSPA descriptor" << std::endl;
            return;
        }

        ladspaDescriptor->UniqueID = 8286 + ladspaDescriptors_.size();
        ladspaDescriptor->Properties = 0;
        ladspaDescriptor->Label = LocalStrdup(label);
        ladspaDescriptor->Name = LocalStrdup("Audio processor and crossover");
        ladspaDescriptor->Maker = LocalStrdup("Max Klimenko @ dePhonica sound labs");
        ladspaDescriptor->Copyright = LocalStrdup("Mail.RU");
        ladspaDescriptor->ImplementationData = LocalStrdup(configFileJson);

        ConstructPorts(ladspaDescriptor);

        ladspaDescriptor->instantiate = instantiatePipeline;
        ladspaDescriptor->connect_port = connectPortToPipeline;
        ladspaDescriptor->activate = activatePipeline;
        ladspaDescriptor->run = runPipeline;
        ladspaDescriptor->run_adding = NULL;
        ladspaDescriptor->set_run_adding_gain = NULL;
        ladspaDescriptor->deactivate = NULL;
        ladspaDescriptor->cleanup = cleanupPipeline;

        ladspaDescriptors_.push_back(ladspaDescriptor);
    }

    void DestructDescriptors()
    {
        for (auto descriptor : ladspaDescriptors_)
        {
            if (descriptor == NULL)
            {
                continue;
            }

            delete[] descriptor->Label;
            delete[] descriptor->Name;
            delete[] descriptor->Maker;
            delete[] descriptor->Copyright;
            delete[] descriptor->PortDescriptors;

            for (size_t lIndex = 0; lIndex < descriptor->PortCount; lIndex++)
            {
                delete[] descriptor->PortNames[lIndex];
            }

            delete[] descriptor->PortNames;
            delete[] descriptor->PortRangeHints;
            delete[] static_cast<char*>(descriptor->ImplementationData);
            delete descriptor;
        }

        ladspaDescriptors_.clear();
    }

public:
    LadspaWrapper()
    {
        ConstructDescriptors("deph_crossover_woofer", "/etc/dephonica/woofer.json");
        ConstructDescriptors("deph_crossover_tweeter", "/etc/dephonica/tweeter.json");
    }

    ~LadspaWrapper() { DestructDescriptors(); }

    const LADSPA_Descriptor* GetDescriptor(unsigned long index)
    {
        if (index < ladspaDescriptors_.size())
        {
            return ladspaDescriptors_[index];
        }

        return NULL;
    }
};

/****************************************************************************/

static LadspaWrapper ladspaWrapper;

const LADSPA_Descriptor* ladspa_descriptor(unsigned long index)
{
    return ladspaWrapper.GetDescriptor(index);
}
