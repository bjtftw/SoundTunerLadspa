#pragma once

#include <algorithm>
#include <map>
#include <string>

#include "Buffers/SingleBuffer.h"
#include "Configuration.h"

#define MEASURE_CHUNKS_COUNT 4

namespace dePhonica {
namespace Core {

struct PeakLevelAccumulator
{
    float GetLevel() { return LevelHistory.size() > 0 ? *std::max_element(LevelHistory.begin(), LevelHistory.end()) : 0; }

    std::vector<float> LevelHistory;
    size_t SamplesCount = 0;
};

class PipelineReflection
{
private:
    unsigned sampleRate_;
    size_t peakMonitoringPeriodSeconds_;

    std::map<std::string, PeakLevelAccumulator> peakLevels_;
    std::map<std::string, float> variables_;

    float MeasurePeaks(const Buffers::SingleBuffer<PCMTYPE>& samplesBuffer)
    {
        auto& dataSamples = samplesBuffer.BufferDataConst();

        size_t samplesInBuffer = samplesBuffer.DataLengthSamples();
        size_t samplesPerChunk = samplesInBuffer / MEASURE_CHUNKS_COUNT;

        size_t sampleIndex = 0;
        float averageSample = 0;

        for (int chunk = 0; chunk < MEASURE_CHUNKS_COUNT; chunk++)
        {
            float peakSample = 0;

            for (size_t n = 0; n < samplesPerChunk && sampleIndex < samplesInBuffer; n++, sampleIndex++)
            {
                auto sample = std::abs(dataSamples[sampleIndex]);

                if (sample > peakSample)
                {
                    peakSample = sample;
                }
            }

            averageSample += peakSample;
        }

        return averageSample / MEASURE_CHUNKS_COUNT;
    }

public:
    PipelineReflection(unsigned sampleRate, int peakMonitoringPeriodSeconds)
        : sampleRate_(sampleRate)
        , peakMonitoringPeriodSeconds_(peakMonitoringPeriodSeconds)
    {
    }

    void PushPeakLevel(const std::string bindingName, const Buffers::SingleBuffer<PCMTYPE>& samplesBuffer)
    {
        float currentPeak = MeasurePeaks(samplesBuffer);

        if (peakLevels_.find(bindingName) != peakLevels_.end())
        {
            auto& levelAccumulator = peakLevels_[bindingName];

            levelAccumulator.SamplesCount += samplesBuffer.DataLengthSamples();

            if (levelAccumulator.SamplesCount >= sampleRate_)
            {
                levelAccumulator.SamplesCount = 0;

                if (levelAccumulator.LevelHistory.size() >= peakMonitoringPeriodSeconds_)
                {
                    levelAccumulator.LevelHistory.erase(levelAccumulator.LevelHistory.begin());
                }

                levelAccumulator.LevelHistory.push_back(currentPeak);
            }
            else
            {
                auto& levelHistory = levelAccumulator.LevelHistory;
                size_t historySize = levelHistory.size();

                if (currentPeak > levelHistory[historySize - 1])
                {
                    levelHistory[historySize - 1] = currentPeak;
                }
            }
        }
        else
        {
            PeakLevelAccumulator levelAccumulator;
            levelAccumulator.LevelHistory.push_back(currentPeak);
            levelAccumulator.SamplesCount = samplesBuffer.DataLengthSamples();

            peakLevels_[bindingName] = levelAccumulator;
        }
    }

    float GetPeakLevel(const std::string bindingName)
    {
        auto filteredPeakValueIterator = peakLevels_.find(bindingName);

        return filteredPeakValueIterator != peakLevels_.end() ? (*filteredPeakValueIterator).second.GetLevel() : 0;
    }

    void FlushPeakLevel(const std::string bindingName)
    {
        if (peakLevels_.find(bindingName) != peakLevels_.end())
        {
            auto& levelAccumulator = peakLevels_[bindingName];
            levelAccumulator.LevelHistory.clear();
            levelAccumulator.LevelHistory.push_back(0);
        }
    }

    void SetVariable(const std::string variableName, float value)
    {
        variables_[variableName] = value;
    }

    float GetVariable(const std::string variableName)
    {
        auto variableIterator = variables_.find(variableName);
        return variableIterator != variables_.end() ? (*variableIterator).second : 0.0;
    }
};

} // namespace Core
} // namespace dePhonica
