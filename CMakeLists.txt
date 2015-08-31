# CASA - Common Astronomy Software Applications
# Copyright (C) 2010 by ESO (in the framework of the ALMA collaboration)
# Copyright (C) 2010-2013 Associated Universities, Inc. Washington DC, USA.
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

cmake_minimum_required( VERSION 2.8 )

SET(BUILD_SHARED_LIBS ON)

option (CXX11 "Compile as C++11 if possible" NO)

set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/install )

include( config )
include( target )

set( CASA_MAJOR_VERSION 4 )
set( CASA_MINOR_VERSION 3 )
set( CASA_PATCH_VERSION 0 )
set( CASA_API_VERSION "${CASA_MAJOR_VERSION}.${CASA_MINOR_VERSION}.${CASA_PATCH_VERSION}" )

###
### default name for casa-maintained dbus C++ library... (but could also be called just "dbus-cpp")
###
set( dbus-cpp-library-name casa-dbus-cpp )
###
### by default, look for Qt4 in the standard places... (with casa01-* RPMs and linux, we avoid system places)...
###
set( qt4 system-qt )

enable_language (Fortran)

set( SO so )


project( CASA C CXX Fortran )



# Test driver which requires that test programs are up-to-date
# before running. (The built-in "test" does not.)
add_custom_target( check ${CMAKE_CTEST_COMMAND} )

#set( CASA_assay ${CMAKE_SOURCE_DIR}/install/assay )


set( CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined")

set( dbusxx dbus-c++ )
set( casa_packages /usr/lib64/casapy )
set( qt4 system-qt )



set( boost_components regex program_options filesystem thread serialization system)
find_package( Boost REQUIRED ${boost_components} )


# CFITSIO
casa_find( CFITSIO
  VERSION 3.006
  INCLUDES fitsio.h fitsio2.h
  INCLUDES_SUFFIXES cfitsio
  LIBS cfitsio
  PREFIX_HINTS ${CFITSIO_ROOT_DIR}
  DEPENDS Boost
  CPP_VERSION CFITSIO_VERSION
  RUN_VERSION "(ffvers(&v), v)" )

# X11.
if( NOT APPLE )
  find_package( X11 REQUIRED )
  # Rename
  set( X11_INCLUDE_DIRS ${X11_INCLUDE_DIR} )
endif()


SET(NO_SOVERSION FALSE CACHE BOOL "do not add version information to shared libraries")
if( NOT NO_SOVERSION )
    set( epochdelta 1385614800 )
    if ( EXISTS ${CMAKE_INSTALL_PREFIX}/${arch}/casa_sover.txt )
        execute_process( COMMAND perl -e "while (<>) { chomp and print if (! m/^\#/ ) }" ${CMAKE_INSTALL_PREFIX}/${arch}/casa_sover.txt
                         OUTPUT_VARIABLE __casa_soversion )
    elseif( EXISTS ${CMAKE_INSTALL_PREFIX}/casa_sover.txt )
        execute_process( COMMAND perl -e "while (<>) { chomp and print if (! m/^#/ ) }" ${CMAKE_INSTALL_PREFIX}/casa_sover.txt
                         OUTPUT_VARIABLE __casa_soversion )
    else( )
        execute_process( COMMAND perl -e "$t=time( )-${epochdelta};$z=$t & 0xff; $y=($t>>8)&0xff; $x=($t>>16)&0xffff; print \"$x.$y.$z\""
                         OUTPUT_VARIABLE __casa_soversion )
    endif( )
    set(casa_soversion ${__casa_soversion} CACHE STRING "version for shared objects")
    message( STATUS "Shared object version number ${casa_soversion}" )
else( )
    message( STATUS "User disabled shared library versioning" )
endif( )


if( NOT FORTRAN_LIBRARIES )

  message( STATUS "Looking for Fortran runtime libraries" )
  set( _try  ${CMAKE_BINARY_DIR}/try_fortran.cc )
  file( WRITE ${_try}
    "int main() { return 0; }\n"
    )

  if( _gfortran_lib_path )
    try_compile( _have_gfortran ${CMAKE_BINARY_DIR} ${_try}
      CMAKE_FLAGS -Wdev "-DCMAKE_EXE_LINKER_FLAGS=${_gfortran_lib_path}"
      )
  else()
    try_compile( _have_gfortran ${CMAKE_BINARY_DIR} ${_try}
      CMAKE_FLAGS -Wdev "-DCMAKE_EXE_LINKER_FLAGS=-lgfortran"
      )
  endif()
  try_compile( _have_g2c ${CMAKE_BINARY_DIR} ${_try}
    CMAKE_FLAGS -Wdev "-DCMAKE_EXE_LINKER_FLAGS=-lg2c"
    )

  if( _have_gfortran )
    if( _gfortran_lib_path )
      set( FORTRAN_LIBRARIES ${_gfortran_lib_path}
        CACHE STRING "Fortran library linker option" FORCE )
    else()
      set( FORTRAN_LIBRARIES -lgfortran
        CACHE STRING "Fortran library linker option" FORCE )
    endif()
    message( STATUS "Looking for Fortran runtime libraries -- ${FORTRAN_LIBRARIES}" )
  elseif( _have_g2c )
    set( FORTRAN_LIBRARIES -lg2c
      CACHE STRING "Fortran library linker option" FORCE )
    message( STATUS "Looking for Fortran runtime libraries -- ${FORTRAN_LIBRARIES}" )
  else()
    set( FORTRAN_LIBRARIES ""
      CACHE STRING "Fortran library linker option" FORCE )
    message( STATUS "Looking for Fortran runtime libraries -- <none>" )
    # Not a fatal error because it might work, if all Fortran dependencies were
    # already linked statically to the Fortran runtime...
  endif()
endif()

# PGPLOT (FORTRAN plotting package).
# depends on X11
if( NOT SKIP_PGPLOT )
  if( APPLE )
    set( _deps FORTRAN )
  else()
    set( _deps X11 FORTRAN )
  endif()
  if (CMAKE_SYSTEM MATCHES ^Darwin-11 AND NOT LLVMCOMPILER )
     set( CMAKE_SHARED_LINKER_FLAGS "-Wl,-undefined -Wl,error -framework AppKit -lgfortran" )
  else()
     if (APPLE) 
        set( CMAKE_SHARED_LINKER_FLAGS "-Wl,-undefined -Wl,error -framework AppKit" )
     else()
        set( CMAKE_SHARED_LINKER_FLAGS "-Wl,-undefined -Wl,error" )
     endif()
  endif()
  casa_find( PGPLOT
    VERSION 5.3.1
    PREFIX_HINTS ${PGPLOT_ROOT_DIR}
    INCLUDES cpgplot.h
    LIBS pgplot${pgplot_ext} cpgplot${pgplot_ext}
    DEPENDS ${_deps} )
endif()

# WCSLIB
if( NOT SKIP_PGPLOT )
  casa_find( WCSLIB
    VERSION 4.3
    PREFIX_HINTS ${WCSLIB_ROOT_DIR}
    INCLUDES wcslib/wcsconfig.h
    LIBS wcs
         pgsbox${pgplot_ext}
    # WCSLIB does "#define WCSLIB_VERSION 4.3.4" without quotes,
    # hence "cout << WCSLIB_VERSION" would not work. The following
    # code makes a string out of WCSLIB_VERSION.
    CPP_VERSION "\"\";
#define CASA_STRINGIFY(x) #x
#define CASA_TOSTRING(x) CASA_STRINGIFY(x)
    std::cout << CASA_TOSTRING(WCSLIB_VERSION)"
    DEFINITIONS -DSIGNBIT 
    DEPENDS PGPLOT )
else()
  casa_find( WCSLIB
    VERSION 4.3
    INCLUDES wcslib/wcsconfig.h
    LIBS wcs
    # WCSLIB does "#define WCSLIB_VERSION 4.3.4" without quotes,
    # hence "cout << WCSLIB_VERSION" would not work. The following
    # code makes a string out of WCSLIB_VERSION.
    CPP_VERSION "\"\";
#define CASA_STRINGIFY(x) #x
#define CASA_TOSTRING(x) CASA_STRINGIFY(x)
    std::cout << CASA_TOSTRING(WCSLIB_VERSION)"
    DEFINITIONS -DSIGNBIT )
endif()

# FFTW
# Does not provide its version number in a header nor
# in the library, which is a pity because CASACore needs
# version >= 3.2.2 and is subject to a bug in fftw 3.2.1
casa_find( FFTW3
           PREFIX_HINTS ${FFTW3_ROOT_DIR}
           INCLUDES fftw3.h
           LIBS fftw3_threads fftw3f_threads fftw3f fftw3 )

# BLAS
casa_find( BLAS
           LIBS blas )

# LAPACK
casa_find( LAPACK
           LIBS lapack 
           DEPENDS BLAS )

# DL
set( DL_LIBRARIES ${CMAKE_DL_LIBS} CACHE STRING "dl libraries" FORCE )
if( DL_LIBRARIES STREQUAL "dl" )
  set( DL_LIBRARIES "-ldl" CACHE STRING "dl libraries" FORCE )
endif()

# CASACore
# - Some header files exported by CASACore include WCSLIB headers
# - CASACore header files declare symbols which are defined in LAPACK and BLAS
# - There are several, incompatible, ways of building CASACore.

if( EXISTS ${CMAKE_INSTALL_PREFIX}/lib/libcasacore.dylib OR
    EXISTS ${CMAKE_INSTALL_PREFIX}/lib/libcasacore.so OR
    EXISTS ${CMAKE_INSTALL_PREFIX}/lib64/libcasacore.so OR USE_LIBCASACORE )
  set( casacore_libs casacore )
  ##
  ## by default the GNUmakefile for casacore installs include
  ## files into ${casaroot}.
  ##
  set( casacore_prefix ${CMAKE_INSTALL_PREFIX}/.. )
else()
  if( EXISTS ${CMAKE_INSTALL_PREFIX}/../include/casacore/casa/BasicMath.h )
      set( casacore_prefix ${CMAKE_INSTALL_PREFIX}/.. )
  endif()
  set( casacore_libs
       casa_casa 
       casa_coordinates 
       casa_fits
       casa_images
       casa_lattices
       casa_measures
       casa_mirlib
       casa_ms
       casa_msfits
       casa_scimath
       casa_scimath_f
       casa_tables )
endif()

casa_find( CASACORE
           VERSION 2.0.0
           PREFIX_HINTS ${CASACORE_ROOT_DIR} ${Boost_INCLUDE_DIR}
           INCLUDES casa/aips.h
                    casa/version.h
                    scimath/Mathematics/MatrixMathLA.h  # declares symbols defined in LAPACK
           INCLUDES_SUFFIXES casacore
           LIBS ${casacore_libs}
           PREFIX_HINTS ${casacore_prefix}
           CPP_VERSION CASACORE_VERSION
           RUN_VERSION "casa::getVersionCASA()"
           DEPENDS LAPACK CFITSIO WCSLIB FFTW3 DL Boost)
# Listing explicitly CASACore's dependency libraries is not necessary if the CASACore libraries 
# were built with correct linking information, which they are not.

set( CASACORE_DEFINITIONS ${CASACORE_DEFINITIONS}
  -DCASA_USECASAPATH
  -DCASACORE_NEEDS_RETHROW
  -DAIPS_STDLIB
  -DAIPS_AUTO_STL
  -D_GNU_SOURCE )

if( CMAKE_SYSTEM_NAME STREQUAL Linux )
  set( CASACORE_DEFINITIONS ${CASACORE_DEFINITIONS}
    -D_FILE_OFFSET_BITS=64
    -D_LARGEFILE_SOURCE 
    )
endif()

casa_find( GSLCBLAS	  
              LIBS gslcblas )

casa_find( GSL 
	      LIBS gsl
   	      DEPENDS GSLCBLAS
            )

## Java
#casa_find( Java
#  VERSION 1.5
#  PROGRAMS java
#  PROG_VERSION "-version")

if ( ${dbusxx} STREQUAL dbus-c++ )
  # C++ DBus
  #
  # The header file dbus/dbus-arch-deps.h exists on some but not all systems, in a directory
  # for itself
  #
  # Form all possible DBus prefixes and use them as hints to casa_find()
  #
  set( dbus_prefix "" )
  foreach( _p ${CMAKE_INSTALL_PREFIX} ${casa_packages}  /sw /opt/local /usr/local /opt /usr )
    foreach( _l lib lib64 )
      list( APPEND dbus_prefix ${_p}/dbus-1.0 )
      list( APPEND dbus_prefix ${_p}/${_l}/dbus-1.0 )
      list( APPEND dbus_prefix ${_p}/${_l}/qt-4.3.4/dbus )
      list( APPEND dbus_prefix ${_p}/${_l}/qt-4.3.4/dbus/${_l}/dbus-1.0 )
    endforeach()
  endforeach()

  casa_find( DBUSXX_ARCH
    INCLUDES dbus/dbus-arch-deps.h
    PREFIX_HINTS /usr/lib64/dbus-1.0 ${DBUS_ARCH_DIR} ${dbus_prefix}
    NO_CHECK   # This header does not compile if included directly
    NO_REQUIRE
    IGNORE /usr/lib64/casapy
  )
  if( DBUSXX_ARCH_FOUND )
    set( dbus_depends DBUSXX_ARCH )
  endif()

  include(FindPkgConfig)
  pkg_search_module(DBUS REQUIRED dbus-c++-1)
  find_program(dbus-xml-2-cxx NAMES dbusxx-xml2cpp)
else( )
  casa_find( DBUSPP_ARCH
    INCLUDES dbus/dbus-arch-deps.h
    PREFIX_HINTS ${DBUS_ARCH_DIR} ${casa_packages}/lib/dbus-1.0 ${dbus-arch-path-guess}
    NO_CHECK   # This header does not compile if included directly
    NO_REQUIRE
  )
  if( DBUSPP_ARCH_FOUND )
    set( dbus_depends DBUSPP_ARCH )
  endif()

  casa_find( DBUSPP
    INCLUDES dbus/dbus.h
             dbus-cpp/dbus.h
    INCLUDES_SUFFIXES dbus-1.0 dbus-cpp
    PREFIX_HINTS ${DBUS_ROOT_DIR} ${casa_packages}
    LIBS ${dbus-cpp-library-name}
    PROGRAMS dbuspp-xml2cpp
    DEPENDS ${dbus_depends}
  )
  set( dbus-xml-2-cxx ${DBUSPP_dbuspp-xml2cpp_EXECUTABLE} )
  set( DBUS_INCLUDE_DIRS ${DBUSPP_INCLUDE_DIRS} )
  set( DBUS_LIBRARIES ${DBUSPP_LIBRARIES} )
endif( )



# XML2
casa_find( LIBXML2
  VERSION 2.6.16
  PREFIX_HINTS ${LIBXML2_ROOT_DIR}
  INCLUDES libxml/xmlversion.h
  INCLUDES_SUFFIXES libxml2
  LIBS xml2
  CPP_VERSION LIBXML_DOTTED_VERSION 
  RUN_VERSION "LIBXML_DOTTED_VERSION; LIBXML_TEST_VERSION;" 
  )


# Do not reuse FindLibXml2.cmake, except this excerpt
IF (NOT WIN32)
   # use pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   FIND_PACKAGE(PkgConfig)
   PKG_CHECK_MODULES(PC_LIBXML libxml-2.0)
   SET(LIBXML2_DEFINITIONS ${LIBXML2_DEFINITIONS} ${PC_LIBXML_CFLAGS_OTHER})
ENDIF (NOT WIN32)



# Readline
casa_find( READLINE 
  VERSION 4.3
  INCLUDES readline/readline.h #depends on <cstdio> to compile
  LIBS readline ncurses
  PREFIX_HINTS ${READLINE_ROOT_DIR}
  CPP_VERSION "RL_VERSION_MAJOR << '.' << RL_VERSION_MINOR"
  RUN_VERSION rl_library_version
)

#GSL
find_package ( GSL 1.13)
if( NOT GSL_FOUND )
  message (WARNING "GSL package not found")
endif ( NOT GSL_FOUND )

set( DL_LIBRARIES ${CMAKE_DL_LIBS} )



casa_config_end()

# The modules must be defined in dependency order!
# This will set up include paths, and which libraries to link to

# for linux RPM & binary distros libgraphics contains pgplot (linked in)...
if( APPLE )
  casa_add_module( graphics CASACORE PGPLOT WCSLIB Boost)
else()
  if( NOT SKIP_PGPLOT )
    casa_add_module( graphics CASACORE PGPLOT X11 WCSLIB Boost)
  else()
    casa_add_module( graphics CASACORE X11 WCSLIB Boost)
  endif()
endif()

casa_add_module( atmosphere )
casa_add_module( casadbus CASACORE DBUS Boost)
casa_add_module( asdmstman CASACORE )
casa_add_module( msvis CASACORE Boost asdmstman )
casa_add_module( stdcasa CASACORE )
casa_add_module( components stdcasa CASACORE GSL)
casa_add_module( imageanalysis CASACORE Boost components)
casa_add_module( mstransform CASACORE stdcasa imageanalysis msvis)
casa_add_module( synthesis CASACORE mstransform msvis casadbus imageanalysis atmosphere)
