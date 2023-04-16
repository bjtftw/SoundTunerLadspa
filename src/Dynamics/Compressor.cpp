#include "Compressor.h"
#include "LogConversions.h"

#include <algorithm>
#include <stdio.h>

namespace dePhonica {
namespace Dynamics {

Compressor::Compressor(unsigned sampleRate, const CompressorDescription& compressorDescription)
    : compressorDescription_(compressorDescription)
    , linearSlope_(0.0)
    , attackCoefficient_(MACROMIN(1.0, 1.0 / (compressorDescription_.AttackMilliseconds * sampleRate / 4000.0)))
    , releaseCoefficient_(MACROMIN(1.0, 1.0 / (compressorDescription_.ReleaseMilliseconds * sampleRate / 4000.0)))
    , threshold_(Math::LogConversions::DecibelsToValue(compressorDescription.ThresholdDb))
    , linearKneeStart_(threshold_ / std::sqrt(compressorDescription.Knee))
    , adjustedKneeStart_(linearKneeStart_ * linearKneeStart_)
    , linearKneeStop_(threshold_ * std::sqrt(compressorDescription.Knee))
    , adjustedKneeStop_(linearKneeStop_ * linearKneeStop_)
    , kneeStart_(std::log(linearKneeStart_))
    , kneeStop_(std::log(linearKneeStop_))
    , compressedKneeStart_((kneeStart_ - threshold_) / compressorDescription.Ratio + threshold_)
    , compressedKneeStop((kneeStop_ - threshold_) / compressorDescription.Ratio + threshold_)
    , makeupGain_(Math::LogConversions::DecibelsToValue(compressorDescription.MakeupGainDb))
    , sideChainGain_(Math::LogConversions::DecibelsToValue(compressorDescription.SideChainGainDb))
{
}

// A fake infinity value (because real infinity may break some hosts)
#define FAKE_INFINITY (65536.0 * 65536.0)

// Check for infinity (with appropriate-ish tolerance)
#define IS_FAKE_INFINITY(value) (fabs(value - FAKE_INFINITY) < 1.0)

static inline double HermiteInterpolation(double x, double x0, double x1, double p0, double p1, double m0, double m1)
{
    double width = x1 - x0;
    double t = (x - x0) / width;
    double t2, t3;
    double ct0, ct1, ct2, ct3;

    m0 *= width;
    m1 *= width;

    t2 = t * t;
    t3 = t2 * t;
    ct0 = p0;
    ct1 = m0;

    ct2 = -3 * p0 - 2 * m0 + 3 * p1 - m1;
    ct3 = 2 * p0 + m0 - 2 * p1 + m1;

    return ct3 * t3 + ct2 * t2 + ct1 * t + ct0;
}

static double CalculateOutputGain(double linearSlope,
                                  double ratio,
                                  double threshold,
                                  double knee,
                                  double kneeStart,
                                  double kneeStop,
                                  double compressedKneeStart,
                                  double compressedKneeStop,
                                  bool isRmsDetector,
                                  bool isUpward)
{
    double slope = log(linearSlope);
    double gain = 0.0;
    double delta = 0.0;

    if (isRmsDetector)
    {
        slope *= 0.5;
    }

    if (IS_FAKE_INFINITY(ratio))
    {
        gain = threshold;
        delta = 0.0;
    }
    else
    {
        gain = (slope - threshold) / ratio + threshold;
        delta = 1.0 / ratio;
    }

    if (isUpward)
    {
        if (knee > 1.0 && slope > kneeStart)
            gain = HermiteInterpolation(slope, kneeStop, kneeStart, kneeStop, compressedKneeStart, 1.0, delta);
    }
    else
    {
        if (knee > 1.0 && slope < kneeStop)
            gain = HermiteInterpolation(slope, kneeStart, kneeStop, kneeStart, compressedKneeStop, 1.0, delta);
    }

    return exp(gain - slope);
}

void Compressor::ApplyCompression(const Buffers::SingleBuffer<PCMTYPE>& inputBuffer, Buffers::SingleBuffer<PCMTYPE>& outputBuffer)
{
    const size_t inputSamplesCount = inputBuffer.DataLengthSamples();
    const int sourceChannelCount = inputBuffer.Channels();

    const bool isMaxChannelSample = compressorDescription_.AreSidechainChannelsAveraged == false;
    const bool isRmsDetector = compressorDescription_.IsRmsDetector;
    const bool isUpward = compressorDescription_.IsUpward;

    const double kneeStartLinear = isRmsDetector ? adjustedKneeStart_ : linearKneeStart_;
    const double kneeStopLinear = isRmsDetector ? adjustedKneeStop_ : linearKneeStop_;

    const float ratio = compressorDescription_.Ratio;
    const float knee = compressorDescription_.Knee;

    auto& sourceSamples = inputBuffer.BufferDataConst();
    auto& targetSamples = outputBuffer.BufferData();

    size_t samplePointer = 0;

    for (size_t iz = 0; iz < inputSamplesCount; iz++)
    {
        double abs_sample = std::fabs(sourceSamples[samplePointer] * sideChainGain_);

        if (isMaxChannelSample)
        {
            for (int c = 1; c < sourceChannelCount; c++)
            {
                abs_sample = MACROMAX(std::fabs(sourceSamples[samplePointer + c] * sideChainGain_), abs_sample);
            }
        }
        else
        {
            for (int c = 1; c < sourceChannelCount; c++)
            {
                abs_sample += std::fabs(sourceSamples[samplePointer + c] * sideChainGain_);
            }

            abs_sample /= sourceChannelCount;
        }

        if (isRmsDetector)
        {
            abs_sample *= abs_sample;
        }

        linearSlope_ += (abs_sample - linearSlope_) * (abs_sample > linearSlope_ ? attackCoefficient_ : releaseCoefficient_);

        bool detected = isUpward ? linearSlope_ < kneeStopLinear : linearSlope_ > kneeStartLinear;

        double gain = (linearSlope_ > 0.0 && detected) ? CalculateOutputGain(linearSlope_,
                                                                             ratio,
                                                                             threshold_,
                                                                             knee,
                                                                             kneeStart_,
                                                                             kneeStop_,
                                                                             compressedKneeStart_,
                                                                             compressedKneeStop,
                                                                             isRmsDetector,
                                                                             isUpward)
                                                       : 1.0;

        for (int c = 0; c < sourceChannelCount; c++)
        {
            targetSamples[samplePointer + c] = sourceSamples[samplePointer + c] * gain * makeupGain_;
        }

        samplePointer += sourceChannelCount;
    }
}

void Compressor::Apply(Buffers::SingleBuffer<PCMTYPE>& inputBuffer)
{
    ApplyCompression(inputBuffer, inputBuffer);
}

} // namespace Dynamics
} // namespace dePhonica
