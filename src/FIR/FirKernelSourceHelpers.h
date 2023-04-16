#include "EnvelopePoint.h"
#include <vector>
#include <iostream>
#include <algorithm>

namespace dePhonica {
namespace Fir {

class FirKernelSourceHelpers
{
private:
    static void FillHole(std::vector<EnvelopePoint>& envelopePoints, size_t fromPoint, size_t toPoint)
    {
        size_t pointsCount = toPoint - fromPoint;

        auto fromGain = envelopePoints[fromPoint].Gain;
        auto fromPhase = envelopePoints[fromPoint].Phase;

        auto toGain = envelopePoints[toPoint].Gain;
        auto toPhase = envelopePoints[toPoint].Phase;

        auto gainStepPerPoint = (toGain - fromGain) / pointsCount;
        auto phaseStepPerPoint = (toPhase - fromPhase) / pointsCount;

        auto currentGain = fromGain;
        auto currentPhase = fromPhase;

        for (size_t n = fromPoint; n < toPoint; n++)
        {
            envelopePoints[n].Gain = currentGain;
            envelopePoints[n].Phase = currentPhase;

            currentGain += gainStepPerPoint;
            currentPhase += phaseStepPerPoint;
        }
    }

    static void FillDecay(std::vector<EnvelopePoint>& envelopePoints, int fromPoint, int toPoint)
    {
        auto currentGainValue = envelopePoints[fromPoint].Gain;
        auto currentPhaseValue = envelopePoints[fromPoint].Phase;

        int step = fromPoint < toPoint ? 1 : -1;

        for (int n = fromPoint; n != toPoint; n += step)
        {
            envelopePoints[n].Gain = currentGainValue;
            envelopePoints[n].Phase = currentPhaseValue;
        }
    }

public:
    static bool IsEnvelopeNormalizedToGrid(const std::vector<EnvelopePoint>& envelopePoints, double sampleRate)
    {
        if (envelopePoints.size() < 1)
        {
            return true;
        }

        auto frequencyPerPointStep = sampleRate / 2 / envelopePoints.size();
        auto currentFrequency = 0.0;

        for (size_t n = 0; n < envelopePoints.size(); n++)
        {
            if (std::abs(envelopePoints[n].Frequency - currentFrequency) >= 1)
            {
                return false;
            }

            currentFrequency += frequencyPerPointStep;
        }

        return true;
    }

    static std::vector<EnvelopePoint> NormalizeEnvelopeToGrid(const std::vector<EnvelopePoint>& inputPoints, double sampleRate)
    {
        if (IsEnvelopeNormalizedToGrid(inputPoints, sampleRate))
        {
            return inputPoints;
        }

        auto minFrequencyStep = 1000000.0;

        for (size_t n = 0; n < inputPoints.size() - 1; n++)
        {
            auto frequencyStep = std::abs(inputPoints[n + 1].Frequency - inputPoints[n].Frequency);

            if (frequencyStep < minFrequencyStep)
            {
                minFrequencyStep = frequencyStep;
            }
        }

        if (minFrequencyStep > 100000)
        {
            std::cerr << "Unable to normalize envelope to grid - minimal frequency step in input data is too big" << std::endl;
            return inputPoints;
        }

        if (minFrequencyStep < 0.1)
        {
            std::cerr << "Unable to normalize envelope to grid - minimal frequency step in input data is too small" << std::endl;
            return inputPoints;
        }

        int resultPointsCount = (sampleRate / 2 / minFrequencyStep) + 1;

        std::vector<EnvelopePoint> resultEnvelope(resultPointsCount);
        std::vector<uint8_t> filledFlag(resultPointsCount);

        // Fill result frequencies
        double resultFrequency = 0.0;
        for (int n = 0; n < resultPointsCount; n++, resultFrequency += minFrequencyStep)
        {
            resultEnvelope[n].Frequency = (float)resultFrequency;
        }

        // Map input points into target grid
        for (auto& point : inputPoints)
        {
            size_t resultIndex = point.Frequency / minFrequencyStep;

            filledFlag[resultIndex] = 1;
            resultEnvelope[resultIndex].Gain = point.Gain;
            resultEnvelope[resultIndex].Phase = point.Phase;
        }

        int lastFilledIndex = -1;

        for (int n = 0; n < resultPointsCount; n++)
        {
            if (filledFlag[n] != 0)
            {
                if (lastFilledIndex == -1)
                {
                    FillDecay(resultEnvelope, n, -1);
                }
                else if (n - lastFilledIndex > 1)
                {
                    FillHole(resultEnvelope, lastFilledIndex, n);
                }

                lastFilledIndex = n;
            }
        }

        if (lastFilledIndex != -1 && lastFilledIndex < resultPointsCount)
        {
            FillDecay(resultEnvelope, lastFilledIndex, resultPointsCount);
        }

        return resultEnvelope;
    }
};

} // namespace Fir
} // namespace dePhonica
