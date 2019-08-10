/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield
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

#ifndef OSGSHADOW_EXPORT_
#define OSGSHADOW_EXPORT_ 1

#include<osg/Config>

#if defined(_MSC_VER) && defined(OSG_DISABLE_MSVC_WARNINGS)
    #pragma warning( disable : 4244 )
    #pragma warning( disable : 4251 )
    #pragma warning( disable : 4267 )
    #pragma warning( disable : 4275 )
    #pragma warning( disable : 4290 )
    #pragma warning( disable : 4786 )
    #pragma warning( disable : 4305 )
    #pragma warning( disable : 4996 )
#endif

#if defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__) || defined( __BCPLUSPLUS__)
    #  if defined( OSG_LIBRARY_STATIC )
    #    define OSGSHADOW_EXPORT
    #  elif defined( OSGSHADOW_LIBRARY )
    #    define OSGSHADOW_EXPORT   __declspec(dllexport)
    #  else
    #    define OSGSHADOW_EXPORT   __declspec(dllimport)
    #  endif
#else
    #  define OSGSHADOW_EXPORT
#endif

/**

\namespace osgShadow

The osgShadow library is a NodeKit that extends the core scene graph to add support for a range of shadow techniques.
*/

#endif
