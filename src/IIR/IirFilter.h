#pragma once

#include "IirFilterDescription.h"
#include "Buffers/SingleBuffer.h"
#include "Configuration.h"

#include "DspFilters/Dsp.h"

namespace dePhonica {
namespace Iir {

class IirFilter
{
private:
    const IirFilterDescription& filterDescription_;

    Dsp::SimpleFilter <Dsp::Butterworth::BandPass <16>, 1> *bandPass_[2];
	Dsp::SimpleFilter <Dsp::Butterworth::LowPass <16>, 1> *lowPass_[2];
	Dsp::SimpleFilter <Dsp::Butterworth::HighPass <16>, 1> *highPass_[2];
	Dsp::SimpleFilter <Dsp::Butterworth::LowShelf <16>, 1> *lowShelf_[2];    
	Dsp::SimpleFilter <Dsp::Butterworth::HighShelf <16>, 1> *highShelf_[2];
    Dsp::SimpleFilter <Dsp::Butterworth::BandShelf <16>, 1> *bandShelf_[2];

    void CreateFilter(int filterIndex, IirFilterTypes filterType, double sampleRate, int order,
        double centerFrequency, double bandWidth, double gainDb);

public:
    IirFilter(unsigned sampleRate, const IirFilterDescription& filterDescription);
    ~IirFilter();

    void Apply(Buffers::SingleBuffer<PCMTYPE>& processingBuffer);
    void Flush();
};

} // namespace Iir
} // namespace dePhonica