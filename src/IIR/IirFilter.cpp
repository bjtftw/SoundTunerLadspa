#include "IirFilter.h"

namespace dePhonica {
namespace Iir {

IirFilter::IirFilter(unsigned sampleRate, const IirFilterDescription &filterDescription) :
    filterDescription_(filterDescription)
{
    bandPass_[0] = bandPass_[1] = NULL;
    lowPass_[0] = lowPass_[1] = NULL;
    highPass_[0] = highPass_[1] = NULL;
    lowShelf_[0] = lowShelf_[1] = NULL;
    highShelf_[0] = highShelf_[1] = NULL;
    bandShelf_[0] = bandShelf_[1] = NULL;

    if (filterDescription.IsCrossover)
    {
        auto order = (filterDescription.Order + (filterDescription.Order % 2)) / 2;
        auto gainDb = filterDescription.GainDb / 2;

        CreateFilter(0, filterDescription.FilterType, sampleRate, 
            order, filterDescription.CenterFrequency, filterDescription.BandWidth, gainDb);
        CreateFilter(1, filterDescription.FilterType, sampleRate,
            order, filterDescription.CenterFrequency, filterDescription.BandWidth, gainDb);
    } else
    {
        CreateFilter(0, filterDescription.FilterType, sampleRate,
            filterDescription.Order, filterDescription.CenterFrequency, filterDescription.BandWidth, filterDescription.GainDb);
    }
}

void IirFilter::CreateFilter(int filterIndex, IirFilterTypes filterType, double sampleRate, int order, 
    double centerFrequency, double bandWidth, double gainDb)
{
    switch (filterType)
    {
    case IirFilterTypes::BandPass:
        bandPass_[filterIndex] = new Dsp::SimpleFilter<Dsp::Butterworth::BandPass<16>, 1>();
        bandPass_[filterIndex]->setup(order, sampleRate, centerFrequency, bandWidth);
        break;

    case IirFilterTypes::HighPass:
        highPass_[filterIndex] = new Dsp::SimpleFilter<Dsp::Butterworth::HighPass<16>, 1>();
        highPass_[filterIndex]->setup(order, sampleRate, centerFrequency);
        break;

    case IirFilterTypes::HighShelf:
        highShelf_[filterIndex] = new Dsp::SimpleFilter<Dsp::Butterworth::HighShelf<16>, 1>();
        highShelf_[filterIndex]->setup(order, sampleRate, centerFrequency, gainDb);
        break;

    case IirFilterTypes::LowPass:
        lowPass_[filterIndex] = new Dsp::SimpleFilter<Dsp::Butterworth::LowPass<16>, 1>();
        lowPass_[filterIndex]->setup(order, sampleRate, centerFrequency);
        break;

    case IirFilterTypes::LowShelf:
        lowShelf_[filterIndex] = new Dsp::SimpleFilter<Dsp::Butterworth::LowShelf<16>, 1>();
        lowShelf_[filterIndex]->setup(order, sampleRate, centerFrequency, gainDb);
        break;

    case IirFilterTypes::BandShelf:
        bandShelf_[filterIndex] = new Dsp::SimpleFilter<Dsp::Butterworth::BandShelf<16>, 1>();
        bandShelf_[filterIndex]->setup(order, sampleRate, centerFrequency, bandWidth, gainDb);
        break;
    }
}

IirFilter::~IirFilter()
{
    if (bandPass_[0]) delete bandPass_[0];
    if (bandPass_[1]) delete bandPass_[1];
    if (lowPass_[0]) delete lowPass_[0];
    if (lowPass_[1]) delete lowPass_[1];
    if (highPass_[0]) delete highPass_[0];
    if (highPass_[1]) delete highPass_[1];
    if (lowShelf_[0]) delete lowShelf_[0];
    if (lowShelf_[1]) delete lowShelf_[1];
    if (highShelf_[0]) delete highShelf_[0];
    if (highShelf_[1]) delete highShelf_[1];
    if (bandShelf_[0]) delete bandShelf_[0];
    if (bandShelf_[1]) delete bandShelf_[1];
}

void IirFilter::Flush()
{
    if (bandPass_[0]) bandPass_[0]->reset();
    if (bandPass_[1]) bandPass_[1]->reset();
    if (lowPass_[0]) lowPass_[0]->reset();
    if (lowPass_[1]) lowPass_[1]->reset();
    if (highPass_[0]) highPass_[0]->reset();
    if (highPass_[1]) highPass_[1]->reset();
    if (lowShelf_[0]) lowShelf_[0]->reset();
    if (lowShelf_[1]) lowShelf_[1]->reset();
    if (highShelf_[0]) highShelf_[0]->reset();
    if (highShelf_[1]) highShelf_[1]->reset();
    if (bandShelf_[0]) bandShelf_[0]->reset();
    if (bandShelf_[1]) bandShelf_[1]->reset();
}

void IirFilter::Apply(Buffers::SingleBuffer<PCMTYPE>& processingBuffer)
{
    PCMTYPE *channels[] = {processingBuffer.BufferData().data()};

    switch (filterDescription_.FilterType)
    {
    case IirFilterTypes::BandPass:
        bandPass_[0]->process(processingBuffer.DataLengthSamples(), channels);
        if (bandPass_[1]) bandPass_[1]->process(processingBuffer.DataLengthSamples(), channels);
        break;

    case IirFilterTypes::HighPass:
        highPass_[0]->process(processingBuffer.DataLengthSamples(), channels);
        if (highPass_[1]) highPass_[1]->process(processingBuffer.DataLengthSamples(), channels);
        break;

    case IirFilterTypes::HighShelf:
        highShelf_[0]->process(processingBuffer.DataLengthSamples(), channels);
        if (highShelf_[1]) highShelf_[1]->process(processingBuffer.DataLengthSamples(), channels);
        break;

    case IirFilterTypes::LowPass:
        lowPass_[0]->process(processingBuffer.DataLengthSamples(), channels);
        if (lowPass_[1]) lowPass_[1]->process(processingBuffer.DataLengthSamples(), channels);
        break;

    case IirFilterTypes::LowShelf:
        lowShelf_[0]->process(processingBuffer.DataLengthSamples(), channels);
        if (lowShelf_[1]) lowShelf_[1]->process(processingBuffer.DataLengthSamples(), channels);
        break;

    case IirFilterTypes::BandShelf:
        bandShelf_[0]->process(processingBuffer.DataLengthSamples(), channels);
        if (bandShelf_[1]) bandShelf_[1]->process(processingBuffer.DataLengthSamples(), channels);
        break;
    }
}

} // namespace Iir
} // namespace dePhonica
