#pragma once

#include <vector>
#include "Configuration.h"
#include "LogConversions.h"

namespace dePhonica {
namespace Dynamics {

struct DynamicCurvePoint
{
    float InputDb;
    float OutputDb;
};

struct CompressorDescription
{
    float SideChainGainDb;
    float MakeupGainDb;

    bool IsUpward;
    bool IsRmsDetector;
    bool AreSidechainChannelsAveraged;

    float AttackMilliseconds;
    float ReleaseMilliseconds;

    float Knee;

    float ThresholdDb;
    float Ratio;
};

} // namespace Dynamics
} // namespace dePhonica
