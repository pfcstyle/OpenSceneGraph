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

#ifndef OSGSHADOW_SHADOWEMAP
#define OSGSHADOW_SHADOWEMAP 1

#include <osg/Camera>
#include <osg/Material>
#include <osg/MatrixTransform>
#include <osg/LightSource>

#include <osgShadow/ShadowTechnique>

namespace osgShadow {

/** ShadowedTexture provides an implementation of shadow textures.*/
class OSGSHADOW_EXPORT ShadowMap : public ShadowTechnique
{
    public :
        ShadowMap();

        ShadowMap(const ShadowMap& es, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);

        META_Object(osgShadow, ShadowMap);

        /** Set the texture unit that the shadow texture will be applied on.*/
        void setTextureUnit(unsigned int unit);

        /** Get the texture unit that the shadow texture will be applied on.*/
        unsigned int getTextureUnit() const { return _shadowTextureUnit; }

        /** set the polygon offset used initially */
        void setPolygonOffset(const osg::Vec2& polyOffset);

        /** get the used polygon offset */
        const osg::Vec2& getPolygonOffset() const { return _polyOffset; }

        /** Set the values for the ambient bias the shader will use.*/
        void setAmbientBias(const osg::Vec2& ambientBias );

        /** Get the values that are used for the ambient bias in the shader.*/
        const osg::Vec2& getAmbientBias() const { return _ambientBias; }

        /** set the size in pixels x / y for the shadow texture.*/
        void setTextureSize(const osg::Vec2s& textureSize);

        /** Get the values that are used for the ambient bias in the shader.*/
        const osg::Vec2s& getTextureSize() const { return _textureSize; }

        /** Set the Light that will cast shadows */
        void setLight(osg::Light* light);
        void setLight(osg::LightSource* ls);

        typedef std::vector< osg::ref_ptr<osg::Uniform> > UniformList;

        typedef std::vector< osg::ref_ptr<osg::Shader> > ShaderList;

        /** Add a shader to internal list, will be used instead of the default ones */
        inline void addShader(osg::Shader* shader) { _shaderList.push_back(shader); }

        template<class T> void addShader( const osg::ref_ptr<T>& shader ) { addShader(shader.get()); }

        /** Reset internal shader list */
        inline void clearShaderList() { _shaderList.clear(); }

        /** initialize the ShadowedScene and local cached data structures.*/
        virtual void init();

        /** run the update traversal of the ShadowedScene and update any loca chached data structures.*/
        virtual void update(osg::NodeVisitor& nv);

        /** run the cull traversal of the ShadowedScene and set up the rendering for this ShadowTechnique.*/
        virtual void cull(osgUtil::CullVisitor& cv);

        /** Clean scene graph from any shadow technique specific nodes, state and drawables.*/
        virtual void cleanSceneGraph();


        /** Resize any per context GLObject buffers to specified size. */
        virtual void resizeGLObjectBuffers(unsigned int maxSize);

        /** If State is non-zero, this function releases any associated OpenGL objects for
           * the specified graphics context. Otherwise, releases OpenGL objects
           * for all graphics contexts. */
        virtual void releaseGLObjects(osg::State* = 0) const;


        // debug methods

        osg::ref_ptr<osg::Camera> makeDebugHUD();

    protected:
        virtual ~ShadowMap(void) {};

        /** Create the managed Uniforms */
        virtual void createUniforms();

        virtual void createShaders();

        // forward declare, interface and implementation provided in ShadowMap.cpp
        class DrawableDrawWithDepthShadowComparisonOffCallback;

        osg::ref_ptr<osg::Camera>       _camera;
        osg::ref_ptr<osg::TexGen>       _texgen;
        osg::ref_ptr<osg::Texture2D>    _texture;
        osg::ref_ptr<osg::StateSet>     _stateset;
        osg::ref_ptr<osg::Program>      _program;
        osg::ref_ptr<osg::Light>        _light;

        osg::ref_ptr<osg::LightSource>  _ls;

        osg::ref_ptr<osg::Uniform>      _ambientBiasUniform;
        UniformList                     _uniformList;
        ShaderList                      _shaderList;
        unsigned int                    _baseTextureUnit;
        unsigned int                    _shadowTextureUnit;
        osg::Vec2                        _polyOffset;
        osg::Vec2                       _ambientBias;
        osg::Vec2s                      _textureSize;

    };

}

#endif
