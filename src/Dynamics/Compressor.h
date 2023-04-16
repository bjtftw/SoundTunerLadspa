#pragma once

#include "Buffers/SingleBuffer.h"
#include "CompressorDescription.h"
#include "Configuration.h"

#define COMPRESSION_GRAPH_POINTS        8193
#define COMPRESSION_GRAPH_MIN           (-100.0)
#define ATTACK_RELEASE_CONST_DB         10.0

namespace dePhonica {
namespace Dynamics {

struct CompressionGraphPoint
{
    float StartXDb, StartYDb;
    float Gain;
};

#define MACROMAX(a,b) ((a) > (b) ? (a) : (b))
#define MACROMIN(a,b) ((a) > (b) ? (b) : (a))

class Compressor
{
private:
    const CompressorDescription compressorDescription_;

    double linearSlope_;
    double attackCoefficient_, releaseCoefficient_;

    double threshold_;
    double linearKneeStart_, adjustedKneeStart_;
    double linearKneeStop_, adjustedKneeStop_;
    double kneeStart_, kneeStop_;
    double compressedKneeStart_, compressedKneeStop;    

    float makeupGain_, sideChainGain_;

    Buffers::SingleBuffer<PCMTYPE> processingBuffer_, gainBuffer_;

    void ApplyCompression(const Buffers::SingleBuffer<PCMTYPE>& inputBuffer, Buffers::SingleBuffer<PCMTYPE>& outputBuffer);

public:
    Compressor(unsigned sampleRate, const CompressorDescription& compressorDescription);

    void Apply(Buffers::SingleBuffer<PCMTYPE>& inputBuffer);

    void Flush() 
    {
        processingBuffer_.DataLengthSamples(0);
        linearSlope_ = 0.0;
    }
};

} // namespace Dynamics
} // namespace dePhonica
