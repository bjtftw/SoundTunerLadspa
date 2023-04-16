#pragma once

#include <string>
#include <vector>

#include "JSON/reader.h"
#include "Dynamics/CompressorDescription.h"
#include "FIR/EnvelopePoint.h"
#include "IIR/IirFilterDescription.h"
#include "Gain/AutoGainDescription.h"

#include "Configuration.h"

namespace dePhonica {
namespace Core {

struct PipelineBandDescription
{
    bool IsInverted = false;

    std::vector<Iir::IirFilterDescription> IirFilters;
    std::vector<Dynamics::CompressorDescription> Compressors;
    Gain::AutoGainDescription AutoGain;
};

struct PipelineDescription
{
private:
    static std::vector<Fir::EnvelopePoint> ReadCorrectionEnvelope(const std::string& fileName);
    static std::vector<PipelineBandDescription> ReadBandPipelines(json::Object& jsonDescription);

    static PipelineBandDescription ReadSubBandDescription(json::Object subBand);
    static std::vector<Iir::IirFilterDescription> ReadIirFilters(json::Array& iirFilterDescriptions);
    static std::vector<Dynamics::CompressorDescription> ReadCompressors(json::Array& compressorDescriptions);
    static Gain::AutoGainDescription ReadAutoGain(const json::Object& autoGainJson);
        
    static void ProcessFlags(std::string flags, PipelineBandDescription& pipelineBandDescription);

public:
    size_t InitialSamplesBuffered = 0;

    float CorrectionGain = 1.0;
    std::vector<Fir::EnvelopePoint> CorrectionEnvelope;

    PipelineBandDescription PreProcessing;

    std::vector<PipelineBandDescription> SubBandProcessings;

    PipelineBandDescription MasterProcessing;

    int PeakMonitoringPeriodSeconds = 10;

    const static PipelineDescription FromFile(std::string descriptionFileName);
};

} // namespace Core
} // namespace dePhonica
