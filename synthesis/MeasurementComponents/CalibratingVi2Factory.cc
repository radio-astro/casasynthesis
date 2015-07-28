//# CalibratingVi2Factory.cc: Implementation of the CalibratingVi2Factory class.
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

#include <synthesis/MeasurementComponents/CalibratingVi2Factory.h>
#include <msvis/MSVis/VisibilityIterator2.h>
#include <msvis/MSVis/VisibilityIteratorImpl2.h>
#include <msvis/MSVis/VisBuffer2.h>


namespace {
  casa::vi::CalibratingVi2FactoryI* generateCViFac(casa::MeasurementSet* ms, 
						   const casa::Record& calrec,
						   const casa::vi::IteratingParameters& iterpar) {
    return new casa::vi::CalibratingVi2Factory(ms,calrec,iterpar);  // A concrete one from the synthesis side
  }

  bool initCViFacGenerator = casa::vi::CalibratingVi2FactoryI::setGenerator(generateCViFac);

}

namespace casa { //# NAMESPACE CASA - BEGIN
namespace vi { //# NAMESPACE VI - BEGIN


// -----------------------------------------------------------------------
CalibratingVi2Factory::CalibratingVi2Factory(MeasurementSet* ms,
					     const CalibratingParameters& calpar, 
					     const IteratingParameters& iterpar) :
  ms_p(ms),
  calpar_p(calpar),
  iterpar_p(iterpar)
{}


// -----------------------------------------------------------------------
CalibratingVi2Factory::CalibratingVi2Factory(MeasurementSet* ms,
					     const Record& calrec,
					     const IteratingParameters& iterpar) :
  ms_p(ms),
  calpar_p(calrec),
  iterpar_p(iterpar)
{}


// -----------------------------------------------------------------------
CalibratingVi2Factory::~CalibratingVi2Factory()
{}


// -----------------------------------------------------------------------
vi::ViImplementation2 * CalibratingVi2Factory::createVi (vi::VisibilityIterator2 * vi2) const
{

  //cout << "Making plain ViImpl2 for the user." << endl;
  vi::ViImplementation2 * plainViI(NULL);  // generic
  // Create a simple VI implementation to perform the reading.
  plainViI = new vi::VisibilityIteratorImpl2 (vi2,
					      Block<const MeasurementSet*>(1,ms_p),
					      iterpar_p.getSortColumns(),
					      iterpar_p.getChunkInterval(),
					      vi::VbPlain,
					      True); // writable!
  
  return this->createVi(vi2,plainViI);

}



// -----------------------------------------------------------------------
vi::ViImplementation2 * CalibratingVi2Factory::createVi (vi::VisibilityIterator2 * vi2,
							 vi::ViImplementation2 * vii) const
{



  vi::ViImplementation2 * vii2(NULL);  // generic
  if (vii) {
    //    cout << "Using user-supplied underlying ViImpl2!" << endl;
    // Use user-supplied ivii
    vii2=vii;
  }
  else
    // Create a simple VI implementation to perform the reading.
    vii2 = new vi::VisibilityIteratorImpl2 (vi2,
					    Block<const MeasurementSet*>(1,ms_p),
					    iterpar_p.getSortColumns(),
					    iterpar_p.getChunkInterval(),
					    vi::VbPlain,
					    True); // writable!
    
  // Create output VisibilityIterator
  //   (Get base MS name cleverly, because ms_p might be a reference table)
  String msantname=ms_p->antenna().tableName();
  String msname=msantname.before("/ANTENNA");
  CalibratingVi2 *calVI = new CalibratingVi2(vi2,vii2,calpar_p,msname);
  
  return calVI;
}

} //# NAMESPACE VI - END
} //# NAMESPACE CASA - END


