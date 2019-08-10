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

#ifndef OSG_FOG
#define OSG_FOG 1

#include <osg/StateAttribute>
#include <osg/Vec4>

#ifndef GL_FOG_DISTANCE_MODE_NV
    #define GL_FOG_DISTANCE_MODE_NV 0x855A
#endif
#ifndef GL_EYE_PLANE_ABSOLUTE_NV
    #define GL_EYE_PLANE_ABSOLUTE_NV 0x855C
#endif
#ifndef GL_EYE_RADIAL_NV
    #define GL_EYE_RADIAL_NV 0x855B
#endif


#ifndef GL_FOG_COORDINATE
    #define GL_FOG_COORDINATE   0x8451
#endif
#ifndef GL_FRAGMENT_DEPTH
    #define GL_FRAGMENT_DEPTH   0x8452
#endif

#ifndef GL_FOG
    #define GL_FOG 0x0B60
    #define GL_EXP      0x0800
    #define GL_EXP2     0x0801
#endif

#ifndef GL_FOG_HINT
    #define GL_FOG_HINT 0x0C54
#endif

namespace osg {


/** Fog - encapsulates OpenGL fog state. */
class OSG_EXPORT Fog : public StateAttribute
{
    public :

        Fog();

        /** Copy constructor using CopyOp to manage deep vs shallow copy.*/
        Fog(const Fog& fog,const CopyOp& copyop=CopyOp::SHALLOW_COPY):
            StateAttribute(fog,copyop),
            _mode(fog._mode),
            _density(fog._density),
            _start(fog._start),
            _end(fog._end),
            _color(fog._color),
            _fogCoordinateSource(fog._fogCoordinateSource),
            _useRadialFog(fog._useRadialFog)    {}

        META_StateAttribute(osg, Fog,FOG);

        /** return -1 if *this < *rhs, 0 if *this==*rhs, 1 if *this>*rhs.*/
        virtual int compare(const StateAttribute& sa) const
        {
            // check the types are equal and then create the rhs variable
            // used by the COMPARE_StateAttribute_Parameter macros below.
            COMPARE_StateAttribute_Types(Fog,sa)

            // compare each parameter in turn against the rhs.
            COMPARE_StateAttribute_Parameter(_mode)
            COMPARE_StateAttribute_Parameter(_density)
            COMPARE_StateAttribute_Parameter(_start)
            COMPARE_StateAttribute_Parameter(_end)
            COMPARE_StateAttribute_Parameter(_color)
            COMPARE_StateAttribute_Parameter(_fogCoordinateSource)
            COMPARE_StateAttribute_Parameter(_useRadialFog)

            return 0; // passed all the above comparison macros, must be equal.
        }

        virtual bool getModeUsage(StateAttribute::ModeUsage& usage) const
        {
            usage.usesMode(GL_FOG);
            return true;
        }

        enum Mode {
            LINEAR = GL_LINEAR,
            EXP    = GL_EXP,
            EXP2   = GL_EXP2
        };

        inline void  setMode( Mode mode )         { _mode = mode; }
        inline Mode getMode() const                     { return _mode; }

        inline void  setDensity( float density )  { _density = density; }
        inline float getDensity() const           { return _density; }

        inline void  setStart( float start )      { _start = start; }
        inline float getStart() const             { return _start; }

        inline void  setEnd( float end )          { _end = end; }
        inline float getEnd() const               { return _end; }

        inline void  setColor( const Vec4 &color ) { _color = color; }
        inline const Vec4& getColor() const        { return _color; }

        inline void setUseRadialFog( bool useRadialFog ) { _useRadialFog = useRadialFog; }
        inline bool getUseRadialFog() const              { return _useRadialFog; }

        enum FogCoordinateSource
        {
            FOG_COORDINATE = GL_FOG_COORDINATE,
            FRAGMENT_DEPTH = GL_FRAGMENT_DEPTH
        };

        inline void setFogCoordinateSource(GLint source) { _fogCoordinateSource = source; }
        inline GLint getFogCoordinateSource() const { return _fogCoordinateSource; }

        virtual void apply(State& state) const;

    protected :

        virtual ~Fog();

        Mode    _mode;
        float   _density;
        float   _start;
        float   _end;
        Vec4    _color;
        GLint   _fogCoordinateSource;
        bool    _useRadialFog;
};

}

#endif
