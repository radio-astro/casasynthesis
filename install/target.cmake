#
# CASA - Common Astronomy Software Applications
# Copyright (C) 2010 by ESO (in the framework of the ALMA collaboration)
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




macro( casa_add_library module )

  add_definitions( ${${module}_DEFINITIONS} )
  include_directories( ${${module}_INCLUDE_DIRS} )

  # Create the target lib<module>, but set the output library
  # filename to lib<module>.<suffix> (which would have defaulted
  # to liblib<module>.<suffix>). The target named <module> is the 
  # umbrella target for all targets (libraries, executables, ...) 
  # in this subdirectory.

  add_library( lib${module} ${ARGN} )
  set_target_properties( lib${module} PROPERTIES OUTPUT_NAME ${module} )


  target_link_libraries( lib${module} ${${module}_LINK_TO} )

  if( NOT NO_SOVERSION )
      set_target_properties( lib${module} PROPERTIES SOVERSION ${casa_soversion} )
  endif()

  install( TARGETS lib${module}
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib )

endmacro()

#
# casa_add_executable( module name source1 [source2...] )
#
#     Add an executable, and define the install rules.
#
macro( casa_add_executable module name )

  set( _sources ${ARGN} )

  add_executable( ${name} ${_sources} )

  target_link_libraries( ${name} lib${module} )

  install( TARGETS ${name} RUNTIME DESTINATION bin )

endmacro()

#
# casa_add_test( module source1 [source2 ...])
#
#      Add a unit test. The name of the test will be the basename of
#      of the source file.
#
#      Secondary source files without a path will be provided with the
#      path of the primary source file (i.e., source1).
#
macro( casa_add_test module source )

  get_filename_component( _tname ${source} NAME_WE )

  add_executable( ${_tname} EXCLUDE_FROM_ALL ${source} )
  target_link_libraries( ${_tname} lib${module} )
  add_test( ${_tname} ${CMAKE_CURRENT_BINARY_DIR}/${_tname} )
  add_dependencies( check ${_tname} )

endmacro()

#
# casa_add_assay( module source ...)
#
#      Add an assay unit test. The name of the test will be the basename of
#      of the source file.
#
macro( casa_add_assay module source )

  get_filename_component( _tname ${source} NAME_WE )

  get_filename_component(path ${source} PATH)

  string( REGEX REPLACE " ([a-zA-Z_0-9]+[.]cc)" " ${path}/\\1" result " ${ARGN}")
  string( REGEX REPLACE "^ " "" result ${result})

  add_executable( ${_tname} EXCLUDE_FROM_ALL ${source} ${result} )
  target_link_libraries( ${_tname} lib${module} )
  add_test( ${_tname} ${CASA_assay} ${CMAKE_CURRENT_BINARY_DIR}/${_tname} )
  add_dependencies( check ${_tname} )

endmacro()


#
# Add a CASA subdirectory (module)
#
#  The following variables are set
#     <module>_INCLUDE_DIRS
#     <module>_DEFINITIONS
#     <module>_LINK_TO
# 
# Fixme: move qt4, dbus, alma special cases
# to client code

macro( casa_add_module module )

  set( _dependencies ${ARGN} )

  # Internal target to update this module including dependencies,
  # then install this module, excluding dependencies
  add_custom_target( 
    ${module}_install
    COMMAND ${CMAKE_BUILD_TOOL} install 
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${module}
    )


  # Target to update and install this module, including dependency modules
  add_custom_target( ${module} COMMENT "Updating ${module}..." )
  add_dependencies( ${module} ${module}_install )
  

  # Include path always include code/
  set( ${module}_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/ )
  set( ${module}_DEFINITIONS "" )

  # Which libraries to link against (internally used, not exported, by the module)
  set( ${module}_LINK_TO "" )

  foreach( _dep ${_dependencies} )

    if( NOT DEFINED ${_dep}_INCLUDE_DIRS AND NOT DEFINED ${_dep}_LIBRARIES )
      if( NOT _dep STREQUAL DL )  # the dl libraries may be empty on Mac
        message( FATAL_ERROR "${_dep} is listed as dependency for ${module}, but both ${_dep}_INCLUDE_DIRS and ${_dep}_LIBRARIES are undefined!" )
      endif()
    endif()
    
    # INCLUDE_DIRS 
    # Add each of the dependent's include dirs
    # to this module's include dirs
    # (if not already contained in _INCLUDE_DIRS)
    foreach( _dir ${${_dep}_INCLUDE_DIRS} )

      casa_append( ${module}_INCLUDE_DIRS ${_dir} )

    endforeach()

    # DEFINITIONS 
    foreach( _def ${${_dep}_DEFINITIONS} )

      casa_append( ${module}_DEFINITIONS ${_def} )

    endforeach()

    # LINK_TO 
    # Unlike include dirs, libraries do not have
    # to be transitively propagated.
    # E.g. if A links to (depends on) B and B links to C
    # we just need to explicitly link A to B and
    # the linker will make sure that C is eventually
    # linked to A

    if( DEFINED ${_dep}_LIBRARIES OR _dep STREQUAL DL )

      # External library

      set( ${module}_LINK_TO 
        ${${module}_LINK_TO}
        ${${_dep}_LIBRARIES} )

    else()
      # Another CASA module

      set(
        ${module}_LINK_TO 
        ${${module}_LINK_TO} 
        lib${_dep} )

      add_dependencies( ${module} ${_dep}_install )

    endif()

    # Generated code need build dir in include path
    if( ${_dep} STREQUAL "DBUS" OR
        ${_dep} STREQUAL "QT4" )
      
      set( ${module}_INCLUDE_DIRS 
        ${${module}_INCLUDE_DIRS}
        ${CMAKE_BINARY_DIR} )

    endif()

  endforeach()

  set( all_modules ${all_modules} ${module} )

  add_subdirectory( ${module} )

endmacro()


