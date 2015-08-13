#if ! defined (Msvis_AveragingTvi2_H_121211_1236)
#define Msvis_AveragingTvi2_H_121211_1236

#include <casa/aips.h>
#include <msvis/MSVis/TransformingVi2.h>
#include <msvis/MSVis/VisibilityIterator2.h>
#include <msvis/MSVis/AveragingVi2Factory.h>

namespace casa {

namespace ms {

class MsRow;

};

namespace vi {

class AveragingOptions;

namespace avg {

class MsRowAvg;
class VbAvg;

}

class AveragingTvi2 : public TransformingVi2 {

public:

    AveragingTvi2 (VisibilityIterator2 * vi, ViImplementation2 * inputVii,
                   const AveragingParameters & averagingParameters);
    ~AveragingTvi2 ();

    /////////////////////////////////////////////////////////////////////////
    //
    // Chunk/Subchunk structure in the AveragingTvi2
    //
    // The averaging interval, in seconds, is specified at construction time.
    // The interval (i.e., the size of the chunk in time) is also specified
    // at creation time; the interval must be an integer multiple of the
    // averaging interval.
    //
    // The input VI's setting must be compatible with those of the
    // AveragingTvi2.  This means that the chunk size of the input VI must
    // have the same duration as the averaging VI.  Although the input VI
    // and the averaging VI have the same duration, the averaging VI will
    // have fewer subchunks since N input subchunks will be averaged down
    // to create a single output subchunk.
    //
    // The input VI will also define the averaging boundaries by its
    // definition of a chunk.  For example, if the input VI allows data with
    // different scans to be in the same chunk, then they will potentially
    // be averaged together.
    //
    // The input VI must use the data description ID as a sort column so that
    // a chunk will only contain data from a single DDID setting.

    void originChunks (Bool forceRewind = False);
    void nextChunk ();
    Bool moreChunks () const;

    void origin ();
    void next ();
    Bool more () const;

    void writeFlag (const Cube<Bool> & flag);
    void writeFlagRow (const Vector<Bool> & rowflags);

    static inline Float weightToSigma (Float weight)
    {
    	return weight > FLT_MIN ? 1.0 / std::sqrt (weight) : -1.0; // bogosity indicator
    }

    static inline Float sigmaToWeight (Float sigma)
    {
        return sigma > FLT_MIN ? 1.0 / (sigma * sigma) : 0.0; // bad sample
    }


    static Vector<Float> average (const Matrix<Float> &data, const Matrix<Bool> &flags);
    static Matrix<Float> average (const Cube<Float> &data, const Cube<Bool> &flags);

protected:

    void advanceInputVii ();
    Int determineDdidToUse () const;
    Bool inputExceedsTimeDistance (ms::MsRow * rowInput, avg::MsRowAvg * rowAveraged);
    Bool inputExceedsUvwDistance (ms::MsRow * rowInput, avg::MsRowAvg * rowAveraged);
    void produceSubchunk ();
    void processInputSubchunk (const VisBuffer2 *);
    Bool reachedAveragingBoundary();
    void captureIterationInfo (const VisBuffer2 * vb2);
    bool subchunksReady () const;
    void validateInputVi (ViImplementation2 *);

private:

    const Double averagingInterval_p; // averaging interval in seconds
    AveragingOptions averagingOptions_p;
    AveragingParameters averagingParameters_p;
    Int ddidLastUsed_p; // ddId last used to produce a subchunk.
    Bool inputViiAdvanced_p; // true if input VII was advanced but data not used
    Bool more_p;
    Subchunk subchunk_p;
    Bool subchunkExists_p;
    avg::VbAvg * vbAvg_p;
    WeightScaling * weightScaling_p;
    uInt startBuffer_p;
    uInt endBuffer_p;
};

} // end namespace vi

} // end namespace casa

#endif // ! defined (Msvis_AveragingTvi2_H_121211_1236)
