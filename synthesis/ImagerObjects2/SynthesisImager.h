//# SynthesisImager.h: Imager functionality sits here; 
//# Copyright (C) 2012-2013
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

#ifndef SYNTHESIS_VB2_SYNTHESISIMAGER_H
#define SYNTHESIS_VB2_SYNTHESISIMAGER_H

#include <casa/aips.h>
#include <casa/OS/Timer.h>
#include <casa/Containers/Record.h>
#include <ms/MeasurementSets/MeasurementSet.h>
#include <casa/Arrays/IPosition.h>
#include <casa/Quanta/Quantum.h>
#include <measures/Measures/MDirection.h>
#include<synthesis/ImagerObjects/SynthesisUtilMethods.h>

#include<synthesis/ImagerObjects/SIMapperCollection.h>
#include <msvis/MSVis/ViFrequencySelection.h>

#include <msvis/MSVis/VisibilityIterator2.h>
#include <synthesis/TransformMachines2/FTMachine.h>

#include <boost/scoped_ptr.hpp>

namespace casa { //# NAMESPACE CASA - BEGIN
// Forward declarations
class MeasurementSet;
template<class T> class ImageInterface;
 class SIIterBot;
 class VisImagingWeight;

// <summary> Class that contains functions needed for imager </summary>
  namespace refim { //# NAMESPACE refim - BEGIN

class SynthesisImager 
{
 public:
  // Default constructor

  SynthesisImager();
  virtual ~SynthesisImager();

  // Copy constructor and assignment operator
  //Imager(const Imager&);
  //Imager& operator=(const Imager&);

  virtual Bool selectData(const SynthesisParamsSelect& selpars);

  // make all pure-inputs const
  virtual Bool selectData(const String& msname, 
			  const String& spw="*", 
			  const String& freqBeg="", 
			  const String& freqEnd="",
			  const MFrequency::Types freqFrame=MFrequency::LSRK, 
			  const String& field="*", 
			  const String& antenna="",  
			  const String& timestr="", 
			  const String& scan="", 
			  const String& obs="",
			  const String& state="",
			  const String& uvdist="", 
			  const String& taql="",
			  const Bool usescratch=False, 
			  const Bool readonly=False, 
			  const Bool incrementModel=False);

  virtual Bool defineImage(SynthesisParamsImage& impars, const SynthesisParamsGrid& gridpars);

  //When having a facetted image ...call with (facets > 1)  first and  once only ..
  //Easier to keep track of the imstores that way
  ////CAREFUL: make sure you donot overwrite if you want to predict the model or subtract it to make residual
  virtual Bool defineImage(const String& imagename, const Int nx, const Int ny,
			   const Quantity& cellx, const Quantity& celly,
			   const String& stokes,
			   const MDirection& phaseCenter, 
			   const Int nchan,
			   const Quantity& freqStart,
			   const Quantity& freqStep, 
			   const Vector<Quantity>& restFreq,
			   const Int facets=1,
			   const String ftmachine="GridFT",
			   const Int nTaylorTerms=1,
			   const Quantity& refFreq = Quantity(0,"Hz"),
			   const Projection& projection=Projection::SIN,
			   const Quantity& distance=Quantity(0,"m"),
			   const MFrequency::Types& freqFrame=MFrequency::LSRK,
			   const Bool trackSource=False, 
			   const MDirection& trackDir=MDirection(Quantity(0.0, "deg"), Quantity(90.0, "deg")), 
			   const Bool overwrite=False,
			   const Float padding=1.0, 
			   const Bool useAutocorr=False, 
			   const bool useDoublePrec=True, 
			   const Int wprojplanes=1, 
			   const String convFunc="SF", 
			   const String startmodel="",
			   // The extra params for WB-AWP
			   const Bool aTermOn    = True,
			   const Bool psTermOn   = True,
			   const Bool mTermOn    = False,
			   const Bool wbAWP      = True,
			   const String cfCache  = "",
			   const Bool doPointing = False,
			   const Bool doPBCorr   = True,
			   const Bool conjBeams  = True,
			   const Float computePAStep=360.0,
			   const Float rotatePAStep=5.0
			   );
  //Define image via a predefine SIImageStore object
  virtual Bool defineImage(CountedPtr<SIImageStore> imstor, 
			   const String& ftmachine);
  //Defining componentlist to use while degriding
  //This should be called once...if multiple lists are used..they can be merged in one
  //if sdgrid=True then image plane degridding is done
  virtual void setComponentList(const ComponentList& cl, 
				Bool sdgrid=False);
  Bool weight(const String& type="natural", 
	      const String& rmode="norm",
	      const Quantity& noise=Quantity(0.0, "Jy"), 
	      const Double robust=0.0,
	      const Quantity& fieldofview=Quantity(0.0, "arcsec"),
	      const Int npixels=0, 
	      const Bool multiField=False,
	      const String& filtertype=String("Gaussian"),
	      const Quantity& filterbmaj=Quantity(0.0,"deg"),
	      const Quantity& filterbmin=Quantity(0.0,"deg"),
	      const Quantity& filterbpa=Quantity(0.0,"deg")  );

  //the following get rid of the mappers in this object
  void resetMappers();

  CountedPtr<SIImageStore> imageStore(const Int id=0);

  //Record getMajorCycleControls();
  void executeMajorCycle(Record& controls);

  // make the psf images  i.e grid weight rather than data
  void makePSF();

  void predictModel();
  /* Access method to the Loop Controller held in this class */
  //SIIterBot& getLoopControls();

 

protected:
 
  /////////////// Internal Functions

  // Choose between different types of FTMs
  void createFTMachine(CountedPtr<refim::FTMachine>& theFT, 
		       CountedPtr<refim::FTMachine>& theIFT,  
		       const String& ftname,
		       const uInt nTaylorTerms=1, 
		       const String mType="default",
		       const Int facets=1,
		       //------------------------------
		       const Int wprojplane=1,
		       const Float padding=1.0,
		       const Bool useAutocorr=False,
		       const Bool useDoublePrec=True,
		       const String gridFunction=String("SF"),
		       //------------------------------
		       const Bool aTermOn    = True,
		       const Bool psTermOn   = True,
		       const Bool mTermOn    = False,
		       const Bool wbAWP      = True,
		       const String cfCache  = "",
		       const Bool doPointing = False,
		       const Bool doPBCorr   = True,
		       const Bool conjBeams  = True,
		       const Float computePAStep   = 360.0,
		       const Float rotatePAStep    = 5.0,
		       const String interpolation = String("linear"),
		       const Bool freqFrameValid = True,
		       const Int cache=1000000000,
		       const Int tile=16);

  // Choose between different types of ImageStore types (single term, multiterm, faceted)
  CountedPtr<SIImageStore> createIMStore(String imageName, 
					 CoordinateSystem& cSys,
					 IPosition imShape, 
					 const Bool overwrite,
					 String mappertype="default", 
					 uInt ntaylorterms=1,
					 Quantity distance=Quantity(0.0, "m"),
					 uInt facets=1,
					 Bool useweightimage=False);
  
  // Choose between different types of Mappers (single term, multiterm, imagemosaic, faceted)
  CountedPtr<SIMapper> createSIMapper(String mappertype,  
					  CountedPtr<SIImageStore> imagestore, //// make this inside !!!!!
				      CountedPtr<refim::FTMachine> ftmachine,
				      CountedPtr<refim::FTMachine> iftmachine,
					  uInt ntaylorterms=1);

  Block<CountedPtr<SIImageStore> > createFacetImageStoreList(
							     CountedPtr<SIImageStore> imagestore,
							     Int facets);
  void setPsfFromOneFacet();

  Bool toUseWeightImage(CountedPtr<refim::FTMachine>& ftm, String mappertype);

  void createVisSet(const Bool writeaccess=False);
  
  void createAWPFTMachine(CountedPtr<FTMachine>& theFT, CountedPtr<FTMachine>& theIFT, 
			  const String& ftmName,
			  const Int facets,          
			  //----------------------------
			  const Int wprojPlane,     
			  const Float padding,      
			  const Bool useAutocorr,   
			  const Bool useDoublePrec, 
			  const String gridFunction,
			  //---------------------------
			  const Bool aTermOn,      
			  const Bool psTermOn,     
			  const Bool mTermOn,      
			  const Bool wbAWP,        
			  const String cfCache,    
			  const Bool doPointing,   
			  const Bool doPBCorr,     
			  const Bool conjBeams,    
			  const Float computePAStep,
			  const Float rotatePAStep, 
			  const Int cache,          
			  const Int tile);
  ATerm* createTelescopeATerm(const MeasurementSet& ms, const Bool& isATermOn);

  void runMajorCycle(const Bool dopsf=False, const Bool savemodel=False);

  /////This function should be called at every define image
  /////It associated the ftmachine with a given field
  ////For facetted image distinct  ft machines will associated with each facets and 
  //// Only one facetted image allowed
  //  void appendToMapperList(String imagename, CoordinateSystem& csys, String ftmachine,
  //		  	  Quantity distance=Quantity(0.0, "m"), Int facets=1, const Bool overwrite=False);

  void appendToMapperList(String imagename, 
			  CoordinateSystem& csys, 
			  IPosition imshape,
			  CountedPtr<refim::FTMachine>& ftm,
			  CountedPtr<refim::FTMachine>& iftm,
		  	  Quantity distance=Quantity(0.0, "m"), 
			  Int facets=1, 
			  const Bool overwrite=False,
			  String mappertype=String("default"),
			  uInt ntaylorterms=1);
  /////////////// Member Objects

  SIMapperCollection itsMappers;
  ///if facetting this storage will keep the unsliced version 
  CountedPtr<SIImageStore> unFacettedImStore_p;


  CoordinateSystem itsMaxCoordSys;
  IPosition itsMaxShape;


  /////////////// All input parameters

  // Data Selection
  // Image Definition
  // Imaging/Gridding

  // Other Options
  Block<const MeasurementSet *> mss_p;
  vi::FrequencySelections fselections_p;
  CountedPtr<vi::VisibilityIterator2>  vi_p;

  /////////////////////////////////////////////////////////////////////////////////
  Bool writeAccess_p;
  MPosition mLocation_p;
  MDirection phaseCenter_p;
  Int facetsStore_p;
  VisImagingWeight imwgt_p;
  Bool imageDefined_p;
  Bool useScratch_p,readOnly_p;
  //
  //  Bool freqFrameValid_p;

  refim::FTMachine::Type datacol_p;

};

  }//# NAMESPACE refim - END
} //# NAMESPACE CASA - END

#endif
