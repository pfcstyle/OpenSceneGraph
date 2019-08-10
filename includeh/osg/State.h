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

#ifndef OSG_STATE
#define OSG_STATE 1

#include <osg/Export>
#include <osg/GLExtensions>
#include <osg/StateSet>
#include <osg/Matrix>
#include <osg/Uniform>
#include <osg/BufferObject>
#include <osg/Observer>
#include <osg/Timer>
#include <osg/VertexArrayState>

#include <osg/ShaderComposer>
#include <osg/FrameStamp>
#include <osg/DisplaySettings>
#include <osg/Polytope>
#include <osg/Viewport>
#include <osg/AttributeDispatchers>
#include <osg/GraphicsCostEstimator>

#include <iosfwd>
#include <vector>
#include <map>
#include <set>
#include <string>

#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif

namespace osg {

/** macro for use with osg::StateAttribute::apply methods for detecting and
  * reporting OpenGL error messages.*/
#define OSG_GL_DEBUG(message) \
    if (state.getFineGrainedErrorDetection()) \
    { \
        GLenum errorNo = glGetError(); \
        if (errorNo!=GL_NO_ERROR) \
        { \
            osg::notify(WARN)<<"Warning: detected OpenGL error '"<<gluErrorString(errorNo)<<" "<<message<<endl; \
        }\
    }


// forward declare GraphicsContext, View and State
class GraphicsContext;

/** Encapsulates the current applied OpenGL modes, attributes and vertex arrays settings,
  * implements lazy state updating and provides accessors for querying the current state.
  * The venerable Red Book says that "OpenGL is a state machine", and this class
  * represents the OpenGL state in OSG. Furthermore, \c State also has other
  * important features:
  * - It works as a stack of states (see \c pushStateSet() and
  *   \c popStateSet()). Manipulating this stack of OpenGL states manually is
  *   seldom needed, since OSG does this in the most common situations.
  * - It implements lazy state updating. This means that, if one requests a
  *   state change and that particular state is already in the requested state,
  *   no OpenGL call will be made. This ensures that the OpenGL pipeline is not
  *   stalled by unnecessary state changes.
  * - It allows to query the current OpenGL state without calls to \c glGet*(),
  *   which typically stall the graphics pipeline (see, for instance,
  *   \c captureCurrentState() and \c getModelViewMatrix()).
  */
class OSG_EXPORT State : public Referenced
{
    public :

        State();


        /** Set the graphics context associated with that owns this State object.*/
        void setGraphicsContext(GraphicsContext* context) { _graphicsContext = context; }

        /** Get the graphics context associated with that owns this State object.*/
        GraphicsContext* getGraphicsContext() { return _graphicsContext; }

        /** Get the const graphics context associated with that owns this State object.*/
        const GraphicsContext* getGraphicsContext() const { return _graphicsContext; }


        /** Set the current OpenGL context uniqueID.
         *  The ContextID is used by classes like osg::StateAttribute's and osg::Drawable's to
         *  help manage separate OpenGL objects, such as display lists, vertex buffer objects
         *  and texture object for each graphics context. The ContextID simply acts as an index
         *  into arrays that these classes maintain for the purpose of storing GL object handles.
         *
         *  Note, osgViewer::GraphicsWindow will automatically set up the ContextID for you,
         *  so you will rearely need to set this yourself.
         *
         *  The exception is when creating your own graphics context, where you should set
         *  the ContextID uniquely for each graphics context.
         *
         *  Typical settings for ContextID are 0,1,2,3... up to the maximum
         *  number of graphics contexts you have set up. By default contextID is 0.
         */
        inline void setContextID(unsigned int contextID) { _contextID=contextID; }

        /** Get the current OpenGL context unique ID.*/
        inline unsigned int getContextID() const { return _contextID; }


        // ExtensionMap contains GL Extentsions objects used by StateAttribue to call OpenGL extensions/advanced features
        typedef std::map<const std::type_info*, osg::ref_ptr<osg::Referenced> > ExtensionMap;
        ExtensionMap _extensionMap;

        /** Get a specific GL extensions object or GraphicsObjectManager, initialize if not already present.
          * Note, must only be called from a the graphics context thread associated with this osg::State. */
        template<typename T>
        T* get()
        {
            const std::type_info* id(&typeid(T));
            osg::ref_ptr<osg::Referenced>& ptr = _extensionMap[id];
            if (!ptr)
            {
                ptr = new T(_contextID);
            }
            return static_cast<T*>(ptr.get());
        }

        /** Get a specific GL extensions object or GraphicsObjectManager if it already exists in the extension map.
          * Note, safe to call outwith a the graphics context thread associated with this osg::State.
          * Returns NULL if the desired extension object has not been created yet.*/
        template<typename T>
        const T* get() const
        {
            const std::type_info* id(&typeid(T));
            ExtensionMap::const_iterator itr = _extensionMap.find(id);
            if (itr==_extensionMap.end()) return 0;
            else return itr->second.get();
        }

        /** Set a specific GL extensions object pr GraphicsObjectManager. */
        template<typename T>
        void set(T* ptr)
        {
            const std::type_info* id(&typeid(T));
            _extensionMap[id] = ptr;
        }

        /* deprecated.*/
        void setShaderCompositionEnabled(bool flag) { _shaderCompositionEnabled = flag; }

        /* deprecated.*/
        bool getShaderCompositionEnabled() const { return _shaderCompositionEnabled; }

        /** deprecated.*/
        void setShaderComposer(ShaderComposer* sc) { _shaderComposer = sc; }

        /** deprecated.*/
        ShaderComposer* getShaderComposer() { return _shaderComposer.get(); }

        /** deprecated.*/
        const ShaderComposer* getShaderComposer() const { return _shaderComposer.get(); }

        /** Get the unform list in which to inject any uniforms that StateAttribute::apply(State&) methods provide.*/
        StateSet::UniformList& getCurrentShaderCompositionUniformList() { return _currentShaderCompositionUniformList; }

        /** Convenience method for StateAttribute::apply(State&) methods to pass on their uniforms to osg::State so it can apply them at the appropriate point.*/
        void applyShaderCompositionUniform(const osg::Uniform* uniform, StateAttribute::OverrideValue value=StateAttribute::ON)
        {
            StateSet::RefUniformPair& up = _currentShaderCompositionUniformList[uniform->getName()];
            up.first = const_cast<Uniform*>(uniform);
            up.second = value;
        }


        /** Push stateset onto state stack.*/
        void pushStateSet(const StateSet* dstate);

        /** Pop stateset off state stack.*/
        void popStateSet();

        /** pop all statesets off state stack, ensuring it is empty ready for the next frame.
          * Note, to return OpenGL to default state, one should do any state.popAllStatSets(); state.apply().*/
        void popAllStateSets();

        /** Insert stateset onto state stack.*/
        void insertStateSet(unsigned int pos,const StateSet* dstate);

        /** Pop stateset off state stack.*/
        void removeStateSet(unsigned int pos);

        /** Get the number of StateSet's on the StateSet stack.*/
        unsigned int getStateSetStackSize() { return static_cast<unsigned int>(_stateStateStack.size()); }

        /** Pop StateSet's for the StateSet stack till its size equals the specified size.*/
        void popStateSetStackToSize(unsigned int size) { while (_stateStateStack.size()>size) popStateSet(); }

        typedef std::vector<const StateSet*> StateSetStack;

        /** Get the StateSet stack.*/
        StateSetStack& getStateSetStack() { return _stateStateStack; }


        /** Copy the modes and attributes which capture the current state.*/
        void captureCurrentState(StateSet& stateset) const;

        /** Release all OpenGL objects associated cached by this osg::State object.*/
        void releaseGLObjects();

        /** reset the state object to an empty stack.*/
        void reset();

        inline const Viewport* getCurrentViewport() const
        {
            return static_cast<const Viewport*>(getLastAppliedAttribute(osg::StateAttribute::VIEWPORT));
        }


        void setInitialViewMatrix(const osg::RefMatrix* matrix);

        inline const osg::Matrix& getInitialViewMatrix() const { return *_initialViewMatrix; }
        inline const osg::Matrix& getInitialInverseViewMatrix() const { return _initialInverseViewMatrix; }

        void applyProjectionMatrix(const osg::RefMatrix* matrix);

        inline const osg::Matrix& getProjectionMatrix() const { return *_projection; }

        void applyModelViewMatrix(const osg::RefMatrix* matrix);
        void applyModelViewMatrix(const osg::Matrix&);

        const osg::Matrix& getModelViewMatrix() const { return *_modelView; }

        void setUseModelViewAndProjectionUniforms(bool flag) { _useModelViewAndProjectionUniforms = flag; }
        bool getUseModelViewAndProjectionUniforms() const { return _useModelViewAndProjectionUniforms; }

        void updateModelViewAndProjectionMatrixUniforms();

        void applyModelViewAndProjectionUniformsIfRequired();

        osg::Uniform* getModelViewMatrixUniform() { return _modelViewMatrixUniform.get(); }
        osg::Uniform* getProjectionMatrixUniform() { return _projectionMatrixUniform.get(); }
        osg::Uniform* getModelViewProjectionMatrixUniform() { return _modelViewProjectionMatrixUniform.get(); }
        osg::Uniform* getNormalMatrixUniform() { return _normalMatrixUniform.get(); }


        Polytope getViewFrustum() const;


        void setUseVertexAttributeAliasing(bool flag);
        bool getUseVertexAttributeAliasing() const { return _useVertexAttributeAliasing ; }

        typedef std::vector<VertexAttribAlias> VertexAttribAliasList;

        /** Reset the vertex attribute aliasing to osg's default. This method needs to be called before render anything unless you really know what you're doing !*/
        void resetVertexAttributeAlias(bool compactAliasing=true, unsigned int numTextureUnits=8);

        /** Set the vertex attribute aliasing for "vertex". This method needs to be called before render anything unless you really know what you're doing !*/
        void setVertexAlias(const VertexAttribAlias& alias) { _vertexAlias = alias; }
        const VertexAttribAlias& getVertexAlias() { return _vertexAlias; }

        /** Set the vertex attribute aliasing for "normal". This method needs to be called before render anything unless you really know what you're doing !*/
        void setNormalAlias(const VertexAttribAlias& alias) { _normalAlias = alias; }
        const VertexAttribAlias& getNormalAlias() { return _normalAlias; }

        /** Set the vertex attribute aliasing for "color". This method needs to be called before render anything unless you really know what you're doing !*/
        void setColorAlias(const VertexAttribAlias& alias) { _colorAlias = alias; }
        const VertexAttribAlias& getColorAlias() { return _colorAlias; }

        /** Set the vertex attribute aliasing for "secondary color". This method needs to be called before render anything unless you really know what you're doing !*/
        void setSecondaryColorAlias(const VertexAttribAlias& alias) { _secondaryColorAlias = alias; }
        const VertexAttribAlias& getSecondaryColorAlias() { return _secondaryColorAlias; }

        /** Set the vertex attribute aliasing for "fog coord". This method needs to be called before render anything unless you really know what you're doing !*/
        void setFogCoordAlias(const VertexAttribAlias& alias) { _fogCoordAlias = alias; }
        const VertexAttribAlias& getFogCoordAlias() { return _fogCoordAlias; }

        /** Set the vertex attribute aliasing list for texture coordinates. This method needs to be called before render anything unless you really know what you're doing !*/
        void setTexCoordAliasList(const VertexAttribAliasList& aliasList) { _texCoordAliasList = aliasList; }
        const VertexAttribAliasList& getTexCoordAliasList() { return _texCoordAliasList; }

        /** Set the vertex attribute binding list. This method needs to be called before render anything unless you really know what you're doing !*/
        void  setAttributeBindingList(const Program::AttribBindingList& attribBindingList) { _attributeBindingList = attribBindingList; }
        const Program::AttribBindingList&  getAttributeBindingList() { return _attributeBindingList; }

        bool convertVertexShaderSourceToOsgBuiltIns(std::string& source) const;


        /** Apply stateset.*/
        void apply(const StateSet* dstate);

        /** Updates the OpenGL state so that it matches the \c StateSet at the
          * top of the stack of <tt>StateSet</tt>s maintained internally by a
          * \c State.
         */
        void apply();

        /** Apply any shader composed state.*/
        void applyShaderComposition();


        void glDrawBuffer(GLenum buffer);
        GLenum getDrawBuffer() const { return _drawBuffer; }

        void glReadBuffer(GLenum buffer);
        GLenum getReadBuffer() const { return _readBuffer; }


        /** Set whether a particular OpenGL mode is valid in the current graphics context.
          * Use to disable OpenGL modes that are not supported by current graphics drivers/context.*/
        inline void setModeValidity(StateAttribute::GLMode mode,bool valid)
        {
            ModeStack& ms = _modeMap[mode];
            ms.valid = valid;
        }

        /** Get whether a particular OpenGL mode is valid in the current graphics context.
          * Use to disable OpenGL modes that are not supported by current graphics drivers/context.*/
        inline bool getModeValidity(StateAttribute::GLMode mode)
        {
            ModeStack& ms = _modeMap[mode];
            return ms.valid;
        }

        inline void setGlobalDefaultModeValue(StateAttribute::GLMode mode,bool enabled)
        {
            ModeStack& ms = _modeMap[mode];
            ms.global_default_value = enabled;
        }

        inline bool getGlobalDefaultModeValue(StateAttribute::GLMode mode)
        {
            return _modeMap[mode].global_default_value;
        }

        inline bool getLastAppliedModeValue(StateAttribute::GLMode mode)
        {
            return _modeMap[mode].last_applied_value;
        }

        /** Proxy helper class for applyig a model in a local scope, with the preivous value being resotred automatically on leaving the scope that proxy was created.*/
        struct ApplyModeProxy
        {
            inline ApplyModeProxy(osg::State& state, GLenum mode, bool value):_state(state), _mode(mode)
            {
                _previous_value = _state.getLastAppliedModeValue(mode);
                _need_to_apply_value = (_previous_value!=value);
                if (_need_to_apply_value) _state.applyMode(_mode, value);
            }
            inline ~ApplyModeProxy()
            {
                if (_need_to_apply_value) _state.applyMode(_mode, _previous_value);
            }

            osg::State& _state;
            GLenum      _mode;
            bool        _previous_value;
            bool        _need_to_apply_value;
        };

        struct ApplyTextureModeProxy
        {
            inline ApplyTextureModeProxy(osg::State& state, unsigned int unit, GLenum mode, bool value):_state(state), _unit(unit), _mode(mode)
            {
                _previous_value = _state.getLastAppliedTextureModeValue(_unit, _mode);
                _need_to_apply_value = (_previous_value!=value);
                if (_need_to_apply_value) _state.applyTextureMode(_unit, _mode, value);
            }
            inline ~ApplyTextureModeProxy()
            {
                if (_need_to_apply_value) _state.applyTextureMode(_unit, _mode, _previous_value);
            }

            osg::State&     _state;
            unsigned int    _unit;
            GLenum          _mode;
            bool            _previous_value;
            bool            _need_to_apply_value;
        };

        /** Apply an OpenGL mode if required. This is a wrapper around
          * \c glEnable() and \c glDisable(), that just actually calls these
          * functions if the \c enabled flag is different than the current
          * state.
          * @return \c true if the state was actually changed. \c false
          *         otherwise. Notice that a \c false return does not indicate
          *         an error, it just means that the mode was already set to the
          *         same value as the \c enabled parameter.
        */
        inline bool applyMode(StateAttribute::GLMode mode,bool enabled)
        {
            ModeStack& ms = _modeMap[mode];
            ms.changed = true;
            return applyMode(mode,enabled,ms);
        }

        inline void setGlobalDefaultTextureModeValue(unsigned int unit, StateAttribute::GLMode mode,bool enabled)
        {
            ModeMap& modeMap = getOrCreateTextureModeMap(unit);
            ModeStack& ms = modeMap[mode];
            ms.global_default_value = enabled;
        }

        inline bool getGlobalDefaultTextureModeValue(unsigned int unit, StateAttribute::GLMode mode)
        {
            ModeMap& modeMap = getOrCreateTextureModeMap(unit);
            ModeStack& ms = modeMap[mode];
            return ms.global_default_value;
        }

        inline bool applyTextureMode(unsigned int unit, StateAttribute::GLMode mode,bool enabled)
        {
            ModeMap& modeMap = getOrCreateTextureModeMap(unit);
            ModeStack& ms = modeMap[mode];
            ms.changed = true;
            return applyModeOnTexUnit(unit,mode,enabled,ms);
        }

        inline bool getLastAppliedTextureModeValue(unsigned int unit, StateAttribute::GLMode mode)
        {
            ModeMap& modeMap = getOrCreateTextureModeMap(unit);
            ModeStack& ms = modeMap[mode];
            return ms.last_applied_value;
        }

        inline void setGlobalDefaultAttribute(const StateAttribute* attribute)
        {
            AttributeStack& as = _attributeMap[attribute->getTypeMemberPair()];
            as.global_default_attribute = attribute;
        }

        inline const StateAttribute* getGlobalDefaultAttribute(StateAttribute::Type type, unsigned int member=0)
        {
            AttributeStack& as = _attributeMap[StateAttribute::TypeMemberPair(type,member)];
            return as.global_default_attribute.get();
        }

        /** Apply an attribute if required. */
        inline bool applyAttribute(const StateAttribute* attribute)
        {
            AttributeStack& as = _attributeMap[attribute->getTypeMemberPair()];
            as.changed = true;
            return applyAttribute(attribute,as);
        }

        inline void setGlobalDefaultTextureAttribute(unsigned int unit, const StateAttribute* attribute)
        {
            AttributeMap& attributeMap = getOrCreateTextureAttributeMap(unit);
            AttributeStack& as = attributeMap[attribute->getTypeMemberPair()];
            as.global_default_attribute = attribute;
        }

        inline const StateAttribute* getGlobalDefaultTextureAttribute(unsigned int unit, StateAttribute::Type type, unsigned int member = 0)
        {
            AttributeMap& attributeMap = getOrCreateTextureAttributeMap(unit);
            AttributeStack& as = attributeMap[StateAttribute::TypeMemberPair(type,member)];
            return as.global_default_attribute.get();
        }


        inline bool applyTextureAttribute(unsigned int unit, const StateAttribute* attribute)
        {
            AttributeMap& attributeMap = getOrCreateTextureAttributeMap(unit);
            AttributeStack& as = attributeMap[attribute->getTypeMemberPair()];
            as.changed = true;
            return applyAttributeOnTexUnit(unit,attribute,as);
        }

        /** Mode has been set externally, update state to reflect this setting.*/
        void haveAppliedMode(StateAttribute::GLMode mode,StateAttribute::GLModeValue value);

        /** Mode has been set externally, therefore dirty the associated mode in osg::State
          * so it is applied on next call to osg::State::apply(..)*/
        void haveAppliedMode(StateAttribute::GLMode mode);

        /** Attribute has been applied externally, update state to reflect this setting.*/
        void haveAppliedAttribute(const StateAttribute* attribute);

        /** Attribute has been applied externally,
          * and therefore this attribute type has been dirtied
          * and will need to be re-applied on next osg::State.apply(..).
          * note, if you have an osg::StateAttribute which you have applied externally
          * then use the have_applied(attribute) method as this will cause the osg::State to
          * track the current state more accurately and enable lazy state updating such
          * that only changed state will be applied.*/
        void haveAppliedAttribute(StateAttribute::Type type, unsigned int member=0);

        /** Get whether the current specified mode is enabled (true) or disabled (false).*/
        bool getLastAppliedMode(StateAttribute::GLMode mode) const;

        /** Get the current specified attribute, return NULL if one has not yet been applied.*/
        const StateAttribute* getLastAppliedAttribute(StateAttribute::Type type, unsigned int member=0) const;

        /** texture Mode has been set externally, update state to reflect this setting.*/
        void haveAppliedTextureMode(unsigned int unit, StateAttribute::GLMode mode,StateAttribute::GLModeValue value);

        /** texture Mode has been set externally, therefore dirty the associated mode in osg::State
          * so it is applied on next call to osg::State::apply(..)*/
        void haveAppliedTextureMode(unsigned int unit, StateAttribute::GLMode mode);

        /** texture Attribute has been applied externally, update state to reflect this setting.*/
        void haveAppliedTextureAttribute(unsigned int unit, const StateAttribute* attribute);

        /** texture Attribute has been applied externally,
          * and therefore this attribute type has been dirtied
          * and will need to be re-applied on next osg::State.apply(..).
          * note, if you have an osg::StateAttribute which you have applied externally
          * then use the have_applied(attribute) method as this will the osg::State to
          * track the current state more accurately and enable lazy state updating such
          * that only changed state will be applied.*/
        void haveAppliedTextureAttribute(unsigned int unit, StateAttribute::Type type, unsigned int member=0);

        /** Get whether the current specified texture mode is enabled (true) or disabled (false).*/
        bool getLastAppliedTextureMode(unsigned int unit, StateAttribute::GLMode mode) const;

        /** Get the current specified texture attribute, return NULL if one has not yet been applied.*/
        const StateAttribute* getLastAppliedTextureAttribute(unsigned int unit, StateAttribute::Type type, unsigned int member=0) const;





        /** Dirty the modes previously applied in osg::State.*/
        void dirtyAllModes();

        /** Dirty the modes attributes previously applied in osg::State.*/
        void dirtyAllAttributes();


        /** Proxy helper class for applyig a VertexArrayState in a local scope, with the preivous value being resotred automatically on leaving the scope that proxy was created.*/
        struct SetCurrentVertexArrayStateProxy
        {
            SetCurrentVertexArrayStateProxy(osg::State& state, VertexArrayState* vas):_state(state) { _state.setCurrentVertexArrayState(vas); }
            ~SetCurrentVertexArrayStateProxy() { _state.setCurrentToGlobalVertexArrayState(); }
            osg::State& _state;
        };

        /** Set the CurrentVetexArrayState object that take which vertex arrays are bound.*/
        void setCurrentVertexArrayState(VertexArrayState* vas) { _vas = vas; }

        /** Get the CurrentVetexArrayState object that take which vertex arrays are bound.*/
        VertexArrayState* getCurrentVertexArrayState() const { return _vas; }

        /** Set the getCurrentVertexArrayState to the GlobalVertexArrayState.*/
        void setCurrentToGlobalVertexArrayState() { _vas = _globalVertexArrayState.get(); }


        /** disable the vertex, normal, color, tex coords, secondary color, fog coord and index arrays.*/
        void disableAllVertexArrays();


        void lazyDisablingOfVertexAttributes() { _vas->lazyDisablingOfVertexAttributes(); }
        void applyDisablingOfVertexAttributes() { _vas->applyDisablingOfVertexAttributes(*this); }

        void setCurrentVertexBufferObject(osg::GLBufferObject* vbo) { _vas->setCurrentVertexBufferObject(vbo); }
        const GLBufferObject* getCurrentVertexBufferObject() { return _vas->getCurrentVertexBufferObject(); }

        void bindVertexBufferObject(osg::GLBufferObject* vbo) { _vas->bindVertexBufferObject(vbo); }
        void unbindVertexBufferObject() { _vas->unbindVertexBufferObject(); }

        void setCurrentElementBufferObject(osg::GLBufferObject* ebo) { _vas->setCurrentElementBufferObject(ebo); }
        const GLBufferObject* getCurrentElementBufferObject() { return _vas->getCurrentElementBufferObject(); }

        void bindElementBufferObject(osg::GLBufferObject* ebo) { _vas->bindElementBufferObject(ebo); }
        void unbindElementBufferObject() { _vas->unbindElementBufferObject(); }


        void setCurrentPixelBufferObject(osg::GLBufferObject* pbo) { _currentPBO = pbo; }
        const GLBufferObject* getCurrentPixelBufferObject() const { return _currentPBO; }

        inline void bindPixelBufferObject(osg::GLBufferObject* pbo)
        {
            if (pbo)
            {
                if (pbo == _currentPBO) return;

                if (pbo->isDirty()) pbo->compileBuffer();
                else pbo->bindBuffer();

                _currentPBO = pbo;
            }
            else
            {
                unbindPixelBufferObject();
            }
        }

        inline void unbindPixelBufferObject()
        {
            if (!_currentPBO) return;

            _glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB,0);
            _currentPBO = 0;
        }


        inline void bindDrawIndirectBufferObject(osg::GLBufferObject* ibo)
        {
            if (ibo->isDirty())
            {
                ibo->compileBuffer();
                _currentDIBO = ibo;
            }
            else if (ibo != _currentDIBO)
            {
                ibo->bindBuffer();
                _currentDIBO = ibo;
            }
        }

        inline void unbindDrawIndirectBufferObject()
        {
            if (!_currentDIBO) return;
            _glBindBuffer(GL_DRAW_INDIRECT_BUFFER,0);
            _currentDIBO = 0;
        }

        void setCurrentVertexArrayObject(GLuint vao) { _currentVAO = vao; }
        GLuint getCurrentVertexArrayObject() const { return _currentVAO; }

        inline void bindVertexArrayObject(const VertexArrayState* vas) { bindVertexArrayObject(vas->getVertexArrayObject()); }

        inline void bindVertexArrayObject(GLuint vao) { if (_currentVAO!=vao) { _glExtensions->glBindVertexArray(vao); _currentVAO = vao; } }

        inline void unbindVertexArrayObject() { if (_currentVAO!=0) { _glExtensions->glBindVertexArray(0); _currentVAO = 0; } }


        typedef std::vector<GLushort> IndicesGLushort;
        IndicesGLushort _quadIndicesGLushort[4];

        typedef std::vector<GLuint> IndicesGLuint;
        IndicesGLuint _quadIndicesGLuint[4];

        void drawQuads(GLint first, GLsizei count, GLsizei primCount=0);

        inline void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount)
        {
            if (primcount>=1 && _glDrawArraysInstanced!=0) _glDrawArraysInstanced(mode, first, count, primcount);
            else glDrawArrays(mode, first, count);
        }

        inline void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount )
        {
            if (primcount>=1 && _glDrawElementsInstanced!=0) _glDrawElementsInstanced(mode, count, type, indices, primcount);
            else glDrawElements(mode, count, type, indices);
        }


        inline void Vertex(float x, float y, float z, float w=1.0f)
        {
        #if defined(OSG_GL_VERTEX_FUNCS_AVAILABLE) && !defined(OSG_GLES1_AVAILABLE)
            if (_useVertexAttributeAliasing) _glVertexAttrib4f( _vertexAlias._location, x,y,z,w);
            else glVertex4f(x,y,z,w);
        #else
            _glVertexAttrib4f( _vertexAlias._location, x,y,z,w);
        #endif
        }

        inline void Color(float r, float g, float b, float a=1.0f)
        {
        #ifdef OSG_GL_VERTEX_FUNCS_AVAILABLE
            if (_useVertexAttributeAliasing) _glVertexAttrib4f( _colorAlias._location, r,g,b,a);
            else glColor4f(r,g,b,a);
        #else
            _glVertexAttrib4f( _colorAlias._location, r,g,b,a);
        #endif
        }

        void Normal(float x, float y, float z)
        {
        #ifdef OSG_GL_VERTEX_FUNCS_AVAILABLE
            if (_useVertexAttributeAliasing) _glVertexAttrib4f( _normalAlias._location, x,y,z,0.0);
            else glNormal3f(x,y,z);
        #else
            _glVertexAttrib4f( _normalAlias._location, x,y,z,0.0);
        #endif
        }

        void TexCoord(float x, float y=0.0f, float z=0.0f, float w=1.0f)
        {
        #if !defined(OSG_GLES1_AVAILABLE)
            #ifdef OSG_GL_VERTEX_FUNCS_AVAILABLE
                if (_useVertexAttributeAliasing) _glVertexAttrib4f( _texCoordAliasList[0]._location, x,y,z,w);
                else glTexCoord4f(x,y,z,w);
            #else
                _glVertexAttrib4f( _texCoordAliasList[0]._location, x,y,z,w);
            #endif
        #endif
        }

        void MultiTexCoord(unsigned int unit, float x, float y=0.0f, float z=0.0f, float w=1.0f)
        {
        #if !defined(OSG_GLES1_AVAILABLE)
            #ifdef OSG_GL_VERTEX_FUNCS_AVAILABLE
                if (_useVertexAttributeAliasing) _glVertexAttrib4f( _texCoordAliasList[unit]._location, x,y,z,w);
                else _glMultiTexCoord4f(GL_TEXTURE0+unit,x,y,z,w);
            #else
                _glVertexAttrib4f( _texCoordAliasList[unit]._location, x,y,z,w);
            #endif
        #endif
        }

        void VerteAttrib(unsigned int location, float x, float y=0.0f, float z=0.0f, float w=0.0f)
        {
            _glVertexAttrib4f( location, x,y,z,w);
        }


        /** Wrapper around glInterleavedArrays(..).
          * also resets the internal array points and modes within osg::State to keep the other
          * vertex array operations consistent. */
        void setInterleavedArrays( GLenum format, GLsizei stride, const GLvoid* pointer) { _vas->setInterleavedArrays( *this, format, stride, pointer); }

        /** Set the vertex pointer using an osg::Array, and manage any VBO that are required.*/
        inline void setVertexPointer(const Array* array) { _vas->setVertexArray(*this, array); }
        inline void setVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr, GLboolean normalized=GL_FALSE) { _vas->setVertexArray( *this, size, type, stride, ptr, normalized); }
        inline void disableVertexPointer() { _vas->disableVertexArray(*this); }


        inline void setNormalPointer(const Array* array) { _vas->setNormalArray(*this, array); }
        inline void setNormalPointer( GLenum type, GLsizei stride, const GLvoid *ptr, GLboolean normalized=GL_FALSE ) { _vas->setNormalArray( *this, type, stride, ptr, normalized); }
        inline void disableNormalPointer() { _vas->disableNormalArray(*this); }

        inline void setColorPointer(const Array* array) { _vas->setColorArray(*this, array); }
        inline void setColorPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *ptr, GLboolean normalized=GL_TRUE ) { _vas->setColorArray(*this, size, type, stride, ptr, normalized); }
        inline void disableColorPointer() { _vas->disableColorArray(*this); }

        inline bool isSecondaryColorSupported() const { return _isSecondaryColorSupported; }
        inline void setSecondaryColorPointer(const Array* array) { _vas->setSecondaryColorArray(*this, array); }
        inline void disableSecondaryColorPointer() { _vas->disableSecondaryColorArray(*this); }

        inline bool isFogCoordSupported() const { return _isFogCoordSupported; }
        inline void setFogCoordPointer(const Array* array) { _vas->setFogCoordArray(*this, array); }
        inline void disableFogCoordPointer() { _vas->disableFogCoordArray(*this); }


        inline void setTexCoordPointer(unsigned int unit, const Array* array) { _vas->setTexCoordArray(*this, unit, array); }
        inline void setTexCoordPointer( unsigned int unit, GLint size, GLenum type, GLsizei stride, const GLvoid *ptr, GLboolean normalized=GL_FALSE ) { _vas->setTexCoordArray(*this, unit, size, type, stride, ptr, normalized); }
        inline void disableTexCoordPointer( unsigned int unit ) { _vas->disableTexCoordArray(*this, unit); }
        inline void disableTexCoordPointersAboveAndIncluding( unsigned int unit ) { _vas->disableTexCoordArrayAboveAndIncluding(*this, unit); }

        /// For GL>=2.0 uses GL_MAX_TEXTURE_COORDS, for GL<2 uses GL_MAX_TEXTURE_UNITS
        inline GLint getMaxTextureCoords() const { return _glMaxTextureCoords; }

        /// For GL>=2.0 uses GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, for GL<2 uses GL_MAX_TEXTURE_UNITS
        inline GLint getMaxTextureUnits() const { return _glMaxTextureUnits; }


        /** Set the current texture unit, return true if selected,
          * false if selection failed such as when multi texturing is not supported.
          * note, only updates values that change.*/
        inline bool setActiveTextureUnit( unsigned int unit );

        /** Get the current texture unit.*/
        unsigned int getActiveTextureUnit() const { return _currentActiveTextureUnit; }

        /** Set the current tex coord array texture unit, return true if selected,
          * false if selection failed such as when multi texturing is not supported.
          * note, only updates values that change.*/
        bool setClientActiveTextureUnit( unsigned int unit );

        /** Get the current tex coord array texture unit.*/
        unsigned int getClientActiveTextureUnit() const;

        inline void setVertexAttribPointer(unsigned int unit, const Array* array) { _vas->setVertexAttribArray(*this, unit, array); }
        inline void setVertexAttribLPointer(unsigned int unit, const Array* array) { _vas->setVertexAttribArray(*this, unit, array); }
        inline void setVertexAttribIPointer(unsigned int unit, const Array* array) { _vas->setVertexAttribArray(*this, unit, array); }

        inline void disableVertexAttribPointer( unsigned int index ) { _vas->disableVertexAttribArray(*this, index); }
        inline void disableVertexAttribPointersAboveAndIncluding( unsigned int index ) { _vas->disableVertexAttribArray(*this, index); }


        /** dirty the vertex, normal, color, tex coords, secondary color, fog coord and index arrays.*/
        void dirtyAllVertexArrays();


        inline bool isVertexBufferObjectSupported() const { return _isVertexBufferObjectSupported; }
        inline bool useVertexBufferObject(bool useVBO) const { return _forceVertexBufferObject || (_isVertexBufferObjectSupported && useVBO); }

        inline bool isVertexArrayObjectSupported() const { return _isVertexArrayObjectSupported; }
        inline bool useVertexArrayObject(bool useVAO) const { return _forceVertexArrayObject || (_isVertexArrayObjectSupported && useVAO); }


        inline void setLastAppliedProgramObject(const Program::PerContextProgram* program)
        {
            if (_lastAppliedProgramObject!=program)
            {
                _lastAppliedProgramObject = program;
            }
        }
        inline const Program::PerContextProgram* getLastAppliedProgramObject() const { return _lastAppliedProgramObject; }

        inline GLint getUniformLocation( unsigned int uniformNameID ) const { return _lastAppliedProgramObject ? _lastAppliedProgramObject->getUniformLocation(uniformNameID) : -1; }
        /**
          * Alternative version of getUniformLocation( unsigned int uniformNameID )
          * retrofited into OSG for backward compatibility with osgCal,
          * after uniform ids were refactored from std::strings to GLints in OSG version 2.9.10.
          *
          * Drawbacks: This method is not particularly fast. It has to access mutexed static
          * map of uniform ids. So don't overuse it or your app performance will suffer.
        */
        inline GLint getUniformLocation( const std::string & uniformName ) const { return _lastAppliedProgramObject ? _lastAppliedProgramObject->getUniformLocation(uniformName) : -1; }
        inline GLint getAttribLocation( const std::string& name ) const { return _lastAppliedProgramObject ? _lastAppliedProgramObject->getAttribLocation(name) : -1; }

        typedef std::pair<const StateAttribute*,StateAttribute::OverrideValue>  AttributePair;
        typedef std::vector<AttributePair>                                      AttributeVec;

        AttributeVec& getAttributeVec( const osg::StateAttribute* attribute )
        {
                AttributeStack& as = _attributeMap[ attribute->getTypeMemberPair() ];
                return as.attributeVec;
        }

        /** Set the frame stamp for the current frame.*/
        inline void setFrameStamp(FrameStamp* fs) { _frameStamp = fs; }

        /** Get the frame stamp for the current frame.*/
        inline FrameStamp* getFrameStamp() { return _frameStamp.get(); }

        /** Get the const frame stamp for the current frame.*/
        inline const FrameStamp* getFrameStamp() const { return _frameStamp.get(); }


        /** Set the DisplaySettings. Note, nothing is applied, the visual settings are just
          * used in the State object to pass the current visual settings to Drawables
          * during rendering. */
        inline void setDisplaySettings(DisplaySettings* vs) { _displaySettings = vs; }

        /** Get the const DisplaySettings */
        inline const DisplaySettings* getDisplaySettings() const { return _displaySettings.get(); }

        /** Get the DisplaySettings that is current active DisplaySettings to be used by osg::State, - if DisplaySettings is not directly assigned then fallback to DisplaySettings::instance(). */
        inline DisplaySettings* getActiveDisplaySettings() { return _displaySettings.valid() ? _displaySettings.get() : osg::DisplaySettings::instance().get(); }

        /** Get the const DisplaySettings that is current active DisplaySettings to be used by osg::State, - if DisplaySettings is not directly assigned then fallback to DisplaySettings::instance(). */
        inline const DisplaySettings* getActiveDisplaySettings() const { return _displaySettings.valid() ? _displaySettings.get() : osg::DisplaySettings::instance().get(); }


        /** Set flag for early termination of the draw traversal.*/
        void setAbortRenderingPtr(bool* abortPtr) { _abortRenderingPtr = abortPtr; }

        /** Get flag for early termination of the draw traversal,
          * if true steps should be taken to complete rendering early.*/
        bool getAbortRendering() const { return _abortRenderingPtr!=0?(*_abortRenderingPtr):false; }


        struct DynamicObjectRenderingCompletedCallback : public osg::Referenced
        {
            virtual void completed(osg::State*) = 0;
        };

        /** Set the callback to be called when the dynamic object count hits 0.*/
        void setDynamicObjectRenderingCompletedCallback(DynamicObjectRenderingCompletedCallback* cb){ _completeDynamicObjectRenderingCallback = cb; }

        /** Get the callback to be called when the dynamic object count hits 0.*/
        DynamicObjectRenderingCompletedCallback* getDynamicObjectRenderingCompletedCallback() { return _completeDynamicObjectRenderingCallback.get(); }

        /** Set the number of dynamic objects that will be rendered in this graphics context this frame.*/
        void setDynamicObjectCount(unsigned int count, bool callCallbackOnZero = false)
        {
            if (_dynamicObjectCount != count)
            {
                _dynamicObjectCount = count;
                if (_dynamicObjectCount==0 && callCallbackOnZero && _completeDynamicObjectRenderingCallback.valid())
                {
                    _completeDynamicObjectRenderingCallback->completed(this);
                }
            }
        }

        /** Get the number of dynamic objects that will be rendered in this graphics context this frame.*/
        unsigned int getDynamicObjectCount() const { return _dynamicObjectCount; }

        /** Decrement the number of dynamic objects left to render this frame, and once the count goes to zero call the
          * DynamicObjectRenderingCompletedCallback to inform of completion.*/
        inline void decrementDynamicObjectCount()
        {
            --_dynamicObjectCount;
            if (_dynamicObjectCount==0 && _completeDynamicObjectRenderingCallback.valid())
            {
                _completeDynamicObjectRenderingCallback->completed(this);
            }
        }

        void setMaxTexturePoolSize(unsigned int size);
        unsigned int getMaxTexturePoolSize() const { return _maxTexturePoolSize; }

        void setMaxBufferObjectPoolSize(unsigned int size);
        unsigned int getMaxBufferObjectPoolSize() const { return _maxBufferObjectPoolSize; }


        enum CheckForGLErrors
        {
            /** NEVER_CHECK_GL_ERRORS hints that OpenGL need not be checked for, this
                is the fastest option since checking for errors does incur a small overhead.*/
            NEVER_CHECK_GL_ERRORS,
            /** ONCE_PER_FRAME means that OpenGL errors will be checked for once per
                frame, the overhead is still small, but at least OpenGL errors that are occurring
                will be caught, the reporting isn't fine grained enough for debugging purposes.*/
            ONCE_PER_FRAME,
            /** ONCE_PER_ATTRIBUTE means that OpenGL errors will be checked for after
                every attribute is applied, allow errors to be directly associated with
                particular operations which makes debugging much easier.*/
            ONCE_PER_ATTRIBUTE
        };

        /** Set whether and how often OpenGL errors should be checked for.*/
        void setCheckForGLErrors(CheckForGLErrors check) { _checkGLErrors = check; }

        /** Get whether and how often OpenGL errors should be checked for.*/
        CheckForGLErrors getCheckForGLErrors() const { return _checkGLErrors; }

        bool checkGLErrors(const char* str1=0, const char* str2=0) const;
        bool checkGLErrors(StateAttribute::GLMode mode) const;
        bool checkGLErrors(const StateAttribute* attribute) const;

        /** print out the internal details of osg::State - useful for debugging.*/
        void print(std::ostream& fout) const;

        /** Initialize extension used by osg::State.*/
        void initializeExtensionProcs();

        /** Get the helper class for dispatching osg::Arrays as OpenGL attribute data.*/
        inline AttributeDispatchers& getAttributeDispatchers() { return _arrayDispatchers; }


        /** Set the helper class that provides applications with estimate on how much different graphics operations will cost.*/
        inline void setGraphicsCostEstimator(GraphicsCostEstimator* gce) { _graphicsCostEstimator = gce; }

        /** Get the helper class that provides applications with estimate on how much different graphics operations will cost.*/
        inline GraphicsCostEstimator* getGraphicsCostEstimator() { return _graphicsCostEstimator.get(); }

        /** Get the cont helper class that provides applications with estimate on how much different graphics operations will cost.*/
        inline const GraphicsCostEstimator* getGraphicsCostEstimator() const { return _graphicsCostEstimator.get(); }



        /** Support for synchronizing the system time and the timestamp
         * counter available with ARB_timer_query. Note that State
         * doesn't update these values itself.
         */
        Timer_t getStartTick() const { return _startTick; }
        void setStartTick(Timer_t tick) { _startTick = tick; }
        Timer_t getGpuTick() const { return _gpuTick; }

        double getGpuTime() const
        {
            return osg::Timer::instance()->delta_s(_startTick, _gpuTick);
        }
        GLuint64 getGpuTimestamp() const { return _gpuTimestamp; }

        void setGpuTimestamp(Timer_t tick, GLuint64 timestamp)
        {
            _gpuTick = tick;
            _gpuTimestamp = timestamp;
        }
        int getTimestampBits() const { return _timestampBits; }
        void setTimestampBits(int bits) { _timestampBits = bits; }

        /** called by the GraphicsContext just before GraphicsContext::swapBuffersImplementation().*/
        virtual void frameCompleted();


        struct ModeStack
        {
            typedef std::vector<StateAttribute::GLModeValue> ValueVec;

            ModeStack()
            {
                valid = true;
                changed = false;
                last_applied_value = false;
                global_default_value = false;
            }

            void print(std::ostream& fout) const;

            bool        valid;
            bool        changed;
            bool        last_applied_value;
            bool        global_default_value;
            ValueVec    valueVec;
        };

        struct AttributeStack
        {
            AttributeStack()
            {
                changed = false;
                last_applied_attribute = 0L;
                last_applied_shadercomponent = 0L;
                global_default_attribute = 0L;

            }

            void print(std::ostream& fout) const;

            /** apply an attribute if required, passing in attribute and appropriate attribute stack */
            bool                    changed;
            const StateAttribute*   last_applied_attribute;
            const ShaderComponent*  last_applied_shadercomponent;
            ref_ptr<const StateAttribute> global_default_attribute;
            AttributeVec            attributeVec;
        };


        struct UniformStack
        {
            typedef std::pair<const Uniform*,StateAttribute::OverrideValue>         UniformPair;
            typedef std::vector<UniformPair>                                        UniformVec;

            UniformStack() {}

            void print(std::ostream& fout) const;

            UniformVec              uniformVec;
        };

        struct DefineStack
        {
            typedef std::vector<StateSet::DefinePair> DefineVec;

            DefineStack():
                changed(false) {}

            void print(std::ostream& fout) const;

            bool                   changed;
            DefineVec              defineVec;
        };

        struct DefineMap
        {
            DefineMap():
                changed(false) {}

            typedef std::map<std::string, DefineStack> DefineStackMap;
            DefineStackMap map;
            bool changed;
            StateSet::DefineList currentDefines;

            bool updateCurrentDefines();

        };

        typedef std::map<StateAttribute::GLMode,ModeStack>              ModeMap;
        typedef std::vector<ModeMap>                                    TextureModeMapList;

        typedef std::map<StateAttribute::TypeMemberPair,AttributeStack> AttributeMap;
        typedef std::vector<AttributeMap>                               TextureAttributeMapList;

        typedef std::map<std::string, UniformStack>                     UniformMap;


        typedef std::vector< ref_ptr<const Matrix> >                     MatrixStack;

        inline const ModeMap&                                           getModeMap() const {return _modeMap;}
        inline const AttributeMap&                                      getAttributeMap() const {return _attributeMap;}
        inline const UniformMap&                                        getUniformMap() const {return _uniformMap;}
        inline DefineMap&                                               getDefineMap() {return _defineMap;}
        inline const DefineMap&                                         getDefineMap() const {return _defineMap;}
        inline const TextureModeMapList&                                getTextureModeMapList() const {return _textureModeMapList;}
        inline const TextureAttributeMapList&                           getTextureAttributeMapList() const {return _textureAttributeMapList;}

        std::string getDefineString(const osg::ShaderDefines& shaderDefines);
        bool supportsShaderRequirements(const osg::ShaderDefines& shaderRequirements);
        bool supportsShaderRequirement(const std::string& shaderRequirement);

    protected:

        virtual ~State();

        GraphicsContext*            _graphicsContext;
        unsigned int                _contextID;

        osg::ref_ptr<VertexArrayState>  _globalVertexArrayState;
        VertexArrayState*               _vas;

        bool                            _shaderCompositionEnabled;
        bool                            _shaderCompositionDirty;
        osg::ref_ptr<ShaderComposer>    _shaderComposer;
        osg::Program*                   _currentShaderCompositionProgram;
        StateSet::UniformList           _currentShaderCompositionUniformList;

        ref_ptr<FrameStamp>         _frameStamp;

        GLenum                      _drawBuffer;
        GLenum                      _readBuffer;

        ref_ptr<const RefMatrix>    _identity;
        ref_ptr<const RefMatrix>    _initialViewMatrix;
        ref_ptr<const RefMatrix>    _projection;
        ref_ptr<const RefMatrix>    _modelView;
        ref_ptr<RefMatrix>          _modelViewCache;

        bool                        _useModelViewAndProjectionUniforms;
        ref_ptr<Uniform>            _modelViewMatrixUniform;
        ref_ptr<Uniform>            _projectionMatrixUniform;
        ref_ptr<Uniform>            _modelViewProjectionMatrixUniform;
        ref_ptr<Uniform>            _normalMatrixUniform;

        Matrix                      _initialInverseViewMatrix;

        ref_ptr<DisplaySettings>    _displaySettings;

        bool*                       _abortRenderingPtr;
        CheckForGLErrors            _checkGLErrors;


        bool                        _useVertexAttributeAliasing;
        VertexAttribAlias           _vertexAlias;
        VertexAttribAlias           _normalAlias;
        VertexAttribAlias           _colorAlias;
        VertexAttribAlias           _secondaryColorAlias;
        VertexAttribAlias           _fogCoordAlias;
        VertexAttribAliasList       _texCoordAliasList;

        Program::AttribBindingList  _attributeBindingList;

        void setUpVertexAttribAlias(VertexAttribAlias& alias, GLuint location, const std::string glName, const std::string osgName, const std::string& declaration);

        /** Apply an OpenGL mode if required, passing in mode, enable flag and
          * appropriate mode stack. This is a wrapper around \c glEnable() and
          * \c glDisable(), that just actually calls these functions if the
          * \c enabled flag is different than the current state.
          * @return \c true if the state was actually changed. \c false
          *         otherwise. Notice that a \c false return does not indicate
          *         an error, it just means that the mode was already set to the
          *         same value as the \c enabled parameter.
        */
        inline bool applyMode(StateAttribute::GLMode mode,bool enabled,ModeStack& ms)
        {
            if (ms.valid && ms.last_applied_value != enabled)
            {
                ms.last_applied_value = enabled;

                if (enabled) glEnable(mode);
                else glDisable(mode);

                if (_checkGLErrors==ONCE_PER_ATTRIBUTE) checkGLErrors(mode);

                return true;
            }
            else
                return false;
        }

        inline bool applyModeOnTexUnit(unsigned int unit,StateAttribute::GLMode mode,bool enabled,ModeStack& ms)
        {
            if (ms.valid && ms.last_applied_value != enabled)
            {
                if (setActiveTextureUnit(unit))
                {
                    ms.last_applied_value = enabled;

                    if (enabled) glEnable(mode);
                    else glDisable(mode);

                    if (_checkGLErrors==ONCE_PER_ATTRIBUTE) checkGLErrors(mode);

                    return true;
                }
                else
                    return false;
            }
            else
                return false;
        }

        /** apply an attribute if required, passing in attribute and appropriate attribute stack */
        inline bool applyAttribute(const StateAttribute* attribute,AttributeStack& as)
        {
            if (as.last_applied_attribute != attribute)
            {
                if (!as.global_default_attribute.valid()) as.global_default_attribute = attribute->cloneType()->asStateAttribute();

                as.last_applied_attribute = attribute;
                attribute->apply(*this);

                const ShaderComponent* sc = attribute->getShaderComponent();
                if (as.last_applied_shadercomponent != sc)
                {
                    as.last_applied_shadercomponent = sc;
                    _shaderCompositionDirty = true;
                }

                if (_checkGLErrors==ONCE_PER_ATTRIBUTE) checkGLErrors(attribute);

                return true;
            }
            else
                return false;
        }

        inline bool applyAttributeOnTexUnit(unsigned int unit,const StateAttribute* attribute,AttributeStack& as)
        {
            if (as.last_applied_attribute != attribute)
            {
                if (setActiveTextureUnit(unit))
                {
                    if (!as.global_default_attribute.valid()) as.global_default_attribute = attribute->cloneType()->asStateAttribute();

                    as.last_applied_attribute = attribute;
                    attribute->apply(*this);

                    const ShaderComponent* sc = attribute->getShaderComponent();
                    if (as.last_applied_shadercomponent != sc)
                    {
                        as.last_applied_shadercomponent = sc;
                        _shaderCompositionDirty = true;
                    }

                    if (_checkGLErrors==ONCE_PER_ATTRIBUTE) checkGLErrors(attribute);

                    return true;
                }
                else
                    return false;
            }
            else
                return false;
        }


        inline bool applyGlobalDefaultAttribute(AttributeStack& as)
        {
            if (as.last_applied_attribute != as.global_default_attribute.get())
            {
                as.last_applied_attribute = as.global_default_attribute.get();
                if (as.global_default_attribute.valid())
                {
                    as.global_default_attribute->apply(*this);
                    const ShaderComponent* sc = as.global_default_attribute->getShaderComponent();
                    if (as.last_applied_shadercomponent != sc)
                    {
                        as.last_applied_shadercomponent = sc;
                        _shaderCompositionDirty = true;
                    }

                    if (_checkGLErrors==ONCE_PER_ATTRIBUTE) checkGLErrors(as.global_default_attribute.get());
                }
                return true;
            }
            else
                return false;
        }

        inline bool applyGlobalDefaultAttributeOnTexUnit(unsigned int unit,AttributeStack& as)
        {
            if (as.last_applied_attribute != as.global_default_attribute.get())
            {
                if (setActiveTextureUnit(unit))
                {
                    as.last_applied_attribute = as.global_default_attribute.get();
                    if (as.global_default_attribute.valid())
                    {
                        as.global_default_attribute->apply(*this);
                        const ShaderComponent* sc = as.global_default_attribute->getShaderComponent();
                        if (as.last_applied_shadercomponent != sc)
                        {
                            as.last_applied_shadercomponent = sc;
                            _shaderCompositionDirty = true;
                        }
                        if (_checkGLErrors==ONCE_PER_ATTRIBUTE) checkGLErrors(as.global_default_attribute.get());
                    }
                    return true;
                }
                else
                    return false;
            }
            else
                return false;
        }

        ModeMap                                                         _modeMap;
        AttributeMap                                                    _attributeMap;
        UniformMap                                                      _uniformMap;
        DefineMap                                                       _defineMap;

        TextureModeMapList                                              _textureModeMapList;
        TextureAttributeMapList                                         _textureAttributeMapList;

        const Program::PerContextProgram*                               _lastAppliedProgramObject;

        StateSetStack                                                   _stateStateStack;

        unsigned int                                                    _maxTexturePoolSize;
        unsigned int                                                    _maxBufferObjectPoolSize;


        unsigned int                    _currentActiveTextureUnit;
        unsigned int                    _currentClientActiveTextureUnit;
        GLBufferObject*                 _currentPBO;
        GLBufferObject*                 _currentDIBO;
        GLuint                          _currentVAO;


        inline ModeMap& getOrCreateTextureModeMap(unsigned int unit)
        {
            if (unit>=_textureModeMapList.size()) _textureModeMapList.resize(unit+1);
            return _textureModeMapList[unit];
        }


        inline AttributeMap& getOrCreateTextureAttributeMap(unsigned int unit)
        {
            if (unit>=_textureAttributeMapList.size()) _textureAttributeMapList.resize(unit+1);
            return _textureAttributeMapList[unit];
        }

        inline void pushModeList(ModeMap& modeMap,const StateSet::ModeList& modeList);
        inline void pushAttributeList(AttributeMap& attributeMap,const StateSet::AttributeList& attributeList);
        inline void pushUniformList(UniformMap& uniformMap,const StateSet::UniformList& uniformList);
        inline void pushDefineList(DefineMap& defineMap,const StateSet::DefineList& defineList);

        inline void popModeList(ModeMap& modeMap,const StateSet::ModeList& modeList);
        inline void popAttributeList(AttributeMap& attributeMap,const StateSet::AttributeList& attributeList);
        inline void popUniformList(UniformMap& uniformMap,const StateSet::UniformList& uniformList);
        inline void popDefineList(DefineMap& uniformMap,const StateSet::DefineList& defineList);

        inline void applyModeList(ModeMap& modeMap,const StateSet::ModeList& modeList);
        inline void applyAttributeList(AttributeMap& attributeMap,const StateSet::AttributeList& attributeList);
        inline void applyUniformList(UniformMap& uniformMap,const StateSet::UniformList& uniformList);
        inline void applyDefineList(DefineMap& uniformMap,const StateSet::DefineList& defineList);

        inline void applyModeMap(ModeMap& modeMap);
        inline void applyAttributeMap(AttributeMap& attributeMap);
        inline void applyUniformMap(UniformMap& uniformMap);

        inline void applyModeListOnTexUnit(unsigned int unit,ModeMap& modeMap,const StateSet::ModeList& modeList);
        inline void applyAttributeListOnTexUnit(unsigned int unit,AttributeMap& attributeMap,const StateSet::AttributeList& attributeList);

        inline void applyModeMapOnTexUnit(unsigned int unit,ModeMap& modeMap);
        inline void applyAttributeMapOnTexUnit(unsigned int unit,AttributeMap& attributeMap);

        void haveAppliedMode(ModeMap& modeMap,StateAttribute::GLMode mode,StateAttribute::GLModeValue value);
        void haveAppliedMode(ModeMap& modeMap,StateAttribute::GLMode mode);
        void haveAppliedAttribute(AttributeMap& attributeMap,const StateAttribute* attribute);
        void haveAppliedAttribute(AttributeMap& attributeMap,StateAttribute::Type type, unsigned int member);
        bool getLastAppliedMode(const ModeMap& modeMap,StateAttribute::GLMode mode) const;
        const StateAttribute* getLastAppliedAttribute(const AttributeMap& attributeMap,StateAttribute::Type type, unsigned int member) const;

        void loadModelViewMatrix();


        bool _isSecondaryColorSupported;
        bool _isFogCoordSupported;
        bool _isVertexBufferObjectSupported;
        bool _isVertexArrayObjectSupported;
        bool _forceVertexBufferObject;
        bool _forceVertexArrayObject;

        typedef void (GL_APIENTRY * ActiveTextureProc) (GLenum texture);
        typedef void (GL_APIENTRY * FogCoordPointerProc) (GLenum type, GLsizei stride, const GLvoid *pointer);
        typedef void (GL_APIENTRY * SecondaryColorPointerProc) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
        typedef void (GL_APIENTRY * MultiTexCoord4fProc) (GLenum target, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
        typedef void (GL_APIENTRY * VertexAttrib4fProc)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
        typedef void (GL_APIENTRY * VertexAttrib4fvProc)(GLuint index, const GLfloat *v);
        typedef void (GL_APIENTRY * VertexAttribPointerProc) (unsigned int, GLint, GLenum, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
        typedef void (GL_APIENTRY * VertexAttribIPointerProc) (unsigned int, GLint, GLenum, GLsizei stride, const GLvoid *pointer);
        typedef void (GL_APIENTRY * VertexAttribLPointerProc) (unsigned int, GLint, GLenum, GLsizei stride, const GLvoid *pointer);
        typedef void (GL_APIENTRY * EnableVertexAttribProc) (unsigned int);
        typedef void (GL_APIENTRY * DisableVertexAttribProc) (unsigned int);
        typedef void (GL_APIENTRY * BindBufferProc) (GLenum target, GLuint buffer);

        typedef void (GL_APIENTRY * DrawArraysInstancedProc)( GLenum mode, GLint first, GLsizei count, GLsizei primcount );
        typedef void (GL_APIENTRY * DrawElementsInstancedProc)( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount );

        bool                        _extensionProcsInitialized;
        GLint                       _glMaxTextureCoords;
        GLint                       _glMaxTextureUnits;
        ActiveTextureProc           _glClientActiveTexture;
        ActiveTextureProc           _glActiveTexture;
        MultiTexCoord4fProc         _glMultiTexCoord4f;
        VertexAttrib4fProc          _glVertexAttrib4f;
        VertexAttrib4fvProc         _glVertexAttrib4fv;
        FogCoordPointerProc         _glFogCoordPointer;
        SecondaryColorPointerProc   _glSecondaryColorPointer;
        VertexAttribPointerProc     _glVertexAttribPointer;
        VertexAttribIPointerProc    _glVertexAttribIPointer;
        VertexAttribLPointerProc    _glVertexAttribLPointer;
        EnableVertexAttribProc      _glEnableVertexAttribArray;
        DisableVertexAttribProc     _glDisableVertexAttribArray;
        BindBufferProc              _glBindBuffer;
        DrawArraysInstancedProc     _glDrawArraysInstanced;
        DrawElementsInstancedProc   _glDrawElementsInstanced;

        osg::ref_ptr<GLExtensions>  _glExtensions;

        unsigned int                                            _dynamicObjectCount;
        osg::ref_ptr<DynamicObjectRenderingCompletedCallback>   _completeDynamicObjectRenderingCallback;

        AttributeDispatchers            _arrayDispatchers;

        osg::ref_ptr<GraphicsCostEstimator> _graphicsCostEstimator;

        Timer_t                      _startTick;
        Timer_t                      _gpuTick;
        GLuint64                     _gpuTimestamp;
        int                          _timestampBits;
};

inline void State::pushModeList(ModeMap& modeMap,const StateSet::ModeList& modeList)
{
    for(StateSet::ModeList::const_iterator mitr=modeList.begin();
        mitr!=modeList.end();
        ++mitr)
    {
        // get the mode stack for incoming GLmode {mitr->first}.
        ModeStack& ms = modeMap[mitr->first];
        if (ms.valueVec.empty())
        {
            // first pair so simply push incoming pair to back.
            ms.valueVec.push_back(mitr->second);
        }
        else if ((ms.valueVec.back() & StateAttribute::OVERRIDE) && !(mitr->second & StateAttribute::PROTECTED)) // check the existing override flag
        {
            // push existing back since override keeps the previous value.
            ms.valueVec.push_back(ms.valueVec.back());
        }
        else
        {
            // no override on so simply push incoming pair to back.
            ms.valueVec.push_back(mitr->second);
        }
        ms.changed = true;
    }
}

inline void State::pushAttributeList(AttributeMap& attributeMap,const StateSet::AttributeList& attributeList)
{
    for(StateSet::AttributeList::const_iterator aitr=attributeList.begin();
        aitr!=attributeList.end();
        ++aitr)
    {
        // get the attribute stack for incoming type {aitr->first}.
        AttributeStack& as = attributeMap[aitr->first];
        if (as.attributeVec.empty())
        {
            // first pair so simply push incoming pair to back.
            as.attributeVec.push_back(
                AttributePair(aitr->second.first.get(),aitr->second.second));
        }
        else if ((as.attributeVec.back().second & StateAttribute::OVERRIDE) && !(aitr->second.second & StateAttribute::PROTECTED)) // check the existing override flag
        {
            // push existing back since override keeps the previous value.
            as.attributeVec.push_back(as.attributeVec.back());
        }
        else
        {
            // no override on so simply push incoming pair to back.
            as.attributeVec.push_back(
                AttributePair(aitr->second.first.get(),aitr->second.second));
        }
        as.changed = true;
    }
}


inline void State::pushUniformList(UniformMap& uniformMap,const StateSet::UniformList& uniformList)
{
    for(StateSet::UniformList::const_iterator aitr=uniformList.begin();
        aitr!=uniformList.end();
        ++aitr)
    {
        // get the attribute stack for incoming type {aitr->first}.
        UniformStack& us = uniformMap[aitr->first];
        if (us.uniformVec.empty())
        {
            // first pair so simply push incoming pair to back.
            us.uniformVec.push_back(
                UniformStack::UniformPair(aitr->second.first.get(),aitr->second.second));
        }
        else if ((us.uniformVec.back().second & StateAttribute::OVERRIDE) && !(aitr->second.second & StateAttribute::PROTECTED)) // check the existing override flag
        {
            // push existing back since override keeps the previous value.
            us.uniformVec.push_back(us.uniformVec.back());
        }
        else
        {
            // no override on so simply push incoming pair to back.
            us.uniformVec.push_back(
                UniformStack::UniformPair(aitr->second.first.get(),aitr->second.second));
        }
    }
}

inline void State::pushDefineList(DefineMap& defineMap,const StateSet::DefineList& defineList)
{
    for(StateSet::DefineList::const_iterator aitr=defineList.begin();
        aitr!=defineList.end();
        ++aitr)
    {
        // get the attribute stack for incoming type {aitr->first}.
        DefineStack& ds = defineMap.map[aitr->first];
        DefineStack::DefineVec& dv = ds.defineVec;
        if (dv.empty())
        {
            // first pair so simply push incoming pair to back.
            dv.push_back(StateSet::DefinePair(aitr->second.first,aitr->second.second));

            ds.changed = true;
            defineMap.changed = true;
        }
        else if ((ds.defineVec.back().second & StateAttribute::OVERRIDE) && !(aitr->second.second & StateAttribute::PROTECTED)) // check the existing override flag
        {
            // push existing back since override keeps the previous value.
            ds.defineVec.push_back(ds.defineVec.back());
        }
        else
        {
            // no override on so simply push incoming pair to back.
            dv.push_back(StateSet::DefinePair(aitr->second.first,aitr->second.second));

            // if the back of the stack has changed since the last then mark it as changed.
            bool changed = (dv[dv.size()-2] != dv.back());
            if (changed)
            {
                ds.changed = true;
                defineMap.changed = true;
            }
        }
    }
}

inline void State::popModeList(ModeMap& modeMap,const StateSet::ModeList& modeList)
{
    for(StateSet::ModeList::const_iterator mitr=modeList.begin();
        mitr!=modeList.end();
        ++mitr)
    {
        // get the mode stack for incoming GLmode {mitr->first}.
        ModeStack& ms = modeMap[mitr->first];
        if (!ms.valueVec.empty())
        {
            ms.valueVec.pop_back();
        }
        ms.changed = true;
    }
}

inline void State::popAttributeList(AttributeMap& attributeMap,const StateSet::AttributeList& attributeList)
{
    for(StateSet::AttributeList::const_iterator aitr=attributeList.begin();
        aitr!=attributeList.end();
        ++aitr)
    {
        // get the attribute stack for incoming type {aitr->first}.
        AttributeStack& as = attributeMap[aitr->first];
        if (!as.attributeVec.empty())
        {
            as.attributeVec.pop_back();
        }
        as.changed = true;
    }
}

inline void State::popUniformList(UniformMap& uniformMap,const StateSet::UniformList& uniformList)
{
    for(StateSet::UniformList::const_iterator aitr=uniformList.begin();
        aitr!=uniformList.end();
        ++aitr)
    {
        // get the attribute stack for incoming type {aitr->first}.
        UniformStack& us = uniformMap[aitr->first];
        if (!us.uniformVec.empty())
        {
            us.uniformVec.pop_back();
        }
    }
}

inline void State::popDefineList(DefineMap& defineMap,const StateSet::DefineList& defineList)
{
    for(StateSet::DefineList::const_iterator aitr=defineList.begin();
        aitr!=defineList.end();
        ++aitr)
    {
        // get the attribute stack for incoming type {aitr->first}.
        DefineStack& ds = defineMap.map[aitr->first];
        DefineStack::DefineVec& dv = ds.defineVec;
        if (!dv.empty())
        {
            // if the stack has less than 2 entries or new back vs old back are different then mark the DefineStack as changed
            if ((dv.size() < 2) || (dv[dv.size()-2] != dv.back()))
            {
                ds.changed = true;
                defineMap.changed = true;
            }
            dv.pop_back();
        }
    }
}

inline void State::applyModeList(ModeMap& modeMap,const StateSet::ModeList& modeList)
{
    StateSet::ModeList::const_iterator ds_mitr = modeList.begin();
    ModeMap::iterator this_mitr=modeMap.begin();

    while (this_mitr!=modeMap.end() && ds_mitr!=modeList.end())
    {
        if (this_mitr->first<ds_mitr->first)
        {

            // note GLMode = this_mitr->first
            ModeStack& ms = this_mitr->second;
            if (ms.changed)
            {
                ms.changed = false;
                if (!ms.valueVec.empty())
                {
                    bool new_value = ms.valueVec.back() & StateAttribute::ON;
                    applyMode(this_mitr->first,new_value,ms);
                }
                else
                {
                    // assume default of disabled.
                    applyMode(this_mitr->first,ms.global_default_value,ms);

                }

            }

            ++this_mitr;

        }
        else if (ds_mitr->first<this_mitr->first)
        {

            // ds_mitr->first is a new mode, therefore
            // need to insert a new mode entry for ds_mistr->first.
            ModeStack& ms = modeMap[ds_mitr->first];

            bool new_value = ds_mitr->second & StateAttribute::ON;
            applyMode(ds_mitr->first,new_value,ms);

            // will need to disable this mode on next apply so set it to changed.
            ms.changed = true;

            ++ds_mitr;

        }
        else
        {
            // this_mitr & ds_mitr refer to the same mode, check the override
            // if any otherwise just apply the incoming mode.

            ModeStack& ms = this_mitr->second;

            if (!ms.valueVec.empty() && (ms.valueVec.back() & StateAttribute::OVERRIDE) && !(ds_mitr->second & StateAttribute::PROTECTED))
            {
                // override is on, just treat as a normal apply on modes.

                if (ms.changed)
                {
                    ms.changed = false;
                    bool new_value = ms.valueVec.back() & StateAttribute::ON;
                    applyMode(this_mitr->first,new_value,ms);

                }
            }
            else
            {
                // no override on or no previous entry, therefore consider incoming mode.
                bool new_value = ds_mitr->second & StateAttribute::ON;
                if (applyMode(ds_mitr->first,new_value,ms))
                {
                    ms.changed = true;
                }
            }

            ++this_mitr;
            ++ds_mitr;
        }
    }

    // iterator over the remaining state modes to apply any previous changes.
    for(;
        this_mitr!=modeMap.end();
        ++this_mitr)
    {
        // note GLMode = this_mitr->first
        ModeStack& ms = this_mitr->second;
        if (ms.changed)
        {
            ms.changed = false;
            if (!ms.valueVec.empty())
            {
                bool new_value = ms.valueVec.back() & StateAttribute::ON;
                applyMode(this_mitr->first,new_value,ms);
            }
            else
            {
                // assume default of disabled.
                applyMode(this_mitr->first,ms.global_default_value,ms);

            }

        }
    }

    // iterator over the remaining incoming modes to apply any new mode.
    for(;
        ds_mitr!=modeList.end();
        ++ds_mitr)
    {
        ModeStack& ms = modeMap[ds_mitr->first];

        bool new_value = ds_mitr->second & StateAttribute::ON;
        applyMode(ds_mitr->first,new_value,ms);

        // will need to disable this mode on next apply so set it to changed.
        ms.changed = true;
    }
}

inline void State::applyModeListOnTexUnit(unsigned int unit,ModeMap& modeMap,const StateSet::ModeList& modeList)
{
    StateSet::ModeList::const_iterator ds_mitr = modeList.begin();
    ModeMap::iterator this_mitr=modeMap.begin();

    while (this_mitr!=modeMap.end() && ds_mitr!=modeList.end())
    {
        if (this_mitr->first<ds_mitr->first)
        {

            // note GLMode = this_mitr->first
            ModeStack& ms = this_mitr->second;
            if (ms.changed)
            {
                ms.changed = false;
                if (!ms.valueVec.empty())
                {
                    bool new_value = ms.valueVec.back() & StateAttribute::ON;
                    applyModeOnTexUnit(unit,this_mitr->first,new_value,ms);
                }
                else
                {
                    // assume default of disabled.
                    applyModeOnTexUnit(unit,this_mitr->first,ms.global_default_value,ms);

                }

            }

            ++this_mitr;

        }
        else if (ds_mitr->first<this_mitr->first)
        {

            // ds_mitr->first is a new mode, therefore
            // need to insert a new mode entry for ds_mistr->first.
            ModeStack& ms = modeMap[ds_mitr->first];

            bool new_value = ds_mitr->second & StateAttribute::ON;
            applyModeOnTexUnit(unit,ds_mitr->first,new_value,ms);

            // will need to disable this mode on next apply so set it to changed.
            ms.changed = true;

            ++ds_mitr;

        }
        else
        {
            // this_mitr & ds_mitr refer to the same mode, check the override
            // if any otherwise just apply the incoming mode.

            ModeStack& ms = this_mitr->second;

            if (!ms.valueVec.empty() && (ms.valueVec.back() & StateAttribute::OVERRIDE) && !(ds_mitr->second & StateAttribute::PROTECTED))
            {
                // override is on, just treat as a normal apply on modes.

                if (ms.changed)
                {
                    ms.changed = false;
                    bool new_value = ms.valueVec.back() & StateAttribute::ON;
                    applyModeOnTexUnit(unit,this_mitr->first,new_value,ms);

                }
            }
            else
            {
                // no override on or no previous entry, therefore consider incoming mode.
                bool new_value = ds_mitr->second & StateAttribute::ON;
                if (applyModeOnTexUnit(unit,ds_mitr->first,new_value,ms))
                {
                    ms.changed = true;
                }
            }

            ++this_mitr;
            ++ds_mitr;
        }
    }

    // iterator over the remaining state modes to apply any previous changes.
    for(;
        this_mitr!=modeMap.end();
        ++this_mitr)
    {
        // note GLMode = this_mitr->first
        ModeStack& ms = this_mitr->second;
        if (ms.changed)
        {
            ms.changed = false;
            if (!ms.valueVec.empty())
            {
                bool new_value = ms.valueVec.back() & StateAttribute::ON;
                applyModeOnTexUnit(unit,this_mitr->first,new_value,ms);
            }
            else
            {
                // assume default of disabled.
                applyModeOnTexUnit(unit,this_mitr->first,ms.global_default_value,ms);

            }

        }
    }

    // iterator over the remaining incoming modes to apply any new mode.
    for(;
        ds_mitr!=modeList.end();
        ++ds_mitr)
    {
        ModeStack& ms = modeMap[ds_mitr->first];

        bool new_value = ds_mitr->second & StateAttribute::ON;
        applyModeOnTexUnit(unit,ds_mitr->first,new_value,ms);

        // will need to disable this mode on next apply so set it to changed.
        ms.changed = true;
    }
}

inline void State::applyAttributeList(AttributeMap& attributeMap,const StateSet::AttributeList& attributeList)
{
    StateSet::AttributeList::const_iterator ds_aitr=attributeList.begin();

    AttributeMap::iterator this_aitr=attributeMap.begin();

    while (this_aitr!=attributeMap.end() && ds_aitr!=attributeList.end())
    {
        if (this_aitr->first<ds_aitr->first)
        {

            // note attribute type = this_aitr->first
            AttributeStack& as = this_aitr->second;
            if (as.changed)
            {
                as.changed = false;
                if (!as.attributeVec.empty())
                {
                    const StateAttribute* new_attr = as.attributeVec.back().first;
                    applyAttribute(new_attr,as);
                }
                else
                {
                    applyGlobalDefaultAttribute(as);
                }
            }

            ++this_aitr;

        }
        else if (ds_aitr->first<this_aitr->first)
        {

            // ds_aitr->first is a new attribute, therefore
            // need to insert a new attribute entry for ds_aitr->first.
            AttributeStack& as = attributeMap[ds_aitr->first];

            const StateAttribute* new_attr = ds_aitr->second.first.get();
            applyAttribute(new_attr,as);

            as.changed = true;

            ++ds_aitr;

        }
        else
        {
            // this_mitr & ds_mitr refer to the same attribute, check the override
            // if any otherwise just apply the incoming attribute

            AttributeStack& as = this_aitr->second;

            if (!as.attributeVec.empty() && (as.attributeVec.back().second & StateAttribute::OVERRIDE) && !(ds_aitr->second.second & StateAttribute::PROTECTED))
            {
                // override is on, just treat as a normal apply on attribute.

                if (as.changed)
                {
                    as.changed = false;
                    const StateAttribute* new_attr = as.attributeVec.back().first;
                    applyAttribute(new_attr,as);
                }
            }
            else
            {
                // no override on or no previous entry, therefore consider incoming attribute.
                const StateAttribute* new_attr = ds_aitr->second.first.get();
                if (applyAttribute(new_attr,as))
                {
                    as.changed = true;
                }
            }

            ++this_aitr;
            ++ds_aitr;
        }
    }

    // iterator over the remaining state attributes to apply any previous changes.
    for(;
        this_aitr!=attributeMap.end();
        ++this_aitr)
    {
        // note attribute type = this_aitr->first
        AttributeStack& as = this_aitr->second;
        if (as.changed)
        {
            as.changed = false;
            if (!as.attributeVec.empty())
            {
                const StateAttribute* new_attr = as.attributeVec.back().first;
                applyAttribute(new_attr,as);
            }
            else
            {
                applyGlobalDefaultAttribute(as);
            }
        }
    }

    // iterator over the remaining incoming attribute to apply any new attribute.
    for(;
        ds_aitr!=attributeList.end();
        ++ds_aitr)
    {
        // ds_aitr->first is a new attribute, therefore
        // need to insert a new attribute entry for ds_aitr->first.
        AttributeStack& as = attributeMap[ds_aitr->first];

        const StateAttribute* new_attr = ds_aitr->second.first.get();
        applyAttribute(new_attr,as);

        // will need to update this attribute on next apply so set it to changed.
        as.changed = true;
    }

}

inline void State::applyAttributeListOnTexUnit(unsigned int unit,AttributeMap& attributeMap,const StateSet::AttributeList& attributeList)
{
    StateSet::AttributeList::const_iterator ds_aitr=attributeList.begin();

    AttributeMap::iterator this_aitr=attributeMap.begin();

    while (this_aitr!=attributeMap.end() && ds_aitr!=attributeList.end())
    {
        if (this_aitr->first<ds_aitr->first)
        {

            // note attribute type = this_aitr->first
            AttributeStack& as = this_aitr->second;
            if (as.changed)
            {
                as.changed = false;
                if (!as.attributeVec.empty())
                {
                    const StateAttribute* new_attr = as.attributeVec.back().first;
                    applyAttributeOnTexUnit(unit,new_attr,as);
                }
                else
                {
                    applyGlobalDefaultAttributeOnTexUnit(unit,as);
                }
            }

            ++this_aitr;

        }
        else if (ds_aitr->first<this_aitr->first)
        {

            // ds_aitr->first is a new attribute, therefore
            // need to insert a new attribute entry for ds_aitr->first.
            AttributeStack& as = attributeMap[ds_aitr->first];

            const StateAttribute* new_attr = ds_aitr->second.first.get();
            applyAttributeOnTexUnit(unit,new_attr,as);

            as.changed = true;

            ++ds_aitr;

        }
        else
        {
            // this_mitr & ds_mitr refer to the same attribute, check the override
            // if any otherwise just apply the incoming attribute

            AttributeStack& as = this_aitr->second;

            if (!as.attributeVec.empty() && (as.attributeVec.back().second & StateAttribute::OVERRIDE) && !(ds_aitr->second.second & StateAttribute::PROTECTED))
            {
                // override is on, just treat as a normal apply on attribute.

                if (as.changed)
                {
                    as.changed = false;
                    const StateAttribute* new_attr = as.attributeVec.back().first;
                    applyAttributeOnTexUnit(unit,new_attr,as);
                }
            }
            else
            {
                // no override on or no previous entry, therefore consider incoming attribute.
                const StateAttribute* new_attr = ds_aitr->second.first.get();
                if (applyAttributeOnTexUnit(unit,new_attr,as))
                {
                    as.changed = true;
                }
            }

            ++this_aitr;
            ++ds_aitr;
        }
    }

    // iterator over the remaining state attributes to apply any previous changes.
    for(;
        this_aitr!=attributeMap.end();
        ++this_aitr)
    {
        // note attribute type = this_aitr->first
        AttributeStack& as = this_aitr->second;
        if (as.changed)
        {
            as.changed = false;
            if (!as.attributeVec.empty())
            {
                const StateAttribute* new_attr = as.attributeVec.back().first;
                applyAttributeOnTexUnit(unit,new_attr,as);
            }
            else
            {
                applyGlobalDefaultAttributeOnTexUnit(unit,as);
            }
        }
    }

    // iterator over the remaining incoming attribute to apply any new attribute.
    for(;
        ds_aitr!=attributeList.end();
        ++ds_aitr)
    {
        // ds_aitr->first is a new attribute, therefore
        // need to insert a new attribute entry for ds_aitr->first.
        AttributeStack& as = attributeMap[ds_aitr->first];

        const StateAttribute* new_attr = ds_aitr->second.first.get();
        applyAttributeOnTexUnit(unit,new_attr,as);

        // will need to update this attribute on next apply so set it to changed.
        as.changed = true;
    }

}

inline void State::applyUniformList(UniformMap& uniformMap,const StateSet::UniformList& uniformList)
{
    if (!_lastAppliedProgramObject) return;

    StateSet::UniformList::const_iterator ds_aitr=uniformList.begin();

    UniformMap::iterator this_aitr=uniformMap.begin();

    while (this_aitr!=uniformMap.end() && ds_aitr!=uniformList.end())
    {
        if (this_aitr->first<ds_aitr->first)
        {
            // note attribute type = this_aitr->first
            UniformStack& as = this_aitr->second;
            if (!as.uniformVec.empty())
            {
                _lastAppliedProgramObject->apply(*as.uniformVec.back().first);
            }

            ++this_aitr;

        }
        else if (ds_aitr->first<this_aitr->first)
        {
            _lastAppliedProgramObject->apply(*(ds_aitr->second.first.get()));

            ++ds_aitr;
        }
        else
        {
            // this_mitr & ds_mitr refer to the same attribute, check the override
            // if any otherwise just apply the incoming attribute

            UniformStack& as = this_aitr->second;

            if (!as.uniformVec.empty() && (as.uniformVec.back().second & StateAttribute::OVERRIDE) && !(ds_aitr->second.second & StateAttribute::PROTECTED))
            {
                // override is on, just treat as a normal apply on uniform.
                _lastAppliedProgramObject->apply(*as.uniformVec.back().first);
            }
            else
            {
                // no override on or no previous entry, therefore consider incoming attribute.
                _lastAppliedProgramObject->apply(*(ds_aitr->second.first.get()));
            }

            ++this_aitr;
            ++ds_aitr;
        }
    }

    // iterator over the remaining state attributes to apply any previous changes.
    for(;
        this_aitr!=uniformMap.end();
        ++this_aitr)
    {
        // note attribute type = this_aitr->first
        UniformStack& as = this_aitr->second;
        if (!as.uniformVec.empty())
        {
            _lastAppliedProgramObject->apply(*as.uniformVec.back().first);
        }
    }

    // iterator over the remaining incoming attribute to apply any new attribute.
    for(;
        ds_aitr!=uniformList.end();
        ++ds_aitr)
    {
        _lastAppliedProgramObject->apply(*(ds_aitr->second.first.get()));
    }

}

inline void State::applyDefineList(DefineMap& defineMap, const StateSet::DefineList& defineList)
{
    StateSet::DefineList::const_iterator dl_itr = defineList.begin();
    DefineMap::DefineStackMap::iterator dm_itr = defineMap.map.begin();

    defineMap.changed = false;
    defineMap.currentDefines.clear();

    while (dm_itr!=defineMap.map.end() && dl_itr!=defineList.end())
    {
        if (dm_itr->first<dl_itr->first)
        {
            DefineStack& ds = dm_itr->second;
            DefineStack::DefineVec& dv = ds.defineVec;
            if (!dv.empty() && (dv.back().second & StateAttribute::ON)!=0) defineMap.currentDefines[dm_itr->first] = dv.back();

            ++dm_itr;
        }
        else if (dl_itr->first<dm_itr->first)
        {
            if ((dl_itr->second.second & StateAttribute::ON)!=0) defineMap.currentDefines[dl_itr->first] = dl_itr->second;

            ++dl_itr;
        }
        else
        {
            // this_mitr & ds_mitr refer to the same mode, check the override
            // if any otherwise just apply the incoming mode.

            DefineStack& ds = dm_itr->second;
            DefineStack::DefineVec& dv = ds.defineVec;

            if (!dv.empty() && (dv.back().second & StateAttribute::OVERRIDE)!=0 && !(dl_itr->second.second & StateAttribute::PROTECTED))
            {
                // override is on, just treat as a normal apply on modes.
                if ((dv.back().second & StateAttribute::ON)!=0) defineMap.currentDefines[dm_itr->first] = dv.back();
            }
            else
            {
                // no override on or no previous entry, therefore consider incoming mode.
                if ((dl_itr->second.second & StateAttribute::ON)!=0) defineMap.currentDefines[dl_itr->first] = dl_itr->second;
            }

            ++dm_itr;
            ++dl_itr;
        }
    }

    // iterator over the remaining state modes to apply any previous changes.
    for(;
        dm_itr!=defineMap.map.end();
        ++dm_itr)
    {
        // note GLMode = this_mitr->first
        DefineStack& ds = dm_itr->second;
        DefineStack::DefineVec& dv = ds.defineVec;
        if (!dv.empty() && (dv.back().second & StateAttribute::ON)!=0) defineMap.currentDefines[dm_itr->first] = dv.back();
    }

    // iterator over the remaining incoming modes to apply any new mode.
    for(;
        dl_itr!=defineList.end();
        ++dl_itr)
    {
        if ((dl_itr->second.second & StateAttribute::ON)!=0) defineMap.currentDefines[dl_itr->first] = dl_itr->second;
    }
}

inline void State::applyModeMap(ModeMap& modeMap)
{
    for(ModeMap::iterator mitr=modeMap.begin();
        mitr!=modeMap.end();
        ++mitr)
    {
        // note GLMode = mitr->first
        ModeStack& ms = mitr->second;
        if (ms.changed)
        {
            ms.changed = false;
            if (!ms.valueVec.empty())
            {
                bool new_value = ms.valueVec.back() & StateAttribute::ON;
                applyMode(mitr->first,new_value,ms);
            }
            else
            {
                // assume default of disabled.
                applyMode(mitr->first,ms.global_default_value,ms);
            }

        }
    }
}

inline void State::applyModeMapOnTexUnit(unsigned int unit,ModeMap& modeMap)
{
    for(ModeMap::iterator mitr=modeMap.begin();
        mitr!=modeMap.end();
        ++mitr)
    {
        // note GLMode = mitr->first
        ModeStack& ms = mitr->second;
        if (ms.changed)
        {
            ms.changed = false;
            if (!ms.valueVec.empty())
            {
                bool new_value = ms.valueVec.back() & StateAttribute::ON;
                applyModeOnTexUnit(unit,mitr->first,new_value,ms);
            }
            else
            {
                // assume default of disabled.
                applyModeOnTexUnit(unit,mitr->first,ms.global_default_value,ms);
            }

        }
    }
}

inline void State::applyAttributeMap(AttributeMap& attributeMap)
{
    for(AttributeMap::iterator aitr=attributeMap.begin();
        aitr!=attributeMap.end();
        ++aitr)
    {
        AttributeStack& as = aitr->second;
        if (as.changed)
        {
            as.changed = false;
            if (!as.attributeVec.empty())
            {
                const StateAttribute* new_attr = as.attributeVec.back().first;
                applyAttribute(new_attr,as);
            }
            else
            {
                applyGlobalDefaultAttribute(as);
            }

        }
    }
}

inline void State::applyAttributeMapOnTexUnit(unsigned int unit,AttributeMap& attributeMap)
{
    for(AttributeMap::iterator aitr=attributeMap.begin();
        aitr!=attributeMap.end();
        ++aitr)
    {
        AttributeStack& as = aitr->second;
        if (as.changed)
        {
            as.changed = false;
            if (!as.attributeVec.empty())
            {
                const StateAttribute* new_attr = as.attributeVec.back().first;
                applyAttributeOnTexUnit(unit,new_attr,as);
            }
            else
            {
                applyGlobalDefaultAttributeOnTexUnit(unit,as);
            }

        }
    }
}

inline void State::applyUniformMap(UniformMap& uniformMap)
{
    if (!_lastAppliedProgramObject) return;

    for(UniformMap::iterator aitr=uniformMap.begin();
        aitr!=uniformMap.end();
        ++aitr)
    {
        UniformStack& as = aitr->second;
        if (!as.uniformVec.empty())
        {
            _lastAppliedProgramObject->apply(*as.uniformVec.back().first);
        }
    }
}

inline bool State::setActiveTextureUnit( unsigned int unit )
{
    if (unit!=_currentActiveTextureUnit)
    {
        if (_glActiveTexture && unit < (unsigned int)(maximum(_glMaxTextureCoords,_glMaxTextureUnits)) )
        {
            _glActiveTexture(GL_TEXTURE0+unit);
            _currentActiveTextureUnit = unit;
        }
        else
        {
            return unit==0;
        }
    }
    return true;
}



// forward declare speciailization of State::get() method
template<> inline GLExtensions* State::get<GLExtensions>() { return _glExtensions.get(); }
template<> inline const GLExtensions* State::get<GLExtensions>() const { return _glExtensions.get(); }
template<> inline void State::set<GLExtensions>(GLExtensions* ptr) { _glExtensions = ptr; }

}

#endif
