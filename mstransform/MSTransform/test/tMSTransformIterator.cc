//# tMSTransformDataHandlerr.cc: This file contains the unit tests of the MsTransformDataHandler class.
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

#include <mstransform/MSTransform/MSTransformIteratorFactory.h>
#include <msvis/MSVis/VisibilityIterator2.h>
#include <msvis/MSVis/VisBuffer2.h>
#include <string>
#include <iostream>
#include <ctime>

using namespace casa;
using namespace std;

#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

Record parseConfiguration(int argc, char **argv)
{
	string parameter,value;
	Record configuration;
	Bool autoMode = True;

	for (unsigned short i=0;i<argc-1;i++)
	{
		parameter = string(argv[i]);
		value = string(argv[i+1]);

		if (parameter == string("-vis"))
		{
			configuration.define ("inputms", value);
			autoMode = False;
		}
		else if (parameter == string("-field"))
		{
			configuration.define ("field", value);
		}
		else if (parameter == string("-spw"))
		{
			configuration.define ("spw", value);
		}
		else if (parameter == string("-scan"))
		{
			configuration.define ("scan", value);
		}
		else if (parameter == string("-antenna"))
		{
			configuration.define ("antenna", value);
		}
		else if (parameter == string("-correlation"))
		{
			configuration.define ("correlation", value);
		}
		else if (parameter == string("-timerange"))
		{
			configuration.define ("timerange", value);
		}
		else if (parameter == string("-intent"))
		{
			configuration.define ("intent", value);
		}
		else if (parameter == string("-array"))
		{
			configuration.define ("array", value);
		}
		else if (parameter == string("-uvrange"))
		{
			configuration.define ("uvrange", value);
		}
		else if (parameter == string("-observation"))
		{
			configuration.define ("observation", value);
		}
		else if (parameter == string("-feed"))
		{
			configuration.define ("feed", value);
		}
		else if (parameter == string("-datacolumn"))
		{
			configuration.define ("datacolumn", value);
		}
		else if (parameter == string("-combinespws"))
		{
			Bool tmp = Bool(atoi(value.c_str()));
			configuration.define ("combinespws", tmp);
		}
		else if (parameter == string("-chanaverage"))
		{
			Bool tmp = Bool(atoi(value.c_str()));
			configuration.define ("chanaverage", tmp);
		}
		else if (parameter == string("-chanbin"))
		{
			Int tmp = Int(atoi(value.c_str()));
			configuration.define ("chanbin", tmp);
		}
		else if (parameter == string("-useweights"))
		{
			configuration.define ("useweights", value);
		}
		else if (parameter == string("-hanning"))
		{
			Bool tmp = Bool(atoi(value.c_str()));
			configuration.define ("hanning", tmp);
		}
		else if (parameter == string("-regridms"))
		{
			Bool tmp = Bool(atoi(value.c_str()));
			configuration.define ("regridms", tmp);
		}
		else if (parameter == string("-mode"))
		{
			configuration.define ("mode", value);
		}
		else if (parameter == string("-nchan"))
		{
			Int tmp = Int(atoi(value.c_str()));
			configuration.define ("nchan", tmp);
		}
		else if (parameter == string("-start"))
		{
			Int tmp = Int(atoi(value.c_str()));
			configuration.define ("start", tmp);
		}
		else if (parameter == string("-width"))
		{
			Int tmp = Int(atoi(value.c_str()));
			configuration.define ("width", tmp);
		}
		else if (parameter == string("-nspw"))
		{
			Int tmp = Int(atoi(value.c_str()));
			configuration.define ("nspw", tmp);
		}
		else if (parameter == string("-interpolation"))
		{
			configuration.define ("interpolation", value);
		}
		else if (parameter == string("-phasecenter"))
		{
			configuration.define ("phasecenter", value);
		}
		else if (parameter == string("-restfreq"))
		{
			configuration.define ("restfreq", value);
		}
		else if (parameter == string("-outframe"))
		{
			configuration.define ("outframe", value);
		}
		else if (parameter == string("-veltype"))
		{
			configuration.define ("veltype", value);
		}
		else if (parameter == string("-timeaverage"))
		{
			Bool tmp = Bool(atoi(value.c_str()));
			configuration.define ("timeaverage", tmp);
		}
		else if (parameter == string("-timebin"))
		{
			configuration.define ("timebin", value);
		}
		else if (parameter == string("-timespan"))
		{
			configuration.define ("timespan", value);
		}
		else if (parameter == string("-maxuvwdistance"))
		{
			Double tmp = Double(atof(value.c_str()));
			configuration.define ("maxuvwdistance", tmp);
		}
		else if (parameter == string("-minbaselines"))
		{
			Int tmp = Int(atoi(value.c_str()));
			configuration.define ("minbaselines", tmp);
		}
		else if (parameter == string("-callib"))
		{
			Float calfactor = Float(atof(value.c_str()));
			Record calrec;
			calrec.define("calfactor",calfactor);
			configuration.defineRecord ("callib", calrec);
		}
		else if (parameter == string("-reindex"))
		{
			Bool tmp = Bool(atoi(value.c_str()));
			configuration.define ("reindex", tmp);
		}
		else if (parameter == string("-test"))
		{
			configuration.define ("test", value);
		}
	}

	if (autoMode)
	{
		char* pathChar = getenv ("CASAPATH");
		if (pathChar != NULL)
		{
			// Get base path
			String pathStr(pathChar);
			string res[2];
			casa::split(pathChar,res,2,string(" "));

			// Generate filename
			string filename(res[0]);
			filename += string("/data/regression/unittest/flagdata/Four_ants_3C286.ms");

			// Check if file exists
			string command = string ("cp -r ") + filename + string(" .");
			Int exists = system(command.c_str());
			if (exists == 0)
			{
				configuration.define ("inputms", String("Four_ants_3C286.ms"));
				configuration.define ("timeaverage", True);
				configuration.define ("timebin", "4s");
				configuration.define ("chanaverage", False);
				configuration.define ("chanbin", 8);
				configuration.define ("datacolumn", string("ALL"));
				configuration.define ("reindex", False);
				configuration.define ("spw", "1");
				configuration.define ("correlation", "RR,LL");
			}
			else
			{
				cout << RED;
				cout << "AUTOMATIC MODE DOES NOT FIND TESTING FILE: " << filename << endl;
				cout << RESET;
				exit(1);
			}

		}
		else
		{
			cout << RED;
			cout << "AUTOMATIC MODE DOES NOT FIND ENV. VARIABLE CASAPATH" << endl;
			cout << RESET;
			exit(1);
		}
	}

	return configuration;
}

String produceTmpTransformedMSToCompare(Record configuration, dataColMap &myDataColMap)
{
	String tmpFileName = File::newUniqueName("").absoluteName();
	configuration.define("realmodelcol",True);
	configuration.define("usewtspectrum",True);
	configuration.define("outputms",tmpFileName);
	// Define data column as ALL if user didn't specify it
	if (configuration.fieldNumber ("datacolumn") < 0)
	{
		configuration.define("datacolumn",string("ALL"));
	}

	MSTransformManager *manager = new MSTransformManager(configuration);
	manager->open();
	manager->setup();

	// Store data col map
	myDataColMap = manager->getDataColMap();

	vi::VisibilityIterator2 *visIter = manager->getVisIter();
	vi::VisBuffer2 *vb = visIter->getVisBuffer();
	visIter->originChunks();
	while (visIter->moreChunks())
	{
		visIter->origin();
		while (visIter->more())
		{
			manager->fillOutputMs(vb);
			visIter->next();
		}

		visIter->nextChunk();
	}

	manager->close();
	delete manager;
	return tmpFileName;
}

template <class T> Int compareVector(const Vector<T> &inp,const Vector<T> &ref,Float tolerance = FLT_EPSILON)
{
	Int res = -1;
	uInt nRowsToCompare = min(ref.size(),inp.size());

	for (uInt index=0;index < nRowsToCompare; index++)
	{
		if (abs(inp(index) - ref(index)) > tolerance )
		{
			res = index;
			break;
		}
	}

	return res;
}

template <class T> IPosition compareMatrix(const Matrix<T> &inp,const Matrix<T> &ref,Float tolerance = FLT_EPSILON)
{
	IPosition res;
	IPosition shape = inp.shape();

	uInt nRowsToCompare = min(inp.shape()(1),ref.shape()(1));


	for (uInt row=0;row < nRowsToCompare; row++)
	{
		for (uInt col=0;col < shape(0); col++)
		{
			if (abs(inp(col,row) - ref(col,row)) > tolerance )
			{
				res = IPosition(2,col,row);
				break;
			}
		}
	}

	return res;
}

template <class T> IPosition compareCube(const Cube<T> &inp,const Cube<T> &ref,Float tolerance = FLT_EPSILON)
{
	IPosition res;
	const IPosition &shape = inp.shape();

	uInt nRowsToCompare = min(inp.shape()(2),ref.shape()(2));


	for (uInt row=0;row < nRowsToCompare; row++)
	{
		for (uInt chan=0;chan < shape(1); chan++)
		{
			for (uInt corr=0;corr < shape(0); corr++)
			{
				if (abs(inp(corr,chan,row) - ref(corr,chan,row)) > tolerance )
				{
					res.resize(3,False);
					res = IPosition(3,corr,chan,row);
					break;
				}
			}
		}
	}

	return res;
}

const Cube<Complex> & getVisCubeToCompare(vi::VisBuffer2 *visBufferRef, MS::PredefinedColumns myCol, dataColMap &myDataColMap)
{
	switch (myDataColMap[myCol])
	{
		case MS::DATA:
			return visBufferRef->visCube();
			break;
		case MS::CORRECTED_DATA:
			return visBufferRef->visCubeCorrected();
			break;
		case MS::MODEL_DATA:
			return visBufferRef->visCubeModel();
			break;
		default:
			cerr << RED;
			cerr << "Required column does not exist compare vs visCube by default" << endl;
			return visBufferRef->visCube();
	}
}

Bool test_compareTransformedFileWithTransformingBuffer(Record configuration, String tmpFileName,dataColMap &myDataColMap)
{
	// Set tolerance for comparisons
	Float tolerance = 1E-6; // FLT_EPSILON is 1.19209290e-7F

	// Declare tmp variables
	Int chunk = 0,buffer = 0,row = 0;
	size_t totalBuffers = 0;
	IPosition pos;
	Bool keepIterating = True;

	// Initialize logger for printing data structures
	LogIO logger;

	// Open up transformed file
	MeasurementSet ms(tmpFileName,Table::Old);

	// Prepare Iterator
	Block<Int> sortCols(7);
	sortCols[0] = MS::OBSERVATION_ID;
	sortCols[1] = MS::ARRAY_ID;
	sortCols[2] = MS::SCAN_NUMBER;
	sortCols[3] = MS::STATE_ID;
	sortCols[4] = MS::FIELD_ID;
	sortCols[5] = MS::DATA_DESC_ID;
	sortCols[6] = MS::TIME;
	vi::VisibilityIterator2 visIterRef(ms,vi::SortColumns (sortCols,false));
	//visIterRef.setRowBlocking(ms.nrow());
	vi::VisBuffer2 *visBufferRef = visIterRef.getVisBuffer();

	// Get TVI, and associated buffer
	MSTransformIteratorFactory *factory = new MSTransformIteratorFactory(configuration);
	vi::VisibilityIterator2 *visIter = new vi::VisibilityIterator2 (*factory);
	vi::VisBuffer2 *visBuffer = visIter->getVisBuffer();

	// Access Transformed SPW sub-tables
	// MSField fieldSubTable = visIter->ms().field();
	// MSFieldColumns fieldeCols(fieldSubTable);
	// Vector<String> sourceNames = fieldeCols.name().getColumn();

	visIter->originChunks();
	visIterRef.originChunks();
	while (visIter->moreChunks() and visIterRef.moreChunks() and keepIterating)
	{
		chunk += 1;
		buffer = 0;

		visIter->origin();
		visIterRef.origin();

		while (visIter->more() and visIterRef.more() and keepIterating)
		{
			buffer += 1;
			totalBuffers += 1;

			cout << BLUE;
			cout << " COMPARING CHUNK " << chunk << " BUFFER " << buffer << endl;

			// Number of Correlations
			if (visBuffer->nCorrelations() != visBufferRef->nCorrelations())
			{
				cout << RED;
				cout 	<< " nCorrelations does not match in chunk " << chunk << " buffer " << buffer
						<< " transformBuffer=" << visBuffer->nCorrelations()
						<< " transformFile=" << visBufferRef->nCorrelations() << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout << "=>nCorrelations match" << endl;
			}

			// Number of Channels
			if (visBuffer->nChannels() != visBufferRef->nChannels())
			{
				cout << RED;
				cout 	<< " nChannels does not match in chunk " << chunk << " buffer " << buffer
						<< " transformBuffer=" << visBuffer->nChannels()
						<< " transformFile=" << visBufferRef->nChannels() << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout << "=>nChannels match" << endl;
			}

			// Number of rows
			if (visBuffer->nRows() != visBufferRef->nRows())
			{
				cout << RED;
				cout 	<< " nRows does not match in chunk " << chunk << " buffer " << buffer
						<< " transformBuffer=" << visBuffer->nRows()
						<< " transformFile=" << visBufferRef->nRows() << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout << "=>nRows match" << endl;
			}

			// Re-indexable Vectors

			/*
			row = compareVector(visBuffer->dataDescriptionIds(),visBufferRef->dataDescriptionIds(),0);
			if (row >= 0)
			{
				cout << RED;
				cout 	<< " dataDescriptionIds does not match in row " << row
						<< " transformBuffer=" << visBuffer->dataDescriptionIds()
						<< " transformFile=" << visBufferRef->dataDescriptionIds() << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout << "=>dataDescriptionIds match" << endl;
			}

			row = compareVector(visBuffer->spectralWindows(),visBufferRef->spectralWindows(),0);
			if (row >= 0)
			{
				cout << RED;
				cout 	<< " spectralWindows does not match in row " << row
						<< " transformBuffer=" << visBuffer->spectralWindows()
						<< " transformFile=" << visBufferRef->spectralWindows() << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout << "=>spectralWindows match" << endl;
			}
			*/

			row = compareVector(visBuffer->observationId(),visBufferRef->observationId(),0);
			if (row >= 0)
			{
				cout << RED;
				cout << " observationId does not match in row " << row
						<< " transformBuffer=" << visBuffer->observationId()(row)
						<< " transformFile=" << visBufferRef->observationId()(row)<< endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout << "=>observationId match" << endl;
			}


			row = compareVector(visBuffer->arrayId(),visBufferRef->arrayId(),0);
			if (row >= 0)
			{
				cout << RED;
				cout << " arrayId does not match in row " << row
						<< " transformBuffer=" << visBuffer->arrayId()(row)
						<< " transformFile=" << visBufferRef->arrayId()(row) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>arrayId match"<< endl;
			}


			row = compareVector(visBuffer->fieldId(),visBufferRef->fieldId(),0);
			if (row >= 0)
			{
				cout << RED;
				cout << " fieldId does not match in row " << row
						<< " transformBuffer=" << visBuffer->fieldId()(row)
						<< " transformFile=" << visBufferRef->fieldId()(row) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout << "=>fieldId match"<< endl;
				//cout << "Source is " << sourceNames(visBuffer->fieldId()[0]).c_str() << endl;
			}


			row = compareVector(visBuffer->stateId(),visBufferRef->stateId(),0);
			if (row >= 0)
			{
				cout << RED;
				cout << " stateId does not match in row " << row
						<< " transformBuffer=" << visBuffer->stateId()(row)
						<< " transformFile=" << visBufferRef->stateId()(row) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>stateId match" << endl;
			}


			row = compareVector(visBuffer->antenna1(),visBufferRef->antenna1(),0);
			if (row >= 0)
			{
				cout << RED;
				cout << " antenna1 does not match in row " << row
						<< " transformBuffer=" << visBuffer->antenna1()(row)
						<< " transformFile=" << visBufferRef->antenna1()(row) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>antenna1 match"<< endl;
			}


			row = compareVector(visBuffer->antenna2(),visBufferRef->antenna2(),0);
			if (row >= 0)
			{
				cout << RED;
				cout << " antenna2 does not match in row " << row
						<< " transformBuffer=" << visBuffer->antenna2()(row)
						<< " transformFile=" << visBufferRef->antenna2()(row) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>antenna2 match" << endl;
			}

			// Not-Re-indexable Vectors

			row = compareVector(visBuffer->scan(),visBufferRef->scan(),0);
			if (row >= 0)
			{
				cout << RED;
				cout << " scan does not match in row " << row
						<< " transformBuffer=" << visBuffer->scan()(row)
						<< " transformFile=" << visBufferRef->scan()(row)<< endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>scan match"<< endl;
			}


			row = compareVector(visBuffer->processorId(),visBufferRef->processorId(),0);
			if (row >= 0)
			{
				cout << RED;
				cout << " processorId does not match in row " << row
						<< " transformBuffer=" << visBuffer->processorId()(row)
						<< " transformFile=" << visBufferRef->processorId()(row) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>processorId match" << endl;
			}


			row = compareVector(visBuffer->feed1(),visBufferRef->feed1(),0);
			if (row >= 0)
			{
				cout << RED;
				cout << " feed1 does not match in row " << row
						<< " transformBuffer=" << visBuffer->feed1()(row)
						<< " transformFile=" << visBufferRef->feed1()(row) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>feed1 match" << endl;
			}


			row = compareVector(visBuffer->feed2(),visBufferRef->feed2(),0);
			if (row >= 0)
			{
				cout << RED;
				cout << " feed2 does not match in row " << row
						<< " transformBuffer=" << visBuffer->feed2()(row)
						<< " transformFile=" << visBufferRef->feed2()(row) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>feed2 match" << endl;
			}


			// Non re-indexable Vectors
			row = compareVector(visBuffer->time(),visBufferRef->time(),tolerance);
			if (row >= 0)
			{
				cout << RED;
				cout << " time does not match in row " << row
						<< " transformBuffer=" << visBuffer->time()(row)
						<< " transformFile=" << visBufferRef->time()(row) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>time match" << endl;
			}

			row = compareVector(visBuffer->timeCentroid(),visBufferRef->timeCentroid(),tolerance);
			if (row >= 0)
			{
				cout << RED;
				cout << " timeCentroid does not match in row " << row
						<< " transformBuffer=" << visBuffer->timeCentroid()(row)
						<< " transformFile=" << visBufferRef->timeCentroid()(row) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>timeCentroid match" << endl;
			}


			row = compareVector(visBuffer->timeInterval(),visBufferRef->timeInterval(),tolerance);
			if (row >= 0)
			{
				cout << RED;
				cout << " timeInterval does not match in row " << row
						<< " transformBuffer=" << visBuffer->timeInterval()(row)
						<< " transformFile=" << visBufferRef->timeInterval()(row) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>timeInterval match" << endl;
			}


			row = compareVector(visBuffer->exposure(),visBufferRef->exposure(),tolerance);
			if (row >= 0)
			{
				cout << RED;
				cout << " exposure does not match in row " << row
						<< " transformBuffer=" << visBuffer->exposure()(row)
						<< " transformFile=" << visBufferRef->exposure()(row) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>exposure match" << endl;
			}


			row = compareVector(visBuffer->flagRow(),visBufferRef->flagRow(),0);
			if (row >= 0)
			{
				cout << RED;
				cout << " flagRow does not match in row "<< row
						<< " transformBuffer=" << visBuffer->flagRow()(row)
						<< " transformFile=" << visBufferRef->flagRow()(row) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>flagRow match" << endl;
			}


			// Derived cols: This is just a non-crash test, derived cols are not re-indexed
			Double time0 = visBufferRef->time()(0);
			visBuffer->feedPa(time0);
			visBuffer->parang0(time0);
			visBuffer->parang(time0);
			visBuffer->azel0(time0);
			visBuffer->azel(time0);
			visBuffer->hourang(time0);
			///////////////////////////////////////////////////////////////////////////////

			if (abs(visBuffer->getFrequency(0,0)-visBufferRef->getFrequency(0,0)) > 1E-3)
			{
				cout << RED;
				cout << " getFrequency does not match in row "<< 0
						<< " transformBuffer=" << visBuffer->getFrequency(0,0)
						<< " transformFile=" << visBufferRef->getFrequency(0,0) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>getFrequency match" << endl;
			}

			row = compareVector(visBuffer->getFrequencies(0),visBufferRef->getFrequencies(0));
			if (row >= 0)
			{
				cout << RED;
				cout << " getFrequencies does not match in row "<< row
						<< " transformBuffer=" << visBuffer->getFrequencies(0)(row)
						<< " transformFile=" << visBufferRef->getFrequencies(0)(row) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout << "=>getFrequencies match" << endl;
			}

			if (abs(visBuffer->getChannelNumber(0,0)-visBufferRef->getChannelNumber(0,0)) > 0)
			{
				cout << RED;
				cout << " getChannelNumber does not match in row "<< 0
						<< " transformBuffer=" << visBuffer->getChannelNumber(0,0)
						<< " transformFile=" << visBufferRef->getChannelNumber(0,0) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>getChannelNumber match" << endl;
			}

			row = compareVector(visBuffer->getChannelNumbers(0),visBufferRef->getChannelNumbers(0),0);
			if (row >= 0)
			{
				cout << RED;
				cout << " getChannelNumbers does not match in row "<< row
						<< " transformBuffer=" << visBuffer->getChannelNumbers(0)(row)
						<< " transformFile=" << visBufferRef->getChannelNumbers(0)(row) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>getChannelNumbers match" << endl;
			}

			row = compareVector(visBuffer->rowIds(),visBufferRef->rowIds(),0);
			if (row >= 0)
			{
				cout << RED;
				cout << " rowIds does not match in row "<< row
						<< " transformBuffer=" << visBuffer->rowIds()(row)
						<< " transformFile=" << visBufferRef->rowIds()(row) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>rowIds match" << endl;
			}

			// Vis cubes
			dataColMap::iterator iter;

			pos.resize(0,False);
			pos = compareCube(visBuffer->flagCube(),visBufferRef->flagCube(),0);
			if (pos.size() == 3)
			{
				cout << RED;
				cout << " flagCube does not match in position (row,chan,corr)="
					<< "("<< pos(2) << "," << pos(1) << "," << pos(0) << ")"
					<< " transformBuffer=" << visBuffer->flagCube()(pos)
					<< " transformFile=" << visBufferRef->flagCube()(pos) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>flagCube match" << endl;
			}

			pos.resize(0,False);
			iter = myDataColMap.find(MS::DATA);
			if (iter != myDataColMap.end())
			{
				const Cube<Complex> &visCubeRef = getVisCubeToCompare(visBufferRef,MS::DATA,myDataColMap);
				pos = compareCube(	visBuffer->visCube(),
									visCubeRef,
									tolerance);

				if (pos.size() == 3)
				{
					cout << RED;
					cout << " visCube does not match in position (row,chan,corr)="
							<< "("<< pos(2) << "," << pos(1) << "," << pos(0) << ")"
							<< " transformBuffer=" << visBuffer->visCube()(pos)
							<< " transformFile=" << visCubeRef(pos) << endl;
					keepIterating = False;
				}
				else
				{
					cout << GREEN;
					cout 	<< "=>visCube match" << endl;
				}
			}


			pos.resize(0,False);
			iter = myDataColMap.find(MS::CORRECTED_DATA);
			if (iter != myDataColMap.end())
			{
				const Cube<Complex> &visCubeRef = getVisCubeToCompare(visBufferRef,MS::CORRECTED_DATA,myDataColMap);
				pos = compareCube(	visBuffer->visCubeCorrected(),
									visCubeRef,
									tolerance);

				if (pos.size() == 3)
				{
					cout << RED;
					cout << " visCubeCorrected does not match in position (row,chan,corr)="
							<< "("<< pos(2) << "," << pos(1) << "," << pos(0) << ")"
							<< " transformBuffer=" << visBuffer->visCubeCorrected()(pos)
							<< " transformFile=" << visCubeRef(pos) << endl;
					keepIterating = False;
				}
				else
				{
					cout << GREEN;
					cout 	<< "=>visCubeCorrected match" << endl;
				}
			}

			// CAS-7412: Check that visModel is available no matter what
			if (visBuffer->visCubeModel().shape() == visBufferRef->flagCube().shape())
			{
				cout << GREEN;
				cout 	<< "=>visCubeModel available" << endl;
			}
			else
			{
				cout << RED;
				cout 	<< "=>visCubeModel not available" << endl;
				keepIterating = False;
			}

			pos.resize(0,False);
			iter = myDataColMap.find(MS::MODEL_DATA);
			if (True)
			{
				const Cube<Complex> &visCubeRef = getVisCubeToCompare(visBufferRef,MS::MODEL_DATA,myDataColMap);
				pos = compareCube(	visBuffer->visCubeModel(),
									visCubeRef,
									tolerance);

				if (pos.size() == 3)
				{
					cout << RED;
					cout << " visCubeModel does not match in position (row,chan,corr)="
							<< "("<< pos(2) << "," << pos(1) << "," << pos(0) << ")"
							<< " transformBuffer=" << visBuffer->visCubeModel()(pos)
							<< " transformFile=" << visCubeRef(pos) << endl;
					keepIterating = False;
				}
				else
				{
					cout << GREEN;
					cout 	<< "=>visCubeModel match" << endl;
				}
			}

			pos.resize(0,False);
			iter = myDataColMap.find(MS::FLOAT_DATA);
			if (iter != myDataColMap.end())
			{
				pos = compareCube(	visBuffer->visCubeFloat(),
									visBufferRef->visCubeFloat(),
									tolerance);

				if (pos.size() == 3)
				{
					cout << RED;
					cout << " visCubeFloat does not match in position (row,chan,corr)="
							<< "("<< pos(2) << "," << pos(1) << "," << pos(0) << ")"
							<< " transformBuffer=" << visBuffer->visCubeFloat()(pos)
							<< " transformFile=" << visBufferRef->visCubeFloat()(pos) << endl;
					keepIterating = False;
				}
				else
				{
					cout << GREEN;
					cout 	<< "=>visCubeFloat match" << endl;
				}
			}

			pos.resize(0,False);
			pos = compareCube(visBuffer->weightSpectrum(),visBufferRef->weightSpectrum(),tolerance);
			if (pos.size() == 3)
			{
				cout << RED;
				cout << " weightSpectrum does not match in position (row,chan,corr)="
						<< "("<< pos(2) << "," << pos(1) << "," << pos(0) << ")"
						<< " transformBuffer=" << visBuffer->weightSpectrum()(pos)
						<< " transformFile=" << visBufferRef->weightSpectrum()(pos) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>weightSpectrum match" << endl;
			}

			pos.resize(0,False);
			pos = compareCube(visBuffer->sigmaSpectrum(),visBufferRef->sigmaSpectrum(),tolerance);
			if (pos.size() == 3)
			{
				cout << RED;
				cout << " sigmaSpectrum does not match in position (row,chan,corr)="
						<< "("<< pos(2) << "," << pos(1) << "," << pos(0) << ")"
						<< " transformBuffer=" << visBuffer->sigmaSpectrum()(pos)
						<< " transformFile=" << visBufferRef->sigmaSpectrum()(pos) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>sigmaSpectrum match" << endl;
			}

			// Matrix cols
			pos.resize(0,False);
			pos = compareMatrix(visBuffer->uvw(),visBufferRef->uvw(),tolerance);
			if (pos.size() == 2)
			{
				cout << RED;
				cout << " uvw does not match in position (row,col)="
						<< "("<< pos(1) << "," << pos(0) <<  ")"
						<< " transformBuffer=" << visBuffer->uvw()(pos)
						<< " transformFile=" << visBufferRef->uvw()(pos) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>uvw match" << endl;
			}

			pos.resize(0,False);
			pos = compareMatrix(visBuffer->weight(),visBufferRef->weight(),tolerance);
			if (pos.size() == 2)
			{
				cout << RED;
				cout << " weight does not match in position (row,pol)="
						<< "("<< pos(1) << "," << pos(0) <<  ")"
						<< " transformBuffer=" << visBuffer->weight()(pos)
						<< " transformFile=" << visBufferRef->weight()(pos) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>weight match" << endl;
			}

			pos.resize(0,False);
			pos = compareMatrix(visBuffer->sigma(),visBufferRef->sigma(),tolerance);
			if (pos.size() == 2)
			{
				cout << RED;
				cout << " sigma not match in position (row,pol)="
						<< "("<< pos(1) << "," << pos(0) <<  ")"
						<< " transformBuffer=" << visBuffer->sigma()(pos)
						<< " transformFile=" << visBufferRef->sigma()(pos) << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout 	<< "=>sigma match" << endl;
			}

			// CAS-7601: Get correlation types
			for (uInt idx=0;idx<visBuffer->getCorrelationTypes().size();idx++)
			{
				uInt corrIdx = visBuffer->getCorrelationTypes()[idx];
				Stokes::StokesTypes corrType = (Stokes::StokesTypes)visBuffer->correlationTypes()[corrIdx];
				String corrTypeStr = Stokes::name(corrType);
				cout << "Correlation product selected: " << corrTypeStr << endl;
			}

			// CAS-7601: New methods to access correlation types
			for (uInt idx=0;idx<visBuffer->getCorrelationTypesDefined().size();idx++)
			{
				Stokes::StokesTypes corrType = visBuffer->getCorrelationTypesDefined()[idx];
				String corrTypeStr = Stokes::name(corrType);
				cout << "getCorrelationTypesDefined product selected: " << corrTypeStr << endl;
			}

			// CAS-7601: New methods to access correlation types
			for (uInt idx=0;idx<visBuffer->getCorrelationTypesSelected().size();idx++)
			{
				Stokes::StokesTypes corrType = visBuffer->getCorrelationTypesSelected()[idx];
				String corrTypeStr = Stokes::name(corrType);
				cout << "getCorrelationTypesSelected product selected: " << corrTypeStr << endl;
			}


			// CAS-7315: Phase shifting
			visBuffer->phaseCenterShift(0.1,0.1);
			visBufferRef->phaseCenterShift(0.1,0.1);

			// CAS-7393: Propagate flags to the input VI //////////////////////////////////////////////////////////////

			IPosition shape = visBuffer->getShape();
			Cube<Bool> flagCube(visBuffer->getShape(),False);
			Vector<Bool> flagCubeRow(visBuffer->getShape()(2),False);
			size_t nCorr = shape(0);
			size_t nChan = shape(1);
			size_t nRows = shape(2);

			// Switch each other buffer the sign of the flag of the first block of channels
			Bool firstChanBlockFlag;
			if (buffer % 2)
			{
				firstChanBlockFlag = True;
			}
			else
			{
				firstChanBlockFlag = False;
			}

			// Fill flag cube alternating flags per blocks channels
			for (size_t row_i =0;row_i<nRows;row_i++)
			{
				// Row completely flagged
				if (row_i % 2)
				{
					flagCube.xyPlane(row_i) = True;
				}
				else
				{
					for (size_t chan_i =0;chan_i<nChan;chan_i++)
					{
						// Set the flags in each other block of channels
						Bool chanBlockFlag;
						if (chan_i % 2)
						{
							chanBlockFlag = firstChanBlockFlag;
						}
						else
						{
							chanBlockFlag = !firstChanBlockFlag;
						}

						for (size_t corr_i =0;corr_i<nCorr;corr_i++)
						{
							flagCube(corr_i,chan_i,row_i) = chanBlockFlag;
						}
					}
				}
			}

			// Not needed according to the flag cube - flag row convention agreement
			visIter->writeFlag(flagCube);

			visIter->next();
			visIterRef.next();
		}

		visIter->nextChunk();
		visIterRef.nextChunk();
	}

	cout << RESET << endl;

	delete factory;
	delete visIter;
	return keepIterating;
}

Bool test_bufferStructure(Record configuration)
{
	// Initialize logger for printing data structures
	LogIO logger;

	// Declare tmp variables
	Int chunk = 0,buffer = 0;
	size_t totalBuffers = 0;
	Bool keepIterating = True;
	time_t startTime, endTime;

	// Initialize factory and pre-calculate buffer structure
	MSTransformIteratorFactory *factory = new MSTransformIteratorFactory(configuration);
	logger << "Start pre-calculation of buffer structure, this includes MSTransformManager initialization" << LogIO::POST;
	startTime = std::time(NULL);
	std::vector<IPosition> visBufferStructure = factory->getVisBufferStructure();
	endTime = std::time(NULL);
	logger << "End pre-calculate buffer structure, including MSTransformManager initialization: " << endTime-startTime << LogIO::POST;

	// Prepare TVI, and associated buffer
	vi::VisibilityIterator2 *visIter = new vi::VisibilityIterator2 (*factory);
	vi::VisBuffer2 *visBuffer = visIter->getVisBuffer();

	/*

	// Start timer
	logger << "Start TVI loop" << LogIO::POST;
	startTime = std::time(NULL);


	// Swap MS loading data cols
	visIter->originChunks();
	while (visIter->moreChunks() and keepIterating)
	{
		chunk += 1;
		buffer = 0;

		visIter->origin();

		while (visIter->more() and keepIterating)
		{
			buffer += 1;
			totalBuffers += 1;

			cout << BLUE;
			cout << " COMPARING CHUNK " << chunk << " BUFFER " << buffer << endl;

			// Compare shapes
			if (visBuffer->getShape() != visBufferStructure[totalBuffers-1])
			{
				cout << RED;
				cout 	<< " pre-calculated buffer shape does not match for " << chunk << " buffer " << buffer
						<< " transformBuffer=" << visBuffer->getShape()
						<< " precalculated=" << visBufferStructure[totalBuffers-1] << endl;
				keepIterating = False;
			}
			else
			{
				cout << GREEN;
				cout << "=>pre-calculated shape match" << endl;
			}

			if (keepIterating) visIter->next();
		}

		if (keepIterating) visIter->nextChunk();
	}
	cout << RESET << endl;

	// Stop timer
	endTime = std::time(NULL);
	logger << "Start TVI loop, total time: " << endTime-startTime << LogIO::POST;

	*/

	delete factory;
	delete visIter;
	return keepIterating;
}

int main(int argc, char **argv)
{
	// Declare working vars
	String tmpFileName;
	dataColMap myDataColMap;

	// Read config
	Record configuration = parseConfiguration(argc, argv);

	// Determine test to run
	String test("compare");
	int fieldPos = 0;
	fieldPos = configuration.fieldNumber ("test");
	if (fieldPos >= 0)
	{
		configuration.get (fieldPos, test);
	}

	Bool result;
	if (test == "buffer")
	{
		result = test_bufferStructure(configuration);
	}
	else if (test == "compare")
	{
		dataColMap myDataColMap;
		tmpFileName = produceTmpTransformedMSToCompare(configuration,myDataColMap);
		result = test_compareTransformedFileWithTransformingBuffer(configuration,tmpFileName,myDataColMap);
	}

	if (result)
	{
		if (test == "compare")
		{
			Table::deleteTable(tmpFileName,True);
		}

		exit(0);
	}
	else
	{
		exit(1);
	}
}
