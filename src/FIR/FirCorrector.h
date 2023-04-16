#pragma once

#include <vector>

#include "EnvelopePoint.h"
#include "Buffers/SingleBuffer.h"
#include "FirStreamConvolver.h"

#include "Configuration.h"

namespace dePhonica {
namespace Fir {

class FirCorrector
{
private:
    Buffers::SingleBuffer<PCMTYPE> outputBuffer_;

    FirStreamConvolver streamConvolver_;

    float gain_;

    bool isDisabled_;

public:
    FirCorrector(unsigned sampleRate, const std::vector<EnvelopePoint>& filterEnvelope, float gain, 
        size_t initialSamplesBuffered);

    void Push(const Buffers::SingleBuffer<PCMTYPE>& inputBuffer);

    const Buffers::SingleBuffer<PCMTYPE>& Pop() const
    {
        return outputBuffer_;
    }

    void Flush()
    {
        streamConvolver_.Flush();
    }
};
    
} // namespace Fir
} // namespace dePhonica