
# CASA - Common Astronomy Software Applications
# Copyright (C) 2012 by AUI  
#
# This file is part of CASA.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# Make sure that compile time definitions (build time, svn revision number)
# are recomputed in every build.

# This target is executed unconditionally and before any other target that
# depends on the definitions file
add_custom_target( stdcasa_definitions COMMAND ${CMAKE_COMMAND} -E remove -f definitions )

casa_add_library( stdcasa
	StdCasa/CasacSupport.cc
	StdCasa/record.cc
	StdCasa/variant.cc
    StdCasa/version.cc
	)
add_dependencies( libstdcasa      stdcasa_definitions )
install (FILES
	xerces.h
	Quantity.h
	array.h
	record.h
	variant.h
	casac.h
	version.h
    cboost_foreach.h
	DESTINATION include/casacode/stdcasa
	)
install (FILES
	StdCasa/CasacSupport.h
	StdCasa/string_conversions
	DESTINATION include/casacode/stdcasa/StdCasa
	)
