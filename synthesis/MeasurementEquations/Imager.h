//# Imager.h: Imager functionality sits here; 
//# Copyright (C) 1996,1997,1998,1999,2000,2001,2002,2003
//# Associated Universities, Inc. Washington DC, USA.
//#
//# This library is free software; you can redistribute it and/or modify it
//# under the terms of the GNU Library General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or (at your
//# option) any later version.
//#
//# This library is distributed in the hope that it will be useful, but WITHOUT
//# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
//# License for more details.
//#
//# You should have received a copy of the GNU Library General Public License
//# along with this library; if not, write to the Free Software Foundation,
//# Inc., 675 Massachusetts Ave, Cambridge, MA 02139, USA.
//#
//# Correspondence concerning AIPS++ should be addressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#
//# $Id$

#ifndef SYNTHESIS_IMAGER_H
#define SYNTHESIS_IMAGER_H

#include <casa/aips.h>
#include <casa/OS/Timer.h>
#include <casa/Containers/Record.h>
#include <ms/MeasurementSets/MeasurementSet.h>
#include <casa/Arrays/IPosition.h>
#include <casa/Quanta/Quantum.h>
#include <components/ComponentModels/ConstantSpectrum.h>

#include <measures/Measures/MDirection.h>
#include <components/ComponentModels/FluxStandard.h>
//#include <measures/Measures/MDirection.h
#include <measures/Measures/MPosition.h>
#include <measures/Measures/MRadialVelocity.h>

#include <synthesis/TransformMachines/FTMachine.h>
#include <synthesis/TransformMachines/StokesImageUtil.h>

#include <synthesis/MeasurementComponents/CleanImageSkyModel.h>
#include <synthesis/TransformMachines/EVLAAperture.h>
#include <synthesis/TransformMachines/BeamSquint.h>
#include <synthesis/MeasurementComponents/WFCleanImageSkyModel.h>
#include <synthesis/MeasurementComponents/ClarkCleanImageSkyModel.h>
#include <synthesis/MeasurementEquations/SkyEquation.h>
#include <synthesis/TransformMachines/ATerm.h>
#include <graphics/GenericPlotter/SimplePlotter.h>


namespace casa { //# NAMESPACE CASA - BEGIN

// Forward declarations
class VisSet;
class VisImagingWeight_p;
class MSHistoryHandler;
class PBMath;
class MeasurementSet;
class MDirection;
class MFrequency;
class File;
class VPSkyJones;
class EPJones;
class ViewerProxy;
template<class T> class ImageInterface;

// <summary> Class that contains functions needed for imager </summary>


class Imager 
{
 public:
  // Default constructor

  Imager();

  Imager(MeasurementSet& ms, Bool compress=False, Bool useModel=False);
  Imager(MeasurementSet& ms, Bool compress=False);

  // Copy constructor and assignment operator
  Imager(const Imager&);
  Imager& operator=(const Imager&);

  // Destructor
  virtual ~Imager();
  
  // open all the subtables as userNoReadLock
  virtual Bool openSubTables();


  // Lock the ms and its subtables
  virtual Bool lock();

  // Unlock the ms and its subtables
  virtual Bool unlock();


  // Utility function to do channel selection

  Bool selectDataChannel(Vector<Int>& spectralwindowids, 
				 String& dataMode, 
				 Vector<Int>& dataNchan, 
				 Vector<Int>& dataStart, Vector<Int>& dataStep,
				 MRadialVelocity& mDataStart, 
				 MRadialVelocity& mDataStep);
  //Utility function to check coordinate match with existing image

  virtual Bool checkCoord(const CoordinateSystem& coordsys, 
			  const String& imageName); 

  virtual void setImageParam(Int& nx, Int& ny, Int& npol, Int& nchan); 

  //VisSet and resort 
  virtual void makeVisSet(MeasurementSet& ms, 
			  Bool compress=False, Bool mosaicOrder=False);
  //Just to create the SORTED_TABLE if one can
  //virtual void makeVisSet(MeasurementSet& ms, 
  //			  Bool compress=False, Bool mosaicOrder=False);

  virtual void writeHistory(LogIO& os);

  virtual void writeCommand(LogIO& os);

  //make an empty image
  Bool makeEmptyImage(CoordinateSystem& imageCoord, String& name, Int fieldID=0);

  //Functions to make Primary beams 
  Bool makePBImage(ImageInterface<Float>& pbImage, 
		   Bool useSymmetricBeam=True);
  Bool makePBImage(const CoordinateSystem& imageCoord, 
		   const String& telescopeName, const String& diskPBName, 
		   Bool useSymmetricBeam=True, Double dishdiam=-1.0);
  
  Bool makePBImage(const CoordinateSystem& imageCoord, 
		   const Table& vpTable, const String& diskPBName);
  
  Bool makePBImage(const Table& vpTable, ImageInterface<Float>& pbImage);
  
  Bool makePBImage(const CoordinateSystem& imageCoord, PBMath& pbMath, const String& diskPBName);
  
  Bool makePBImage(PBMath& pbMath, ImageInterface<Float>& pbImage);
  
  void setObsInfo(ObsInfo& obsinfo);
  ObsInfo& latestObsInfo();
// Close the current ms, and replace it with the supplied ms.
  // Optionally compress the attached calibration data
  // columns if they are created here.
  Bool open(MeasurementSet &thems, Bool compress=False, Bool useModel=False);
  
  // Flush the ms to disk and detach from the ms file. All function
  // calls after this will be a no-op.
  Bool close();
  
  // Return the name of the MeasurementSet
  String name() const;
  
  // The following setup methods define the state of the imager.
  // <group>
  // Set image construction parameters
  virtual Bool setimage(const Int nx, const Int ny,
		const Quantity& cellx, const Quantity& celly,
		const String& stokes,
                Bool doShift,
		const MDirection& phaseCenter, 
                const Quantity& shiftx, const Quantity& shifty,
		const String& mode, const Int nchan,
                const Int start, const Int step,
		const MRadialVelocity& mStart, const MRadialVelocity& mStep,
		const Vector<Int>& spectralwindowids, const Int fieldid,
		const Int facets, const Quantity& distance);

  virtual Bool defineImage(const Int nx, const Int ny,
			   const Quantity& cellx, const Quantity& celly,
			   const String& stokes,
			   const MDirection& phaseCenter, 
			   const Int fieldid,
			   const String& mode, const Int nchan,
			   const Int start, const Int step,
			   const MFrequency& mFreqStart,
			   const MRadialVelocity& mStart, 
			   const Quantity& qStep,
			   const Vector<Int>& spectralwindowids, 
			   const Int facets=1, 
			   const Quantity& restFreq=Quantity(0,"Hz"),
                           const MFrequency::Types& mFreqFrame=MFrequency::LSRK,
			   const Quantity& distance=Quantity(0,"m"),
			   const Bool trackSource=False, const MDirection& 
			   trackDir=MDirection(Quantity(0.0, "deg"), 
					       Quantity(90.0, "deg")));
  // Set the data selection parameters
 
  // The parameters useModelcol and readonly is dummy here
  //as they are useful for the ImagerMultiMS version only
  virtual  Bool setDataPerMS(const String& msname, const String& mode, 
			     const Vector<Int>& nchan, 
			     const Vector<Int>& start,
			     const Vector<Int>& step,
			     const Vector<Int>& spectralwindowids,
			     const Vector<Int>& fieldid,
			     const String& msSelect="",
                             const String& timerng="",
			     const String& fieldnames="",
			     const Vector<Int>& antIndex=Vector<Int>(),
			     const String& antnames="",
			     const String& spwstring="",
			     const String& uvdist="",
                             const String& scan="",
                             const String& intent="",
                             const String& obs="",
                             const Bool useModelCol=False,
			     const Bool readonly=False);

  // Select some data.
  // Sets nullSelect_p and returns !nullSelect_p.
  // be_calm: lowers the logging level of some messages if True.
  Bool setdata(const String& mode, const Vector<Int>& nchan, 
	       const Vector<Int>& start,
	       const Vector<Int>& step, const MRadialVelocity& mStart,
	       const MRadialVelocity& mStep,
	       const Vector<Int>& spectralwindowids,
	       const Vector<Int>& fieldid,
	       const String& msSelect="",
	       const String& timerng="",
	       const String& fieldnames="",
	       const Vector<Int>& antIndex=Vector<Int>(),
	       const String& antnames="",
	       const String& spwstring="",
	       const String& uvdist="",
               const String& scan="",
               const String& intent="",
               const String& obs="",
               const Bool usemodelCol=False,
               const Bool be_calm=false);
  
  // Set the processing options
  Bool setoptions(const String& ftmachine, const Long cache, const Int tile,
		  const String& gridfunction, const MPosition& mLocation,
                  const Float padding,
		  const Int wprojplanes=-1,
		  const String& epJTableName="",
		  const Bool applyPointingOffsets=True,
		  const Bool doPointingCorrection=True,
		  const String& cfCacheDirName="", 
		  const Float& rotpastep=5.0,
		  const Float& computepastep=360.0,
		  const Float& pbLimit=5.0e-2,
		  const String& freqinterpmethod="linear",
		  const Int imageTileSizeInPix=0,
		  const Bool singleprecisiononly=False,
		  const Int numthreads=-1,
		  const Bool psTermOn=True,
		  const Bool aTermOn=True,
		  const Bool mTermOn=False,
		  const Bool wbAWP=False,
		  const Bool conjBeams=True);

  // Set the single dish processing options
  Bool setsdoptions(const Float scale, const Float weight, 
		    const Int convsupport=-1, String pointingColToUse="DIRECTION",
                    const Quantity truncate=Quantity(),
                    const Quantity gwidth=Quantity(),
                    const Quantity jwidth=Quantity(),
		    const Float minweight=0.);

  // Set the voltage pattern
  Bool setvp(const Bool dovp,
	     const Bool defaultVP,
	     const String& vpTable,
	     const Bool doSquint,
	     const Quantity &parAngleInc,
	     const Quantity &skyPosThreshold,
	     String defaultTel="",
             const Bool verbose=true);

  // Set the scales to be searched in Multi Scale clean
  Bool setscales(const String& scaleMethod,          // "nscales"  or  "uservector"
		 const Int inscales,
		 const Vector<Float>& userScaleSizes);
  // set bias
  Bool setSmallScaleBias(const Float inbias);

  // Set the number of taylor series terms in the expansion of the
  // image as a function of frequency.
  Bool settaylorterms(const Int intaylor, 
		      const Double inreffreq);

  // </group>
  
  // Advise on suitable values
  Bool advise(const Bool takeAdvice, const Float amplitudeloss,
              const Quantity& fieldOfView,
	      Quantity& cell, Int& npixels, Int& facets,
	      MDirection& phaseCenter);

  // Output a summary of the state of the object
  Bool summary();
  
  // Return the state of the object as a string
  String state();
  
  // Return the # of visibilities accessible to *rvi, optionally excluding
  // flagged ones (if unflagged_only is true) and/or ones without imaging
  // weights (if must_have_imwt is true).
  uInt count_visibilities(ROVisibilityIterator *rvi,
                          const Bool unflagged_only, const Bool must_have_imwt);

  // Return the image coordinates
  Bool imagecoordinates(CoordinateSystem& coordInfo, const Bool verbose=true);
  // new version
  Bool imagecoordinates2(CoordinateSystem& coordInfo, const Bool verbose=true);

  // Return the image shape
  IPosition imageshape() const;

  // Weight the MeasurementSet
  //For some time of weighting briggs/uniform ...one can do it on a per field basis to calculate 
  //weight density distribution. If that is what is wanted multiField should be set to True
  //multifield is inoperative for natural, radial weighting
  Bool weight(const String& algorithm, const String& rmode,
	      const Quantity& noise, const Double robust,
              const Quantity& fieldofview, const Int npixels, const Bool multiField=False);
  
  // Filter the MeasurementSet
  Bool filter(const String& type, const Quantity& bmaj, const Quantity& bmin,
	      const Quantity& bpa);
  
  // Apply a uvrange
  Bool uvrange(const Double& uvmin, const Double& uvmax);
  
  // Sensitivity
  Bool sensitivity(Quantity& pointsourcesens, Double& relativesens, Double& sumwt,
		   Double& effectiveBandwidth, Double& effectiveIntegration, Int& mBaselines,
		   Matrix<Int>& mssChanSel,
		   Vector<Vector<Int> >& nData,
		   Vector<Vector<Double> >& sumwtChan,
		   Vector<Vector<Double> >& sumwtsqChan,
		   Vector<Vector<Double> >& sumInverseVarianceChan);
  
  // Apparent sensitivity calculator 
  //   Accounts for gridding weights and assumes MS weights have correct units
  Bool apparentSensitivity(Double& effSensitivity,
			   Double& relToNat);

  // Make plain image + keep the complex image as well if complexImageName != "".
  Bool makeimage(const String& type, const String& imageName,
                 const String& complexImageName="", const Bool verbose=true);
  
  // Fill in a region of a mask
  Bool boxmask(const String& mask, const Vector<Int>& blc,
	       const Vector<Int>& trc,const Float value);

  //Make a region either from record or array of blc trc 
  //(Matrix(nboxes,4)) into a mask image
  //value is the value of the mask pixels
  //circular masks has form Matrix(ncircles,3)
  //where the 3 values on a row are radius, x, y pixel values 
  Bool regionmask(const String& maskimage, Record* imageRegRec, 
		  Matrix<Quantity>& blctrcs, Matrix<Float>& circles, 
		  const Float& value=1.0);

  static Bool regionToImageMask(const String& maskimage, Record* imageRegRec, 
				Matrix<Quantity>& blctrcs, 
				Matrix<Float>& circles, 
				const Float& value=1.0);
  // Clip on Stokes I
  Bool clipimage(const String& image, const Quantity& threshold);

  // Make a mask image
  static Bool mask(const String& mask, const String& imageName,
                   const Quantity& threshold);
  
  // Restore
  Bool restore(const Vector<String>& model, const String& complist,
	       const Vector<String>& image, const Vector<String>& residual);

  // similar to restore except this is to be called if you fiddle with the model and complist
  // outside of this object (say you clip stuff etc) ...keep the sm_p and se_p state but just calculate new residuals and 
  // restored images. Will throw an exception is se_p or sm_p is not valid (i.e you should have used clean, mem etc before hand).
  Bool updateresidual(const Vector<String>& model, const String& complist,
	       const Vector<String>& image, const Vector<String>& residual);

  // Setbeam
  Bool setbeam(const ImageBeamSet& beam);

  // Residual
  Bool residual(const Vector<String>& model, const String& complist,
	       const Vector<String>& image);

  // Approximate PSF
  Bool approximatepsf(const String& psf);

  // Smooth
  Bool smooth(const Vector<String>& model, 
	      const Vector<String>& image, Bool usefit,
	      ImageBeamSet& mbeam,
	      Bool normalizeVolume);

  // Clean algorithm
  Record clean(const String& algorithm,
	     const Int niter, 
	     const Float gain, 
	     const Quantity& threshold, 
	     const Bool displayProgress, 
	     const Vector<String>& model, const Vector<Bool>& fixed,
	     const String& complist,
	     const Vector<String>& mask,
	     const Vector<String>& restored,
	     const Vector<String>& residual,
	     const Vector<String>& psf=Vector<String>(0),
             const Bool firstrun=true);

  Record iClean(const String& algorithm, 
	      const Int niter, 
	      const Double gain,
	      //const String& threshold, 
	      const Quantity& threshold,
	      const Bool displayprogress,
	      const Vector<String>& model,
	      const Vector<Bool>& keepfixed, const String& complist,
	      const Vector<String>& mask,
	      const Vector<String>& image,
	      const Vector<String>& residual,
	      const Vector<String>& psfnames,
	      const Bool interactive, const Int npercycle,
	      const String& masktemplate);
  
  // MEM algorithm
  Bool mem(const String& algorithm,
	   const Int niter, const Quantity& sigma, 
	   const Quantity& targetflux,
	   const Bool constrainflux,
	   const Bool displayProgress, 
	   const Vector<String>& model, const Vector<Bool>& fixed,
	   const String& complist,
	   const Vector<String>& prior,
	   const Vector<String>& mask,
	   const Vector<String>& restored,
	   const Vector<String>& residual);
  
  // pixon algorithm
  Bool pixon(const String& algorithm,
	     const Quantity& sigma, 
	     const String& model);
  
  // NNLS algorithm
  Bool nnls(const String& algorithm, const Int niter, const Float tolerance,
	    const Vector<String>& model, const Vector<Bool>& fixed,
	    const String& complist,
	    const Vector<String>& fluxMask, const Vector<String>& dataMask,
	    const Vector<String>& restored,
	    const Vector<String>& residual);

  // Multi-field control parameters
  //flat noise is the parameter that control the search of clean components
  //in a flat noise image or an optimum beam^2 image
  Bool setmfcontrol(const Float cyclefactor,
		    const Float cyclespeedup,
		    const Float cyclemaxpsffraction,
		    const Int stoplargenegatives, 
		    const Int stoppointmode,
		    const String& scaleType,
		    const Float  minPB,
		    const Float constPB,
		    const Vector<String>& fluxscale,
		    const Bool flatnoise=True);
  
  // Feathering algorithm
  Bool feather(const String& image,
	       const String& highres,
	       const String& lowres,
	       const String& lowpsf, const Float dishDiam=-1.0, const Bool lowPassFilterSD=False);
  
  // Apply or correct for Primary Beam or Voltage Pattern
  Bool pb(const String& inimage,
	  const String& outimage,
	  const String& incomps,
	  const String& outcomps,
	  const String& operation,
	  const MDirection& pointngCenter,
	  const Quantity& pa,
	  const String& pborvp);

  // Make a linear mosaic of several images
  Bool linearmosaic(const String& mosaic,
		    const String& fluxscale,
		    const String& sensitivity,
		    const Vector<String>& images,
		    const Vector<Int>& fieldids);
  
  // Fourier transform the model and componentlist.
  // Returns its nominal success value.
  Bool ft(const Vector<String>& model, const String& complist,
	  Bool incremental=False);

  // Compute the model visibility using specified source flux densities
  Bool setjy(const Int fieldid, const Int spectralwindowid,
	     const Vector<Double>& fluxDensity, const String& standard);
  Bool setjy(const Vector<Int>& fieldid, const Vector<Int>& spectralwindowid, 
	     const String& fieldnames, const String& spwstring, 
	     const Vector<Double>& fluxDensity, const String& standard);
  
  //Setjy with model image. If chanDep=True then the scaling is calculated on a 
  //per channel basis for the model image...otherwise the whole spw gets the
  //same flux density.
  //Bool setjy(const Vector<Int>& fieldid, 
  Record setjy(const Vector<Int>& fieldid, 
	     const Vector<Int>& spectralwindowid, 
	     const String& fieldnames, const String& spwstring, 
	     const String& model,
	     const Vector<Double>& fluxDensity, const String& standard, 
	     const Bool chanDep=False, //const Double spix=0.0,
             const Vector<Double>& spix=Vector<Double>(),
             const MFrequency& reffreq=MFrequency(Quantity(1.0, "GHz"),
                                                  MFrequency::LSRK),
             const Vector<Double>& pipars=Vector<Double>(),
             const Vector<Double>& papars=Vector<Double>(),
             const Double& rotMeas=0.0, 
             const String& timerange="", const String& scanstr="",
             const String& intentstr="", const String& obsidstr="",
             const String& interpolation="nearest");

  // Make an empty image
  Bool make(const String& model);

  // make a model from a SD image. 
  // This model then can be used as initial clean model to include the 
  // shorter spacing.
  Bool makemodelfromsd(const String& sdImage, const String& modelimage,
		       const String& lowPSF,
		       String& maskImage);

  // Write a component list to disk, starting with prefix, using a setjy
  // standard, and return the name of the list.
  String make_comp(const String& objName, const String& standard,
		   const MEpoch& mtime, const Vector<MFrequency>& freqv,
		   const String& prefix);

  // Clone an image
  static Bool clone(const String& imageName, const String& newImageName);
  
  // Fit the psf
  Bool fitpsf(const String& psf, ImageBeamSet& mbeam);

  // Correct the visibility data (OBSERVED->CORRECTED)
  Bool correct(const Bool doparallactic, const Quantity& t);

  // Plot the uv plane
  Bool plotuv(const Bool rotate);

  // Plot the visibilities
  Bool plotvis(const String& type, const Int increment);

  // Plot the weights
  Bool plotweights(const Bool gridded, const Int increment);

  // Plot a summary
  Bool plotsummary();

  // Clip visibilities
  Bool clipvis(const Quantity& threshold);


  //Check if can proceed with this object
  Bool valid() const;


  //Interactive mask drawing
  //forceReload..forces the viewer to dump previous image that is being displayed
  Int interactivemask(const String& imagename, const String& maskname, 
		      Int& niter, Int& ncycles, String& threshold, const Bool forceReload=False);


  //helper function to copy a mask from one image to another

  static Bool copyMask(ImageInterface<Float>& out, const ImageInterface<Float>& in, String maskname="mask0", Bool setdefault=True); 


  // Supports the "[] or -1 => everything" convention using the rule:
  // If v is empty or only has 1 element, and it is < 0, 
  //     replace v with 0, 1, ..., nelem - 1.
  // Returns whether or not it modified v.
  //   If so, v is modified in place.
  static Bool expand_blank_sel(Vector<Int>& v, const uInt nelem);  

  //spectral gridding calculation for output images (use SubMS::calcChanFreqs)
  Bool calcImFreqs(Vector<Double>& imfreqs, Vector<Double>& imfreqres,
                   const MFrequency::Types& oldRefFrame,
                   const MEpoch& obsEpoch, const MPosition& obsPosition,
                   const Double& restFreq);

  // Advise the chanselection needed for the frequency range or
  // give the frequency range for a give spwselection  if getFreqRange==True
  // if the parameter msname is used then the MSs associated associated with
  // this object (that have been either 'open'ed or 'selectvis'ed) are ignored
  // In this mode it is a helper function to the general world ...no need to
  // open or selectvis. You need to specify the field_id for which this calculation is 
  // being done for in the helper mode. 
  // If you have already set MS's and selected data and msname="" then 
  // the calulation is done for the field(s) selected in selectvis.
  // getFreqRange=True then the freqrange in the frame and spwselection  you choose is 
  // returned in freqStart and freqEnd (in the case of msname="" then it is for the fields 
  //and spw you have chosen in selectvis). 
  Bool adviseChanSelex(Double& freqStart, Double& freqEnd, 
		       const Double& freqStep,  const MFrequency::Types& freqframe,
		       Vector< Vector<Int> >& spw, Vector< Vector<Int> >& start,
		       Vector< Vector<Int> >& nchan, const String& msname="", 
		       const Int fieldid=0, const Bool getFreqRange=False, 
		       const String spwselection="");


  //These are utility functions when weights from different imager instances 
  //need to reconciled in parallel gridding by different instances of imagers 
  //for example.
  // when type is "imaging"
  // getweightGrid will get the weight density for uniform style imaging weight
  // the Block elements are for different fields if independent field weighting 
  // was done.
  // when type is "ftweight"..then a Vector of string is expected in weightimage 
  // which is of the same length as the number of models put in clean etc 

  Bool getWeightGrid(Block<Matrix<Float> >&weightgrid, const String& type, const Vector<String>& weightImagenames=Vector<String>());
  Bool setWeightGrid(const Block<Matrix<Float> >& weightgrid, const String& type);
  String dQuantitytoString(const Quantity& dq);

protected:

  CountedPtr<MeasurementSet> ms_p;
  CountedPtr<MSHistoryHandler> hist_p;
  Table antab_p;
  Table datadesctab_p;
  Table feedtab_p;
  Table fieldtab_p;
  Table obstab_p;
  Table pointingtab_p;
  Table poltab_p;
  Table proctab_p;
  Table spwtab_p;
  Table statetab_p;
  Table dopplertab_p;
  Table flagcmdtab_p;
  Table freqoffsettab_p;
  Table historytab_p;
  Table sourcetab_p;
  Table syscaltab_p;
  Table weathertab_p;
  Int lockCounter_p;
  Int nx_p, ny_p, npol_p, nchan_p;
  ObsInfo latestObsInfo_p;
  //What should be the tile volume on disk
  Int imageTileVol_p;



  String msname_p;
  CountedPtr<MeasurementSet> mssel_p;
  VisSet *vs_p;
  ROVisibilityIterator* rvi_p;
  VisibilityIterator* wvi_p;
  FTMachine *ft_p;
  ComponentFTMachine *cft_p;
  SkyEquation* se_p;
  CleanImageSkyModel* sm_p;
  VPSkyJones* vp_p;
  VPSkyJones* gvp_p;

  Bool setimaged_p, nullSelect_p;
  Bool redoSkyModel_p;   // if clean is run multiply ..use this to check
                         // if setimage was changed hence redo the skyModel.
  Float rotPAStep_p, computePAStep_p, pbLimit_p;
  Int facets_p;
  Int wprojPlanes_p;
  Quantity mcellx_p, mcelly_p;
  String stokes_p;
  String dataMode_p;
  String imageMode_p;           // channel, (optical)velocity, mfs, or frequency
  Vector<Int> dataNchan_p;
  Int imageNchan_p;
  Vector<Int> dataStart_p, dataStep_p;
  Int imageStart_p, imageStep_p;
  MRadialVelocity mDataStart_p, mImageStart_p;
  MRadialVelocity mDataStep_p,  mImageStep_p;
  MFrequency mfImageStart_p, mfImageStep_p;
  MFrequency::Types freqFrame_p;
  MDirection phaseCenter_p;
  Quantity restFreq_p;
  Quantity distance_p;
  Bool doShift_p;
  Quantity shiftx_p;
  Quantity shifty_p;
  String ftmachine_p, gridfunction_p;
  Bool wfGridding_p;
  Long cache_p;
  Int  tile_p;
  MPosition mLocation_p;
  Bool doVP_p;
  ImageBeamSet beam_p;
  Bool beamValid_p;
  Float padding_p;
  Float sdScale_p;
  Float sdWeight_p;
  Int sdConvSupport_p;

  Quantity qtruncate_p;
  Quantity qgwidth_p;
  Quantity qjwidth_p;

  Float minWeight_p;

  // special mf control parms, etc
  Float cyclefactor_p;
  Float cyclespeedup_p;
  Float cyclemaxpsffraction_p;
  Int stoplargenegatives_p;
  Int stoppointmode_p;
  Vector<String> fluxscale_p;
  String scaleType_p;		// type of image-plane scaling: NONE, SAULT
  Float minPB_p;		// minimum value of generalized-PB pattern
  Float constPB_p;		// above this level, constant flux-scale

  Vector<Int> spectralwindowids_p;
  Int fieldid_p;

  Vector<Int> dataspectralwindowids_p;
  Vector<Int> datadescids_p;
  Vector<Int> datafieldids_p;
  //TT
  Cube<Int> spwchansels_p;
  Matrix<Double> freqrange_p;
  Matrix<Double> mssFreqSel_p;
  Matrix<Int> mssChanSel_p;

  Int numMS_p;

  String telescope_p;
  String vpTableStr_p;         // description of voltage patterns for various telescopes
                               //  in the MS
  Quantity parAngleInc_p;
  Quantity skyPosThreshold_p;
  BeamSquint::SquintType  squintType_p;
  Bool doDefaultVP_p;          // make default VPs, rather than reading in a vpTable


  Bool  doMultiFields_p;      // Do multiple fields?
  Bool  multiFields_p; 	      // multiple fields have been specified in setdata

  Bool doWideBand_p;          // Do Multi Frequency Synthesis Imaging
  String freqInterpMethod_p; //frequency interpolation mode

  Bool flatnoise_p;

  // Set the defaults
  void defaults();

  // check if it is  dettahced from ms.
  Bool detached() const;

  // Create the FTMachines when necessary or when the control parameters
  // have changed. 
  Bool createFTMachine();

  void openSubTable (const Table & otherTable, Table & table, const TableLock & tableLock);

  Bool removeTable(const String& tablename);
  Bool updateSkyModel(const Vector<String>& model,
		      const String complist);
  Bool createSkyEquation(const String complist="");
  Bool createSkyEquation(const Vector<String>& image, 
			 const Vector<Bool>& fixed,
			 const String complist="");
  Bool createSkyEquation(const Vector<String>& image, 
			 const String complist="");
  Bool createSkyEquation(const Vector<String>& image, 
			 const Vector<Bool>& fixed,
			 const Vector<String>& mask,
			 const String complist="");
  Bool createSkyEquation(const Vector<String>& image, 
			 const Vector<Bool>& fixed,
			 const Vector<String>& mask,
			 const Vector<String>& fluxMask,
			 const String complist="");
  ATerm* createTelescopeATerm(MeasurementSet& ms, const Bool& isATermOn=True);
  void destroySkyEquation();

  //add residual to the private vars or create residual images
  Bool addResiduals(const Vector<String>& residual);
  // Add the residuals to the SkyEquation
  Bool addResidualsToSkyEquation(const Vector<String>& residual);

  // Add or replace the masks
  Bool addMasksToSkyEquation(const Vector<String>& mask, const Vector<Bool>& fixed=Vector<Bool>(0));

  // Get the rest frequency ..returns 1 element in restfreq 
  // if user specified or try to get the info from the SOURCE table 
  Bool getRestFreq(Vector<Double>& restFreq, const Int& spw);

  Bool restoreImages(const Vector<String>& restored, Bool modresiduals=True);

  // names of flux scale images
  Bool writeFluxScales(const Vector<String>& fluxScaleNames);

  // Helper functions to hide some setjy code.
  Unit sjy_setup_arrs(Vector<Vector<Flux<Double> > >& returnFluxes,
                      Vector<Vector<Flux<Double> > >& returnFluxErrs,
                      Vector<Vector<Double> >& fluxUsed, // mainly for logging purpose
                      Vector<String>& tempCLs,
                      Vector<Vector<MFrequency> >& mfreqs,
                      const ROMSSpWindowColumns& spwcols, const uInt nspws,
                      const Vector<Int>& selToRawSpwIds, const Bool chanDep);
  // Returns whether it might have made any visibilities.
  Bool sjy_make_visibilities(TempImage<Float> *tmodimage, LogIO& os,
  //Bool sjy_make_visibilities(Block<CountedPtr<TempImage<Float> > >& tmodimages, LogIO& os,
                             //const Int rawspwid, const Int fldid,
  // for new one                           
  //                           const Vector<Int>& rawspwids, const Int fldid,
                             const Int rawspwid, const Int fldid,
                             const String& clname, const String& timerange="",
                             const String& scanstr="", 
			     const String& obsidstr="",
                             const String& intentstr="", 
			    const Vector<Double>& freqofscale=Vector<Double>(0),
			     const Vector<Double>& scale=Vector<Double>(0) );
  // Concatenate multiple CLs 
  Bool sjy_concatComponentLists(LogIO& os, const Vector<String>& tempCLs, const String& outTempCL);
  // Returns whether it found a source.
  Bool sjy_computeFlux(LogIO& os, FluxStandard& fluxStd,
                       Vector<Vector<Flux<Double> > >& returnFluxes,
                       Vector<Vector<Flux<Double> > >& returnFluxErrs,
                       Vector<String>& tempCLs, 
                       //Vector<Double>& fluxUsed,
                       Vector<Vector<Double> >& fluxUsed,
                       String& fluxScaleName, MEpoch& aveEpoch,
                       const Vector<Vector<MFrequency> >& mfreqs,
                       const String& model, const String& fieldName, 
                       const ROMSColumns& msc, const Int fldid,
                       const MDirection& fieldDir, const Vector<Int>& selToRawSpwIds,
                       const String& standard);

  void sjy_makeComponentList(LogIO& os, Vector<String>& tempCLs,
                             Vector<Vector<Flux<Double> > >& returnFluxes,
                             const Vector<Double>& fluxUsed,
                             const Vector<Int>& selToRawSpwIds,
                             const Vector<Vector<MFrequency> >& mfreqs,
                             const String& fieldName,
                             const MDirection& fieldDir,
                             const Vector<Double>& spix,
                             const Vector<Double>& pipars,
                             const Vector<Double>& papars,
                             const Double& rotMeas,
                             //const Vector<Double>& cppars,
                             const MFrequency& reffreq,
                             const MEpoch& aveEpoch,
                             const Int fldId);
  //
  // Returns NULL if no image is prepared.
  TempImage<Float>* sjy_prepImage(LogIO& os, FluxStandard& fluxStd,
                                  Vector<Double>& fluxUsed, 
				  Vector<Double>& freq, 
				  Vector<Double>& scale, const String& model,
                                  const ROMSSpWindowColumns& spwcols,
                                  //const Int rawspwid, const Bool chanDep,
                                  const Vector<Int> rawspwids, const Bool chanDep,
                                  const Vector<Vector<MFrequency> >& mfreqs,
                                  //const uInt selspw, const String& fieldName,
                                  const String& fieldName,
                                  const MDirection& fieldDir, const Unit& freqUnit,
                                  const Vector<Double>& fluxdens,
                                  const Bool precompute, 
                                  //const Double spix,
                                  const Vector<Double>& spix,
                                  const MFrequency& reffreq, 
				  const MEpoch& aveEpoch, const Int fieldId);
  // Returns True or throws up.
  Bool sjy_regridCubeChans(TempImage<Float>* tmodimage,
                           PagedImage<Float>& modimage, Int freqAxis);

  // set a radius limit if the model image is one of the known source
  // otherwise simply copy modimage to tmodimage
  Bool sjy_setRadiusLimit(TempImage<Float>* tmodimage,
                          PagedImage<Float>& modimage, const String& model,
                          DirectionCoordinate& dircsys);

  Bool sjy_calciflux(const Vector<MFrequency>& freqs, const MFrequency& reffreq, 
                     const Double refflux, const Vector<Double>& vspix, Vector<Double>& iflux);

  Bool sjy_calcquflux(const Vector<Double>& pipars, const Vector<Double>& papars,
                      const Vector<Double>& iflux, const Double rotMeas,
                      const Vector<MFrequency>& freqs, 
                      const MFrequency& reffreq, Vector<Double>& qflux,
                      Vector<Double>& uflux);

  String imageName();

  Bool pbguts(ImageInterface<Float>& in,  
	      ImageInterface<Float>& out, 
	      const MDirection&,
	      const Quantity&);

  // Helper func for printing clean's restoring beam to the logger.  May find
  // the restoring beam as a side effect, so sm_p can't be const.
  void printbeam(CleanImageSkyModel *sm_p, LogIO &os, const Bool firstrun=true);

  // Helper func for createFTMachine().  Returns phaseCenter_p as a String,
  // *assuming* it is set.  It does not check!
  String tangentPoint();
  

  Bool assertDefinedImageParameters() const;
 // Virtual methods to set the ImageSkyModel and SkyEquation.
  // This allows derived class pimager to set parallelized
  // specializations.
  //
  virtual void setWFCleanImageSkyModel() 
    {sm_p = new WFCleanImageSkyModel(facets_p, wfGridding_p); return;}; 
    
  virtual void setClarkCleanImageSkyModel()
    {sm_p = new ClarkCleanImageSkyModel(); return;};
  virtual void setSkyEquation();
    
  virtual void savePSF(const Vector<String>& psf);

  String frmtTime(const Double time);

  //copy imageregion to pixels on image as value given
  static Bool regionToMask(ImageInterface<Float>& maskImage, ImageRegion& imagreg, const Float& value=1.0);

  //set the mosaic ft machine and right convolution function
  virtual void setMosaicFTMachine(Bool useDoublePrec=False); 

  // Makes a component list on disk containing cmp (with fluxval and cspectrum)
  // named msname_p.fieldName.spw<spwid>.tempcl and returns the name.
  String makeComponentList(const String& fieldName, const Int spwid,
                           const Flux<Double>& fluxval,
                           const ComponentShape& cmp,
                           const ConstantSpectrum& cspectrum) const;

  Vector<Int> decideNPolPlanes(Bool checkwithMS);

  //returns if mLocation_p is set (= non-default MPosition)
  Bool nonDefaultLocation();
 
  ComponentList* componentList_p;

  String scaleMethod_p;   // "nscales"   or  "uservector"
  Int nscales_p;
  Int ntaylor_p;
  Double reffreq_p;
  Bool useNewMTFT_p;
  Vector<Float> userScaleSizes_p;
  Bool scaleInfoValid_p;  // This means that we have set the information, not the scale beams
  Float smallScaleBias_p; //ms-clean
  Int nmodels_p;
  // Everything here must be a real class since we make, handle and
  // destroy these.
  Block<CountedPtr<PagedImage<Float> > > images_p;
  Block<CountedPtr<PagedImage<Float> > > masks_p;
  Block<CountedPtr<PagedImage<Float> > > fluxMasks_p;
  Block<CountedPtr<PagedImage<Float> > > residuals_p;
  
  // Freq frame is good and valid conversions can be done (or not)
  Bool freqFrameValid_p;

  // Preferred complex polarization representation
  StokesImageUtil::PolRep polRep_p;

  //Whether to use model column or use it in memory on the fly
  Bool useModelCol_p;

  //Force single precision always
  Bool singlePrec_p;
  //sink used to store history mainly
  LogSink logSink_p;


  //
  // Objects required for pointing correction (ftmachine=PBWProject)
  //
  EPJones *epJ;
  String epJTableName_p, cfCacheDirName_p;
  Bool doPointing, doPBCorr, psTermOn_p, aTermOn_p, mTermOn_p, wbAWP_p, conjBeams_p;
  //SimplePlotterPtr plotter_p;
  Record interactiveState_p;

  //Track moving source stuff
  Bool doTrackSource_p;
  MDirection trackDir_p;
  String pointingDirCol_p;
  VisImagingWeight imwgt_p;

  // viewer connection
  ViewerProxy *viewer_p;
  int clean_panel_p;
  int image_id_p;
  int mask_id_p;
  int prev_image_id_p;
  int prev_mask_id_p;
  //numthreads
  Int numthreads_p;
  Bool avoidTempLatt_p;
};


} //# NAMESPACE CASA - END

#endif
