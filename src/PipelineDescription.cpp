#include "PipelineDescription.h"

#include <fstream>
#include <locale>

#include "JSON/reader.h"
#include "LogConversions.h"
#include "StringHelpers.h"

namespace dePhonica {
namespace Core {

std::vector<Fir::EnvelopePoint> PipelineDescription::ReadCorrectionEnvelope(const std::string& fileName)
{
    std::ifstream envelopeStream(fileName);
    std::vector<unsigned char> envelopeBuffer(std::istreambuf_iterator<char>(envelopeStream), {});

    int pointsCount = envelopeBuffer[0] * (1 << 24) + envelopeBuffer[1] * (1 << 16) + envelopeBuffer[2] * (1 << 8) + envelopeBuffer[3];

    auto envelopeBytes = envelopeBuffer.data();
    //float averageGain = *reinterpret_cast<float*>(envelopeBytes + 4);

    std::vector<Fir::EnvelopePoint> envelopePoints(pointsCount);

    auto envelopePointer = envelopeBytes + 8;

    for (int n = 0; n < pointsCount; n++)
    {
        envelopePoints[n].Frequency = *reinterpret_cast<float*>(envelopePointer);
        envelopePointer = envelopePointer + 4;

        envelopePoints[n].Gain = *reinterpret_cast<float*>(envelopePointer);
        envelopePointer = envelopePointer + 4;

        envelopePoints[n].Phase = *reinterpret_cast<float*>(envelopePointer);
        envelopePointer = envelopePointer + 4;
    }

    return envelopePoints;
}

const PipelineDescription PipelineDescription::FromFile(std::string descriptionFileName)
{
    std::ifstream jsonDescriptionStream(descriptionFileName);

    json::Object jsonDescription;
    json::Reader::Read(jsonDescription, jsonDescriptionStream);

    PipelineDescription pipelineDescription;

    pipelineDescription.InitialSamplesBuffered = static_cast<json::Number>(jsonDescription["initialSamplesBuffered"]);

    pipelineDescription.CorrectionGain = Math::LogConversions::DecibelsToValue(
        static_cast<json::Number>(jsonDescription["correctionGainDb"]));

    auto correctionEnvelopeFile = static_cast<std::string>(static_cast<json::String>(jsonDescription["correctionFile"]));

    if (correctionEnvelopeFile.length() > 0)
    {
        pipelineDescription.CorrectionEnvelope = ReadCorrectionEnvelope(correctionEnvelopeFile);
    }

    pipelineDescription.SubBandProcessings = ReadBandPipelines(jsonDescription);

    if (jsonDescription.Find("preProcess") != jsonDescription.End())
    {
        pipelineDescription.PreProcessing = ReadSubBandDescription(jsonDescription["preProcess"]);
    }

    if (jsonDescription.Find("postProcess") != jsonDescription.End())
    {
        pipelineDescription.MasterProcessing = ReadSubBandDescription(jsonDescription["postProcess"]);
    }

    return pipelineDescription;
}

std::vector<PipelineBandDescription> PipelineDescription::ReadBandPipelines(json::Object& jsonDescription)
{
    std::vector<PipelineBandDescription> subBandProcessings;

    if (jsonDescription.Find("subBands") != jsonDescription.End())
    {
        auto& subBands = static_cast<json::Array&>(jsonDescription["subBands"]);

        for (auto subBandIterator = subBands.Begin(); subBandIterator != subBands.End(); subBandIterator++)
        {
            auto subBand = static_cast<json::Object>(*subBandIterator);
            subBandProcessings.push_back(ReadSubBandDescription(subBand));
        }
    }

    return subBandProcessings;
}

PipelineBandDescription PipelineDescription::ReadSubBandDescription(json::Object subBand)
{
    PipelineBandDescription bandDescription;

    if (subBand.Find("iirFilters") != subBand.End())
    {
        bandDescription.IirFilters = ReadIirFilters(static_cast<json::Array&>(subBand["iirFilters"]));
    }

    if (subBand.Find("compressors") != subBand.End())
    {
        bandDescription.Compressors = ReadCompressors(static_cast<json::Array&>(subBand["compressors"]));
    }

    if (subBand.Find("autoGain") != subBand.End())
    {
        bandDescription.AutoGain = ReadAutoGain(static_cast<json::Object>(subBand["autoGain"]));
    }

    if (subBand.Find("flags") != subBand.End())
    {
        ProcessFlags(static_cast<json::String>(subBand["flags"]), bandDescription);
    }

    return bandDescription;
}

Gain::AutoGainDescription PipelineDescription::ReadAutoGain(const json::Object& autoGainJson)
{
    Gain::AutoGainDescription autoGainDescription;
    autoGainDescription.IsBypassed = false;

    for (auto tokenIterator = autoGainJson.Begin(); tokenIterator != autoGainJson.End(); tokenIterator++)
    {
        auto& autoMember = *tokenIterator;
        auto name = String::toLower(autoMember.name);

        if (name == "ismaster")
        {
            autoGainDescription.IsMaster = static_cast<json::Boolean>(autoMember.element);
        }

        if (name == "binding")
        {
            autoGainDescription.Binding = static_cast<json::String>(autoMember.element);
        }

        if (name == "gainstepvariable")
        {
            autoGainDescription.GainStepVariableName = static_cast<json::String>(autoMember.element);
        }

        if (name == "gainincreasethresholddb")
        {
            autoGainDescription.GainIncreaseThresholdDb = static_cast<json::Number>(autoMember.element);
        }

        if (name == "gainreducethresholddb")
        {
            autoGainDescription.GainReduceThresholdDb = static_cast<json::Number>(autoMember.element);
        }

        if (name == "gainstepdb")
        {
            autoGainDescription.GainStepValueDb = static_cast<json::Number>(autoMember.element);
        }

        if (name == "maxgainsteps")
        {
            autoGainDescription.MaxGainSteps = static_cast<json::Number>(autoMember.element);
        }

        if (name == "gainincreaseperiodms")
        {
            autoGainDescription.GainIncreasePeriodMs = static_cast<json::Number>(autoMember.element);
        }
    }

    return autoGainDescription;
}

void PipelineDescription::ProcessFlags(std::string flags, PipelineBandDescription& pipelineBandDescription)
{
    auto flagsLowerCase = String::toLower(flags);

    if (flagsLowerCase.find("invert"))
    {
        pipelineBandDescription.IsInverted = true;
    }
}

std::vector<Dynamics::CompressorDescription> PipelineDescription::ReadCompressors(json::Array& compressorDescriptions)
{
    std::vector<Dynamics::CompressorDescription> compressors;

    for (auto compressorIterator = compressorDescriptions.Begin(); compressorIterator != compressorDescriptions.End(); compressorIterator++)
    {
        auto compressor = static_cast<json::Object>(*compressorIterator);

        Dynamics::CompressorDescription compressorDescription;

        compressorDescription.IsRmsDetector = true;
        compressorDescription.IsUpward = false;
        compressorDescription.AreSidechainChannelsAveraged = false;
        compressorDescription.AttackMilliseconds = 20;
        compressorDescription.ReleaseMilliseconds = 250;
        compressorDescription.MakeupGainDb = 0.0;
        compressorDescription.SideChainGainDb = 0.0;
        compressorDescription.ThresholdDb = -18;
        compressorDescription.Ratio = 2.0;
        compressorDescription.Knee = 2.82843;

        for (auto tokenIterator = compressor.Begin(); tokenIterator != compressor.End(); tokenIterator++)
        {
            auto& compressorMember = *tokenIterator;
            auto name = String::toLower(compressorMember.name);

            if (name == "isrmsdetector")
            {
                compressorDescription.IsRmsDetector = static_cast<json::Boolean>(compressorMember.element);
            }

            if (name == "isupward")
            {
                compressorDescription.IsUpward = static_cast<json::Boolean>(compressorMember.element);
            }

            if (name == "isupward")
            {
                compressorDescription.IsUpward = static_cast<json::Boolean>(compressorMember.element);
            }

            if (name == "aresidechainchannelsaveraged")
            {
                compressorDescription.AreSidechainChannelsAveraged = static_cast<json::Boolean>(compressorMember.element);
            }

            if (name == "attackms")
            {
                compressorDescription.AttackMilliseconds = static_cast<json::Number>(compressorMember.element);
            }

            if (name == "releasems")
            {
                compressorDescription.ReleaseMilliseconds = static_cast<json::Number>(compressorMember.element);
            }

            if (name == "makeupgaindb")
            {
                compressorDescription.MakeupGainDb = static_cast<json::Number>(compressorMember.element);
            }

            if (name == "sidechaingaindb")
            {
                compressorDescription.SideChainGainDb = static_cast<json::Number>(compressorMember.element);
            }

            if (name == "thresholddb")
            {
                compressorDescription.ThresholdDb = static_cast<json::Number>(compressorMember.element);
            }

            if (name == "ratio")
            {
                compressorDescription.Ratio = static_cast<json::Number>(compressorMember.element);
            }

            if (name == "knee")
            {
                compressorDescription.Knee = static_cast<json::Number>(compressorMember.element);
            }
        }

        compressors.push_back(compressorDescription);

        printf("Compressor is rms: %d, attack ms: %f release ms: %f, threshold db: %f, ratio: %f, knee: %f, makeup gain db: %f, side chain "
               "gain db: %f, is upward: %d, are sc averaged: %d\n",
               (int)compressorDescription.IsRmsDetector,
               compressorDescription.AttackMilliseconds,
               compressorDescription.ReleaseMilliseconds,
               compressorDescription.ThresholdDb,
               compressorDescription.Ratio,
               compressorDescription.Knee,
               compressorDescription.MakeupGainDb,
               compressorDescription.SideChainGainDb,
               (int)compressorDescription.IsUpward,
               (int)compressorDescription.AreSidechainChannelsAveraged);
    }

    return compressors;
}

std::vector<Iir::IirFilterDescription> PipelineDescription::ReadIirFilters(json::Array& iirFilterDescriptions)
{
    std::vector<Iir::IirFilterDescription> iirFilters;

    std::vector<std::string> filterTypeStrings = { "lowpass", "highpass", "bandpass", "lowshelf", "highshelf", "bandshelf" };

    for (auto iirFilterIterator = iirFilterDescriptions.Begin(); iirFilterIterator != iirFilterDescriptions.End(); iirFilterIterator++)
    {
        auto iirFilter = static_cast<json::Object>(*iirFilterIterator);

        Iir::IirFilterDescription iirDescription;

        iirDescription.IsCrossover = false;
        iirDescription.FilterType = Iir::IirFilterTypes::HighPass;
        iirDescription.CenterFrequency = 1000;
        iirDescription.BandWidth = 100;
        iirDescription.GainDb = 0;
        iirDescription.Order = 2;

        for (auto tokenIterator = iirFilter.Begin(); tokenIterator != iirFilter.End(); tokenIterator++)
        {
            auto& iirMember = *tokenIterator;
            auto name = String::toLower(iirMember.name);

            if (name == "iscrossover")
            {
                iirDescription.IsCrossover = static_cast<json::Boolean>(iirMember.element);
            }

            if (name == "type")
            {
                std::string typeString = String::toLower(static_cast<json::String>(iirMember.element));

                for (size_t n = 0; n < filterTypeStrings.size(); n++)
                {
                    if (filterTypeStrings[n] == typeString)
                    {
                        iirDescription.FilterType = static_cast<Iir::IirFilterTypes>(n);
                        break;
                    }
                }
            }

            if (name == "order")
            {
                iirDescription.Order = static_cast<json::Number>(iirMember.element);
            }

            if (name == "centerfrequencyhz")
            {
                iirDescription.CenterFrequency = static_cast<json::Number>(iirMember.element);
            }

            if (name == "bandwidth")
            {
                iirDescription.BandWidth = static_cast<json::Number>(iirMember.element);
            }

            if (name == "gaindb")
            {
                iirDescription.GainDb = static_cast<json::Number>(iirMember.element);
            }
        }

        /*
        printf("IIR type: %s (%d), order: %d, center: %.2f, width: %.2f, gain: %.2f, is crossover: %d\n",
            filterTypeStrings[(int) iirDescription.FilterType].c_str(),
            (int) iirDescription.FilterType,
            (int) iirDescription.Order,
            iirDescription.CenterFrequency, iirDescription.BandWidth, iirDescription.GainDb, 
            (int) iirDescription.IsCrossover);*/

        iirFilters.push_back(iirDescription);
    }

    return iirFilters;
}

} // namespace Core
} // namespace dePhonica
