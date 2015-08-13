//# Conversion.h: conversion routines for dbus
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

#ifndef DBUS_CONVERSION_H_
#define DBUS_CONVERSION_H_
#include <map>
#include <string>
#include <casa/Containers/Record.h>
#include <dbus-c++-1/dbus-c++/dbus.h>
#include <casadbus/types/variant.h>
#include <casadbus/types/record.h>
#include <casa/Arrays/Vector.h>

namespace casa {

    namespace dbus {
		Record toRecord( const std::map<std::string,DBus::Variant> &mapIn );
		std::map<std::string,DBus::Variant> fromRecord( const Record &record );

		variant toVariant( const DBus::Variant &src );

		std::map<std::string,variant> toStdMap( const std::map<std::string,DBus::Variant> &src );
		std::map<std::string,DBus::Variant> fromStdMap( const std::map<std::string,variant> &src );

		DBus::Variant fromVariant( const variant &src );
		void show( const variant &v );

		template<class t> std::vector<double> af( const Vector<t> &other ) {
			std::vector<double> result(other.nelements( ));
			for (unsigned int x=0; x < result.size(); ++x)
				result[x] = (double) other[x];
			return result;
		}
    }
}

#endif
