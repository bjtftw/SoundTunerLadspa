#pragma once

#include <string>

#include "Configuration.h"

namespace dePhonica {
namespace Gain {

struct AutoGainDescription
{
    bool IsBypassed = true;
    bool IsMaster;

    std::string Binding;
    std::string GainStepVariableName;

    float GainIncreaseThresholdDb = -6;
    float GainReduceThresholdDb = -2;
    float GainStepValueDb = -1.5;
    int MaxGainSteps = 10;

    int GainIncreasePeriodMs = 10000;
};

} // namespace Gain
} // namespace dePhonica
