#pragma once

#include "DspFilters/Dsp.h"

namespace dePhonica {
namespace Iir {

enum class IirFilterTypes
{
    LowPass = 0,
    HighPass,
    BandPass,
    LowShelf,
    HighShelf,
    BandShelf
};

struct IirFilterDescription
{
    bool IsCrossover;
    IirFilterTypes FilterType;

    int Order;
    double CenterFrequency;
    double BandWidth;
    double GainDb;
};

} // namespace Iir
} // namespace dePhonica