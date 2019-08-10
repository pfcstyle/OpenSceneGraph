/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2009 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

#ifndef OSGVOLUME_VERSION
#define OSGVOLUME_VERSION 1

#include <osgVolume/Export>

extern "C" {

/**
 * osgVolumeGetVersion() returns the library version number.
 * Numbering convention : OpenSceneGraph-1.0 will return 1.0 from osgVolumeGetVersion.
 *
 * This C function can be also used to check for the existence of the OpenSceneGraph
 * library using autoconf and its m4 macro AC_CHECK_LIB.
 *
 * Here is the code to add to your configure.in:
 \verbatim
 #
 # Check for the OpenSceneGraph (OSG) Volume library
 #
 AC_CHECK_LIB(osg, osgVolumeGetVersion, ,
    [AC_MSG_ERROR(OpenSceneGraph Volume library not found. See http://www.openscenegraph.org)],)
 \endverbatim
*/
extern OSGVOLUME_EXPORT const char* osgVolumeGetVersion();

/**
 * osgVolumeGetLibraryName() returns the library name in human friendly form.
*/
extern OSGVOLUME_EXPORT const char* osgVolumeGetLibraryName();

}

#endif
