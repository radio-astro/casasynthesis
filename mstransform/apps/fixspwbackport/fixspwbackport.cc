/*
 * fixspwbackport
 *
 * Allow MS created post 4.3 to be ported to older versions of casa due to
 * the addition of Undefined spectral frame
//#
//# Copyright (C) 2014
//# Associated Universities, Inc. Washington DC, USA.
//#
//# This program is free software; you can redistribute it and/or modify it
//# under the terms of the GNU General Public License as published by the Free
//# Software Foundation; either version 2 of the License, or (at your option)
//# any later version.
//#
//# This program is distributed in the hope that it will be useful, but WITHOUT
//# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//# more details.
//#
//# You should have received a copy of the GNU General Public License along
//# with this program; if not, write to the Free Software Foundation, Inc.,
//# 675 Massachusetts Ave, Cambridge, MA 02139, USA.
//#
//# Correspondence concerning AIPS++ should be addressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#
//# $Id$
 *  Created on: Oct 06, 2014
 *      Author: kgolap
 */
#include <casacore/casa/Inputs/Input.h>
#include <casacore/casa/aips.h>
#include <casacore/casa/Containers/Record.h>
#include <casacore/casa/Containers/RecordInterface.h>
#include <casacore/casa/IO/AipsIO.h>
#include <casacore/tables/Tables/TableProxy.h>
#include <casacore/tables/Tables/TableIter.h>
#include <casacore/tables/Tables/TableColumn.h>
#include <casacore/tables/Tables/TableRecord.h>
#include <casacore/ms/MeasurementSets/MeasurementSet.h>
#include <casacore/coordinates/Coordinates/CoordinateSystem.h>
#include <casacore/casa/namespace.h>
#include <iomanip>
using namespace std;

int main(int argc, char **argv) {
  if (argc<2) {
    cout <<"Usage: fixspwbackport ms-table-name "<<endl;
    exit(1);
  }
  String msname(argv[1]);
  if(!Table::isWritable(msname)){
    cerr << "Cannot modify " << msname << endl;
    exit(1);
  }
 String specName=msname + String("/SPECTRAL_WINDOW");
	Table spwTab=Table(specName, Table::Update);
	TableProxy tprox(spwTab);
	Vector<String> colnames=tprox.columnNames();
	for (uInt k=0; k < colnames.nelements(); ++k){
	TableColumn cfcol(spwTab, colnames[k]);
	TableRecord& colRec=cfcol.rwKeywordSet();
		if(colRec.isDefined("MEASINFO")){
			if(colRec.asrwRecord("MEASINFO").isDefined("TabRefTypes")){
				Vector<String> nrf(9);
				nrf[0]="REST"; nrf[1]="LSRK"; nrf[2]="LSRD"; nrf[3]="BARY";
				nrf[4]="GEO"; nrf[5]="TOPO"; nrf[6]="GALACTO"; nrf[7]="LGROUP";
				nrf[8]="CMB";
				colRec.asrwRecord("MEASINFO").removeField("TabRefTypes");
				colRec.asrwRecord("MEASINFO").define("TabRefTypes", nrf);

			
				//cerr << colRec.asrwRecord("MEASINFO").asArrayString("TabRefTypes")<< endl;
				//cerr << colRec << endl;
			}
			if(colRec.asrwRecord("MEASINFO").isDefined("TabRefCodes")){
				Vector<uInt> nrc(9);
				indgen(nrc);
				colRec.asrwRecord("MEASINFO").removeField("TabRefCodes");
				colRec.asrwRecord("MEASINFO").define("TabRefCodes", nrc);
			
			}
		}
	}



	return 0;
}



