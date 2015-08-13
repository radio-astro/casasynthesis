//# MSTransformManager.h: This file contains the interface definition of the MSTransformManager class.
//#
//#  CASA - Common Astronomy Software Applications (http://casa.nrao.edu/)
//#  Copyright (C) Associated Universities, Inc. Washington DC, USA 2011, All rights reserved.
//#  Copyright (C) European Southern Observatory, 2011, All rights reserved.
//#
//#  This library is free software; you can redistribute it and/or
//#  modify it under the terms of the GNU Lesser General Public
//#  License as published by the Free software Foundation; either
//#  version 2.1 of the License, or (at your option) any later version.
//#
//#  This library is distributed in the hope that it will be useful,
//#  but WITHOUT ANY WARRANTY, without even the implied warranty of
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//#  Lesser General Public License for more details.
//#
//#  You should have received a copy of the GNU Lesser General Public
//#  License along with this library; if not, write to the Free Software
//#  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
//#  MA 02111-1307  USA
//# $Id: $

#ifndef MSTransformManager_H_
#define MSTransformManager_H_

// To handle configuration records
#include <casacore/casa/Containers/Record.h>

// To handle variant parameters
#include <stdcasa/StdCasa/CasacSupport.h>

// Measurement Set Selection
#include <ms/MSSel/MSSelection.h>

// Data handling
#include <mstransform/MSTransform/MSTransformDataHandler.h>

// Regridding
#include <mstransform/MSTransform/MSTransformRegridder.h>

// VisibityIterator / VisibilityBuffer framework
#include <msvis/MSVis/VisibilityIterator2.h>
#include <msvis/MSVis/VisBuffer2.h>
#include <msvis/MSVis/ViFrequencySelection.h>

// TVI framework
#include <msvis/MSVis/AveragingVi2Factory.h>
#include <msvis/MSVis/AveragingTvi2.h>

#include <msvis/MSVis/IteratingParameters.h>
#include <msvis/MSVis/LayeredVi2Factory.h>

// To get observatory position from observatory name
#include <measures/Measures/MeasTable.h>

// To post formatted msgs via ostringstream
#include <iomanip>

// To apply hanning smooth
#include <scimath/Mathematics/Smooth.h>

// To apply fft shift
#include <scimath/Mathematics/FFTServer.h>

// To apply 1D interpolations
#include <scimath/Mathematics/InterpolateArray1D.h>

// to compute partial medians
#include <casa/Arrays/ArrayPartMath.h>

namespace casa { //# NAMESPACE CASA - BEGIN

// Forward declarations
class MSTransformBufferImpl;
class MSTransformIterator;
class MSTransformIteratorFactory;

// MS Transform Framework utilities
namespace MSTransformations
{
	// Returns 1/sqrt(wt) or -1, depending on whether wt is positive..
	Double wtToSigma(Double wt);
	Double sigmaToWeight(Double wt);

	enum InterpolationMethod {
	    // nearest neighbour
	    nearestNeighbour,
	    // linear
	    linear,
	    // cubic
	    cubic,
	    // cubic spline
	    spline,
	    // fft shift
	    fftshift
	  };

	enum WeightingSetup {
		spectrum,
		flags,
		cumSum,
		flat,
		flagSpectrum,
		flagCumSum,
		flagsNonZero,
		flagSpectrumNonZero,
		flagCumSumNonZero
	};

	enum dataCol {
		visCube,
		visCubeCorrected,
		visCubeModel,
		visCubeFloat,
		weightSpectrum,
		sigmaSpectrum
	  };

	enum weightTransformation {

		transformWeight,
		transformWeightIntoSigma,
		weightIntoSigma
	};
}

// Forward declarations
struct spwInfo;
struct channelContribution;

// Map definition
typedef map<MS::PredefinedColumns,MS::PredefinedColumns> dataColMap;
typedef map< pair< pair<uInt,uInt> , uInt >,vector<uInt> > baselineMap;
typedef map<uInt,map<uInt, uInt > > inputSpwChanMap;
typedef map<uInt,vector < channelContribution > >  inputOutputChanFactorMap;
typedef map<uInt,pair < spwInfo, spwInfo > > inputOutputSpwMap;

// Struct definition
struct channelInfo {

	Int SPW_id;
	uInt inpChannel;
	uInt outChannel;
	Double CHAN_FREQ;
	Double CHAN_WIDTH;
	Double EFFECTIVE_BW;
	Double RESOLUTION;

	channelInfo()
	{
		SPW_id = -1;
		inpChannel = 0;
		outChannel = 0;

		CHAN_FREQ = -1;
		CHAN_WIDTH = -1;
		EFFECTIVE_BW = -1;
		RESOLUTION = -1;
	}

	bool operator<(const channelInfo& right_operand) const
	{
		if (CHAN_FREQ<right_operand.CHAN_FREQ)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	Double upperBound() const
	{
		return CHAN_FREQ+0.5*CHAN_WIDTH;
	}

	Double lowerBound() const
	{
		return CHAN_FREQ-0.5*CHAN_WIDTH;
	}

	Double overlap(const channelInfo& other) const
	{

		// The other channel completely covers this channel
		if ((lowerBound() <= other.lowerBound()) and (upperBound() >= other.upperBound()))
		{
			return 1.0;
		}
		// The other channel is completely covered by this channel
		else if ((lowerBound() >= other.lowerBound()) and (upperBound() <= other.upperBound()))
		{
			return CHAN_WIDTH/other.CHAN_WIDTH;
		}
		// Lower end of this channel is overlapping with the other channel
		else if (lowerBound() < other.lowerBound() && other.lowerBound() < upperBound() && upperBound() < other.upperBound())
		{
			return (upperBound()-other.lowerBound())/other.CHAN_WIDTH;
		}
		// Upper end of this channel is overlapping with the other channel
		else if (other.lowerBound() < lowerBound() && lowerBound() < other.upperBound() && other.upperBound() < upperBound())
		{
			return (other.upperBound()-lowerBound())/other.CHAN_WIDTH;
		}
		else
		{
			return 0.0;
		}

	}
};

struct channelContribution {

	Int inpSpw;
	uInt inpChannel;
	uInt outChannel;
	Double weight;
	Bool flag;

	channelContribution()
	{
		inpSpw = 0;
		inpChannel = 0;
		outChannel = 0;
		weight = 0;
		flag = False;
	}

	channelContribution(Int inputSpw, uInt inputChannel, uInt outputChannel,Double fraction)
	{
		inpSpw = inputSpw;
		inpChannel = inputChannel;
		outChannel = outputChannel;
		weight = fraction;
		flag = True;
	}
};

struct spwInfo {

	spwInfo()
	{
		initialize(0);
	}

	spwInfo(uInt nChannels)
	{
		initialize(nChannels);
	}

	spwInfo(Vector<Double> &chanFreq,Vector<Double> &chanWidth)
	{
		reset(chanFreq,chanWidth);
	}

	void reset(Vector<Double> &chanFreq,Vector<Double> &chanWidth)
	{
		initialize(chanFreq.size());
		CHAN_FREQ = chanFreq;
		CHAN_WIDTH = chanWidth;
		update();
	}

	void initialize(uInt nChannels)
	{
		NUM_CHAN = nChannels;
		CHAN_FREQ.resize(nChannels,False);
		CHAN_WIDTH.resize(nChannels,False);
		EFFECTIVE_BW.resize(nChannels,False);
		RESOLUTION.resize(nChannels,False);
		CHAN_FREQ_aux.resize(nChannels,False);
		TOTAL_BANDWIDTH = 0;
		REF_FREQUENCY = 0;
		upperBound = 0;
		lowerBound = 0;
	}

	void update()
	{
		upperBound = CHAN_FREQ(NUM_CHAN-1)+0.5*CHAN_WIDTH(NUM_CHAN-1);
		lowerBound = CHAN_FREQ(0)-0.5*CHAN_WIDTH(0);
		TOTAL_BANDWIDTH = upperBound - lowerBound;
		REF_FREQUENCY = CHAN_FREQ(0);

		CHAN_FREQ_aux = CHAN_FREQ;
		EFFECTIVE_BW = CHAN_WIDTH;
		RESOLUTION = CHAN_WIDTH;
	}

	void resize(uInt nChannels)
	{
		NUM_CHAN = nChannels;
		CHAN_FREQ.resize(nChannels,True);
		CHAN_WIDTH.resize(nChannels,True);
		EFFECTIVE_BW.resize(nChannels,True);
		RESOLUTION.resize(nChannels,True);
		CHAN_FREQ_aux.resize(nChannels,True);
		update();
	}

	uInt NUM_CHAN;
	Vector<Double> CHAN_FREQ;
	Vector<Double> CHAN_WIDTH;
	Vector<Double> EFFECTIVE_BW;
	Vector<Double> RESOLUTION;
	Vector<Double> CHAN_FREQ_aux;
	Double TOTAL_BANDWIDTH;
	Double REF_FREQUENCY;
	Double upperBound;
	Double lowerBound;
};

//  MSTransformManager definition
class MSTransformManager
{

	friend class MSTransformBufferImpl;
	friend class MSTransformIterator;
	friend class MSTransformIteratorFactory;

public:

	MSTransformManager();
	MSTransformManager(Record configuration);

	virtual ~MSTransformManager();

	void initialize();
	void configure(Record &configuration);

	void open();
	void setup();
	void close();

	void setupBufferTransformations(vi::VisBuffer2 *vb);
	void fillOutputMs(vi::VisBuffer2 *vb);

	// For buffer handling classes (MSTransformIterator)

	// Needed by MSTransformIteratorFactory
	vi::VisibilityIterator2 * getVisIter() {return visibilityIterator_p;}

	// Needed by MSTransformIterator
	MeasurementSet * getOutputMs () {return outputMs_p;};
	String getOutputMsName () {return outMsName_p;};

	// Needed by MSTransformBuffer
	vi::VisBuffer2 * getVisBuffer() {return visibilityIterator_p->getVisBuffer();}
	IPosition getShape();
	IPosition getTransformedShape(vi::VisBuffer2 *inputVisBuffer);

	// Need by tMSTransformIterator
	dataColMap getDataColMap() { return dataColMap_p;}



protected:

	void parseMsSpecParams(Record &configuration);
	void parseDataSelParams(Record &configuration);
	void parseFreqTransParams(Record &configuration);
	void parseChanAvgParams(Record &configuration);
	void parseRefFrameTransParams(Record &configuration);
	void parseFreqSpecParams(Record &configuration);
	void parseTimeAvgParams(Record &configuration);
	void parseCalParams(Record &configuration);

	// From input MS
	void initDataSelectionParams();
	void getInputNumberOfChannels();

	// To re-grid SPW subtable
	void initRefFrameTransParams();
	void regridSpwSubTable();
	void regridAndCombineSpwSubtable();
	void regridSpwAux(	Int spwId,
						Vector<Double> &inputCHAN_FREQ,
						Vector<Double> &inputCHAN_WIDTH,
						Vector<Double> &originalCHAN_FREQ,
						Vector<Double> &originalCHAN_WIDTH,
						Vector<Double> &regriddedCHAN_FREQ,
						Vector<Double> &regriddedCHAN_WIDTH,
						string msg);

	void reindexColumn(ScalarColumn<Int> &inputCol, Int value);
	void reindexSourceSubTable();
	void reindexDDISubTable();
	void reindexFeedSubTable();
	void reindexSysCalSubTable();
	void reindexFreqOffsetSubTable();
	void reindexGenericTimeDependentSubTable(const String& subtabname);

	void separateSpwSubtable();
	void separateFeedSubtable();
	void separateSourceSubtable();
	void separateSyscalSubtable();
	void separateFreqOffsetSubtable();
	void separateCalDeviceSubtable();
	void separateSysPowerSubtable();


	// Setters for Weight-based transformation
	void propagateWeights(Bool on);
	void setBufferMode(Bool on);
	void setChannelAverageKernel(uInt mode);

	// Drop channels with non-uniform width when doing channel average
	void dropNonUniformWidthChannels();

	// From output MS
	void getOutputNumberOfChannels();

	// For channel averaging and selection
	void calculateIntermediateFrequencies(	Int spwId,
											Vector<Double> &inputChanFreq,
											Vector<Double> &inputChanWidth,
											Vector<Double> &intermediateChanFreq,
											Vector<Double> &intermediateChanWidth);
	void calculateWeightAndSigmaFactors();
	void calculateNewWeightAndSigmaFactors();

	// Column check
	void checkFillFlagCategory();
	void checkFillWeightSpectrum();
	void checkDataColumnsAvailable();
	void checkDataColumnsToFill();
	void colCheckInfo(const String& inputColName, const String& outputColName);

	// Iterator set-up
	virtual void setIterationApproach();
	void generateIterator();

	void initFrequencyTransGrid(vi::VisBuffer2 *vb);
	void fillIdCols(vi::VisBuffer2 *vb,RefRows &rowRef);
	void fillDataCols(vi::VisBuffer2 *vb,RefRows &rowRef);

	void fillWeightCols(vi::VisBuffer2 *vb,RefRows &rowRef);
	void transformAndWriteSpectrum(	vi::VisBuffer2 *vb,
									RefRows &rowRef,
									const Cube<Float> &inputSpectrum,
									ArrayColumn<Float> &outputCubeCol,
									ArrayColumn<Float> &outputMatrixCol,
									MSTransformations::weightTransformation weightTransformation,
									Bool flushSpectrumCube);

	template <class T> void setTileShape(RefRows &rowRef,ArrayColumn<T> &outputDataCol);

	const Cube<Float>& getApplicableSpectrum(vi::VisBuffer2 *vb, MS::PredefinedColumns datacol);
	ArrayColumn<Float>& getOutputWeightColumn(vi::VisBuffer2 *vb, MS::PredefinedColumns datacol);
	const Cube<Float>& getWeightSpectrumFromSigmaSpectrum(vi::VisBuffer2 *vb);
	const Cube<Float>& getWeightSpectrumFlat(vi::VisBuffer2 *vb);

	// Methods to transform and write vectors

	template <class T> void transformAndWriteNotReindexableVector(	const Vector<T> &inputVector,
																	Vector<T> &outputVector,
																	Bool constant,
																	ScalarColumn<T> &outputCol,
																	RefRows &rowReference);

	template <class T> void transformAndWriteReindexableVector(	const Vector<T> &inputVector,
																Vector<T> &outputVector,
																Bool constant,
																map<uInt,uInt> &inputOutputIndexMap,
																ScalarColumn<T> &outputCol,
																RefRows &rowReference);

	Bool transformDDIVector(const Vector<Int> &inputVector,Vector<Int> &outputVector);

	void mapAndAverageVector(	const Vector<Double> &inputVector,
								Vector<Double> &outputVector);

	void mapAndAverageVector(	const Vector<Bool> &inputVector,
								Vector<Bool> &outputVector);

	// Templates methods to transform vectors that must be available for MSTransformBuffer

	template <class T> Bool transformNotReindexableVector(	const Vector<T> &inputVector,
															Vector<T> &outputVector,
															Bool constant)
	{
		Bool transformed = True;

		if ((combinespws_p) or (nspws_p >1))
		{
			if (constant)
			{
				outputVector = inputVector(0);
			}
			else
			{
				mapVector(inputVector,outputVector);
			}
		}
		else
		{
			transformed = False;
		}

		return transformed;
	};

	template <class T> Bool transformReindexableVector(	const Vector<T> &inputVector,
														Vector<T> &outputVector,
														Bool constant,
														map<uInt,uInt> &inputOutputIndexMap)
	{
		Bool transformed = True;

		if (inputOutputIndexMap.size() == 0)
		{
			transformed = transformNotReindexableVector(inputVector,outputVector,constant);
		}
		else
		{
			if (constant)
			{
				outputVector = inputOutputIndexMap[inputVector(0)];
			}
			else if (combinespws_p)
			{
				mapAndReindexVector(inputVector,outputVector,inputOutputIndexMap);
			}
			else
			{
				reindexVector(inputVector,outputVector,inputOutputIndexMap);
			}
		}

		return transformed;
	};

	template <class T> void mapAndReindexVector(	const Vector<T> &inputVector,
													Vector<T> &outputVector,
													map<uInt,uInt> &inputOutputIndexMap)
	{
		if (nspws_p <2)
		{
			for (uInt index=0; index<rowIndex_p.size();index++)
			{
				outputVector(index) = inputOutputIndexMap[inputVector(rowIndex_p[index])];
			}
		}
		else
		{
			uInt absoluteIndex = 0;
			for (uInt index=0; index<rowIndex_p.size();index++)
			{
				for (uInt spwIndex=0;spwIndex < nspws_p; spwIndex++)
				{
					outputVector(absoluteIndex) = inputOutputIndexMap[inputVector(rowIndex_p[index])];
					absoluteIndex += 1;
				}
			}
		}

		return;
	}


	template <class T> void reindexVector(	const Vector<T> &inputVector,
											Vector<T> &outputVector,
											map<uInt,uInt> &inputOutputIndexMap)
	{
		if (nspws_p <2)
		{
			for (uInt index=0; index<inputVector.shape()[0];index++)
			{
				outputVector(index) = inputOutputIndexMap[inputVector(index)];
			}
		}
		else
		{
			uInt absoluteIndex = 0;
			for (uInt index=0; index<inputVector.shape()[0];index++)
			{
				for (uInt spwIndex=0;spwIndex < nspws_p; spwIndex++)
				{
					outputVector(absoluteIndex) = inputOutputIndexMap[inputVector(index)];
					absoluteIndex += 1;
				}
			}
		}

		return;
	};

	template <class T> void mapVector(	const Vector<T> &inputVector,
										Vector<T> &outputVector)
	{
		if (nspws_p < 2)
		{
			for (uInt index=0; index<rowIndex_p.size();index++)
			{
				outputVector(index) = inputVector(rowIndex_p[index]);
			}
		}
		else
		{
			uInt absoluteIndex = 0;
			for (uInt index=0; index<rowIndex_p.size();index++)
			{
				for (uInt spwIndex=0;spwIndex < nspws_p; spwIndex++)
				{
					outputVector(absoluteIndex) = inputVector(rowIndex_p[index]);
					absoluteIndex += 1;
				}
			}
		}


		return;
	}

	// ------------------------------------------------------------------------------------
	// Fill the data from an input matrix with shape [nCol,nBaselinesxnSPWsxnScans/nStates]
	// into an output matrix with shape [nCol,nBaselinesxnScans/nStates]
	// ------------------------------------------------------------------------------------
	template <class T> void mapMatrix(	const Matrix<T> &inputMatrix,Matrix<T> &outputMatrix)
	{
		// Get number of columns
		uInt nCols = outputMatrix.shape()(0);

		for (uInt index=0; index<rowIndex_p.size();index++)
		{
			for (uInt col = 0; col < nCols; col++)
			{
				outputMatrix(col,index) = inputMatrix(col,rowIndex_p[index]);
			}
		}

		return;
	}


	template <class T> void mapAndAverageMatrix(	const Matrix<T> &inputMatrix,
													Matrix<T> &outputMatrix,
													Bool convolveFlags=False,
													vi::VisBuffer2 *vb=NULL);
	template <class T> void mapAndScaleMatrix(	const Matrix<T> &inputMatrix,
												Matrix<T> &outputMatrix,
												map<uInt,T> scaleMap,
												Vector<Int> spws);
	template <class T> void writeMatrix(	const Matrix<T> &inputMatrix,
											ArrayColumn<T> &outputCol,
											RefRows &rowRef,
											uInt nBlocks);

	// Methods to transform and write cubes

	template <class T> void writeCube(	const Cube<T> &inputCube,
										ArrayColumn<T> &outputCol,
										RefRows &rowRef);

	void transformCubeOfData(	vi::VisBuffer2 *vb,
								RefRows &rowRef,
								const Cube<Complex> &inputDataCube,
								ArrayColumn<Complex> &outputDataCol,
								ArrayColumn<Bool> *outputFlagCol,
								const Cube<Float> &inputWeightCube);
	void transformCubeOfData(	vi::VisBuffer2 *vb,
								RefRows &rowRef,
								const Cube<Float> &inputDataCube,
								ArrayColumn<Float> &outputDataCol,
								ArrayColumn<Bool> *outputFlagCol,
								const Cube<Float> &inputWeightCube);
	void (casa::MSTransformManager::*transformCubeOfDataComplex_p)(	vi::VisBuffer2 *vb,
																		RefRows &rowRef,
																		const Cube<Complex> &inputDataCube,
																		ArrayColumn<Complex> &outputDataCol,
																		ArrayColumn<Bool> *outputFlagCol,
																		const Cube<Float> &inputWeightCube);
	void (casa::MSTransformManager::*transformCubeOfDataFloat_p)(	vi::VisBuffer2 *vb,
																		RefRows &rowRef,
																		const Cube<Float> &inputDataCube,
																		ArrayColumn<Float> &outputDataCol,
																		ArrayColumn<Bool> *outputFlagCol,
																		const Cube<Float> &inputWeightCube);

	template <class T> void copyCubeOfData(	vi::VisBuffer2 *vb,
											RefRows &rowRef,
											const Cube<T> &inputDataCube,
											ArrayColumn<T> &outputDataCol,
											ArrayColumn<Bool> *outputFlagCol,
											const Cube<Float> &inputWeightCube);

	template <class T> void combineCubeOfData(	vi::VisBuffer2 *vb,
												RefRows &rowRef,
												const Cube<T> &inputDataCube,
												ArrayColumn<T> &outputDataCol,
												ArrayColumn<Bool> *outputFlagCol,
												const Cube<Float> &inputWeightCube);

	// Methods to transform data in cubes

	void addWeightSpectrumContribution(	Double &weight,
										uInt &pol,
										uInt &inputChannel,
										uInt &row,
										const Cube<Float> &inputWeightsCube);
	void dontAddWeightSpectrumContribution(	Double &weight,
											uInt &pol,
											uInt &inputChannel,
											uInt &row,
											const Cube<Float> &inputWeightsCube);
	void (casa::MSTransformManager::*addWeightSpectrumContribution_p)(	Double &weight,
																			uInt &pol,
																			uInt &inputChannel,
																			uInt &row,
																			const Cube<Float> &inputWeightsCube);


	void fillWeightsPlane(	uInt pol,
							uInt inputChannel,
							uInt outputChannel,
							uInt inputRow,
							const Cube<Float> &inputWeightsCube,
							Matrix<Float> &inputWeightsPlane,
							Double weight);
	void dontfillWeightsPlane(	uInt ,
								uInt ,
								uInt ,
								uInt ,
								const Cube<Float> &,
								Matrix<Float> &,
								Double ) {return;}
	void (casa::MSTransformManager::*fillWeightsPlane_p)(	uInt pol,
																uInt inputChannel,
																uInt outputChannel,
																uInt inputRow,
																const Cube<Float> &inputWeightsCube,
																Matrix<Float> &inputWeightsPlane,
																Double weight);

	void normalizeWeightsPlane(	uInt pol,
								uInt outputChannel,
								Matrix<Float> &inputPlaneWeights,
								Matrix<Double> &normalizingFactorPlane);
	void dontNormalizeWeightsPlane(	uInt ,
									uInt ,
									Matrix<Float> &,
									Matrix<Double> &) {return;}
	void (casa::MSTransformManager::*normalizeWeightsPlane_p)(	uInt pol,
																	uInt outputChannel,
																	Matrix<Float> &inputPlaneWeights,
																	Matrix<Double> &normalizingFactorPlane);

	template <class T> void averageCubeOfData(	vi::VisBuffer2 *vb,
												RefRows &rowRef,
												const Cube<T> &inputDataCube,
												ArrayColumn<T> &outputDataCol,
												ArrayColumn<Bool> *outputFlagCol,
												const Cube<Float> &inputWeightCube);
	template <class T> void smoothCubeOfData(	vi::VisBuffer2 *vb,
												RefRows &rowRef,
												const Cube<T> &inputDataCube,
												ArrayColumn<T> &outputDataCol,
												ArrayColumn<Bool> *outputFlagCol,
												const Cube<Float> &inputWeightCube);
	template <class T> void regridCubeOfData(	vi::VisBuffer2 *vb,
												RefRows &rowRef,
												const Cube<T> &inputDataCube,
												ArrayColumn<T> &outputDataCol,
												ArrayColumn<Bool> *outputFlagCol,
												const Cube<Float> &inputWeightCube);
	template <class T> void separateCubeOfData(	vi::VisBuffer2 *vb,
												RefRows &rowRef,
												const Cube<T> &inputDataCube,
												ArrayColumn<T> &outputDataCol,
												ArrayColumn<Bool> *outputFlagCol,
												const Cube<Float> &inputWeightCube);

	template <class T> void transformAndWriteCubeOfData(	Int inputSpw,
															RefRows &rowRef,
															const Cube<T> &inputDataCube,
															const Cube<Bool> &inputFlagsCube,
															const Cube<Float> &inputWeightsCube,
															IPosition &outputPlaneShape,
															ArrayColumn<T> &outputDataCol,
															ArrayColumn<Bool> *outputFlagCol);


	void setWeightsPlaneByReference(	uInt inputRow,
										const Cube<Float> &inputWeightsCube,
										Matrix<Float> &inputWeightsPlane);
	void dontsetWeightsPlaneByReference(	uInt ,
											const Cube<Float> &,
											Matrix<Float> &) {return;}
	void (casa::MSTransformManager::*setWeightsPlaneByReference_p)(	uInt inputRow,
																		const Cube<Float> &inputWeightsCube,
																		Matrix<Float> &inputWeightsPlane);

	template <class T> void transformAndWritePlaneOfData(	Int inputSpw,
															uInt row,
															Matrix<T> &inputDataPlane,
															Matrix<Bool> &inputFlagsPlane,
															Matrix<Float> &inputWeightsPlane,
															Matrix<T> &outputDataPlane,
															Matrix<Bool> &outputFlagsPlane,
															ArrayColumn<T> &outputDataCol,
															ArrayColumn<Bool> *outputFlagCol);
	void setWeightStripeByReference(	uInt corrIndex,
										Matrix<Float> &inputWeightsPlane,
										Vector<Float> &inputWeightsStripe);
	void dontSetWeightStripeByReference(	uInt ,
											Matrix<Float> &,
											Vector<Float> &) {return;}
	void (casa::MSTransformManager::*setWeightStripeByReference_p)(	uInt corrIndex,
																		Matrix<Float> &inputWeightsPlane,
																		Vector<Float> &inputWeightsStripe);

	void setOutputbuffer(Cube<Complex> *& dataBufferPointer,Cube<Bool> *& flagBufferPointer);
	void setOutputbuffer(Cube<Float> *& dataBufferPointer,Cube<Bool> *& flagBufferPointer);

	template <class T> void bufferOutputPlanes(	uInt row,
												Matrix<T> &outputDataPlane,
												Matrix<Bool> &outputFlagsPlane,
												ArrayColumn<T> &outputDataCol,
												ArrayColumn<Bool> &outputFlagCol);
	template <class T> void bufferOutputPlanesInSlices(	uInt row,
														Matrix<T> &outputDataPlane,
														Matrix<Bool> &outputFlagsPlane,
														ArrayColumn<T> &outputDataCol,
														ArrayColumn<Bool> &outputFlagCol);

	void writeOutputPlanes(	uInt row,
							Matrix<Complex> &outputDataPlane,
							Matrix<Bool> &outputFlagsPlane,
							ArrayColumn<Complex> &outputDataCol,
							ArrayColumn<Bool> &outputFlagCol);
	void writeOutputPlanes(	uInt row,
							Matrix<Float> &outputDataPlane,
							Matrix<Bool> &outputFlagsPlane,
							ArrayColumn<Float> &outputDataCol,
							ArrayColumn<Bool> &outputFlagCol);
	void (casa::MSTransformManager::*writeOutputPlanesComplex_p)(	uInt row,
																		Matrix<Complex> &outputDataPlane,
																		Matrix<Bool> &outputFlagsPlane,
																		ArrayColumn<Complex> &outputDataCol,
																		ArrayColumn<Bool> &outputFlagCol);
	void (casa::MSTransformManager::*writeOutputPlanesFloat_p)(	uInt row,
																	Matrix<Float> &outputDataPlane,
																	Matrix<Bool> &outputFlagsPlane,
																	ArrayColumn<Float> &outputDataCol,
																	ArrayColumn<Bool> &outputFlagCol);

	template <class T> void writeOutputPlanesInBlock(	uInt row,
														Matrix<T> &outputDataPlane,
														Matrix<Bool> &outputFlagsPlane,
														ArrayColumn<T> &outputDataCol,
														ArrayColumn<Bool> &outputFlagCol);
	void (casa::MSTransformManager::*writeOutputFlagsPlane_p)(	Matrix<Bool> &outputPlane,
																	ArrayColumn<Bool> &outputCol,
																	IPosition &outputPlaneShape,
																	uInt &outputRow);
	void writeOutputFlagsPlane(	Matrix<Bool> &outputPlane,
								ArrayColumn<Bool> &outputCol,
								IPosition &outputPlaneShape,
								uInt &outputRow);
	void dontWriteOutputFlagsPlane(	Matrix<Bool> &,
									ArrayColumn<Bool> &,
									IPosition &,
									uInt &) {return;}

	template <class T> void writeOutputPlanesInSlices(	uInt row,
														Matrix<T> &outputDataPlane,
														Matrix<Bool> &outputFlagsPlane,
														ArrayColumn<T> &outputDataCol,
														ArrayColumn<Bool> &outputFlagCol);
	template <class T> void writeOutputPlaneSlices(	Matrix<T> &outputPlane,
													ArrayColumn<T> &outputDataCol,
													Slice &sliceX,
													Slice &sliceY,
													IPosition &outputPlaneShape,
													uInt &outputRow);
	template <class T> void writeOutputPlaneReshapedSlices(	Matrix<T> &outputPlane,
															ArrayColumn<T> &outputDataCol,
															Slice &sliceX,
															Slice &sliceY,
															IPosition &outputPlaneShape,
															uInt &outputRow);
	void (casa::MSTransformManager::*writeOutputFlagsPlaneSlices_p)(	Matrix<Bool> &outputPlane,
																			ArrayColumn<Bool> &outputCol,
																			Slice &sliceX,
																			Slice &sliceY,
																			IPosition &outputPlaneShape,
																			uInt &outputRow);
	void writeOutputFlagsPlaneSlices(	Matrix<Bool> &outputPlane,
										ArrayColumn<Bool> &outputCol,
										Slice &sliceX,
										Slice &sliceY,
										IPosition &outputPlaneShape,
										uInt &outputRow);
	void dontWriteOutputFlagsPlaneSlices(	Matrix<Bool> &,
											ArrayColumn<Bool> &,
											Slice &,
											Slice &,
											IPosition &,
											uInt &) {return;}
	void (casa::MSTransformManager::*writeOutputFlagsPlaneReshapedSlices_p)(	Matrix<Bool> &outputPlane,
																					ArrayColumn<Bool> &outputCol,
																					Slice &sliceX,
																					Slice &sliceY,
																					IPosition &outputPlaneShape,
																					uInt &outputRow);
	void writeOutputFlagsPlaneReshapedSlices(	Matrix<Bool> &outputPlane,
												ArrayColumn<Bool> &outputCol,
												Slice &sliceX,
												Slice &sliceY,
												IPosition &outputPlaneShape,
												uInt &outputRow);
	void dontWriteOutputPlaneReshapedSlices(	Matrix<Bool> &,
												ArrayColumn<Bool> &,
												Slice &,
												Slice &,
												IPosition &,
												uInt &) {return;}

	void transformStripeOfData(	Int inputSpw,
								Vector<Complex> &inputDataStripe,
								Vector<Bool> &inputFlagsStripe,
								Vector<Float> &inputWeightsStripe,
								Vector<Complex> &outputDataStripe,
								Vector<Bool> &outputFlagsStripe);
	void transformStripeOfData(	Int inputSpw,
								Vector<Float> &inputDataStripe,
								Vector<Bool> &inputFlagsStripe,
								Vector<Float> &inputWeightsStripe,
								Vector<Float> &outputDataStripe,
								Vector<Bool> &outputFlagsStripe);
	void (casa::MSTransformManager::*transformStripeOfDataComplex_p)(	Int inputSpw,
																			Vector<Complex> &inputDataStripe,
																			Vector<Bool> &inputFlagsStripe,
																			Vector<Float> &inputWeightsStripe,
																			Vector<Complex> &outputDataStripe,
																			Vector<Bool> &outputFlagsStripe);
	void (casa::MSTransformManager::*transformStripeOfDataFloat_p)(	Int inputSpw,
																		Vector<Float> &inputDataStripe,
																		Vector<Bool> &inputFlagsStripe,
																		Vector<Float> &inputWeightsStripe,
																		Vector<Float> &outputDataStripe,
																		Vector<Bool> &outputFlagsStripe);

	template <class T> void average(	Int inputSpw,
										Vector<T> &inputDataStripe,
										Vector<Bool> &inputFlagsStripe,
										Vector<Float> &inputWeightsStripe,
										Vector<T> &outputDataStripe,
										Vector<Bool> &outputFlagsStripe);
	template <class T> void simpleAverage(	uInt width,
											Vector<T> &inputData,
											Vector<T> &outputData);
	void averageKernel(	Vector<Complex> &inputData,
						Vector<Bool> &inputFlags,
						Vector<Float> &inputWeights,
						Vector<Complex> &outputData,
						Vector<Bool> &outputFlags,
						uInt startInputPos,
						uInt outputPos,
						uInt width);
	void averageKernel(	Vector<Float> &inputData,
						Vector<Bool> &inputFlags,
						Vector<Float> &inputWeights,
						Vector<Float> &outputData,
						Vector<Bool> &outputFlags,
						uInt startInputPos,
						uInt outputPos,
						uInt width);
	void (casa::MSTransformManager::*averageKernelComplex_p)(	Vector<Complex> &inputData,
																	Vector<Bool> &inputFlags,
																	Vector<Float> &inputWeights,
																	Vector<Complex> &outputData,
																	Vector<Bool> &outputFlags,
																	uInt startInputPos,
																	uInt outputPos,
																	uInt width);
	void (casa::MSTransformManager::*averageKernelFloat_p)(		Vector<Float> &inputData,
																	Vector<Bool> &inputFlags,
																	Vector<Float> &inputWeights,
																	Vector<Float> &outputData,
																	Vector<Bool> &outputFlags,
																	uInt startInputPos,
																	uInt outputPos,
																	uInt width);
	template <class T> void simpleAverageKernel(	Vector<T> &inputData,
													Vector<Bool> &,
													Vector<Float> &,
													Vector<T> &outputData,
													Vector<Bool> &,
													uInt startInputPos,
													uInt outputPos,
													uInt width);
	template <class T> void flagAverageKernel(	Vector<T> &inputData,
												Vector<Bool> &inputFlags,
												Vector<Float> &,
												Vector<T> &outputData,
												Vector<Bool> &outputFlags,
												uInt startInputPos,
												uInt outputPos,
												uInt width);
	template <class T> void weightAverageKernel(	Vector<T> &inputData,
													Vector<Bool> &,
													Vector<Float> &inputWeights,
													Vector<T> &outputData,
													Vector<Bool> &outputFlags,
													uInt startInputPos,
													uInt outputPos,
													uInt width);
	template <class T> void cumSumKernel(	Vector<T> &inputData,
											Vector<Bool> &,
											Vector<Float> &,
											Vector<T> &outputData,
											Vector<Bool> &,
											uInt startInputPos,
											uInt outputPos,
											uInt width);
	template <class T> void flagWeightAverageKernel(	Vector<T> &inputData,
														Vector<Bool> &inputFlags,
														Vector<Float> &inputWeights,
														Vector<T> &outputData,
														Vector<Bool> &outputFlags,
														uInt startInputPos,
														uInt outputPos,
														uInt width);
	template <class T> void flagCumSumKernel(	Vector<T> &inputData,
												Vector<Bool> &inputFlags,
												Vector<Float> &,
												Vector<T> &outputData,
												Vector<Bool> &,
												uInt startInputPos,
												uInt outputPos,
												uInt width);

	template <class T> void flagNonZeroAverageKernel(	Vector<T> &inputData,
														Vector<Bool> &inputFlags,
														Vector<Float> &,
														Vector<T> &outputData,
														Vector<Bool> &,
														uInt startInputPos,
														uInt outputPos,
														uInt width);
	template <class T> void flagWeightNonZeroAverageKernel(	Vector<T> &inputData,
															Vector<Bool> &inputFlags,
															Vector<Float> &,
															Vector<T> &outputData,
															Vector<Bool> &,
															uInt startInputPos,
															uInt outputPos,
															uInt width);
	template <class T> void flagCumSumNonZeroKernel(	Vector<T> &inputData,
														Vector<Bool> &inputFlags,
														Vector<Float> &,
														Vector<T> &outputData,
														Vector<Bool> &,
														uInt startInputPos,
														uInt outputPos,
														uInt width);

	template <class T> void smooth(	Int ,
									Vector<T> &inputDataStripe,
									Vector<Bool> &inputFlagsStripe,
									Vector<Float> &,
									Vector<T> &outputDataStripe,
									Vector<Bool> &outputFlagsStripe);

	template <class T> void regrid(	Int ,
									Vector<T> &inputDataStripe,
									Vector<Bool> &inputFlagsStripe,
									Vector<Float> &,
									Vector<T> &outputDataStripe,
									Vector<Bool> &outputFlagsStripe);

	void regridCore(	Int inputSpw,
						Vector<Complex> &inputDataStripe,
						Vector<Bool> &inputFlagsStripe,
						Vector<Float> &inputWeightsStripe,
						Vector<Complex> &outputDataStripe,
						Vector<Bool> &outputFlagsStripe);
	void regridCore(	Int inputSpw,
						Vector<Float> &inputDataStripe,
						Vector<Bool> &inputFlagsStripe,
						Vector<Float> &inputWeightsStripe,
						Vector<Float> &outputDataStripe,
						Vector<Bool> &outputFlagsStripe);

	void (casa::MSTransformManager::*regridCoreComplex_p)(		Int inputSpw,
																	Vector<Complex> &inputDataStripe,
																	Vector<Bool> &inputFlagsStripe,
																	Vector<Float> &inputWeightsStripe,
																	Vector<Complex> &outputDataStripe,
																	Vector<Bool> &outputFlagsStripe);
	void (casa::MSTransformManager::*regridCoreFloat_p)(	Int inputSpw,
																Vector<Float> &inputDataStripe,
																Vector<Bool> &inputFlagsStripe,
																Vector<Float> &inputWeightsStripe,
																Vector<Float> &outputDataStripe,
																Vector<Bool> &outputFlagsStripe);

	void fftshift(	Int inputSpw,
					Vector<Complex> &inputDataStripe,
					Vector<Bool> &inputFlagsStripe,
					Vector<Float> &inputWeightsStripe,
					Vector<Complex> &outputDataStripe,
					Vector<Bool> &outputFlagsStripe);
	void fftshift(	Int inputSpw,
					Vector<Float> &inputDataStripe,
					Vector<Bool> &inputFlagsStripe,
					Vector<Float> &inputWeightsStripe,
					Vector<Float> &outputDataStripe,
					Vector<Bool> &outputFlagsStripe);

	template <class T> void interpol1D(	Int inputSpw,
										Vector<T> &inputDataStripe,
										Vector<Bool> &inputFlagsStripe,
										Vector<Float> &,
										Vector<T> &outputDataStripe,
										Vector<Bool> &outputFlagsStripe);

	template <class T> void interpol1Dfftshift(	Int inputSpw,
												Vector<T> &inputDataStripe,
												Vector<Bool> &inputFlagsStripe,
												Vector<Float> &inputWeightsStripe,
												Vector<T> &outputDataStripe,
												Vector<Bool> &outputFlagsStripe);

	template <class T> void averageSmooth(	Int inputSpw,
											Vector<T> &inputDataStripe,
											Vector<Bool> &inputFlagsStripe,
											Vector<Float> &inputWeightsStripe,
											Vector<T> &outputDataStripe,
											Vector<Bool> &outputFlagsStripe);
	template <class T> void averageRegrid(	Int inputSpw,
											Vector<T> &inputDataStripe,
											Vector<Bool> &inputFlagsStripe,
											Vector<Float> &inputWeightsStripe,
											Vector<T> &outputDataStripe,
											Vector<Bool> &outputFlagsStripe);
	template <class T> void smoothRegrid(	Int inputSpw,
											Vector<T> &inputDataStripe,
											Vector<Bool> &inputFlagsStripe,
											Vector<Float> &inputWeightsStripe,
											Vector<T> &outputDataStripe,
											Vector<Bool> &outputFlagsStripe);
	template <class T> void averageSmoothRegrid(	Int inputSpw,
													Vector<T> &inputDataStripe,
													Vector<Bool> &inputFlagsStripe,
													Vector<Float> &inputWeightsStripe,
													Vector<T> &outputDataStripe,
													Vector<Bool> &outputFlagsStripe);


	// MS specification parameters
	String inpMsName_p;
	String outMsName_p;
	String datacolumn_p;
	Bool makeVirtualModelColReal_p;
	Bool makeVirtualCorrectedColReal_p;
	Vector<Int> tileShape_p;

	// Data selection parameters
	String arraySelection_p;
	String fieldSelection_p;
	String scanSelection_p;
	String timeSelection_p;
	String spwSelection_p;
	String baselineSelection_p;
	String uvwSelection_p;
	String polarizationSelection_p;
	String scanIntentSelection_p;
	String observationSelection_p;
	String taqlSelection_p;

	// Input-Output index maps
	map<uInt,uInt> inputOutputObservationIndexMap_p;
	map<uInt,uInt> inputOutputArrayIndexMap_p;
	map<uInt,uInt> inputOutputScanIndexMap_p;
	map<uInt,uInt> inputOutputScanIntentIndexMap_p;
	map<uInt,uInt> inputOutputFieldIndexMap_p;
	map<uInt,uInt> inputOutputSPWIndexMap_p;
	map<uInt,uInt> inputOutputDDIndexMap_p;
	map<uInt,uInt> inputOutputAntennaIndexMap_p;
	map<uInt,uInt> outputInputSPWIndexMap_p;

	// Frequency transformation parameters
	uInt nspws_p;
	Int ddiStart_p;
	Bool combinespws_p;
	Bool channelAverage_p;
	Bool hanningSmooth_p;
	Bool refFrameTransformation_p;
	Vector<Int> freqbin_p;
	String useweights_p;
	uInt weightmode_p;
	String interpolationMethodPar_p;
	casac::variant *phaseCenterPar_p;
	String restFrequency_p;
	String outputReferenceFramePar_p;
	Bool radialVelocityCorrection_p;

	// Frequency specification parameters
	String mode_p;
	String start_p;
	String width_p;
	int nChan_p;
	String velocityType_p;

	// Time transformation parameters
	Bool timeAverage_p;
	Double timeBin_p;
	String timespan_p;
	vi::AveragingOptions timeAvgOptions_p;
	Double maxuvwdistance_p;
	// uInt minbaselines_p;

	// Calibration parameters
	Bool calibrate_p;
	Record callib_p;

	// Weight Spectrum parameters
	Bool usewtspectrum_p;

	// Buffer handling parameters
	Bool bufferMode_p;
	Bool userBufferMode_p;
	Bool reindex_p;

	// MS-related members
	MSTransformDataHandler *dataHandler_p;
	MeasurementSet *inputMs_p;
	MeasurementSet *selectedInputMs_p;
	MeasurementSet *outputMs_p;
	ROMSColumns *selectedInputMsCols_p;
	MSColumns *outputMsCols_p;
	MSFieldColumns *inputMSFieldCols_p;

	// VI/VB related members
	Block<Int> sortColumns_p;
	vi::VisibilityIterator2 *visibilityIterator_p;
	vi::FrequencySelectionUsingChannels *channelSelector_p;

	// Output MS structure related members
	Bool inputFlagCategoryAvailable_p;
	Bool correctedToData_p;
	Bool doingData_p;
	Bool doingCorrected_p;
	Bool doingModel_p;
	dataColMap dataColMap_p;
	MSMainEnums::PredefinedColumns mainColumn_p;
	uInt nRowsToAdd_p;

	// Frequency transformation members
	uInt chansPerOutputSpw_p;
	uInt tailOfChansforLastSpw_p;
	uInt interpolationMethod_p;
	baselineMap baselineMap_p;
	vector<uInt> rowIndex_p;
	inputSpwChanMap spwChannelMap_p;
	inputOutputSpwMap inputOutputSpwMap_p;
	inputOutputChanFactorMap inputOutputChanFactorMap_p;
	map<uInt,uInt> freqbinMap_p;
	map<uInt,uInt> numOfInpChanMap_p;
	map<uInt,uInt> numOfSelChanMap_p;
	map<uInt,uInt> numOfOutChanMap_p;
	map<uInt,uInt> numOfCombInputChanMap_p;
	map<uInt,uInt> numOfCombInterChanMap_p;
	map<uInt,Float> weightFactorMap_p;
	map<uInt,Float> sigmaFactorMap_p;
    map<uInt,Float> newWeightFactorMap_p;
	map<uInt,Float> newSigmaFactorMap_p;

	// Reference Frame Transformation members
	MFrequency::Types inputReferenceFrame_p;
	MFrequency::Types outputReferenceFrame_p;
	MPosition observatoryPosition_p;
	MEpoch referenceTime_p;
	MDirection phaseCenter_p;
	MRadialVelocity radialVelocity_p;
	MFrequency::Convert freqTransEngine_p;
	MFrequency::Convert refTimeFreqTransEngine_p;
    FFTServer<Float, Complex> fFFTServer_p;
    Bool fftShiftEnabled_p;
	Double fftShift_p;
	ROScalarMeasColumn<MEpoch> timeMeas_p;

	// Weight Spectrum members
	Bool spectrumTransformation_p;
	Bool propagateWeights_p;
	Bool inputWeightSpectrumAvailable_p;
	Bool flushWeightSpectrum_p;
	Bool weightSpectrumFlatFilled_p;
	Bool weightSpectrumFromSigmaFilled_p;
	Bool combinationOfSPWsWithDifferentExposure_p;
	Cube<Float> weightSpectrumCube_p;
	Cube<Float> weightSpectrumCubeFlat_p;
	Cube<Float> weightSpectrumCubeDummy_p;

	// Buffer handling members
	uInt dataBuffer_p;
	uInt relativeRow_p;
	Bool spectrumReshape_p;
	Bool cubeTransformation_p;
	Bool dataColumnAvailable_p;
	Bool correctedDataColumnAvailable_p;
	Bool modelDataColumnAvailable_p;
	Bool floatDataColumnAvailable_p;
	Bool lagDataColumnAvailable_p;
	Cube<Bool> *flagCube_p;
	Cube<Complex> *visCube_p;
	Cube<Complex> *visCubeCorrected_p;
	Cube<Complex> *visCubeModel_p;
	Cube<Float> *visCubeFloat_p;
	Cube<Float> *weightSpectrum_p;
	Cube<Float> *sigmaSpectrum_p;
	Matrix<Float> *weight_p;
	Matrix<Float> *sigma_p;
	ArrayColumn<Float> dummyWeightCol_p;

	// Logging
	LogIO logger_p;
};

} //# NAMESPACE CASA - END

#endif /* MSTransformManager_H_ */
