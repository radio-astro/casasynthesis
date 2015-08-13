//# Image2DConvolver.cc:  convolution of an image by given Array
//# Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
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
//   
#include <imageanalysis/ImageAnalysis/Image2DConvolver.h>

#include <casa/aips.h>
#include <casa/Arrays/IPosition.h>
#include <casa/Arrays/Array.h>
#include <casa/Arrays/ArrayMath.h>
#include <casa/Arrays/Vector.h>
#include <casa/Arrays/Matrix.h>
#include <casa/Exceptions/Error.h>
#include <components/ComponentModels/GaussianDeconvolver.h>
#include <components/ComponentModels/GaussianShape.h>
#include <components/ComponentModels/SkyComponentFactory.h>
#include <coordinates/Coordinates/CoordinateUtil.h>
#include <coordinates/Coordinates/CoordinateSystem.h>
#include <coordinates/Coordinates/DirectionCoordinate.h>
#include <lattices/LatticeMath/Fit2D.h>
#include <scimath/Functionals/Gaussian2D.h>
#include <imageanalysis/ImageAnalysis/ImageConvolver.h>
#include <imageanalysis/ImageAnalysis/ImageMetaData.h>
#include <images/Images/PagedImage.h>
#include <images/Images/TempImage.h>
#include <images/Images/ImageInterface.h>
#include <images/Images/ImageInfo.h>
#include <images/Images/ImageUtilities.h>
#include <images/Images/SubImage.h>
#include <casa/Logging/LogIO.h>
#include <scimath/Mathematics/Convolver.h>
#include <casa/Quanta/Quantum.h>
#include <casa/Quanta/MVAngle.h>
#include <casa/Quanta/Unit.h>
#include <casa/Quanta/QLogical.h>
#include <casa/iostream.h>

#include <memory>

namespace casa { //# NAMESPACE CASA - BEGIN

template <class T> 
Image2DConvolver<T>::Image2DConvolver ()
{}

template <class T> Image2DConvolver<T>::Image2DConvolver(
	const SPCIIT image, const Record *const &region,
	const String& mask, const String& outname, const Bool overwrite
) : ImageTask<T>(image, "", region, "", "", "", mask, outname, overwrite),
	_type(VectorKernel::GAUSSIAN),  _scale(0), _major(), _minor(),
	_pa(), _axes(image->coordinates().directionAxesNumbers()), _targetres(False) {
	this->_construct(True);
}

template <class T>
Image2DConvolver<T>::Image2DConvolver(const Image2DConvolver<T> &other)
{
   operator=(other);
}

// TODO use GaussianBeams rather than Vector<Quantity>s, this method
// can probably be eliminated.
template <class T>
Vector<Quantity> Image2DConvolver<T>::_getConvolvingBeamForTargetResolution(
	const Vector<Quantity>& targetBeamParms,
	const GaussianBeam& inputBeam
) {
	GaussianBeam convolvingBeam;
	Vector<Quantity> kernelParms(3);
	GaussianBeam targetBeam(
		targetBeamParms[0], targetBeamParms[1],
		targetBeamParms[2]
	);
	try {
		if(GaussianDeconvolver::deconvolve(convolvingBeam, targetBeam, inputBeam)) {
            // point source, or convolvingBeam nonsensical
            throw AipsError();
        }
    }
	catch (const AipsError& x) {
		ostringstream os;
		os << "Unable to reach target resolution of "
			<< targetBeam << " Input image beam "
			<< inputBeam << " is (nearly) identical "
			<< "to or larger than the output beam size";
		ThrowCc(os.str());
	}
	kernelParms[0] = convolvingBeam.getMajor();
	kernelParms[1] = convolvingBeam.getMinor();
	kernelParms[2] = convolvingBeam.getPA(True);
	return kernelParms;
}

template <class T> void Image2DConvolver<T>::setAxes(
	const std::pair<uInt, uInt>& axes
) {
	uInt ndim = this->_getImage()->ndim();
	ThrowIf(axes.first == axes.second, "Axes must be different");
	ThrowIf(
		axes.first >= ndim || axes.second >= ndim,
		"Axis value must be less than number of axes in image"
	);
	if (_axes.size() != 2) {
		_axes.resize(2, False);
	}
	_axes[0] = axes.first;
	_axes[1] = axes.second;
}


template <class T> void Image2DConvolver<T>::setKernel(
	const String& type, const Quantity& major, const Quantity& minor,
	const Quantity& pa
) {
	ThrowIf (major < minor, "Major axis is less than minor axis");
	_type = VectorKernel::toKernelType(type);
	_major = major;
	_minor = minor;
	_pa = pa;

}

template <class T> SPIIT Image2DConvolver<T>::convolve() {
	Bool autoScale = _scale <= 0;
	Double scale = autoScale ? 1.0 : _scale;
	SPIIF subImage = SubImageFactory<Float>::createImage(
		*this->_getImage(), "", *this->_getRegion(), this->_getMask(),
		this->_getDropDegen(), False, False, this->_getStretch()
	);
	Vector<Quantity> parameters(3);
	parameters(0) = _major;
	parameters(1) = _minor;
	parameters(2) = _pa;
	SPIIT outImage(new TempImage<Float> (subImage->shape(), subImage->coordinates()));
	Image2DConvolver<Float>::convolve(
		*this->_getLog(), outImage, *subImage, _type, _axes,
		parameters, autoScale, scale, True, _targetres
	);
	return this->_prepareOutputImage(*outImage);
}


template <class T>
Image2DConvolver<T> &Image2DConvolver<T>::operator=(const Image2DConvolver<T> &other)
{
   if (this != &other) {
   }
   return *this;
}

template <class T> void Image2DConvolver<T>::convolve(
	LogIO& os, SPIIT imageOut,
	const ImageInterface<T>& imageIn, VectorKernel::KernelTypes kernelType,
	const IPosition& pixelAxes, const Vector<Quantity>& parameters,
	Bool autoScale, Double scale, Bool copyMiscellaneous,
	Bool targetres
) {
	ThrowIf(
		parameters.nelements() != 3,
		"The world parameters vector must be of length 3"
	);
	ThrowIf(
		pixelAxes.nelements() != 2,
		"You must give two pixel axes to convolve"
	);
	const Int nDim = imageIn.ndim();
	ThrowIf(
		pixelAxes(0) < 0 || pixelAxes(0) >= nDim
		|| pixelAxes(1) < 0 || pixelAxes(1) >= nDim,
		"The pixel axes " + String::toString(pixelAxes) + " are illegal"
	);
	ThrowIf(
		nDim < 2,
		"The image axes must have at least 2 pixel axes"
	);
	Vector<Double> inc = imageIn.coordinates().increment();
	Vector<String> units = imageIn.coordinates().worldAxisUnits();
	ThrowIf(
		! near (
			Quantity(fabs(inc[pixelAxes[0]]), units[pixelAxes[0]]),
			Quantity(fabs(inc[pixelAxes[1]]), units[pixelAxes[1]])
		),
		"Pixels must be square, please regrid your image so that they are"
	);
	const IPosition& inShape = imageIn.shape();
	const IPosition& outShape = imageOut->shape();
	ThrowIf(
		!inShape.isEqual(outShape),
		"Input and output images must have the same shape"
	);
	// Generate Kernel Array (height unity)
	ThrowIf(
		targetres && kernelType != VectorKernel::GAUSSIAN,
		"targetres can only be true for a Gaussian convolving kernel"
	);
	Array<T> kernel;
	// initialize to avoid compiler warning, kernelVolume will always be set to something
	// reasonable below before it is used.
	T kernelVolume = -1;
	if (! targetres) {
		kernelVolume = _makeKernel(
			kernel, kernelType, parameters, pixelAxes, imageIn
		);
	}
	Vector<Quantity> kernelParms = parameters.copy();

	// Figure out output image restoring beam (if any), output units and scale
	// factor for convolution kernel array

	GaussianBeam beamOut;
	const CoordinateSystem& cSys = imageIn.coordinates();
	const ImageInfo& imageInfo = imageIn.imageInfo();
	const Unit& brightnessUnit = imageIn.units();
	String brightnessUnitOut;
	ImageInfo iiOut = imageOut->imageInfo();
	if (imageInfo.hasMultipleBeams()) {
		ImageMetaData md(imageOut);
		uInt nChan = md.nChannels();
		uInt nPol = md.nStokes();
		// initialize all beams to be null
		iiOut.setAllBeams(nChan, nPol, GaussianBeam());

		Int specAxis = cSys.spectralAxisNumber();
		Int polAxis = cSys.polarizationAxisNumber();
		IPosition start(imageIn.ndim(), 0);
		IPosition end = imageIn.shape();
		if (nChan > 0) {
			end[specAxis] = 1;
		}
		if (nPol > 0) {
			end[polAxis] = 1;
		}
		Int channel = -1;
		Int polarization = -1;
		if (targetres) {
			iiOut.removeRestoringBeam();
			iiOut.setRestoringBeam(GaussianBeam(parameters));
		}

		uInt count = (nChan > 0 && nPol > 0)
		    ? nChan * nPol
		    : nChan > 0
		      ? nChan
		      : nPol;
		for (uInt i=0; i<count; i++) {
			if (nChan > 0) {
				channel = i % nChan;
				start[specAxis] = channel;
			}
			if (nPol > 0) {
				polarization = nChan > 0
					? (i - channel) % nChan
					: i;
				start[polAxis] = polarization;
			}
			Slicer slice(start, end);
			SubImage<T> subImage(imageIn, slice);
			CoordinateSystem subCsys = subImage.coordinates();
			if (subCsys.hasSpectralAxis()) {
				Vector<Double> subRefPix = subCsys.referencePixel();
				subRefPix[specAxis] = 0;
				subCsys.setReferencePixel(subRefPix);
			}
			GaussianBeam inputBeam = imageInfo.restoringBeam(channel, polarization);
			Bool doConvolve = True;
			if (targetres) {
				os << LogIO::NORMAL;
				if (channel >= 0) {
					os << "Channel " << channel << " of " << nChan;
					if (polarization >= 0) {
						os << ", ";
					}
				}
				if (polarization >= 0) {
					os << "Polarization " << polarization << " of " << nPol;
				}
				os << " ";
				if (near(inputBeam, GaussianBeam(parameters), 1e-5, Quantity(1e-2, "arcsec"))) {
					doConvolve = False;
					os << LogIO::NORMAL << " Input beam is already near target resolution so this "
						<< "plane will not be convolved" << LogIO::POST;
				}
				else {
					kernelParms = _getConvolvingBeamForTargetResolution(
						parameters, inputBeam
					);
					kernelVolume = _makeKernel(
						kernel, kernelType, kernelParms, pixelAxes, imageIn
					);

					os << ": Convolving image which has a beam of " << inputBeam
						<< " with a Gaussian of "
						<< GaussianBeam(kernelParms) << " to reach a target resolution of "
						<< GaussianBeam(parameters) << LogIO::POST;
				}
			}
			TempImage<Float> subImageOut(
				subImage.shape(), subImage.coordinates()
			);
			if (doConvolve) {
				T scaleFactor = _dealWithRestoringBeam(
					os, brightnessUnitOut, beamOut, kernel, kernelVolume,
					kernelType, kernelParms, pixelAxes, subCsys, inputBeam,
					brightnessUnit, autoScale, scale, i == 0
				);
				{
					os << LogIO::NORMAL << "Scaling pixel values by " << scaleFactor
						<< " for ";
					if (channel >= 0) {
						os << "channel number " << channel;
						if (polarization >= 0) {
							os << " and ";
						}
					}
					if (polarization >= 0) {
						os << "polarization number " << polarization;
					}
					os << LogIO::POST;
				}
				if (targetres && near(beamOut.getMajor(), beamOut.getMinor(), 1e-7)) {
					// circular beam should have same PA as given by user if
					// targetres
					beamOut.setPA(parameters[2]);
				}
				ImageConvolver<T> aic;
				aic.convolve(
					os, subImageOut, subImage, scaleFactor*kernel,
					ImageConvolver<T>::NONE, 1.0, copyMiscellaneous
				);
			}
			else {
				brightnessUnitOut = imageIn.units().getName();
				beamOut = inputBeam;
				subImageOut.put(subImage.get());
			}
			{
				Bool doMask = imageOut->isMasked() && imageOut->hasPixelMask();
				Lattice<Bool>* pMaskOut = 0;
				if (doMask) {
					pMaskOut = &imageOut->pixelMask();
					if (! pMaskOut->isWritable()) {
						doMask = False;
					}
				}
				IPosition cursorShape = subImageOut.niceCursorShape();
				IPosition outPos = start;
				LatticeStepper stepper(
					subImageOut.shape(), cursorShape, LatticeStepper::RESIZE
				);
				RO_MaskedLatticeIterator<T> iter(subImageOut, stepper);
				for (iter.reset(); !iter.atEnd(); iter++) {
					IPosition cursorShape = iter.cursorShape();
					imageOut->putSlice(iter.cursor(), outPos);
					if (doMask) {
						pMaskOut->putSlice(iter.getMask(), outPos);
					}
					outPos = outPos + cursorShape;
				}
			}
			if (! targetres) {
				iiOut.setBeam(
					channel, polarization, beamOut
				);
			}
		}
	}
	else {
		GaussianBeam inputBeam = imageInfo.restoringBeam();
		if (targetres) {
            kernelParms = _getConvolvingBeamForTargetResolution(
				parameters, inputBeam
			);
			os << LogIO::NORMAL << "Convolving image that has a beam of "
				<< inputBeam << " with a Gaussian of "
				<< GaussianBeam(kernelParms) << " to reach a target resolution of "
				<< GaussianBeam(parameters) << LogIO::POST;
			kernelVolume = _makeKernel(
				kernel, kernelType, kernelParms, pixelAxes, imageIn
			);
		}
        T scaleFactor = _dealWithRestoringBeam(
			os, brightnessUnitOut, beamOut, kernel, kernelVolume,
			kernelType, kernelParms, pixelAxes, cSys, inputBeam,
			brightnessUnit, autoScale, scale, True
		);
        os << LogIO::NORMAL << "Scaling pixel values by " << scaleFactor << LogIO::POST;
		if (targetres && near(beamOut.getMajor(), beamOut.getMinor(), 1e-7)) {
			// circular beam should have same PA as given by user if
			// targetres
			beamOut.setPA(parameters[2]);
		}
		// Convolve.  We have already scaled the convolution kernel (with some
		// trickery cleverer than what ImageConvolver can do) so no more scaling
		ImageConvolver<T> aic;
		aic.convolve(
			os, *imageOut, imageIn, scaleFactor*kernel, ImageConvolver<T>::NONE,
			1.0, copyMiscellaneous
		);
		// Overwrite some bits and pieces in the output image to do with the
		// restoring beam  and image units
		Bool holdsOneSkyAxis;
		Bool hasSky = CoordinateUtil::holdsSky (holdsOneSkyAxis, cSys, pixelAxes.asVector());
		if (hasSky && ! beamOut.isNull()) {
			iiOut.setRestoringBeam(beamOut);
		}
		else {
			// If one of the axes is in the sky plane, we must
			// delete the restoring beam as it is no longer meaningful
			if (holdsOneSkyAxis) {
				os << LogIO::WARN << "Because you convolved just one of the sky axes" << endl;
				os << "The output image does not have a valid spatial restoring beam" << LogIO::POST;
				iiOut.removeRestoringBeam();
			}
		}
	}
	imageOut->setUnits(brightnessUnitOut);
	imageOut->setImageInfo(iiOut);
}

// Private functions

template <class T> T Image2DConvolver<T>::_makeKernel(
	Array<T>& kernelArray,
	VectorKernel::KernelTypes kernelType,
	const Vector<Quantity>& parameters,
	const IPosition& pixelAxes,
	const ImageInterface<T>& imageIn
) {

// Check number of parameters

   _checkKernelParameters(kernelType, parameters);

// Convert kernel widths to pixels from world.  Demands major and minor
// both in pixels or both in world, else exception

   Vector<Double> dParameters;
   const CoordinateSystem cSys = imageIn.coordinates();

// Use the reference value for the shape conversion direction

   Vector<Quantum<Double> > wParameters(5);
   for (uInt i=0; i<3; i++) {
      wParameters(i+2) = parameters(i);
   }
//
   const Vector<Double> refVal = cSys.referenceValue();
   const Vector<String> units = cSys.worldAxisUnits();
   Int wAxis = cSys.pixelAxisToWorldAxis(pixelAxes(0));
   wParameters(0) = Quantum<Double>(refVal(wAxis), units(wAxis));
   wAxis = cSys.pixelAxisToWorldAxis(pixelAxes(1));
   wParameters(1) = Quantum<Double>(refVal(wAxis), units(wAxis));
   SkyComponentFactory::worldWidthsToPixel (dParameters, wParameters, cSys, pixelAxes, False);

// Create n-Dim kernel array shape

   IPosition kernelShape = _shapeOfKernel (kernelType, dParameters, imageIn.ndim(), pixelAxes);

// Create kernel array. We will fill the n-Dim array (shape non-unity
// only for pixelAxes) through its 2D Matrix incarnation. Aren't we clever.
   kernelArray = 0;
   kernelArray.resize(kernelShape);
   Array<T> kernelArray2 = kernelArray.nonDegenerate (pixelAxes);
   Matrix<T> kernelMatrix = static_cast<Matrix<T> >(kernelArray2);

// Fill kernel Matrix with functional (height unity)

   return _fillKernel (kernelMatrix, kernelType, kernelShape, pixelAxes, dParameters);
}

template <class T> T Image2DConvolver<T>::_dealWithRestoringBeam(
	LogIO& os, String& brightnessUnitOut,
	GaussianBeam& beamOut, const Array<T>& kernelArray,
	const T kernelVolume, const VectorKernel::KernelTypes,
	const Vector<Quantity>& parameters,
	const IPosition& pixelAxes, const CoordinateSystem& cSys,
	const GaussianBeam& beamIn, const Unit& brightnessUnitIn,
	const Bool autoScale, const Double scale, const Bool emitMessage
) {
	os << LogOrigin("Image2DConvolver", __func__);
	// Find out if convolution axes hold the sky.  Scaling from
	// Jy/beam and Jy/pixel only really makes sense if this is True
	Bool holdsOneSkyAxis;
	Bool hasSky = CoordinateUtil::holdsSky (holdsOneSkyAxis, cSys, pixelAxes.asVector());
	if (hasSky) {
		const DirectionCoordinate dc = cSys.directionCoordinate();
		Vector<Double> inc = dc.increment();
		Vector<String> unit = dc.worldAxisUnits();
		Quantity x(inc[0], unit[0]);
		Quantity y(inc[1], unit[1]);
		Quantity diag = sqrt(x*x + y*y);
		Quantity minAx = parameters[1];
		if (minAx.getUnit().startsWith("pix")) {
			minAx.setValue(minAx.getValue()*x.getValue());
			minAx.setUnit(x.getUnit());
		}
		if (minAx < diag) {
			diag.convert(minAx.getFullUnit());
			os << LogIO::WARN << "Convolving kernel has minor axis "
				<< minAx << " which is less than the pixel diagonal "
				<< "length of " << diag << ". Thus, the kernel is poorly sampled, "
				<< "and so the output of this application may not be what you expect. "
				<< "You should consider increasing the kernel size or regridding "
				<< "the image to a smaller pixel size" << LogIO::POST;
		}
		else if (beamIn.getMinor() < diag) {
			diag.convert(beamIn.getMinor().getFullUnit());
			os << LogIO::WARN << "Input beam has minor axis "
				<< beamIn.getMinor() << " which is less than the pixel diagonal "
				<< "length of " << diag << ". Thus, the beam is poorly sampled, "
				<< "and so the output of this application may not be what you expect. "
				<< "You should consider regridding "
				<< "the image to a smaller pixel size." << LogIO::POST;
		}
	}
	if (emitMessage) {
		os << "You are " << (hasSky ? "" : " not ") << "convolving the sky" << LogIO::POST;
	}
	beamOut = GaussianBeam();
	String bUnitIn = upcase(brightnessUnitIn.getName());
	const Vector<Double>& refPix = cSys.referencePixel();
	T scaleFactor = 1;
	brightnessUnitOut = brightnessUnitIn.getName();
	if (hasSky && bUnitIn.contains("/PIXEL")) {
		// Easy case.  Peak of convolution kernel must be unity
		// and output units are Jy/beam.  All other cases require
		// numerical convolution of beams
		brightnessUnitOut = "Jy/beam";
		// Exception already generated if only one of major and minor in pixel units
		Quantity majAx = parameters(0);
		Quantity minAx = parameters(1);
		if (majAx.getFullUnit().getName() == "pix") {
			Vector<Double> pixelParameters(5);
			pixelParameters(0) = refPix(pixelAxes(0));
			pixelParameters(1) = refPix(pixelAxes(1));
			pixelParameters(2) = parameters(0).getValue();
			pixelParameters(3) = parameters(1).getValue();
			pixelParameters(4) = parameters(2).getValue(Unit("rad"));
			GaussianBeam worldParameters;
			SkyComponentFactory::pixelWidthsToWorld(
				worldParameters, pixelParameters,
				cSys, pixelAxes, False
			);
			majAx = worldParameters.getMajor();
			minAx = worldParameters.getMinor();
		}
		beamOut = GaussianBeam(majAx, minAx, parameters(2));
		// Input p.a. is positive N->E
		if (!autoScale) {
			scaleFactor = static_cast<T>(scale);
			os << LogIO::WARN << "Autoscaling is recommended for Jy/pixel convolution"
				<< LogIO::POST;
		}
	}
	else {
		// Is there an input restoring beam and are we convolving the sky to which it
		// pertains ?  If not, all we can do is use user scaling or normalize the convolution
		// kernel to unit volume.  There is no point to convolving the input beam either as it pertains
		// only to the sky
		if (hasSky && ! beamIn.isNull()) {
			// Convert restoring beam parameters to pixels.  Output pa is pos +x -> +y in pixel frame.
			Vector<Quantity> wParameters(5);
			const Vector<Double> refVal = cSys.referenceValue();
			const Vector<String> units = cSys.worldAxisUnits();
			Int wAxis = cSys.pixelAxisToWorldAxis(pixelAxes(0));
			wParameters(0) = Quantity(refVal(wAxis), units(wAxis));
			wAxis = cSys.pixelAxisToWorldAxis(pixelAxes(1));
			wParameters(1) = Quantity(refVal(wAxis), units(wAxis));
			wParameters(2) = beamIn.getMajor();
			wParameters(3) = beamIn.getMinor();
			wParameters(4) = beamIn.getPA(True);
			Vector<Double> dParameters;
			SkyComponentFactory::worldWidthsToPixel(
				dParameters, wParameters, cSys, pixelAxes, False
			);
			// Create 2-D beam array shape
			IPosition dummyAxes(2, 0, 1);
			IPosition beamShape = _shapeOfKernel(
				VectorKernel::GAUSSIAN,
				dParameters, 2, dummyAxes
			);

			// Create beam Matrix and fill with height unity
   
			Matrix<T> beamMatrixIn(beamShape(0), beamShape(1));
			_fillKernel(
				beamMatrixIn, VectorKernel::GAUSSIAN, beamShape,
                dummyAxes, dParameters
			);

			IPosition shape = beamMatrixIn.shape();

			// Get 2-D version of convolution kenrel
			Array<T> kernelArray2 = kernelArray.nonDegenerate(pixelAxes);
			Matrix<T> kernelMatrix = static_cast<Matrix<T> >(kernelArray2);
			// Convolve input restoring beam array by convolution kernel array
			Matrix<T> beamMatrixOut;

			Convolver<T> conv(beamMatrixIn, kernelMatrix.shape());
			conv.linearConv(beamMatrixOut, kernelMatrix);

			// Scale kernel
			T maxValOut = max(beamMatrixOut);

			scaleFactor = autoScale ? 1/maxValOut : (T)scale;
			// Fit output beam matrix with a Gaussian, for better or worse
			// Fit2D is not templated.  So all our templating is useless
			// other than for Float until I template Fit2D
			Fit2D fitter(os);
			const uInt n = beamMatrixOut.shape()(0);
			Vector<Double> bParameters = fitter.estimate(Fit2D::GAUSSIAN, beamMatrixOut);
			Vector<Bool> bParameterMask(bParameters.nelements(), True);
			bParameters(1) = (n-1)/2;          // x centre
			bParameters(2) = bParameters(1);    // y centre
			// Set range so we don't include too many pixels in fit which will make it very slow
			fitter.addModel (Fit2D::GAUSSIAN, bParameters, bParameterMask);
			Array<Float> sigma;
			fitter.setIncludeRange(maxValOut/10.0, maxValOut+0.1);
			Fit2D::ErrorTypes error = fitter.fit (beamMatrixOut, sigma);
			ThrowIf(
				error == Fit2D::NOCONVERGE || error == Fit2D::FAILED
				|| error == Fit2D::NOGOOD,
				"Failed to fit the output beam"
			);
			Vector<Double> bSolution = fitter.availableSolution();
			// Convert to world units.
			Vector<Double> pixelParameters(5);
			pixelParameters(0) = refPix(pixelAxes(0));
			pixelParameters(1) = refPix(pixelAxes(1));
			pixelParameters(2) = bSolution(3);
			pixelParameters(3) = bSolution(4);
			pixelParameters(4) = bSolution(5);
			SkyComponentFactory::pixelWidthsToWorld(
				beamOut, pixelParameters, cSys, pixelAxes, False
			);
			if (brightnessUnitIn.getName().contains("K")) {
				scaleFactor *= beamIn.getArea("arcsec2")/beamOut.getArea("arcsec2");
			}
		}
		else {
			if (autoScale) {
				// Conserve flux is the best we can do
				scaleFactor = 1/kernelVolume;
			}
			else {
				scaleFactor = (T)scale;
			}
		}
	}
	/*
	if (scaleFactor != 1) {
		kernelArray *= scaleFactor;
	}
	*/
	// Put beam position angle into range +/- 180 in case it has eluded us so far
	if (! beamOut.isNull()) {
		MVAngle pa(beamOut.getPA(True).getValue(Unit("rad")));
		pa();
		beamOut = GaussianBeam(
			beamOut.getMajor(), beamOut.getMinor(),
			Quantity(pa.degree(), Unit("deg"))
		);
	}
	return scaleFactor;
}

template <class T> void Image2DConvolver<T>::_checkKernelParameters(
	VectorKernel::KernelTypes kernelType,
	const Vector<Quantity >& parameters
) {
	if (kernelType==VectorKernel::BOXCAR) {
		ThrowCc("Boxcar kernel not yet implemented");
		ThrowIf(
			parameters.nelements() != 3,
			"Boxcar kernels require 3 parameters"
		);
	}
	else if (kernelType==VectorKernel::GAUSSIAN) {
		ThrowIf(
			parameters.nelements() != 3,
			"Gaussian kernels require exactly 3 parameters"
		);
	}
	else {
		ThrowCc(
			"The kernel type " + VectorKernel::fromKernelType(kernelType) + " is not supported"
		);
	}
}

template <class T> IPosition Image2DConvolver<T>::_shapeOfKernel(
	const VectorKernel::KernelTypes kernelType,
	const Vector<Double>& parameters,
	const uInt ndim, const IPosition& axes
) {
//
// Work out how big the array holding the kernel should be.
// Simplest algorithm possible. Shape is presently square.
//

// Find 2D shape

   uInt n;
   if (kernelType==VectorKernel::GAUSSIAN) {
      uInt n1 = _sizeOfGaussian (parameters(0), 5.0);
      uInt n2 = _sizeOfGaussian (parameters(1), 5.0);
      n = max(n1,n2);
      if (n%2==0) n++;                                     // Make shape odd so centres well
   } else if (kernelType==VectorKernel::BOXCAR) {
      n = 2 * Int(max(parameters(0), parameters(1))+0.5);
      if (n%2==0) n++;                                     // Make shape odd so centres well
   } else {
     throw(AipsError("Unrecognized kernel type"));        // Earlier checking should prevent this
   }

// Now find the shape for the image and slot the 2D shape in
// in the correct axis locations

   IPosition shape(ndim,1);
   shape(axes(0)) = n; 
   shape(axes(1)) = n;
//
   return shape;
}
   
template <class T>
uInt Image2DConvolver<T>::_sizeOfGaussian(
	const Double width, const Double nSigma
) {
// +/- 5sigma is a volume error of less than 6e-5%

   Double sigma = width / sqrt(Double(8.0) * C::ln2);
   return  (Int(nSigma*sigma + 0.5) + 1) * 2;
}


template <class T> T Image2DConvolver<T>::_fillKernel(
	Matrix<T>& kernelMatrix,
	VectorKernel::KernelTypes kernelType,
	const IPosition& kernelShape,
	const IPosition& axes,
	const Vector<Double>& parameters
) {

// Centre functional in array (shape is odd)
// Need to think about these T castes for Complex images

   T xCentre = static_cast<T>((kernelShape(axes(0)) - 1) / 2.0);
   T yCentre = static_cast<T>((kernelShape(axes(1)) - 1) / 2.0);
   T height = static_cast<T>(1.0);

// Create functional.  We only have gaussian2d functionals
// at this point.  Later the filling code can be moved out
// of the if statement

   T maxValKernel, volumeKernel;  
   T pa = static_cast<T>(parameters(2));
   T ratio = static_cast<T>(parameters(1) / parameters(0));
   T major = static_cast<T>(parameters(0));
   if (kernelType==VectorKernel::GAUSSIAN) {
       _fillGaussian (maxValKernel, volumeKernel, kernelMatrix, height,
                     xCentre, yCentre, major, ratio, pa);
   } else if (kernelType==VectorKernel::BOXCAR) {
/*
      fillBoxcar (maxValKernel, volumeKernel, kernelMatrix, height,
                  xCentre, yCentre, major, ratio, pa);
*/
   } else {
     throw(AipsError("Unrecognized kernel type"));        // Earlier checking should prevent this
   }
//
   return volumeKernel;
}         

template <class T> void Image2DConvolver<T>::_fillGaussian(
	T& maxVal, T& volume, Matrix<T>& pixels, T height,
	T xCentre, T yCentre, T majorAxis, T ratio,
	T positionAngle
) {
// 
// pa positive in +x ->+y pixel coordinate frame
//
   uInt n1 = pixels.shape()(0);
   uInt n2 = pixels.shape()(1);
   AlwaysAssert(n1==n2,AipsError);
   positionAngle += C::pi_2;        // +y -> -x
   Gaussian2D<T> g2d(height, xCentre, yCentre, majorAxis,
		       ratio, positionAngle);
   maxVal = -1.0e30;
   volume = 0.0;
   Vector<T> pos(2);
   for (uInt j=0; j<n1; j++) {
      pos(1) = static_cast<T>(j);
      for (uInt i=0; i<n1; i++) {
         pos(0) = static_cast<T>(i);
         T val = g2d(pos);
         pixels(i,j) = val;
//
         maxVal = max(val, maxVal);
         volume += val;
      }
   } 
}

} //# NAMESPACE CASA - END

