/*
 * MSUVBin.cc implementation of gridding MSs to a gridded MS
//#
//#  CASA - Common Astronomy Software Applications (http://casa.nrao.edu/)

//# Copyright (C) 2014-2015
//# Associated Universities, Inc. Washington DC, USA.
//#
//# This library is free software; you can redistribute it and/or modify it
//# under the terms of the GNU  General Public License as published by
//# the Free Software Foundation; either version 3 of the License, or (at your
//# option) any later version.
//#
//# This library is distributed in the hope that it will be useful, but WITHOUT
//# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  General Public
//# License for more details.
//#
//# You should have received a copy of the GNU  General Public License
//# along with this library; if not, write to the Free Software Foundation,
//# Inc., 675 Massachusetts Ave, Cambridge, MA 02139, USA.
//#

//#        Postal address:
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#
 *
 *  Created on: Feb 4, 2014
 *      Author: kgolap
 */

//#include <boost/math/special_functions/round.hpp>
#include <casa/Arrays/ArrayMath.h>
#include <casa/Arrays/Array.h>
#include <casa/Arrays/Matrix.h>
#include <casa/Arrays/Cube.h>
#include <casa/Arrays/Vector.h>
#include <casa/OS/HostInfo.h>

#include <casa/System/ProgressMeter.h>
#include <casa/Quanta/QuantumHolder.h>
#include <casa/Utilities/CompositeNumber.h>
#include <measures/Measures/MeasTable.h>
#include <ms/MeasurementSets/MSPolColumns.h>
#include <ms/MeasurementSets/MSPolarization.h>
#include <mstransform/MSTransform/MSUVBin.h>
#include <mstransform/MSTransform/MSTransformDataHandler.h>
#include <coordinates/Coordinates/CoordinateSystem.h>
#include <coordinates/Coordinates/DirectionCoordinate.h>
#include <coordinates/Coordinates/SpectralCoordinate.h>
#include <coordinates/Coordinates/StokesCoordinate.h>
#include <images/Images/PagedImage.h>
#include <images/Images/TempImage.h>
#include <msvis/MSVis/MSUtil.h>
#include <msvis/MSVis/VisBuffer.h>
#include <msvis/MSVis/VisBuffer2Adapter.h>
#include <imageanalysis/Utilities/SpectralImageUtil.h>
#include <msvis/MSVis/VisibilityIterator2.h>
#include <scimath/Mathematics/FFTPack.h>
#include <scimath/Mathematics/ConvolveGridder.h>
#include <wcslib/wcsconfig.h>  /** HAVE_SINCOS **/
#include <math.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#if HAVE_SINCOS
#define SINCOS(a,s,c) sincos(a,&s,&c)
#else
#define SINCOS(a,s,c)                   \
     s = sin(a);                        \
     c = cos(a)
#endif



namespace casa { //# NAMESPACE CASA - BEGIN

MSUVBin::MSUVBin():nx_p(0), ny_p(0), nchan_p(0), npol_p(0),existOut_p(False){
	outMsPtr_p=NULL;
	outMSName_p="OutMS.ms";
	memFraction_p=0.5;
}
MSUVBin::MSUVBin(const MDirection& phaseCenter,
		 const Int nx, const Int ny, const Int nchan, const Int npol, Quantity cellx, Quantity celly, Quantity freqStart, Quantity freqStep, Float memFraction, Bool dow):
		existOut_p(False)

{
	phaseCenter_p=phaseCenter;
	nx_p=nx;
	ny_p=ny;
	nchan_p=nchan;
	npol_p=npol;
	deltas_p.resize(2);
	deltas_p[0]=cellx.getValue("rad");
	deltas_p[1]=celly.getValue("rad");
	freqStart_p=freqStart.getValue("Hz");
	freqStep_p=freqStep.getValue("Hz");
	outMsPtr_p=NULL;
	outMSName_p="OutMS.ms";
	memFraction_p=memFraction;
	doW_p=dow;

}

MSUVBin::~MSUVBin(){

}
Bool MSUVBin::selectData(const String& msname, const String& spw, const String& field,
		const String& baseline, const String& scan,
		const String& uvrange, const String& taql,
		const String& subarray, const String& correlation,const String& intent, const String& obs){

	Vector<Int> fakestep = Vector<Int> (1, 1);

	String elms=msname; // cause the following constructor does not accept a const
	MSTransformDataHandler mshandler(elms, Table::Old);
	//No very well documented what the step is supposed to do
	//using the default value here
	if(mshandler.setmsselect(spw, field, baseline, scan, uvrange,
			taql, fakestep, subarray, correlation, intent, obs)){
		mss_p.resize(mss_p.nelements()+1);
		mshandler.makeSelection();
		// have to make a copy here as the selected ms pointer of mshandler
		// dies with mshandler
		mss_p[mss_p.nelements()-1]=new MeasurementSet(*mshandler.getSelectedInputMS());
	}
	else
		return False;

	//	cerr << "num of ms" << mss_p.nelements() << " ms0 " << (mss_p[0])->tableName() << " nrows " << (mss_p[0])->nrow() << endl;
	return True;
}

void MSUVBin::setOutputMS(const String& msname){
	outMSName_p=msname;
}

void MSUVBin::createOutputMS(const Int nrrow){
	if(Table::isReadable(outMSName_p)){
		Int oldnrows=recoverGridInfo(outMSName_p);
		if(oldnrows != nrrow)
			throw(AipsError("Number of grid points requested "+ String::toString(nrrow)+ " does not match "+String::toString(oldnrows)+ " in outputms"));
		existOut_p=True;
		return;
	}
	if(mss_p.nelements()==0)
		throw(AipsError("no ms selected for input yet"));
	Vector<Int> tileShape(3);
	tileShape[0]=4; tileShape[1]=200; tileShape[2]=500;
	outMsPtr_p=MSTransformDataHandler::setupMS(outMSName_p, nchan_p, npol_p,
				Vector<MS::PredefinedColumns>(1, MS::DATA),
				tileShape);
	outMsPtr_p->addRow(nrrow, True);
	//cerr << "mss Info " << mss_p[0]->tableName() << "  " << mss_p[0]->nrow() <<endl;
	MSTransformDataHandler::addOptionalColumns(mss_p[0]->spectralWindow(),
					outMsPtr_p->spectralWindow());
	///Setup pointing why this is not done in setupMS
		{
		SetupNewTable pointingSetup(outMsPtr_p->pointingTableName(),
		                              MSPointing::requiredTableDesc(), Table::New);
		  // POINTING can be large, set some sensible defaults for storageMgrs
		  IncrementalStMan ismPointing ("ISMPointing");
		  StandardStMan ssmPointing("SSMPointing", 32768);
		  pointingSetup.bindAll(ismPointing, True);
		  pointingSetup.bindColumn(MSPointing::columnName(MSPointing::DIRECTION),
		                           ssmPointing);
		  pointingSetup.bindColumn(MSPointing::columnName(MSPointing::TARGET),
		                           ssmPointing);
		  pointingSetup.bindColumn(MSPointing::columnName(MSPointing::TIME),
		                           ssmPointing);
		  outMsPtr_p->rwKeywordSet().defineTable(MS::keywordName(MS::POINTING),
		                                     Table(pointingSetup));
		  outMsPtr_p->initRefs();
		}
		TableCopy::copySubTables(outMsPtr_p->pointing(), (mss_p[0])->pointing());

	MSColumns msc(*outMsPtr_p);
	msc.data().fillColumn(Matrix<Complex>(npol_p, nchan_p, Complex(0.0)));
	msc.flagRow().fillColumn(True);
	msc.flag().fillColumn(Matrix<Bool>(npol_p, nchan_p, True));
	msc.weight().fillColumn(Vector<Float>(npol_p, 0.0));
	msc.sigma().fillColumn(Vector<Float>(npol_p, 0.0));
	msc.weightSpectrum().fillColumn(Matrix<Float>(npol_p, nchan_p, 0.0));
	//change array id for every 2000 rows
	Vector<Int> arrayID(nrrow,0);
	Int blk=2000;
	Int naid=nrrow/blk;
	IPosition blc(1,0);
	IPosition trc(1,0);
	for (Int k=0; k < naid; ++k){
	  blc[0]=k*blk;
	  trc[0]= (k < (naid-1)) ? (k+1)*blk-1 : (k+1)*blk+nrrow%blk-1;
	  arrayID(blc, trc)=k;
	}
	//cerr << "MINMAX array ID " << min(arrayID) << "  " << max(arrayID) << endl;
	msc.arrayId().putColumn(arrayID);
	outMsPtr_p->flush(True);

	existOut_p=False;

}
Int MSUVBin::recoverGridInfo(const String& msname){
	outMsPtr_p=new MeasurementSet(msname, Table::Update);
	if(!outMsPtr_p->keywordSet().isDefined("MSUVBIN")){
		throw(AipsError("The ms "+msname+" was not made with the UV binner"));
	}
	Record rec=outMsPtr_p->keywordSet().asRecord("MSUVBIN");
	rec.get("nx", nx_p);
	rec.get("ny", ny_p);
	rec.get("nchan", nchan_p);
	rec.get("npol", npol_p);
	LogIO os(LogOrigin("MSUVBin", "recoverInfo", WHERE));
	os << "Output is a binned ms of "<< nx_p << " by " << ny_p << " with "<< npol_p << " pol and "<< nchan_p << " channel " << endl;
	CoordinateSystem *tempCoordsys;
	tempCoordsys=CoordinateSystem::restore(rec, "csys");
	if(tempCoordsys==NULL)
		throw(AipsError("could recover grid info from ms"));
	csys_p=*tempCoordsys;
	delete tempCoordsys;
	return outMsPtr_p->nrow();


}

void MSUVBin::storeGridInfo(){
	if(existOut_p)
		return;
	Record rec;
	rec.define("nx", nx_p);
	rec.define("ny", ny_p);
	rec.define("nchan", nchan_p);
	rec.define("npol", npol_p);
	csys_p.save(rec, "csys");
	outMsPtr_p->rwKeywordSet().defineRecord("MSUVBIN", rec);


}
void MSUVBin::setTileCache()
{
	if(outMsPtr_p->tableType() ==Table::Memory){
		return;
	}
	const ColumnDescSet & cds = outMsPtr_p->tableDesc ().columnDescSet ();

	//uInt startrow = 0;



	Vector<String> columns (6);
	columns (0) = MS::columnName (MS::DATA);            // complex
	columns (1) = MS::columnName (MS::FLAG);            // boolean
	columns (2) = MS::columnName (MS::WEIGHT_SPECTRUM); // float
	columns (3) = MS::columnName (MS::WEIGHT);          // float
	columns (4) = MS::columnName (MS::SIGMA);           // float
	columns (5) = MS::columnName (MS::UVW);             // double

	for (uInt k = 0; k < columns.nelements (); ++k) {

		if (! cds.isDefined (columns (k)))
		{
			continue;
		}

		try {
			//////////////////
			//////Temporary fix for virtual ms of multiple real ms's ...miracle of statics
			//////setting the cache size of hypercube at row 0 of each ms.
			///will not work if each subms of a virtual ms has multi hypecube being
			///accessed.

			{
				Bool usesTiles=False;
				String dataManType = RODataManAccessor (*outMsPtr_p, columns[k], True).dataManagerType ();
				//cerr << "column " << columns[k] << " dataman "<< dataManType << endl;
				usesTiles = dataManType.contains ("Tiled");
				if(usesTiles){
					ROTiledStManAccessor tacc (*outMsPtr_p, columns[k], True);
					tacc.clearCaches (); //One tile only for now ...seems to work faster





					/// If some bucketSize is 0...there is trouble in setting cache
					/// but if slicer is used it gushes anyways if one does not set cache
					/// need to fix the 0 bucket size in the filler anyways...then this is not needed


					tacc.setCacheSize (0, 1);


				}
			}
		}
		catch (AipsError x) {
			//  cerr << "Data man type " << dataManType << "  " << dataManType.contains ("Tiled") << "  && " << (!String (cdesc.dataManagerGroup ()).empty ()) << endl;
			//  cerr << "Failed to set settilecache due to " << x.getMesg () << " column " << columns[k]  <<endl;
			//It failed so leave the caching as is
			continue;
	    }
	}

}

Bool MSUVBin::fillOutputMS(){
  //  Double imagevol=Double(nx_p)*Double(ny_p)*Double(npol_p)*Double(nchan_p)*12.0/1e6; //in MB
  // Double memoryMB=Double(HostInfo::memoryFree())/1024.0 ;
  Bool retval;
   retval= fillNewBigOutputMS();
  return retval;
}

Bool MSUVBin::fillNewBigOutputMS(){
	if(mss_p.nelements()==0)
		throw(AipsError("No valid input MSs defined"));
	Vector<Double> incr;
	Vector<Int> cent;
	Matrix<Double> uvw;
	//need to build or recover csys at this stage
	makeCoordsys();
	Double reffreq=SpectralImageUtil::worldFreq(csys_p, Double(nchan_p/2));
	Int nrrows=makeUVW(reffreq, incr, cent, uvw);
	Vector<Bool> rowFlag(nrrows);
	Vector<Int> ant1(nrrows);
	Vector<Int> ant2(nrrows);
	Vector<Double> timeCen(nrrows);
	Matrix<Float> wght(npol_p, nrrows);
	createOutputMS(nrrows);
	setTileCache();
	ROMSColumns msc(*outMsPtr_p);
	vi::VisibilityIterator2 iter(mss_p, vi::SortColumns(), False);
	vi::VisBuffer2* vb=iter.getVisBuffer();
	iter.originChunks();
	iter.origin();
	if(existOut_p){
		msc.weight().getColumn(wght);
		msc.flagRow().getColumn(rowFlag);
		msc.antenna1().getColumn(ant1);
		msc.antenna2().getColumn(ant2);
		msc.time().getColumn(timeCen);
		msc.uvw().getColumn(uvw);
	}
	else{
		wght.set(0.0);
		rowFlag.set(True);
		timeCen.set(vb->time()(0));
	}
	////////////////////////////////////////////////
	Cube<Complex> convFunc;
	Vector<Int> convSupport;
	Double wScale;
	Int convSampling, convSize;
	if(doW_p){
	  makeWConv(iter, convFunc, convSupport, wScale, convSampling, convSize);
	  //makeSFConv(convFunc, convSupport, wScale, convSampling, convSize);
	  // cerr << "convSupport0 " << convFunc.shape() << "   " << convSupport << endl;
	}
	/////////////////////////////////////////////////
	Int usableNchan=Int(Double(HostInfo::memoryFree())*memFraction_p*1024.0/Double(npol_p)/Double(nx_p*ny_p)/12.0);
	if(usableNchan < nchan_p)
	  cerr << "nchan per pass " << min(usableNchan, nchan_p) << endl;
	Int npass=nchan_p%usableNchan==0 ? nchan_p/usableNchan : nchan_p/usableNchan+1;
	if(npass >1)
	  cerr << "Due to lack of memory will be doing  " << npass << " passes through data"<< endl;
	for (Int pass=0; pass < npass; ++pass){
		Int startchan=pass*usableNchan;
		Int endchan=pass==nchan_p/usableNchan ? nchan_p%usableNchan+ startchan-1 : (pass+1)*usableNchan -1 ;

		Cube<Complex> grid(npol_p, endchan-startchan+1, nrrows);
		//cerr << "shape " << grid.shape() << endl;
		Cube<Float> wghtSpec(npol_p, endchan-startchan+1, nrrows);
		Cube<Bool> flag(npol_p, endchan-startchan+1, nrrows);
		//Matrix<Int> locuv;


		iter.originChunks();
		iter.origin();
		vbutil_p=VisBufferUtil(*vb);
		if(existOut_p){
			//recover the previous data for summing
			Slicer elslice(IPosition(2, 0, startchan), IPosition(2,npol_p, endchan-startchan+1));
			msc.data().getColumn(elslice, grid);
			msc.weightSpectrum().getColumn(elslice, wghtSpec);
			msc.flag().getColumn(elslice, flag);
			//multiply the data with weight here
			{
			  for (Int iz=0; iz< grid.shape()(2); ++iz){
			    for(Int iy=0; iy < grid.shape()(1); ++iy){
			      for(Int ix=0; ix < grid.shape()(0); ++ix){
				grid(ix,iy,iz)= grid(ix,iy,iz)*wghtSpec(ix,iy,iz);
			      }
			    }
			  }
			}

		}
		else{
			grid.set(Complex(0));
			wghtSpec.set(0);
			flag.set(True);
		       
			//cerr << "Zeroing  grid " << grid.shape() << endl;
			//outMsPtr_p->addRow(nrrows, True);
		}
		if(npass > 1)
		  cerr <<"Pass " << pass ;
		ProgressMeter pm(1.0, Double(nrrows),"Gridding data",
                         "", "", "", True);
		Double rowsDone=0.0;
     for (iter.originChunks(); iter.moreChunks(); iter.nextChunk()){
	for(iter.origin(); iter.more(); iter.next()){
	    if(doW_p)
	      gridDataConv(*vb, grid, wght, wghtSpec,flag, rowFlag, uvw,
			   ant1,ant2,timeCen, startchan, endchan, convFunc, convSupport, wScale, convSampling);
	    else
	      gridData(*vb, grid, wght, wghtSpec,flag, rowFlag, uvw,
		       ant1,ant2,timeCen, startchan, endchan);
	    rowsDone+=Double(vb->nRows());
	    pm.update(rowsDone);
	  }

	}

     //Weight Correct the data
     {
       for (Int iz=0; iz< grid.shape()(2); ++iz){
	 for(Int iy=0; iy < grid.shape()(1); ++iy){
	   for(Int ix=0; ix < grid.shape()(0); ++ix){
	     grid(ix,iy,iz)= (!flag(ix,iy,iz) && wghtSpec(ix,iy, iz) !=0) ? grid(ix,iy,iz)/wghtSpec(ix,iy,iz): Complex(0);
	   }
	 }
       }
     }

	saveData(grid, flag, rowFlag, wghtSpec, uvw, ant1, ant2, timeCen, startchan, endchan);
	}
	storeGridInfo();
	return True;
}

Bool MSUVBin::fillSmallOutputMS(){
	if(mss_p.nelements()==0)
		throw(AipsError("No valid input MSs defined"));
	Vector<Double> incr;
	Vector<Int> cent;
	Matrix<Double> uvw;
	//need to build or recover csys at this stage
	makeCoordsys();
	Double reffreq=SpectralImageUtil::worldFreq(csys_p, Double(nchan_p/2));
	Int nrrows=makeUVW(reffreq, incr, cent, uvw);
	Cube<Complex> grid(npol_p, nchan_p, nrrows);
	Matrix<Float> wght(npol_p, nrrows);
	Cube<Float> wghtSpec(npol_p, nchan_p, nrrows);
	Cube<Bool> flag(npol_p, nchan_p, nrrows);
	Vector<Bool> rowFlag(nrrows);
	Vector<Int> ant1(nrrows);
	Vector<Int> ant2(nrrows);
	Vector<Double> timeCen(nrrows);
	createOutputMS(nrrows);
	Matrix<Int> locuv;
	vi::VisibilityIterator2 iter(mss_p, vi::SortColumns(), False);
	vi::VisBuffer2* vb=iter.getVisBuffer();
	
	iter.originChunks();
	iter.origin();
	vbutil_p=VisBufferUtil(*vb);
	if(existOut_p){
        //recover the previous data for summing

		ROMSColumns msc(*outMsPtr_p);
		msc.data().getColumn(grid);
		msc.weightSpectrum().getColumn(wghtSpec);
		msc.weight().getColumn(wght);
		msc.flag().getColumn(flag);
		msc.flagRow().getColumn(rowFlag);
		msc.antenna1().getColumn(ant1);
		msc.antenna2().getColumn(ant2);
		msc.time().getColumn(timeCen);
		msc.uvw().getColumn(uvw);
	}
	else{
		grid.set(Complex(0));
		wght.set(0);
		wghtSpec.set(0);
		flag.set(True);
		rowFlag.set(True);
		//cerr << "SETTING time to val" << vb->time()(0) << endl;
		timeCen.set(vb->time()(0));
		//outMsPtr_p->addRow(nrrows, True);
	}
	ProgressMeter pm(1.0, Double(nrrows),"Gridding data",
                         "", "", "", True);
	Double rowsDone=0.0;
	for (iter.originChunks(); iter.moreChunks(); iter.nextChunk()){
	  for(iter.origin(); iter.more(); iter.next()){
	    locateuvw(locuv, incr, cent, vb->uvw());
	    gridData(*vb, grid, wght, wghtSpec,flag, rowFlag, uvw,
		     ant1,ant2,timeCen, locuv);
	    rowsDone+=Double(vb->nRows());
	    pm.update(rowsDone);
	  }
		      
	}

	saveData(grid, flag, rowFlag, wghtSpec, wght, uvw, ant1, ant2, timeCen);
	storeGridInfo();
	return True;
}

Bool MSUVBin::fillBigOutputMS(){

	if(mss_p.nelements()==0)
			throw(AipsError("No valid input MSs defined"));
	Vector<Double> incr;
	Vector<Int> cent;
	Matrix<Double> uvw;
	//need to build or recover csys at this stage
	makeCoordsys();
	Double reffreq=SpectralImageUtil::worldFreq(csys_p, Double(nchan_p/2));
	Int nrrows=makeUVW(reffreq, incr, cent, uvw);
	createOutputMS(nrrows);
	if(!existOut_p)
	  MSColumns(*outMsPtr_p).uvw().putColumn(uvw);
	nrrows=0;
	for (uInt k=0; k < mss_p.nelements() ; ++k)
	  nrrows+=(mss_p[k])->nrow();
	
	vi::VisibilityIterator2 iter(mss_p, vi::SortColumns(), False);
	vi::VisBuffer2* vb=iter.getVisBuffer();
	iter.originChunks();
	iter.origin();
	vbutil_p=VisBufferUtil(*vb);
	Matrix<Int> locuv;
	
	ProgressMeter pm(1.0, Double(nrrows),"Gridding data",
                         "", "", "", True);
	Double rowsDone=0.0;
	for (iter.originChunks(); iter.moreChunks(); iter.nextChunk()){
				for(iter.origin(); iter.more(); iter.next()){
	
				  //locateuvw(locuv, incr, cent, vb->uvw());
				  inplaceGridData(*vb);
					//
					//gridData(*vb, grid, wght, wghtSpec,flag, rowFlag, uvw,
					//		ant1,ant2,timeCen, locuv);
				  rowsDone+=Double(vb->nRows());
				  pm.update(rowsDone);
				}
	}
	
	if(!existOut_p){
		fillSubTables();
	}
	////Need to do the weight calculation here from spectral weight.
	weightSync();
	storeGridInfo();
	return True;
}

Int MSUVBin::makeUVW(const Double reffreq, Vector<Double>& increment,
		Vector<Int>& center, Matrix<Double>& uvw){
		Vector<Int> shp(2);
		shp(0)=nx_p; shp(1)=ny_p;
		Int directionIndex=csys_p.findCoordinate(Coordinate::DIRECTION);
		DirectionCoordinate thedir=csys_p.directionCoordinate(directionIndex);
		Coordinate *ftcoord=thedir.makeFourierCoordinate(Vector<Bool>(2, True), shp);
		increment=ftcoord->increment();
		increment *= C::c/reffreq;
		//Vector<Float> scale(2);
		//scale(0)=1.0/(nx_p*thedir.increment()(0));
		//scale(1)=1.0/(ny_p*thedir.increment()(1));
		center.resize(2);
		center(0)=nx_p/2;
		center(1)=ny_p/2;
		Vector<Int> npix(2);
		npix(0)=nx_p;
		npix(1)=ny_p;
		uvw.resize(3, nx_p*ny_p);
		uvw.set(0.0);
		Vector<Double> px(2);
		Vector<Double> wld(2);
		Double lambd=C::c/reffreq;
		Int counter=0;
	    for (Int k=0; k < ny_p;++k ){
	    	px(1)=Double(k);
	    	//px(1)=Double(k-ny_p/2)*C::c/reffreq*scale(1);
	    	for(Int j=0; j < nx_p; ++j){
	    		//px(0)=Double(j-nx_p/2)*C::c/reffreq*scale(0);
	    		//uvw(0, k*nx_p+j)=px(0);
	    		//uvw(1, k*nx_p+j)=px(1);
	    		//wld(0)=Double(j);
	    		//wld(1)=Double(k);
	    		px(0)=Double(j);
	    		if(ftcoord->toWorld(wld, px)){
	    			//cerr << "wld " << wld*C::c/reffreq << " px " << uvw(0,k*nx_p+j) << ", "<< uvw(1, k*nx_p+j) << endl;
	    			uvw(0, k*nx_p+j)=wld(0)*lambd;
	    			uvw(1, k*nx_p+j)=wld(1)*lambd;
	    			++counter;
	    		}
	    	}
	    }
		return counter;
}
void MSUVBin::makeCoordsys(){

	//if outMS already has the coordsys..it is recovered
	if(existOut_p)
		return;
	//
	Matrix<Double> xform(2,2);
	xform=0.0;xform.diagonal()=1.0;
	MVAngle ra=phaseCenter_p.getAngle("rad").getValue()(0);
	ra(0.0);
	MVAngle dec=phaseCenter_p.getAngle("rad").getValue()(1);

	DirectionCoordinate myRaDec(MDirection::Types(phaseCenter_p.getRefPtr()->getType()),
			Projection::SIN,
			ra.radian(), dec.radian(),
			deltas_p(0), deltas_p(1),
			xform,
		      Double(nx_p)/2.0, Double(ny_p)/2.0);

	//Now to the spectral part
	//LSRK frame unless REST
	MFrequency::Types outFreqFrame=MFrequency::LSRK;
	Vector<MFrequency::Types> typesInData;
	MSUtil::getSpectralFrames(typesInData, *(mss_p[0]));
	if(anyEQ(typesInData, MFrequency::REST))
		outFreqFrame=MFrequency::REST;
	SpectralCoordinate mySpectral(outFreqFrame, freqStart_p, freqStep_p, 0.0);
	ROMSColumns msc(*mss_p[0]);
	Int ddId=msc.dataDescId()(0);
	Int firstPolId=msc.dataDescription().polarizationId()(ddId);
	Int polType = Vector<Int>(msc.polarization().corrType()(firstPolId))(0);
	Bool isCircular=True;
	if (polType>=Stokes::XX && polType<=Stokes::YY)
		isCircular=False;

	if(isCircular){
		if(npol_p==4){
			whichStokes_p.resize(4);

			whichStokes_p(0)=Stokes::RR; whichStokes_p(1)=Stokes::RL;
			      whichStokes_p(2)=Stokes::LR; whichStokes_p(3)=Stokes::LL;
		}
		else if(npol_p==2){
			whichStokes_p.resize(2);
			whichStokes_p(0)=Stokes::RR; whichStokes_p(1)=Stokes::LL;
		}
		else if(npol_p==1){
			whichStokes_p.resize(1);
			whichStokes_p(0)=Stokes::RR;
		}
	}
	else{
		if(npol_p==4){
			whichStokes_p.resize(4);
			whichStokes_p(0)=Stokes::XX; whichStokes_p(1)=Stokes::XY;
			whichStokes_p(2)=Stokes::YX; whichStokes_p(3)=Stokes::YY;
		}
		else if(npol_p==2){
			whichStokes_p.resize(2);
			whichStokes_p(0)=Stokes::XX; whichStokes_p(1)=Stokes::YY;
		}
		else if(npol_p==1){
			whichStokes_p.resize(1);
			whichStokes_p(0)=Stokes::XX;
		}
	}
	StokesCoordinate myStokes(whichStokes_p);
	csys_p=CoordinateSystem();
	csys_p.addCoordinate(myRaDec);
	csys_p.addCoordinate(myStokes);
	csys_p.addCoordinate(mySpectral);
	ObsInfo obsinf;
	obsinf.setObsDate(msc.timeMeas()(0));
	obsinf.setTelescope(msc.observation().telescopeName()(0));
	obsinf.setPointingCenter(phaseCenter_p.getValue());
	csys_p.setObsInfo(obsinf);
}

void MSUVBin::locateuvw(Matrix<Int>& locuv, const Vector<Double>& increment,
		const Vector<Int>& center, const Matrix<Double>& uvw){
	locuv.resize(2, uvw.shape()(1));
	for (Int k=0; k <uvw.shape()(1); ++k){
		for(Int j=0; j < 2; ++j)
			locuv(j,k)=Int(Double(center(j))+uvw(j,k)/increment(j)+0.5);
	}


}
  Bool MSUVBin::datadescMap(const vi::VisBuffer2& vb, Double& fracbw){
	polMap_p.resize(vb.nCorrelations());
	polMap_p.set(-1);
	//Should ultimately find the real polarization in case it is a  non common MS
	if(npol_p==4){
		if( polMap_p.nelements()==4)
			indgen(polMap_p);
		if(polMap_p.nelements()==2){
			polMap_p(0)=0;
			polMap_p(1)=3;
		}
		//This case sounds bad
		if(polMap_p.nelements()==1)
			polMap_p(0)=0;
	}
	if(npol_p==2){
		if( polMap_p.nelements()==4){
			polMap_p(0)=0;
			polMap_p(3)=1;
		}
		if(polMap_p.nelements()==2)
			indgen(polMap_p);
		//again a bad case ...not resolving this here
		if(polMap_p.nelements()==1)
			polMap_p(0)=0;
	}
	else if(npol_p==1){
		polMap_p(0)=0;
		if(polMap_p.nelements()==2)
		  polMap_p(1)=0;
		if(polMap_p.nelements()==4)
		  polMap_p(3)=0;
	}
	////Now to chanmap
	fracbw=0.0;
	Vector<Double> f(1);
	Vector<Double> c(1,0.0);
	SpectralCoordinate spec=csys_p.spectralCoordinate(2);
	Vector<Double> visFreq=vb.getFrequencies(0, MFrequency::LSRK);
	//Vector<Double> visFreq;
	//vbutil_p.convertFrequency(visFreq, vb, MFrequency::LSRK);
	chanMap_p.resize(visFreq.nelements());
	chanMap_p.set(-1);
	for (Int chan=0; chan < vb.nChannels(); ++chan){
		f[0]=visFreq[chan];
		if(spec.toPixel(c,f)){
			Int pixel=Int(floor(c(0)+0.5));  // round to chan freq at chan center
			if(pixel > -1 && pixel< nchan_p){
				chanMap_p(chan)=pixel;
				c[0]=pixel;
				spec.toWorld(f, c);
				//cout << " pixel " << pixel << " chan " << chan << endl;
				if((abs(visFreq[chan]-f[0])/f[0]) > fracbw)
				  fracbw=(abs(visFreq[chan]-f[0])/f[0]);
			}
		}
	}

	///////////////////////
	//cout << "spw" << vb.spectralWindows()(0) << "  rowid " << vb.rowIds()(0) << " max " << max(chanMap_p) << " sum "<< ntrue(chanMap_p > -1) << endl;
	/*if( ntrue(chanMap_p > -1) > 0){
	  for (uInt k=0; k < visFreq.nelements() ; ++k){
	    if(chanMap_p(k) > -1){
	      cerr << "  " <<k<< "  "<<  visFreq[k] << " =  "  << chanMap_p[k];
	    }
	  }
	  cerr <<endl;
	  cerr << "*************************************" << endl;

	}
	*/
	////////////////////////////////////
	//cerr << "spw " << vb.spectralWindows() << " fracbw " << fracbw << endl;
	if(allLT(chanMap_p ,0)  ||  allLT(polMap_p , 0))
	  return False;

	return True;
}

  void MSUVBin::weightSync(){
    MSColumns msc(*outMsPtr_p);
    Matrix<Float> wght(npol_p, outMsPtr_p->nrow());
    Matrix<Float> wghtSpec;
    for (uInt row=0; row < outMsPtr_p->nrow(); ++row){
      for (uInt pol=0; pol < wght.shape()(0); ++pol){
	
	wghtSpec=msc.weightSpectrum().get(row);
//cerr << "shape min max "<< median(wghtSpec.xyPlane(newrow).row(pol)) << " " << min(wghtSpec.xyPlane(newrow).row(pol)) << "  "<< max(wghtSpec.xyPlane(newrow).row(pol)) << endl;
	    wght(pol,row)=median(wghtSpec.row(pol));
	     //cerr << "pol " << pol << " newrow "<< newrow << " weight "<< wght(pol, newrow) << endl;
      }
    }
    msc.weight().putColumn(wght);	
    Matrix<Float> sigma(wght.shape());
    for (uInt k=0; k<wght.shape()[1]; ++k)
      for (uInt j=0; j<wght.shape()[0]; ++j)
	sigma(j,k)=wght(j,k)>0.0 ? 1/sqrt(wght(j,k)) :0.0;
    msc.sigma().putColumn(sigma);


  }

void MSUVBin::inplaceGridData(const vi::VisBuffer2& vb){
	//we need polmap and chanmap;
        // no point of proceeding if nothing maps in this vb
  Double fracbw;
  if(!datadescMap(vb, fracbw)) return;
  //cerr << "fracbw " << fracbw << endl; 

  if(fracbw >0.05)
    inplaceLargeBW(vb);
  else
    inplaceSmallBW(vb);



		
}

  void MSUVBin::inplaceLargeBW(const vi::VisBuffer2& vb){
	//Dang i thought the new vb will return Data or FloatData if correctedData was
	//not there
	Bool hasCorrected=!(ROMSMainColumns(vb.getVi()->ms()).correctedData().isNull());
	Int nrows=vb.nRows();
	//	Cube<Complex> grid(npol_p, nchan_p, nrows);
	//	Matrix<Float> wght(npol_p,nrows);
	//	Cube<Float> wghtSpec(npol_p,nchan_p,nrows);
	//	Cube<Bool> flag(npol_p, nchan_p,nrows);
	//	Vector<Bool> rowFlag(nrows);
	//	Matrix<Double> uvw(3, nrows);
	//	Vector<Int> ant1(nrows);
	//	Vector<Int> ant2(nrows);
	//	Vector<Double> timeCen(nrows);
	//	Vector<uInt> rowids(nrows);
	//cerr << outMsPtr_p.null() << endl;
	MSColumns msc(*outMsPtr_p);
	SpectralCoordinate spec=csys_p.spectralCoordinate(2);
	DirectionCoordinate thedir=csys_p.directionCoordinate(0);
	Vector<Float> scale(2);
	scale(0)=fabs(nx_p*thedir.increment()(0))/C::c;
	scale(1)=fabs(ny_p*thedir.increment()(1))/C::c;
	///Index
	Vector<Int> rowToIndex(nx_p*ny_p, -1);
	Matrix<Int> locu(vb.nChannels(), nrows, -1);
	Matrix<Int> locv(vb.nChannels(), nrows, -1);
	Vector<Double> visFreq=vb.getFrequencies(0, MFrequency::LSRK);
	Vector<Double> phasor;
	Matrix<Double> eluvw;
	Bool needRot=vbutil_p.rotateUVW(vb, phaseCenter_p, eluvw, phasor);
	Int numUniq=0;
	for (Int k=0; k <nrows; ++k){
	  for(Int chan=0; chan < vb.nChannels(); ++chan ){
	    if(chanMap_p(chan) >=0){
	      locv(chan, k)=Int(Double(ny_p)/2.0+eluvw(1,k)*visFreq(chan)*scale(1)+0.5);
	      locu(chan, k)=Int(Double(nx_p)/2.0+eluvw(0,k)*visFreq(chan)*scale(0)+0.5);
	      Int newrow=locv(chan, k)*nx_p+locu(chan, k);
	      if(rowToIndex[newrow] <0){ 
		rowToIndex[newrow]=numUniq;
		++numUniq;
	      }
	    }
	  }
	}
	//cerr << "numrows " << vb.nRows() << " nuniq " << numUniq << endl;
	Cube<Complex> grid(npol_p, nchan_p, numUniq);
	Matrix<Float> wght(npol_p,numUniq);
	Cube<Float> wghtSpec(npol_p,nchan_p,numUniq);
	Cube<Bool> flag(npol_p, nchan_p,numUniq);
	Vector<Bool> rowFlag(numUniq);
	Matrix<Double> uvw(3, numUniq);
	Vector<Int> ant1(numUniq);
	Vector<Int> ant2(numUniq);
	Vector<Double> timeCen(numUniq);
	Vector<uInt> rowids(numUniq);
	Vector<Bool> rowvisited(numUniq, False);
	
	Vector<Double> invLambda=visFreq/C::c;
	Vector<Double> phasmult(vb.nChannels(),0.0);
	Complex elphas(1.0, 0.0);
	for (Int k=0; k < nrows; ++k){ 
	  if(needRot)
	    phasmult=phasor(k)*invLambda;
	 for(Int chan=0; chan < vb.nChannels(); ++chan ){
	   if(chanMap_p(chan) >=0){ 
	     //Have to reject uvws out of range.
	     if(locu(chan,k) > -1 && locu(chan, k) < nx_p && locv(chan,k)> -1 && locv(chan,k) < ny_p){
	       Int newrow=locv(chan,k)*nx_p+locu(chan,k);
		Int actrow=rowToIndex[newrow];
		rowids[actrow]=uInt(newrow);
		if(!rowvisited[actrow]){
		  rowvisited[actrow]=True;
		  rowFlag[actrow]=msc.flagRow()(newrow);
		  wghtSpec[actrow]=msc.weightSpectrum().get(newrow);
		  flag[actrow]=msc.flag().get(newrow);
		  uvw[actrow]=msc.uvw().get(newrow);
		  ant1[actrow]=msc.antenna1()(newrow);
		  ant2[actrow]=msc.antenna2()(newrow);
		  timeCen[actrow]=msc.time()(newrow);
		  grid[actrow]=msc.data().get(newrow);
		}
		if(rowFlag[actrow] && !(vb.flagRow()(k))){
		  //	  cerr << newrow << " rowFlag " << rowFlag[actrow] <<" rowids " << vb.rowIds()[k]  << " uvw2 " << uvw(2 ,actrow) << endl;
		  rowFlag[actrow]=False;
		  uvw(2,actrow)=eluvw(2,k);
		  //	  cerr << newrow << " rowFlag " << rowFlag[actrow] <<" rowids " << vb.rowIds()[k]  << " uvw2 " << uvw(2 ,actrow) << endl;
		  ant1[actrow]=vb.antenna1()(k);
		  ant2[actrow]=vb.antenna2()(k);
		  timeCen[actrow]=vb.time()(k);		  
		}
		for(Int pol=0; pol < vb.nCorrelations(); ++pol){
		  if((!vb.flagCube()(pol,chan, k)) && (polMap_p(pol)>=0) && (vb.weight()(pol,k)>0.0)){		    
		    Complex toB=hasCorrected ? vb.visCubeCorrected()(pol,chan,k)*vb.weight()(pol,k):
		      vb.visCube()(pol,chan,k)*vb.weight()(pol,k);
		    if(needRot){
		      Double s, c;
		      SINCOS(phasmult(chan), s, c);
		      Complex elphas(c, s);
		      toB *= elphas;
		    }
			grid(polMap_p(pol),chanMap_p(chan),actrow)
			  = (grid(polMap_p(pol),chanMap_p(chan),actrow)*wghtSpec(polMap_p(pol),chanMap_p(chan),actrow)
			     + toB)/(vb.weight()(pol,k)+wghtSpec(polMap_p(pol),chanMap_p(chan),actrow));
			flag(polMap_p(pol),chanMap_p(chan),actrow)=False;
			//cerr << "weights " << max(vb.weight()) << "  spec " << max(vb.weightSpectrum()) << endl;
			//wghtSpec(polMap_p(pol),chanMap_p(chan), newrow)+=vb.weightSpectrum()(pol, chan, k);
			wghtSpec(polMap_p(pol),chanMap_p(chan),actrow) += vb.weight()(pol,k);
		  }
		}
	     }
	   }
	   //This should go for later
	   /*for (Int pol=0; pol < wght.shape()(0); ++pol){
	     //cerr << "shape min max "<< median(wghtSpec.xyPlane(newrow).row(pol)) << " " << min(wghtSpec.xyPlane(newrow).row(pol)) << "  "<< max(wghtSpec.xyPlane(newrow).row(pol)) << endl;
	     wght(pol,actrow)=median(wghtSpec.xyPlane(actrow).row(pol));
	     //cerr << "pol " << pol << " newrow "<< newrow << " weight "<< wght(pol, newrow) << endl;
	     }*/
	 }
	}
	//now lets put back the stuff
	//cerr << "rowids " << rowids << endl;
	RefRows elrow(rowids);
	msc.flagRow().putColumnCells(elrow, rowFlag);
	//reference row
	//Matrix<Complex>matdata(grid.xyPlane(k));
	msc.data().putColumnCells(elrow, grid);
	//Matrix<Float> matwgt(wghtSpec.xyPlane(k));
	msc.weightSpectrum().putColumnCells(elrow, wghtSpec);
	//Matrix<Bool> matflag(flag.xyPlane(k));
	msc.flag().putColumnCells(elrow, flag);
	//Vector<Double> vecuvw(uvw.column(k));
	msc.uvw().putColumnCells(elrow, uvw);
	//cerr << "ant1 " << ant1 << endl;
	msc.antenna1().putColumnCells(elrow, ant1);
	msc.antenna2().putColumnCells(elrow, ant2);
	msc.time().putColumnCells(elrow, timeCen);
	//outMsPtr_p->flush(True);
	///////


  }

  void MSUVBin::inplaceSmallBW(const vi::VisBuffer2& vb){
	//Dang i thought the new vb will return Data or FloatData if correctedData was
	//not there
	Bool hasCorrected=!(ROMSMainColumns(vb.getVi()->ms()).correctedData().isNull());
	Int nrows=vb.nRows();
	MSColumns msc(*outMsPtr_p);
	SpectralCoordinate spec=csys_p.spectralCoordinate(2);
	Double refFreq;
	spec.toWorld(refFreq, Double(nchan_p)/2.0);
	DirectionCoordinate thedir=csys_p.directionCoordinate(0);
	Vector<Float> scale(2);
	scale(0)=(nx_p*thedir.increment()(0))/C::c;
	scale(1)=(ny_p*thedir.increment()(1))/C::c;
	///Index
	Vector<Int> rowToIndex(nx_p*ny_p, -1);
	Vector<Int> locu(nrows, -1);
	Vector<Int> locv(nrows, -1);
	Vector<Double> visFreq=vb.getFrequencies(0, MFrequency::LSRK);
	Int numUniq=0;
	Vector<Double> phasor;
	Matrix<Double> eluvw;
	eluvw=vb.uvw();
	Bool needRot=vbutil_p.rotateUVW(vb, phaseCenter_p, eluvw, phasor);
	//cerr << "SHAPES " << eluvw.shape() << "    " << vb.uvw().shape() << endl;
	for (Int k=0; k <nrows; ++k){
	  locv(k)=Int(Double(ny_p)/2.0+eluvw(1,k)*refFreq*scale(1)+0.5);
	  locu(k)=Int(Double(nx_p)/2.0+eluvw(0,k)*refFreq*scale(0)+0.5);
	  //cerr << "locu " << locu(k) << " locv " << locv(k) <<  " eluvw " << eluvw(0,k) << "  uvw " << vb.uvw()(0,k) << endl;
	  if(locu(k) > -1 && locu(k) < nx_p && locv(k)> -1 && locv(k) < ny_p){
		Int newrow=locv(k)*nx_p+locu(k);
		if(rowToIndex[newrow] <0){ 
		  rowToIndex[newrow]=numUniq;
		  ++numUniq;
		}
	      }
	    
	}
  
	//cerr << "numrows " << vb.nRows() << " nuniq " << numUniq << endl;
	Cube<Complex> grid(npol_p, nchan_p, numUniq);
	Matrix<Float> wght(npol_p,numUniq);
	Cube<Float> wghtSpec(npol_p,nchan_p,numUniq);
	Cube<Bool> flag(npol_p, nchan_p,numUniq);
	Vector<Bool> rowFlag(numUniq);
	Matrix<Double> uvw(3, numUniq);
	Vector<Int> ant1(numUniq);
	Vector<Int> ant2(numUniq);
	Vector<Double> timeCen(numUniq);
	Vector<uInt> rowids(numUniq);
	Vector<Bool> rowvisited(numUniq, False);
	
	Vector<Double> invLambda=visFreq/C::c;
	for (Int k=0; k < nrows; ++k){
	  
	  if(locu(k) > -1 && locu(k) < nx_p && locv(k)> -1 && locv(k) < ny_p){
	    Int newrow=locv(k)*nx_p+locu(k);
	    Int actrow=rowToIndex[newrow];
	    rowids[actrow]=uInt(newrow);
	    if(!rowvisited[actrow]){
	      rowvisited[actrow]=True;
	      rowFlag[actrow]=msc.flagRow()(newrow);
	      wghtSpec[actrow]=msc.weightSpectrum().get(newrow);
	      flag[actrow]=msc.flag().get(newrow);
	      uvw[actrow]=msc.uvw().get(newrow);
	      ant1[actrow]=msc.antenna1()(newrow);
	      ant2[actrow]=msc.antenna2()(newrow);
	      timeCen[actrow]=msc.time()(newrow);
	      grid[actrow]=msc.data().get(newrow);
	    }
	    if(rowFlag[actrow] && !(vb.flagRow()(k))){
		  //	  cerr << newrow << " rowFlag " << rowFlag[actrow] <<" rowids " << vb.rowIds()[k]  << " uvw2 " << uvw(2 ,actrow) << endl;
	      rowFlag[actrow]=False;
	      uvw(2,actrow)=eluvw(2,k);
	      //	  cerr << newrow << " rowFlag " << rowFlag[actrow] <<" rowids " << vb.rowIds()[k]  << " uvw2 " << uvw(2 ,actrow) << endl;
	      ant1[actrow]=vb.antenna1()(k);
	      ant2[actrow]=vb.antenna2()(k);
	      timeCen[actrow]=vb.time()(k);		  
	    }
	    Complex elphas(1.0, 0.0);
	    for(Int chan=0; chan < vb.nChannels(); ++chan ){
	     
	      if(chanMap_p(chan) >=0){ 
		if(needRot){
		  Double phasmult=phasor(k)*invLambda(chan);
		  Double s, c;
		  SINCOS(phasmult, s, c);
		  elphas=Complex(c, s);
		}
		 
		for(Int pol=0; pol < vb.nCorrelations(); ++pol){
		  if((!vb.flagCube()(pol,chan, k)) && (polMap_p(pol)>=0) && (vb.weight()(pol,k)>0.0)){
		    Complex toB=hasCorrected ? vb.visCubeCorrected()(pol,chan,k)*vb.weight()(pol,k):
		      vb.visCube()(pol,chan,k)*vb.weight()(pol,k);
		    if(needRot)
		      toB *= elphas;
		    grid(polMap_p(pol),chanMap_p(chan),actrow)
			  = (grid(polMap_p(pol),chanMap_p(chan),actrow)*wghtSpec(polMap_p(pol),chanMap_p(chan),actrow)
			     + toB)/(vb.weight()(pol,k)+wghtSpec(polMap_p(pol),chanMap_p(chan),actrow));
		    flag(polMap_p(pol),chanMap_p(chan),actrow)=False;
			//cerr << "weights " << max(vb.weight()) << "  spec " << max(vb.weightSpectrum()) << endl;
			//wghtSpec(polMap_p(pol),chanMap_p(chan), newrow)+=vb.weightSpectrum()(pol, chan, k);
		    wghtSpec(polMap_p(pol),chanMap_p(chan),actrow) += vb.weight()(pol,k);
		  }
		}
	     }
	   }
	   //This should go for later
	   /*for (Int pol=0; pol < wght.shape()(0); ++pol){
	     //cerr << "shape min max "<< median(wghtSpec.xyPlane(newrow).row(pol)) << " " << min(wghtSpec.xyPlane(newrow).row(pol)) << "  "<< max(wghtSpec.xyPlane(newrow).row(pol)) << endl;
	     wght(pol,actrow)=median(wghtSpec.xyPlane(actrow).row(pol));
	     //cerr << "pol " << pol << " newrow "<< newrow << " weight "<< wght(pol, newrow) << endl;
	     }*/
	  }
	}
	//now lets put back the stuff
	RefRows elrow(rowids);
	//cerr << "ROWIDS " << rowids << endl;
	msc.flagRow().putColumnCells(elrow, rowFlag);
	//reference row
	//Matrix<Complex>matdata(grid.xyPlane(k));
	msc.data().putColumnCells(elrow, grid);
	//Matrix<Float> matwgt(wghtSpec.xyPlane(k));
	msc.weightSpectrum().putColumnCells(elrow, wghtSpec);
	//Matrix<Bool> matflag(flag.xyPlane(k));
	msc.flag().putColumnCells(elrow, flag);
	//Vector<Double> vecuvw(uvw.column(k));
	msc.uvw().putColumnCells(elrow, uvw);
	//cerr << "ant1 " << ant1 << endl;
	msc.antenna1().putColumnCells(elrow, ant1);
	msc.antenna2().putColumnCells(elrow, ant2);
	msc.time().putColumnCells(elrow, timeCen);
	//outMsPtr_p->flush(True);
	///////


  }



void MSUVBin::gridData(const vi::VisBuffer2& vb, Cube<Complex>& grid,
		Matrix<Float>& /*wght*/, Cube<Float>& wghtSpec,
		Cube<Bool>& flag, Vector<Bool>& rowFlag, Matrix<Double>& uvw, Vector<Int>& ant1,
		Vector<Int>& ant2, Vector<Double>& timeCen, const Matrix<Int>& /*locuv*/){
	//all pixel that are touched the flag and flag Row shall be unset and the w be assigned
		//later we'll deal with multiple w for the same uv
		//we need polmap and chanmap;
  
  Double fracbw;
  if(!datadescMap(vb, fracbw)) return;
  //cerr << "fracbw " << fracbw << endl;
    SpectralCoordinate spec=csys_p.spectralCoordinate(2);
    DirectionCoordinate thedir=csys_p.directionCoordinate(0);
    Double refFreq=SpectralImageUtil::worldFreq(csys_p, Double(nchan_p/2));
    //Double refFreq=SpectralImageUtil::worldFreq(csys_p, Double(0));
    Vector<Float> scale(2);
    scale(0)=fabs(nx_p*thedir.increment()(0))/C::c;
    scale(1)=fabs(ny_p*thedir.increment()(1))/C::c;
    //Dang i thought the new vb will return Data or FloatData if correctedData was
	    //not there
	    Bool hasCorrected=!(ROMSMainColumns(vb.getVi()->ms()).correctedData().isNull());
		//locateuvw(locuv, vb.uvw());
	    Vector<Double> visFreq=vb.getFrequencies(0, MFrequency::LSRK);
		for (Int k=0; k < vb.nRows(); ++k){
		  if(!vb.flagRow()[k]){
		  Int locu, locv;
		  {
		    locv=Int(Double(ny_p)/2.0+vb.uvw()(1,k)*refFreq*scale(1)+0.5);
		    locu=Int(Double(nx_p)/2.0+vb.uvw()(0,k)*refFreq*scale(0)+0.5);

		  }
		  //cerr << "fracbw " << fracbw << " pixel u " << Double(locu-nx_p/2)/refFreq/scale(0) << " data u" << vb.uvw()(0,k) <<
		//		  " pixel v "<< Double(locv-ny_p/2)/refFreq/scale(1) << " data v "<< vb.uvw()(1,k) << endl;
		    //Double newU=	Double(locu-nx_p/2)/refFreq/scale(0);
		    //Double newV= Double(locv-ny_p/2)/refFreq/scale(1);
		    //Double phaseCorr=((newU/vb.uvw()(0,k)-1)+(newV/vb.uvw()(1,k)-1))*vb.uvw()(2,k)*2.0*C::pi*refFreq/C::c;
		    //Double phaseCorr=((newU-vb.uvw()(0,k))+(newV-vb.uvw()(1,k)))*2.0*C::pi*refFreq/C::c;
		    //cerr << "fracbw " << fracbw << " pixel u " << Double(locu-nx_p/2)/refFreq/scale(0) << " data u" << vb.uvw()(0,k) <<
		    //				  " pixel v "<< Double(locv-ny_p/2)/refFreq/scale(1) << " data v "<< vb.uvw()(1,k) << " phase " << phaseCorr << endl;
		  for(Int chan=0; chan < vb.nChannels(); ++chan ){
		    if(chanMap_p(chan) >=0){
		      //Double outChanFreq;
		      //spec.toWorld(outChanFreq, Double(chanMap_p(chan)));
		      if(fracbw > 0.05)
		      {
		    	  locv=Int(Double(ny_p)/2.0+vb.uvw()(1,k)*visFreq(chan)*scale(1)+0.5);
		    	  locu=Int(Double(nx_p)/2.0+vb.uvw()(0,k)*visFreq(chan)*scale(0)+0.5);
		      }
		      if(locv < ny_p && locu < nx_p){ 
				  Int newrow=locv*nx_p+locu;
				  if(rowFlag(newrow) && !(vb.flagRow()(k))){
				    rowFlag(newrow)=False;
				    /////TEST
				    //uvw(0,newrow)=vb.uvw()(0,k);
				    //uvw(1,newrow)=vb.uvw()(1,k);
				    /////
				    uvw(2,newrow)=vb.uvw()(2,k);
				    //cerr << newrow << " rowids " << vb.rowIds()[k]  << " uvw2 " << uvw(2 ,newrow) << endl;
				    ant1(newrow)=vb.antenna1()(k);
				    ant2(newrow)=vb.antenna2()(k);
				    timeCen(newrow)=vb.time()(k);

				  }
				  for(Int pol=0; pol < vb.nCorrelations(); ++pol){
				    if((!vb.flagCube()(pol,chan, k)) && (polMap_p(pol)>=0) && (vb.weight()(pol,k)>0.0)){
				  //  Double newU=	Double(locu-nx_p/2)/refFreq/scale(0);
				 //   Double newV= Double(locv-ny_p/2)/refFreq/scale(1);
				 //   Double phaseCorr=((newU/vb.uvw()(0,k)-1)+(newV/vb.uvw()(1,k)-1))*vb.uvw()(2,k)*2.0*C::pi*refFreq/C::c;
				      Complex toB=hasCorrected ? vb.visCubeCorrected()(pol,chan,k)*vb.weight()(pol,k):
					vb.visCube()(pol,chan,k)*vb.weight()(pol,k);
				    //  Double s, c;
				    //SINCOS(phaseCorr, s, c);
				    //  toB=toB*Complex(c,s);
				      grid(polMap_p(pol),chanMap_p(chan), newrow)
					= (grid(polMap_p(pol),chanMap_p(chan), newrow)*wghtSpec(polMap_p(pol),chanMap_p(chan),newrow)
					   + toB)/(vb.weight()(pol,k)+wghtSpec(polMap_p(pol),chanMap_p(chan),newrow));
				      flag(polMap_p(pol),chanMap_p(chan), newrow)=False;
				      //cerr << "weights " << max(vb.weight()) << "  spec " << max(vb.weightSpectrum()) << endl;
				      //wghtSpec(polMap_p(pol),chanMap_p(chan), newrow)+=vb.weightSpectrum()(pol, chan, k);
				      wghtSpec(polMap_p(pol),chanMap_p(chan), newrow) += vb.weight()(pol,k);
				    }
				    ///We should do that at the end totally
				    //wght(pol,newrow)=median(wghtSpec.xyPlane(newrow).row(pol));
				  }
		      }//locu && locv
		    }
		  }
			//sum wgtspec along channels for weight
			/*for (Int pol=0; pol < wght.shape()(0); ++pol){
				//cerr << "shape min max "<< median(wghtSpec.xyPlane(newrow).row(pol)) << " " << min(wghtSpec.xyPlane(newrow).row(pol)) << "  "<< max(wghtSpec.xyPlane(newrow).row(pol)) << endl;
				wght(pol,newrow)=median(wghtSpec.xyPlane(newrow).row(pol));
				//cerr << "pol " << pol << " newrow "<< newrow << " weight "<< wght(pol, newrow) << endl;
			}
			*/
		  }
		}




}

  void MSUVBin::gridData(const vi::VisBuffer2& vb, Cube<Complex>& grid,
		Matrix<Float>& /*wght*/, Cube<Float>& wghtSpec,
		Cube<Bool>& flag, Vector<Bool>& rowFlag, Matrix<Double>& uvw, Vector<Int>& ant1,
		Vector<Int>& ant2, Vector<Double>& timeCen, const Int startchan, const Int endchan){
	//all pixel that are touched the flag and flag Row shall be unset and the w be assigned
		//later we'll deal with multiple w for the same uv
		//we need polmap and chanmap;

  Double fracbw;
  if(!datadescMap(vb, fracbw)) return;
  //cerr << "fracbw " << fracbw << endl;
    SpectralCoordinate spec=csys_p.spectralCoordinate(2);
    DirectionCoordinate thedir=csys_p.directionCoordinate(0);
    Double refFreq=SpectralImageUtil::worldFreq(csys_p, Double(nchan_p/2));
    //Double refFreq=SpectralImageUtil::worldFreq(csys_p, Double(0));
    Vector<Float> scale(2);
    scale(0)=fabs(nx_p*thedir.increment()(0))/C::c;
    scale(1)=fabs(ny_p*thedir.increment()(1))/C::c;
    //Dang i thought the new vb will return Data or FloatData if correctedData was
	    //not there
	    Bool hasCorrected=!(ROMSMainColumns(vb.getVi()->ms()).correctedData().isNull());
		//locateuvw(locuv, vb.uvw());
	    Vector<Double> visFreq=vb.getFrequencies(0, MFrequency::LSRK);
		for (Int k=0; k < vb.nRows(); ++k){
		  if(!vb.flagRow()[k]){
		  Int locu, locv;
		  {
		    locv=Int(Double(ny_p)/2.0+vb.uvw()(1,k)*refFreq*scale(1)+0.5);
		    locu=Int(Double(nx_p)/2.0+vb.uvw()(0,k)*refFreq*scale(0)+0.5);

		  }
		  //cerr << "fracbw " << fracbw << " pixel u " << Double(locu-nx_p/2)/refFreq/scale(0) << " data u" << vb.uvw()(0,k) <<
		//		  " pixel v "<< Double(locv-ny_p/2)/refFreq/scale(1) << " data v "<< vb.uvw()(1,k) << endl;
		    //Double newU=	Double(locu-nx_p/2)/refFreq/scale(0);
		    //Double newV= Double(locv-ny_p/2)/refFreq/scale(1);
		    //Double phaseCorr=((newU/vb.uvw()(0,k)-1)+(newV/vb.uvw()(1,k)-1))*vb.uvw()(2,k)*2.0*C::pi*refFreq/C::c;
		    //Double phaseCorr=((newU-vb.uvw()(0,k))+(newV-vb.uvw()(1,k)))*2.0*C::pi*refFreq/C::c;
		    //cerr << "fracbw " << fracbw << " pixel u " << Double(locu-nx_p/2)/refFreq/scale(0) << " data u" << vb.uvw()(0,k) <<
		    //				  " pixel v "<< Double(locv-ny_p/2)/refFreq/scale(1) << " data v "<< vb.uvw()(1,k) << " phase " << phaseCorr << endl;
		  for(Int chan=0; chan < vb.nChannels(); ++chan ){
		    if(chanMap_p(chan) >=startchan && chanMap_p(chan) <=endchan){
		      //Double outChanFreq;
		      //spec.toWorld(outChanFreq, Double(chanMap_p(chan)));
		      if(fracbw > 0.05)
		      {
		    	  locv=Int(Double(ny_p)/2.0+vb.uvw()(1,k)*visFreq(chan)*scale(1)+0.5);
		    	  locu=Int(Double(nx_p)/2.0+vb.uvw()(0,k)*visFreq(chan)*scale(0)+0.5);
		      }
		      if(locv < ny_p && locu < nx_p){
				  Int newrow=locv*nx_p+locu;
				  if(rowFlag(newrow) && !(vb.flagRow()(k))){
				    rowFlag(newrow)=False;
				    /////TEST
				    //uvw(0,newrow)=vb.uvw()(0,k);
				    //uvw(1,newrow)=vb.uvw()(1,k);
				    /////
				    uvw(2,newrow)=vb.uvw()(2,k);
				    //cerr << newrow << " rowids " << vb.rowIds()[k]  << " uvw2 " << uvw(2 ,newrow) << endl;
				    ant1(newrow)=vb.antenna1()(k);
				    ant2(newrow)=vb.antenna2()(k);
				    timeCen(newrow)=vb.time()(k);

				  }
				  for(Int pol=0; pol < vb.nCorrelations(); ++pol){
				    if((!vb.flagCube()(pol,chan, k)) && (polMap_p(pol)>=0) && (vb.weight()(pol,k)>0.0)){
				  //  Double newU=	Double(locu-nx_p/2)/refFreq/scale(0);
				 //   Double newV= Double(locv-ny_p/2)/refFreq/scale(1);
				 //   Double phaseCorr=((newU/vb.uvw()(0,k)-1)+(newV/vb.uvw()(1,k)-1))*vb.uvw()(2,k)*2.0*C::pi*refFreq/C::c;
				      Complex toB=hasCorrected ? vb.visCubeCorrected()(pol,chan,k)*vb.weight()(pol,k):
					vb.visCube()(pol,chan,k)*vb.weight()(pol,k);
				    //  Double s, c;
				    //SINCOS(phaseCorr, s, c);
				    //  toB=toB*Complex(c,s);
				      grid(polMap_p(pol),chanMap_p(chan)-startchan, newrow)
					= (grid(polMap_p(pol),chanMap_p(chan)-startchan, newrow)
					   + toB); ///(vb.weight()(pol,k)+wghtSpec(polMap_p(pol),chanMap_p(chan)-startchan,newrow));
				      flag(polMap_p(pol),chanMap_p(chan)-startchan, newrow)=False;
				      //cerr << "weights " << max(vb.weight()) << "  spec " << max(vb.weightSpectrum()) << endl;
				      //wghtSpec(polMap_p(pol),chanMap_p(chan), newrow)+=vb.weightSpectrum()(pol, chan, k);
				      wghtSpec(polMap_p(pol),chanMap_p(chan)-startchan, newrow) += vb.weight()(pol,k);
				    }
				    ///We should do that at the end totally
				    //wght(pol,newrow)=median(wghtSpec.xyPlane(newrow).row(pol));
				  }
		      }//locu && locv
		    }
		  }
			//sum wgtspec along channels for weight
			/*for (Int pol=0; pol < wght.shape()(0); ++pol){
				//cerr << "shape min max "<< median(wghtSpec.xyPlane(newrow).row(pol)) << " " << min(wghtSpec.xyPlane(newrow).row(pol)) << "  "<< max(wghtSpec.xyPlane(newrow).row(pol)) << endl;
				wght(pol,newrow)=median(wghtSpec.xyPlane(newrow).row(pol));
				//cerr << "pol " << pol << " newrow "<< newrow << " weight "<< wght(pol, newrow) << endl;
			}
			*/
		  }
		}




}
void MSUVBin::gridDataConv(const vi::VisBuffer2& vb, Cube<Complex>& grid,
		Matrix<Float>& /*wght*/, Cube<Float>& wghtSpec,
		Cube<Bool>& flag, Vector<Bool>& rowFlag, Matrix<Double>& uvw, Vector<Int>& ant1,
			 Vector<Int>& ant2, Vector<Double>& timeCen, const Int startchan, const Int endchan, const Cube<Complex>& convFunc, const Vector<Int>& convSupport, const Double wScale, const Int convSampling){
  //all pixel that are touched the flag and flag Row shall be unset and the w be assigned
  //later we'll deal with multiple w for the same uv
  //we need polmap and chanmap;
  
  Double fracbw;
  if(!datadescMap(vb, fracbw)) return;
  //cerr << "fracbw " << fracbw << endl;
  SpectralCoordinate spec=csys_p.spectralCoordinate(2);
  DirectionCoordinate thedir=csys_p.directionCoordinate(0);
  Double refFreq=SpectralImageUtil::worldFreq(csys_p, Double(nchan_p/2));
  //Double refFreq=SpectralImageUtil::worldFreq(csys_p, Double(0));
  Vector<Float> scale(2);
  scale(0)=fabs(nx_p*thedir.increment()(0))/C::c;
  scale(1)=fabs(ny_p*thedir.increment()(1))/C::c;
  //Dang i thought the new vb will return Data or FloatData if correctedData was
  //not there
  Bool hasCorrected=!(ROMSMainColumns(vb.getVi()->ms()).correctedData().isNull());
  //locateuvw(locu v, vb.uvw());
  Vector<Double> visFreq=vb.getFrequencies(0, MFrequency::LSRK);
  //cerr << "support " << convSupport << endl;
  Vector<Double> phasor;
  Matrix<Double> eluvw;
  eluvw=vb.uvw();
  Bool needRot=vbutil_p.rotateUVW(vb, phaseCenter_p, eluvw, phasor);
  Vector<Double> invLambda=visFreq/C::c;
  for (Int k=0; k < vb.nRows(); ++k){
    if(!vb.flagRow()[k]){
      Int locu, locv, locw,supp, offu, offv;
      {
	locv=Int(Double(ny_p)/2.0+vb.uvw()(1,k)*refFreq*scale(1)+0.5);
	locu=Int(Double(nx_p)/2.0+vb.uvw()(0,k)*refFreq*scale(0)+0.5);
	offv=Int ((Double(locv)-(Double(ny_p)/2.0+vb.uvw()(1,k)*refFreq*scale(1)))*Double(convSampling)+0.5);
	offu=Int ((Double(locu)-(Double(nx_p)/2.0+vb.uvw()(0,k)*refFreq*scale(0)))*Double(convSampling)+0.5);
	locw=Int(sqrt(fabs(wScale*vb.uvw()(2,k)*refFreq/C::c))+0.5);
	//cerr << "locw " << locw << "   " << " offs " << offu << "  " << offv << endl;
	supp=locw < convSupport.shape()[0] ? convSupport(locw) :convSupport(convSupport.nelements()-1) ;
	//////////////////
	//supp=10;
	/////////////////
      }
		  //cerr << "fracbw " << fracbw << " pixel u " << Double(locu-nx_p/2)/refFreq/scale(0) << " data u" << vb.uvw()(0,k) <<
		//		  " pixel v "<< Double(locv-ny_p/2)/refFreq/scale(1) << " data v "<< vb.uvw()(1,k) << endl;
		    //Double newU=	Double(locu-nx_p/2)/refFreq/scale(0);
		    //Double newV= Double(locv-ny_p/2)/refFreq/scale(1);
		    //Double phaseCorr=((newU/vb.uvw()(0,k)-1)+(newV/vb.uvw()(1,k)-1))*vb.uvw()(2,k)*2.0*C::pi*refFreq/C::c;
		    //Double phaseCorr=((newU-vb.uvw()(0,k))+(newV-vb.uvw()(1,k)))*2.0*C::pi*refFreq/C::c;
		    //cerr << "fracbw " << fracbw << " pixel u " << Double(locu-nx_p/2)/refFreq/scale(0) << " data u" << vb.uvw()(0,k) <<
		    //				  " pixel v "<< Double(locv-ny_p/2)/refFreq/scale(1) << " data v "<< vb.uvw()(1,k) << " phase " << phaseCorr << endl;
		  Vector<Int> newrow((2*supp+1)*(2*supp+1), -1);
		  if(locv < ny_p && locu < nx_p){
		    for (Int yy=0; yy<  Int(2*supp+1); ++yy){
		      Int newlocv=locv+ yy-supp;
		      if(newlocv >0 && newlocv < ny_p){
			for (Int xx=0; xx<  Int(2*supp+1); ++xx){
			  Int newlocu=locu+xx-supp;
			  if(newlocu < nx_p && newlocu >0){
			    newrow(yy*(2*supp+1)+xx)=newlocv*nx_p+newlocu;
			  }			  
			}
		      }
		    }
		  }
		  Complex elphas(1.0, 0.0);
		  for(Int chan=0; chan < vb.nChannels(); ++chan ){
		    if(chanMap_p(chan) >=startchan && chanMap_p(chan) <=endchan){
		      //Double outChanFreq;
		      //spec.toWorld(outChanFreq, Double(chanMap_p(chan)));
		      if(needRot){
			Double phasmult=phasor(k)*invLambda(chan);
			Double s, c;
			SINCOS(phasmult, s, c);
			elphas=Complex(c, s);
		      }
		      if(fracbw > 0.05)
		      {
		    	  locv=Int(Double(ny_p)/2.0+vb.uvw()(1,k)*visFreq(chan)*scale(1)+0.5);
		    	  locu=Int(Double(nx_p)/2.0+vb.uvw()(0,k)*visFreq(chan)*scale(0)+0.5);
		      }
		      if(locv < ny_p && locu < nx_p){
			for (Int yy=0; yy< Int(2*supp+1); ++yy){
			  Int locy=abs((yy-supp)*convSampling+offv);
			  //cerr << "y " << yy << " locy " << locy ;
			  for (Int xx=0; xx< Int(2*supp+1); ++xx){
			    Int locx=abs((xx-supp)*convSampling+offu);
			    //cerr << " x " << xx << " locx " << locx << endl;
			    Int jj=yy*(2*supp+1)+xx;
			    if(newrow[jj] >-1){
			      Complex cwt=convFunc(locx, locy, locw);
			      ////////////////TEST
			      //cwt=Complex(1.0, 0.0);
			      ///////////////////////////////////
			      if(vb.uvw()(2,k) > 0.0)
				      cwt=conj(cwt);
			      if(rowFlag(newrow[jj]) && !(vb.flagRow()(k))){
				rowFlag(newrow[jj])=False;
				/////TEST
				//uvw(0,newrow)=vb.uvw()(0,k);
				//uvw(1,newrow)=vb.uvw()(1,k);
				/////
				uvw(2,newrow[jj])=0;
				    //cerr << newrow << " rowids " << vb.rowIds()[k]  << " uvw2 " << uvw(2 ,newrow) << endl;
				ant1(newrow[jj])=vb.antenna1()(k);
				ant2(newrow[jj])=vb.antenna2()(k);
				timeCen(newrow[jj])=vb.time()(k);

			      }
			      Int lechan=chanMap_p(chan)-startchan;
			      for(Int pol=0; pol < vb.nCorrelations(); ++pol){
				if((!vb.flagCube()(pol,chan, k)) && (polMap_p(pol)>=0) && (vb.weight()(pol,k)>0.0) && (fabs(cwt) > 0.0)){
				  //  Double newU=	Double(locu-nx_p/2)/refFreq/scale(0);
				  //   Double newV= Double(locv-ny_p/2)/refFreq/scale(1);
				  //   Double phaseCorr=((newU/vb.uvw()(0,k)-1)+(newV/vb.uvw()(1,k)-1))*vb.uvw()(2,k)*2.0*C::pi*refFreq/C::c;
				  Complex toB=hasCorrected ? vb.visCubeCorrected()(pol,chan,k)*vb.weight()(pol,k)*cwt:
				    vb.visCube()(pol,chan,k)*vb.weight()(pol,k)*cwt;
				  if(needRot)
				    toB *=elphas;
				    //  Double s, c;
				    //SINCOS(phaseCorr, s, c);
				    //  toB=toB*Complex(c,s);
				  //Float elwgt=vb.weight()(pol,k)* fabs(real(cwt));
				  //////////////TESTING
				  Float elwgt=vb.weight()(pol,k)* real(cwt);
				  ///////////////////////////
				  grid(polMap_p(pol),lechan, newrow[jj])
				    = (grid(polMap_p(pol),lechan, newrow[jj])// *wghtSpec(polMap_p(pol),lechan,newrow[jj])
				       + toB); ///(elwgt+wghtSpec(polMap_p(pol),lechan,newrow[jj]));
				      flag(polMap_p(pol), lechan, newrow[jj])=False;
				      //cerr << "weights " << max(vb.weight()) << "  spec " << max(vb.weightSpectrum()) << endl;
				      //wghtSpec(polMap_p(pol),chanMap_p(chan), newrow)+=vb.weightSpectrum()(pol, chan, k);
				      wghtSpec(polMap_p(pol),lechan, newrow[jj]) += elwgt;
				    }
				    ///We should do that at the end totally
				    //wght(pol,newrow)=median(wghtSpec.xyPlane(newrow).row(pol));
				  }
			  }//if(newrow)
			}
			}
		      }//locu && locv
		    }
		  }
			//sum wgtspec along channels for weight
			/*for (Int pol=0; pol < wght.shape()(0); ++pol){
				//cerr << "shape min max "<< median(wghtSpec.xyPlane(newrow).row(pol)) << " " << min(wghtSpec.xyPlane(newrow).row(pol)) << "  "<< max(wghtSpec.xyPlane(newrow).row(pol)) << endl;
				wght(pol,newrow)=median(wghtSpec.xyPlane(newrow).row(pol));
				//cerr << "pol " << pol << " newrow "<< newrow << " weight "<< wght(pol, newrow) << endl;
			}
			*/
		  }
		}




}

Bool MSUVBin::saveData(const Cube<Complex>& grid, const Cube<Bool>&flag, const Vector<Bool>& rowFlag,
				const Cube<Float>&wghtSpec,
				const Matrix<Double>& uvw, const Vector<Int>& ant1,
				const Vector<Int>& ant2, const Vector<Double>& timeCen, const Int startchan, const Int endchan){
	Bool retval=True;
	MSColumns msc(*outMsPtr_p);
	if(!existOut_p && startchan==0){
		fillSubTables();
		msc.uvw().putColumn(uvw);

	}

	Slicer elslice(IPosition(2, 0, startchan), IPosition(2,npol_p, endchan-startchan+1));
	//lets put ny_p slices
	Int nchan=endchan-startchan+1;
	Slice polslice(0,npol_p);
	Slice chanslice(0,nchan);
	for (Int k=0; k <ny_p; ++k){
		//Slicer rowslice(IPosition(1, k*nx_p), IPosition(1,nx_p));
		RefRows rowslice(k*nx_p, (k+1)*nx_p-1);
		msc.data().putColumnCells(rowslice, elslice, grid(polslice, chanslice, Slice(k*nx_p, nx_p)));
		//msc.data().putColumnRange(rowslice, elslice, grid(polslice, chanslice, Slice(k*nx_p, nx_p)));
		msc.weightSpectrum().putColumnCells(rowslice, elslice, wghtSpec(polslice,chanslice, Slice(k*nx_p, nx_p)));
		//msc.weightSpectrum().putColumnRange(rowslice, elslice, wghtSpec(polslice,chanslice, Slice(k*nx_p, nx_p)));
		//////msc.weight().putColumn(wght);
		msc.flag().putColumnCells(rowslice, elslice,flag(polslice, chanslice, Slice(k*nx_p,nx_p)));
		//msc.flag().putColumnRange(rowslice, elslice,flag(polslice, chanslice, Slice(k*nx_p,nx_p)));
	}
	if(endchan==nchan_p-1){
		msc.flagRow().putColumn(rowFlag);
		msc.antenna1().putColumn(ant1);
		msc.antenna2().putColumn(ant2);
		Cube<Float> spectralweight;
		msc.weightSpectrum().getColumn(spectralweight);
		Matrix<Float> weight(npol_p, wghtSpec.shape()[2]);
		for (Int row=0; row < wghtSpec.shape()[2]; ++row){
				for (Int pol=0; pol < npol_p; ++pol){
				//cerr << "shape min max "<< median(wghtSpec.xyPlane(newrow).row(pol)) << " " << min(wghtSpec.xyPlane(newrow).row(pol)) << "  "<< max(wghtSpec.xyPlane(newrow).row(pol)) << endl;
				weight(pol,row)=max(spectralweight.xyPlane(row).row(pol));
	    //if(!rowFlag(row))
	    //  cerr << "pol " << pol << " row "<< row << " median  "<< weight(pol, row) << " min-max " << (min(wghtSpec.xyPlane(row).row(pol))+max(wghtSpec.xyPlane(row).row(pol)))/2.0 << " mean " << mean(wghtSpec.xyPlane(row).row(pol)) << endl;
			}
		}

		msc.weight().putColumn(weight);
		Matrix<Float> sigma(weight.shape());
		for (uInt k=0; k<weight.shape()[1]; ++k)
			for (uInt j=0; j<weight.shape()[0]; ++j)
				sigma(j,k)=weight(j,k)>0.0 ? 1/sqrt(weight(j,k)) :0.0;
		msc.sigma().putColumn(sigma);

		msc.time().putColumn(timeCen);
		msc.timeCentroid().putColumn(timeCen);
	}
	return retval;


}
Bool MSUVBin::saveData(const Cube<Complex>& grid, const Cube<Bool>&flag, const Vector<Bool>& rowFlag,
				const Cube<Float>&wghtSpec, const Matrix<Float>& /*wght*/,
				const Matrix<Double>& uvw, const Vector<Int>& ant1,
				const Vector<Int>& ant2, const Vector<Double>& timeCen){
	Bool retval=True;
	MSColumns msc(*outMsPtr_p);
	if(!existOut_p){
		fillSubTables();
		msc.uvw().putColumn(uvw);

	}


	msc.data().putColumn(grid);
	msc.weightSpectrum().putColumn(wghtSpec);
	//msc.weight().putColumn(wght);
	msc.flag().putColumn(flag);
	msc.flagRow().putColumn(rowFlag);
	msc.antenna1().putColumn(ant1);
	msc.antenna2().putColumn(ant2);
	Matrix<Float> weight(npol_p, wghtSpec.shape()[2]);
	for (Int row=0; row < wghtSpec.shape()[2]; ++row){
	  for (Int pol=0; pol < npol_p; ++pol){
	    //cerr << "shape min max "<< median(wghtSpec.xyPlane(newrow).row(pol)) << " " << min(wghtSpec.xyPlane(newrow).row(pol)) << "  "<< max(wghtSpec.xyPlane(newrow).row(pol)) << endl;
	    weight(pol,row)=max(wghtSpec.xyPlane(row).row(pol));
	    //if(!rowFlag(row))
	    //  cerr << "pol " << pol << " row "<< row << " median  "<< weight(pol, row) << " min-max " << (min(wghtSpec.xyPlane(row).row(pol))+max(wghtSpec.xyPlane(row).row(pol)))/2.0 << " mean " << mean(wghtSpec.xyPlane(row).row(pol)) << endl;
	  }
	}
	msc.weight().putColumn(weight);
	Matrix<Float> sigma(weight.shape());
	for (uInt k=0; k<weight.shape()[1]; ++k)
		for (uInt j=0; j<weight.shape()[0]; ++j)
			sigma(j,k)=weight(j,k)>0.0 ? 1/sqrt(weight(j,k)) :0.0;
	msc.sigma().putColumn(sigma);
	msc.time().putColumn(timeCen);
	msc.timeCentroid().putColumn(timeCen);

	return retval;


}
void MSUVBin::fillSubTables(){
	fillFieldTable();
	copySubtable("SPECTRAL_WINDOW", mss_p[0]->spectralWindow(), True);
	copySubtable("POLARIZATION", mss_p[0]->polarization(), True);
	copySubtable("DATA_DESCRIPTION", mss_p[0]->dataDescription(), True);
	copySubtable("FEED", mss_p[0]->feed(), False);
	copySubtable("OBSERVATION", mss_p[0]->observation(), False);
	copySubtable("ANTENNA", mss_p[0]->antenna(), False);
	fillDDTables();

}
void MSUVBin::fillDDTables(){
	{
		MSPolarizationColumns mspol(outMsPtr_p->polarization());

		if(outMsPtr_p->polarization().nrow()==0)
			outMsPtr_p->polarization().addRow();
		mspol.numCorr().put(0, npol_p);
		mspol.corrType().put(0,whichStokes_p);
		Matrix<Int> corrProd(2,npol_p);
		corrProd.set(0);
		if(npol_p==2){
			corrProd(0,1)=1;
			corrProd(1,1)=1;
		}
		if(npol_p==4){
			corrProd(0,1)=0;
			corrProd(1,1)=1;
			corrProd(0,2)=1;
			corrProd(1,2)=0;
			corrProd(0,3)=1;
			corrProd(1,3)=1;
		}
		mspol.corrProduct().put(0,corrProd);
		mspol.flagRow().put(0,False);
	}
	///Now with Spectral window
	{
		MSSpWindowColumns msSpW(outMsPtr_p->spectralWindow());
		if(outMsPtr_p->spectralWindow().nrow()==0){
			MSTransformDataHandler::addOptionalColumns(mss_p[0]->spectralWindow(),
					outMsPtr_p->spectralWindow());
			outMsPtr_p->spectralWindow().addRow();
		}
		msSpW.name().put(0,"none");
		msSpW.ifConvChain().put(0,0);
		msSpW.numChan().put(0,nchan_p);
		SpectralCoordinate spec=csys_p.spectralCoordinate(2);
		Double refFreq;
		spec.toWorld(refFreq,0.0);
		Double chanBandWidth=spec.increment()[0];
		Vector<Double> chanFreq(nchan_p),resolution(nchan_p);
		for (Int i=0; i < nchan_p; ++i) {
		    spec.toWorld(chanFreq[i], Double(i));
		}
		resolution=chanBandWidth;
		msSpW.chanFreq().put(0,chanFreq);
		msSpW.chanWidth().put(0,resolution);
		msSpW.effectiveBW().put(0,resolution);
		msSpW.refFrequency().put(0,refFreq);
		msSpW.resolution().put(0,resolution);
		msSpW.totalBandwidth().put(0,abs(nchan_p*chanBandWidth));
		msSpW.netSideband().put(0,1);
		msSpW.freqGroup().put(0,0);
		msSpW.freqGroupName().put(0,"none");
		msSpW.flagRow().put(0,False);
		msSpW.measFreqRef().put(0,MFrequency::LSRK);
	}
	//Now the DD
	{
		MSDataDescColumns msDD(outMsPtr_p->dataDescription());
		if(outMsPtr_p->dataDescription().nrow()==0)
			outMsPtr_p->dataDescription().addRow();
		msDD.spectralWindowId().put(0,0);
		msDD.polarizationId().put(0,0);
		msDD.flagRow().put(0,False);
	}
	MSColumns(*outMsPtr_p).dataDescId().fillColumn(0);


}
void MSUVBin::copySubtable(const String& tabName, const Table& inTab,const Bool norows)
{
	//outMsPtr_p->closeSubTables();
	String outName(outMsPtr_p->tableName() + '/' + tabName);

	if (PlainTable::tableCache()(outName)){
		cerr << "cpy subtable "<< outName << endl;
		Table outTab(outName, Table::Update);
		if(norows){
			Vector<uInt> rownums=outTab.rowNumbers();
			outTab.removeRow(rownums);
		}
		else{
			TableCopy::copySubTables(outTab, inTab);
			TableCopy::copyInfo(outTab, inTab);
			//cerr << "ROWS "<< inTab.nrow() << " " << outTab.nrow() << endl;
			TableCopy::copyRows(outTab, inTab);
			/*outTab=Table();
			cerr << "copying " << tabName << endl;
			inTab.copy(outName, Table::New);
			*/
		}
	}
	else{
		inTab.deepCopy(outName, Table::New, False, Table::AipsrcEndian, norows);
	}
	Table outTab(outName, Table::Update);
	outMsPtr_p->rwKeywordSet().defineTable(tabName, outTab);
	outMsPtr_p->initRefs();

	return;
}

void MSUVBin::fillFieldTable() {
    MSFieldColumns msField(outMsPtr_p->field());

    if(outMsPtr_p->field().nrow()==0)
    	outMsPtr_p->field().addRow();

    msField.sourceId().put(0, 0);
    msField.code().put(0, "");
    String sourceName="MSUVBIN";
    Int fieldId=ROMSColumns(*mss_p[0]).fieldId()(0);

    sourceName=ROMSFieldColumns((mss_p[0])->field()).name()(fieldId);
    msField.name().put(0, sourceName);
    Int numPoly = 0;

    //
    Vector<MDirection> radecMeas(1);
    radecMeas(0)=phaseCenter_p;

    Double obsTime=ROMSColumns(*mss_p[0]).time()(0);
    msField.time().put(0, obsTime);
    msField.numPoly().put(0, numPoly);
    msField.delayDirMeasCol().put(0, radecMeas);
    msField.phaseDirMeasCol().put(0, radecMeas);
    msField.referenceDirMeasCol().put(0, radecMeas);
    msField.flagRow().put(0, False);
    MSColumns(*outMsPtr_p).fieldId().fillColumn(0);
}

Bool MSUVBin::String2MDirection(const String& theString,
		    MDirection& theMeas, const String msname){

  istringstream istr(theString);
  Int fieldid;
  istr >> fieldid;
  if(!istr.fail() && msname != ""){
	  //We'll interprete string as a field id of ms
	  MeasurementSet thems(msname);
	  theMeas=ROMSFieldColumns(thems.field()).phaseDirMeas(fieldid);
	  return True;
  }


  QuantumHolder qh;
  String error;

  Vector<String> str;
  //In case of compound strings with commas or empty space
  sepCommaEmptyToVectorStrings(str, theString);

  if(str.nelements()==3){
	  qh.fromString(error, str[1]);
	  casa::Quantity val1=qh.asQuantity();
      qh.fromString(error, str[2]);
      casa::Quantity val2=qh.asQuantity();
      MDirection::Types tp;
      if(!MDirection::getType(tp, str[0])){
    	  ostringstream oss;
    	  oss << "Could not understand Direction frame...defaulting to J2000 " ;
    	  cerr << oss.str() << endl;
    	  tp=MDirection::J2000;
      }
      theMeas=MDirection(val1,val2,  tp);
      return True;
  }
  else if(str.nelements()==2){
	  qh.fromString(error, str[0]);
      casa::Quantity val1=qh.asQuantity();
      qh.fromString(error, str[1]);
      casa::Quantity val2=qh.asQuantity();
      theMeas=MDirection(val1, val2);
      return True;
 }
  else if(str.nelements()==1){
      //Must be a string like sun, moon, jupiter
	  casa::Quantity val1(0.0, "deg");
      casa::Quantity val2(90.0, "deg");
      theMeas=MDirection(val1, val2);
      MDirection::Types ref;
      Int numAll;
      Int numExtra;
      const uInt *dum;
      const String *allTypes=MDirection::allMyTypes(numAll, numExtra, dum);
      //if it is SUN moon etc
      if(MDirection::getType(ref,str[0])){

    	  theMeas=MDirection(val1, val2, ref);
    	  return True;
      }
      if(MeasTable::Source(theMeas, str[0])){
    	  return True;
      }
      if(!MDirection::getType(ref, str[0])){
    	  Vector<String> all(numExtra);
    	  for(Int k =0; k < numExtra; ++k){
    		  all[k]=*(allTypes+numAll-k-1);
    	  }
    	  ostringstream oss;
    	  oss << "Could not understand Direction string " <<str[0] << "\n"  ;
    	  oss << "Valid ones are " << all;
    	  cerr << oss.str() <<  " or one of the valid known sources in the data repos" << endl;
    	  theMeas=MDirection(val1, val2);
    	  return False;
      }

  }




  ///If i am here i don't know how to interprete this


  return False;
}


void MSUVBin::makeSFConv(Cube<Complex>& convFunc, Vector<Int>& convSupport, Double& wScale, Int& convSampling, Int& convSize){
  Vector<Double> uvOffset(2);
  uvOffset(0)=Double(nx_p)/2.0;
  uvOffset(1)=Double(ny_p)/2.0;
  Vector<Double> uvScale(2);
  //cerr << "increments " << csys_p.increment() << endl;
  uvScale(0)=fabs(nx_p*csys_p.increment()(0));
  uvScale(1)=fabs(ny_p*csys_p.increment()(1));
  /////no w 
  wScale=0.0;
  
  ConvolveGridder<Double, Complex>
    gridder(IPosition(2, nx_p, ny_p), uvScale, uvOffset, "SF");
  convSupport.resize(1);
  convSupport=gridder.cSupport()(0);
  convSize=convSupport(0);
  convSampling=gridder.cSampling();
  //cerr << " support " << gridder.cSupport() <<  "  sampling " << convSampling << endl;
  
  Vector<Double> cfunc1D=gridder.cFunction();

  //cerr << "convFunc " << cfunc1D << endl;
  convFunc.resize(convSampling*convSupport(0), convSampling*convSupport(0), 1);
  convFunc.set(Complex(0.0));
  for (Int k=0; k <  convSampling*convSupport(0); ++k){
    for (int j=0; j <  convSampling*convSupport(0); ++j){
      convFunc(j, k,0)=Complex(cfunc1D(k)*cfunc1D(j));
    }
  }
  
}

void MSUVBin::makeWConv(vi::VisibilityIterator2& iter, Cube<Complex>& convFunc, Vector<Int>& convSupport,
			Double& wScale, Int& convSampling, Int& convSize){
  ///////Let's find  ... the maximum w
  vi::VisBuffer2 *vb=iter.getVisBuffer();
  Double maxW=0.0;
  Double minW=1e99;
  Double nval=0;
  Double rmsW=0.0;
  for (iter.originChunks(); iter.moreChunks(); iter.nextChunk()) {
    for (iter.origin(); iter.more(); iter.next()) {
      maxW=max(maxW, max(abs(vb->uvw().row(2)*max(vb->getFrequencies(0))))/C::c);
	minW=min(minW, min(abs(vb->uvw().row(2)*min(vb->getFrequencies(0))))/C::c);
	///////////some shenanigans as some compilers is confusing * operator for vector
	Vector<Double> elw;
	elw=vb->uvw().row(2);
	elw*=vb->uvw().row(2);
	//////////////////////////////////////////////////
	rmsW+=sum(elw);

	nval+=Double(vb->nRows());
      }
    }
    rmsW=sqrt(rmsW/Double(nval))*max(vb->getFrequencies(0))/C::c;
    Double maxUVW=rmsW < 0.5*(minW+maxW) ? 1.05*maxW: (rmsW /(0.5*((minW)+maxW))*1.05*maxW) ;
    Int wConvSize=Int(maxUVW*fabs(sin(fabs(csys_p.increment()(0))*max(nx_p, ny_p)/2.0)));
    wScale=Double((wConvSize-1)*(wConvSize-1))/maxUVW;
    if(wConvSize <2) 
      ThrowCc("Should not be using wprojection");
    Int maxMemoryMB=HostInfo::memoryTotal(true)/1024;
    Double maxConvSizeConsidered=sqrt(Double(maxMemoryMB)/8.0*1024.0*1024.0/Double(wConvSize));
    CompositeNumber cn(Int(maxConvSizeConsidered/2.0)*2);
    
    convSampling=4;
    convSize=max(nx_p,ny_p);
    convSize=min(convSize,(Int)cn.nearestEven(Int(maxConvSizeConsidered/2.0)*2));
    Int maxConvSize=convSize; 
    CoordinateSystem coords=csys_p;
    DirectionCoordinate dc=coords.directionCoordinate(0);
    Vector<Double> sampling;
    sampling = dc.increment();
    sampling*=Double(convSampling);
    sampling[0]*=Double(cn.nextLargerEven(Int(Float(nx_p)-0.5)))/Double(convSize);
    sampling[1]*=Double(cn.nextLargerEven(Int(Float(ny_p)-0.5)))/Double(convSize);
    dc.setIncrement(sampling);
     Vector<Double> unitVec(2);
  unitVec=convSize/2;
  dc.setReferencePixel(unitVec);
  
  // Set the reference value to that of the image center for sure.
  {
    // dc.setReferenceValue(mTangent_p.getAngle().getValue());
    MDirection wcenter;  
    Vector<Double> pcenter(2);
    pcenter(0) = nx_p/2;
    pcenter(1) = ny_p/2;    
    coords.directionCoordinate(0).toWorld( wcenter, pcenter );
    dc.setReferenceValue(wcenter.getAngle().getValue());
  }
  coords.replaceCoordinate(dc, 0);
  IPosition pbShape(4, convSize, convSize, 1, 1);
  TempImage<Complex> twoDPB(pbShape, coords);
  convFunc.resize(); // break any reference 
  convFunc.resize(convSize/2-1, convSize/2-1, wConvSize);
  convFunc.set(0.0);
  Bool convFuncStor=False;
  Complex *convFuncPtr=convFunc.getStorage(convFuncStor);
  IPosition start(4, 0, 0, 0, 0);
  IPosition pbSlice(4, convSize, convSize, 1, 1);
  Vector<Complex> maxes(wConvSize);
  Bool maxdel;
  Complex* maxptr=maxes.getStorage(maxdel);
  Int inner=convSize/convSampling;
  //cerr << "inner " << inner << endl;
  Matrix<Complex> corr(inner, inner);
  
  Vector<Double> uvOffset(2);
  uvOffset(0)=Double(nx_p)/2.0;
  uvOffset(1)=Double(ny_p)/2.0;
  Vector<Double> uvScale(2);
  Vector<Complex> correction(inner);
  //cerr << "increments " << csys_p.increment() << endl;
  uvScale(0)=fabs(nx_p*csys_p.increment()(0));
  uvScale(1)=fabs(ny_p*csys_p.increment()(1));
 
  
   ConvolveGridder<Double, Complex>
   
   ggridder(IPosition(2, inner, inner), uvScale, uvOffset, "SF");
  /////////////////////////////////////////////////////////
  /////////////Testing
  /*correction.resize(2*inner);
  uvScale *=2.0;
  uvOffset *=2.0;
   ConvolveGridder<Double, Complex>
     ggridder(IPosition(2, 2*inner, 2*inner), uvScale, uvOffset, "SF");
   for (Int iy=-inner/2;iy<inner/2;iy++) {
     
     ggridder.correctX1D(correction, iy+inner);
     corr.row(iy+inner/2)=correction(Slice(inner/2, inner));
   }
  */
  /////////////////////////////////
   //////////////////////////////////////////
   
   /*for (Int iy=-inner/2;iy<inner/2;iy++) {
     
     ggridder.correctX1D(correction, iy+inner/2);
     corr.row(iy+inner/2)=correction;
   }
   
   */
   //Bool cpcor;
   //Complex *cor=corr.getStorage(cpcor);
  Double s1=sampling(1);
  Double s0=sampling(0);
  ///////////Por FFTPack
  Vector<Float> wsave(2*convSize*convSize+15);
  Int lsav=2*convSize*convSize+15;
  Bool wsavesave;
  Float *wsaveptr=wsave.getStorage(wsavesave);
  Int ier;
  FFTPack::cfft2i(convSize, convSize, wsaveptr, lsav, ier);
   //////////
#ifdef _OPENMP
   omp_set_nested(0);
#endif
   //////openmp like to share reference param ...making a copy to reduce shared objects
   Int cpConvSize=convSize;
   Int cpWConvSize=wConvSize;
   Double cpWscale=wScale;
   Int cpConvSamp=convSampling;
  
   //Float max0=1.0;
#pragma omp parallel for default(none) firstprivate(cpWConvSize, cpConvSize, convFuncPtr, s0, s1, wsaveptr, ier, lsav, maxptr, cpWscale,inner ) 

  for (Int iw=0; iw< cpWConvSize;iw++) {
    // First the w term
    Matrix<Complex> screen(cpConvSize, cpConvSize);
    screen=0.0;
    Bool cpscr;
    Complex *scr=screen.getStorage(cpscr);
    Double twoPiW=2.0*C::pi*Double(iw*iw)/cpWscale;
    for (Int iy=-inner/2;iy<inner/2;iy++) {
      Double m=s1*Double(iy);
      Double msq=m*m;
      //////Int offset= (iy+convSize/2)*convSize;
      ///fftpack likes it flipped
      Int offset= (iy>-1 ? iy : (iy+cpConvSize))*cpConvSize;
      for (Int ix=-inner/2;ix<inner/2;ix++) {
	//////	  Int ind=offset+ix+convSize/2;
	///fftpack likes it flipped
	Int ind=offset+(ix > -1 ? ix : ix+cpConvSize);
	Double l=s0*Double(ix);
	Double rsq=l*l+msq;
	if(rsq<1.0) {
	  Double phase=twoPiW*(sqrt(1.0-rsq)-1.0);
	  Double cval, sval;
	  SINCOS(phase, sval, cval);
	  
	  Complex comval(cval, sval);
	  //scr[ind]=(cor[ix+inner/2+ (iy+inner/2)*inner])*comval;
	  scr[ind]=comval;
	  
	}
      }
      
    }
    // Now FFT and get the result back
    /////////Por FFTPack
    Vector<Float>work(2*cpConvSize*cpConvSize);
    Int lenwrk=2*cpConvSize*cpConvSize;
    Bool worksave;
    Float *workptr=work.getStorage(worksave);
    FFTPack::cfft2f(cpConvSize, cpConvSize, cpConvSize, scr, wsaveptr, lsav, workptr, lenwrk, ier);
    
    screen.putStorage(scr, cpscr);
    uInt offset=uInt(iw*(cpConvSize/2-1)*(cpConvSize/2-1));
    maxptr[iw]=screen(0,0);
    for (uInt y=0; y< uInt(cpConvSize/2)-1; ++y){
      for (uInt x=0; x< uInt(cpConvSize/2)-1; ++x){
	convFuncPtr[offset+y*(cpConvSize/2-1)+x] = screen(x,y);
      }
    }


  }
   convFunc.putStorage(convFuncPtr, convFuncStor);
   maxes.putStorage(maxptr, maxdel);
   Complex maxconv=max(abs(maxes));
   convFunc=convFunc/real(maxconv);
   convSupport.resize(wConvSize);
    convSupport=-1;
    Vector<Int> pcsupp=convSupport;
#ifdef _OPENMP
  omp_set_nested(0);
#endif
  Bool delsupstor;
  Int* suppstor=pcsupp.getStorage(delsupstor);
  convFuncPtr=convFunc.getStorage(convFuncStor);
#pragma omp parallel for default(none) firstprivate(suppstor, cpConvSize, cpWConvSize, cpConvSamp, convFuncPtr, maxConvSize)  
  for (Int iw=0;iw<cpWConvSize;iw++) {
    Bool found=False;
    Int trial=0;
    Int ploffset=(cpConvSize/2-1)*(cpConvSize/2-1)*iw;
    /*   
      for (trial=cpConvSize/2-2;trial>0;trial--) {
      // if((abs(convFunc(trial,0,iw))>1e-3)||(abs(convFunc(0,trial,iw))>1e-3) ) {
	     if((abs(convFuncPtr[trial+ploffset])>1e-2)||(abs(convFuncPtr[trial*(cpConvSize/2-1)+ploffset])>1e-2) ) {
	//cout <<"iw " << iw << " x " << abs(convFunc(trial,0,iw)) << " y " 
	//   <<abs(convFunc(0,trial,iw)) << endl; 
	found=True;
	break;
      }
      }
    */
        
       for (trial=0; trial<cpConvSize/2-2;++trial) {
      // if((abs(convFunc(trial,0,iw))>1e-3)||(abs(convFunc(0,trial,iw))>1e-3) ) {
      if((abs(convFuncPtr[trial+ploffset])<1e-3)||(abs(convFuncPtr[trial*(cpConvSize/2-1)+ploffset])<1e-3) ) {
	//cout <<"iw " << iw << " x " << abs(convFunc(trial,0,iw)) << " y " 
	//   <<abs(convFunc(0,trial,iw)) << endl; 
	found=True;
	break;
      }
      }
      
    if(found) {
      suppstor[iw]=Int(0.5+Float(trial)/Float(cpConvSamp))+1;
      if(suppstor[iw]*cpConvSamp*2 >= maxConvSize){
	suppstor[iw]=cpConvSize/2/cpConvSamp-1;
      }
    }
  }
  pcsupp.putStorage(suppstor, delsupstor);
  convSupport=pcsupp;
  ////////////TESTING


  //convSupport.set(0);
  /////////////////


  // Normalize such that plane 0 sums to 1 (when jumping in
  // steps of convSampling)
  Complex pbSum=0.0;
  for (Int iz=0; iz< convSupport.shape()[0]; ++iz){
    pbSum=0.0;
  for (Int iy=-convSupport(iz);iy<=convSupport(iz);iy++) {
    for (Int ix=-convSupport(iz);ix<=convSupport(iz);ix++) {
      pbSum+=convFunc(abs(ix)*cpConvSamp,abs(iy)*cpConvSamp,iz);
    }
  }
  convFunc.xyPlane(iz) = convFunc.xyPlane(iz)/Complex(pbSum);
  }
  cerr << "pbSum " << pbSum << endl;

   Int newConvSize=2*(max(convSupport)+2)*convSampling;
  
  if(newConvSize < convSize){
    IPosition blc(3, 0,0,0);
    IPosition trc(3, (newConvSize/2-2),
		  (newConvSize/2-2),
		  convSupport.shape()(0)-1);
   
    Cube<Complex> newConvFunc=convFunc(blc,trc);
    convFunc.resize();
    convFunc=newConvFunc;
    // convFunctions_p[actualConvIndex_p]->assign(Cube<Complex>(convFunc(blc,trc)));
    convSize=newConvSize;
    cerr << "new convsize " << convSize << endl;
  }


  //////////////////////TESTING
  //convFunc*=Complex(1.0/5.4, 0.0);

  ////////////////////////////
  //// if(pbSum>0.0) {
  ////  convFunc*=Complex(1.0/pbSum,0.0);
  //// }
  ///  else {
  ///  cerr << "Convolution function integral is not positive"
  ///	 << endl;
  /// } 
  /////////////TEST
  
  /* Int maxConvSupp=max(convSupport)+1;
   for (Int iw=1;iw<cpWConvSize; ++iw) {
     Float maxplane=max(amplitude(convFunc.xyPlane(iw)));
     for (Int iy=0; iy< maxConvSupp*cpConvSamp; ++iy){
       for  (Int ix=0; ix< maxConvSupp*cpConvSamp; ++ix){
	 convFunc(ix, iy, iw) /=maxplane;
     }
   }
   }
  */
   /////////////////////////
  /////Write out the SF correction image
  String corrim=outMSName_p+String("/WprojCorrection.image");
  Path elpath(corrim);
  cerr << "Saving the correction image " << elpath.absoluteName() << "\nIt should be used to restore images to a flat noise state " << endl;
  if(!Table::isReadable(corrim)){
    ConvolveGridder<Double, Float>elgridder(IPosition(2, nx_p, ny_p),
					      uvScale, uvOffset,
					      "SF");
    CoordinateSystem newcoord=csys_p;
    StokesCoordinate st(Vector<Int>(1, Stokes::I));
    newcoord.replaceCoordinate(st, 1);
    PagedImage<Float> thisScreen(IPosition(4, nx_p, ny_p, 1, 1), newcoord, corrim);
    thisScreen.set(0.0);
    Vector<Float> correction(nx_p);
    correction=1.0;

    Int npixCorr= max(nx_p,ny_p);
    Vector<Float> sincConv(npixCorr);
    for (Int ix=0;ix<npixCorr;ix++) {
      Float x=C::pi*Float(ix-npixCorr/2)/(Float(npixCorr)*Float(convSampling));
      if(ix==npixCorr/2) {
	sincConv(ix)=1.0;
      }
      else {
	sincConv(ix)=sin(x)/x;
      }
    }
    IPosition cursorShape(4, nx_p, 1, 1, 1);
    IPosition axisPath(4, 0, 1, 2, 3);
    LatticeStepper lsx(thisScreen.shape(), cursorShape, axisPath);
    LatticeIterator<Float> lix(thisScreen, lsx);
    for(lix.reset();!lix.atEnd();lix++) {
      Int iy=lix.position()(1);
      //elgridder.correctX1D(correction, iy);
    
      for (Int ix=0;ix<nx_p; ++ix) {
	correction(ix)=sincConv(ix)*sincConv(iy);
	//correction(ix)*=sincConv(ix)*sincConv(iy);
      }
      lix.rwVectorCursor()=correction;
    }
  }


  // Write out FT of screen as an image
    if(1) {
      CoordinateSystem ftCoords(coords);
      Int directionIndex=ftCoords.findCoordinate(Coordinate::DIRECTION);
      AlwaysAssert(directionIndex>=0, AipsError);
      dc=coords.directionCoordinate(directionIndex);
      Vector<Bool> axes(2); axes(0)=True;axes(1)=True;
      Vector<Int> shape(2); shape(0)=convSize;shape(1)=convSize;
      Coordinate* ftdc=dc.makeFourierCoordinate(axes,shape);
      ftCoords.replaceCoordinate(*ftdc, directionIndex);
      delete ftdc; ftdc=0;
      ostringstream name;
      name << "FTScreen" ;
      if(Table::canDeleteTable(name)) Table::deleteTable(name);
      PagedImage<Complex> thisScreen(IPosition(4, convFunc.shape()(0), convFunc.shape()(1), 1, convFunc.shape()(2)), ftCoords, name);
      thisScreen.put(convFunc.reform(IPosition(4, convFunc.shape()(0), convFunc.shape()(1), 1, convFunc.shape()(2))));
      if(Table::canDeleteTable("CORR2")) Table::deleteTable("CORR2");
       PagedImage<Complex> thisScreen2(IPosition(4, corr.shape()(0), corr.shape()(1), 1, 1), ftCoords, "CORR2");
       thisScreen2.put(corr.reform(IPosition(4, corr.shape()(0), corr.shape()(1), 1, 1)));

      //LatticeExpr<Float> le(real(twoDPB));
      //thisScreen.copyData(le);
      //thisScreen.put(real(screen));
    }
  

  
}

Int MSUVBin::sepCommaEmptyToVectorStrings(Vector<String>& lesStrings,
				 const String& str){

    String oneStr=str;
    Int nsep=0;
    // decide if its comma seperated or empty space seperated
    casa::String sep;
    if((nsep=oneStr.freq(",")) > 0){
      sep=",";
    }
    else {
      nsep=oneStr.freq(" ");
      sep=" ";
    }
    if(nsep == 0){
      lesStrings.resize(1);
      lesStrings=oneStr;
      nsep=1;
    }
    else{
      String *splitstrings = new String[nsep+1];
      nsep=split(oneStr, splitstrings, nsep+1, sep);
      lesStrings.resize(nsep);
      Int index=0;
      for (Int k=0; k < nsep; ++k){
	if((String(splitstrings[k]) == String(""))
	   || (String(splitstrings[k]) == String(" "))){
	  lesStrings.resize(lesStrings.nelements()-1, True);
	}
	else{
	  lesStrings[index]=splitstrings[k];
	  ++index;
	}
      }
      delete [] splitstrings;
    }

    return nsep;

}



} //# NAMESPACE CASA - END

