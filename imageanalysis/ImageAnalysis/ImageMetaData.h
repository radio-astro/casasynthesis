//# ImageMetaData.h: Meta information for Images
//# Copyright (C) 2009
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
//#                        Charlottesville, VA 22903-2475 USA
//#
//# $Id$

#ifndef IMAGES_IMAGEMETADATA_H
#define IMAGES_IMAGEMETADATA_H

#include <imageanalysis/ImageAnalysis/ImageMetaDataBase.h>

#include <images/Images/ImageInterface.h>
#include <casa/aips.h>

namespace casa {

// <summary>
// A class in which to store and allow read-only access to image metadata.
// </summary>

// <use visibility=export>

// <reviewed reviewer="" date="" tests="" demos="">
// </reviewed>

// <prerequisite>
//   <li> <linkto class=ImageInterface>ImageInterface</linkto>
// </prerequisite>

// <etymology>
// The ImageMetaData class name is derived from its role as holding image metadata.
// </etymology>

// <synopsis> 
// The ImageMetaData object is meant to allow access to image metadata (eg, shape,
// coordinate system info such as spectral and polarization axes numbers, etc).
// </synopsis>

// <example>
// Construct an object of this class by passing the associated image to the constructor.
// <srcblock>
// PagedImage<Float> myImage("myImage");
// ImageMetaData<Float> myImageMetaData(myImage);
// </srcblock>
// </example>

// <motivation> 
// This class is meant to provide an object-oriented interface for accessing
// image metadata without polluting the ImageInterface and CoordinateSystem
// classes with these methods.
// </motivation>
// <todo>
// Merge ImageInfo class into this class.
// </todo>

class ImageMetaData : public ImageMetaDataBase {

public:

	ImageMetaData(SPCIIF imagef);
	ImageMetaData(SPIIF imagef);

	ImageMetaData(SPCIIC imagec);
	ImageMetaData(SPIIC imagec);

	~ImageMetaData() {}

	Record toRecord(Bool verbose) const;

protected:

	SPCIIF _getFloatImage() const { return _floatImage; }

	SPCIIC _getComplexImage() const { return _complexImage; }

	ImageMetaData() : ImageMetaDataBase(), _floatImage(), _complexImage() {}

	const ImageInfo& _getInfo() const { return _info; }

	const CoordinateSystem& _getCoords() const { return _csys; }

	Vector<String> _getAxisNames() const;

	Vector<String> _getAxisUnits() const;

	GaussianBeam _getBeam() const;

	String _getBrightnessUnit() const;

	String _getImType() const;

	vector<Quantity> _getIncrements() const;

	Vector<String> _getMasks() const;

	String _getObject() const;

	String _getEquinox() const;

	MEpoch _getObsDate() const;

	String _getObserver() const;

	String _getProjection() const;

	String _getRefFreqType() const;

	Vector<Double> _getRefPixel() const;

	Vector<Quantity> _getRefValue() const;

	Quantity _getRestFrequency() const;

	Record _getStatistics() const;

	String _getTelescope() const;

	Vector<String> _getStokes() const;

private:

	SPCIIF _floatImage;
	SPCIIC _complexImage;
	const ImageInfo _info;
	const CoordinateSystem _csys;

	// These are mutable because they are only to be set once and
	// then cached. If this contract is broken, and they are set elsewhere
	// defects will likely occur.
	mutable Record _header;
	mutable String _bunit, _imtype, _object, _equinox,
		_projection, _observer, _telescope, _reffreqtype;
	mutable MEpoch _obsdate;
	mutable Quantity _restFreq;
	mutable GaussianBeam _beam;
	mutable Vector<String> _masks, _stokes;
	mutable Vector<String> _axisNames, _axisUnits;
	mutable Vector<Double> _refPixel;
	mutable vector<Quantity> _refVal, _increment;
	mutable Record _stats;
};

} //# NAMESPACE CASA - END

#endif
