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

#ifndef OSG_GLEXTENSIONS
#define OSG_GLEXTENSIONS 1

#include <osg/Export>
#include <osg/GLDefines>

#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>


namespace osg {

/** Return floating-point OpenGL/GLES version number.
  * Note: Must only be called within a valid OpenGL context,
  * undefined behavior may occur otherwise.
*/
extern OSG_EXPORT float getGLVersionNumber();

/** Return true if "extension" is contained in "extensionString".
*/
extern OSG_EXPORT bool isExtensionInExtensionString(const char *extension, const char *extensionString);

/** Return true if OpenGL/GLES "extension" is supported.
  * Note: Must only be called within a valid OpenGL context,
  * undefined behavior may occur otherwise.
*/
extern OSG_EXPORT bool isGLExtensionSupported(unsigned int contextID, const char *extension);

/** Return true if either OpenGL/GLES "extension1" or "extension2" is supported.
  * Note: Must only be called within a valid OpenGL context,
  * undefined behavior may occur otherwise.
*/
extern OSG_EXPORT bool isGLExtensionSupported(unsigned int contextID, const char *extension1, const char *extension2);

/** Return true if OpenGL/GLES "extension" or minimum OpenGL version number is supported.
  * Note: Must only be called within a valid OpenGL context,
  * undefined behavior may occur otherwise.
*/
extern OSG_EXPORT bool isGLExtensionOrVersionSupported(unsigned int contextID, const char *extension, float requiredGlVersion);

/** Return the address of the specified OpenGL/GLES function.
  * Return NULL if function not supported by OpenGL library.
  * Note, glGLExtensionFuncPtr is declared inline so that the code
  * is compiled locally to the calling code.  This should get by Windows'
  * dumb implementation of having different GL function ptr's for each
  * library when linked to it.
*/
extern OSG_EXPORT void* getGLExtensionFuncPtr(const char *funcName);

/** Set a list of extensions to disable for different OpenGL renderers. This allows
  * OSG applications to work around OpenGL drivers' bugs which are due to problematic extension support.
  * The format of the string is:
  * "GLRendererString : ExtensionName, ExtensionName; GLRenderString2 : ExtensionName;"
  * An example of is : "SUN_XVR1000:GL_EXT_texture_filter_anisotropic"
  * The default setting of GLExtensionDisableString is obtained from the OSG_GL_EXTENSION_DISABLE
  * environmental variable.
*/
extern OSG_EXPORT void setGLExtensionDisableString(const std::string& disableString);

/** Get the list of extensions that are disabled for various OpenGL renderers. */
extern OSG_EXPORT std::string& getGLExtensionDisableString();

/** Return the address of the specified OpenGL function. If not found then
  * check a second function name, if this fails then return NULL as function is
  * not supported by OpenGL library. This is used for checking something
  * like glActiveTexture (which is in OGL1.3) or glActiveTextureARB.
*/
inline void* getGLExtensionFuncPtr(const char *funcName, const char *fallbackFuncName)
{
    void* ptr = getGLExtensionFuncPtr(funcName);
    return (ptr!=0) ? ptr : getGLExtensionFuncPtr(fallbackFuncName);
}

/** Return the address of the specified OpenGL function. If not found then
  * check a second function name, if this fails then return NULL as function is
  * not supported by OpenGL library. This is used for checking something
  * like glActiveTexture (which is in OGL1.3) or glActiveTextureARB.
*/
inline void* getGLExtensionFuncPtr(const char *funcName1, const char *funcName2, const char *funcName3)
{
    void* ptr = getGLExtensionFuncPtr(funcName1);
    return (ptr!=0) ? ptr : getGLExtensionFuncPtr(funcName2, funcName3);
}

template<typename T, typename R>
bool convertPointer(T& dest, R src)
{
    memcpy(&dest, &src, sizeof(src));
    return src!=0;
}

template<typename T, typename R>
T convertPointerType(R src)
{
    T dest;
    memcpy(&dest, &src, sizeof(src));
    return dest;
}

template<typename T>
bool setGLExtensionFuncPtr(T& t, const char* str1, bool validContext=true)
{
    return convertPointer(t, validContext ? osg::getGLExtensionFuncPtr(str1) : 0);
}

template<typename T>
bool setGLExtensionFuncPtr(T& t, const char* str1, const char* str2, bool validContext=true)
{
    return convertPointer(t, validContext ? osg::getGLExtensionFuncPtr(str1, str2) : 0);
}

template<typename T>
bool setGLExtensionFuncPtr(T& t, const char* str1, const char* str2, const char* str3, bool validContext=true)
{
    return convertPointer(t, validContext ? osg::getGLExtensionFuncPtr(str1, str2, str3) : 0);
}

class VertexAttribAlias
{
    public:
        VertexAttribAlias():
            _location(0) {}

        VertexAttribAlias(const VertexAttribAlias& rhs):
            _location(rhs._location),
            _glName(rhs._glName),
            _osgName(rhs._osgName),
            _declaration(rhs._declaration) {}

        VertexAttribAlias(GLuint location, const std::string glName, const std::string osgName, const std::string& declaration):
            _location(location),
            _glName(glName),
            _osgName(osgName),
            _declaration(declaration) {}

        GLuint      _location;
        std::string _glName;
        std::string _osgName;
        std::string _declaration;
};

/** Main GLExtensions class for managing OpenGL extensions per graphics context.*/
class OSG_EXPORT GLExtensions : public osg::Referenced
{
    public:
        GLExtensions(unsigned int in_contextID);

        /** Function to call to get the extension of a specified context.
          * If the Exentsion object for that context has not yet been created then
          * and the 'createIfNotInitalized' flag been set to false then returns NULL.
          * If 'createIfNotInitalized' is true then the Extensions object is
          * automatically created.  However, in this case the extension object
          * only be created with the graphics context associated with ContextID..*/
        static GLExtensions* Get(unsigned int in_contextID,bool createIfNotInitalized);

        /** allows users to override the extensions across graphics contexts.
          * typically used when you have different extensions supported across graphics pipes
          * but need to ensure that they all use the same low common denominator extensions.*/
        static void Set(unsigned int in_contextID, GLExtensions* extensions);

        // C++-friendly convenience wrapper methods
        GLuint getCurrentProgram() const;
        bool getProgramInfoLog( GLuint program, std::string& result ) const;
        bool getShaderInfoLog( GLuint shader, std::string& result ) const;
        bool getAttribLocation( const char* attribName, GLuint& slot ) const;
        bool getFragDataLocation( const char* fragDataName, GLuint& slot) const;

        unsigned int contextID;
        float glVersion;
        float glslLanguageVersion;

        bool isGlslSupported;
        bool isShaderObjectsSupported;
        bool isVertexShaderSupported;
        bool isFragmentShaderSupported;
        bool isLanguage100Supported;
        bool isGeometryShader4Supported;
        bool areTessellationShadersSupported;
        bool isGpuShader4Supported;
        bool isUniformBufferObjectSupported;
        bool isGetProgramBinarySupported;
        bool isGpuShaderFp64Supported;
        bool isShaderAtomicCountersSupported;
        bool isRectangleSupported;
        bool isCubeMapSupported;
        bool isClipControlSupported;

        void (GL_APIENTRY * glDrawBuffers)(GLsizei n, const GLenum *bufs);
        void (GL_APIENTRY * glAttachShader)(GLuint program, GLuint shader);
        void (GL_APIENTRY * glBindAttribLocation)(GLuint program, GLuint index, const GLchar *name);
        void (GL_APIENTRY * glCompileShader)(GLuint shader);
        GLuint (GL_APIENTRY * glCreateProgram)(void);
        GLuint (GL_APIENTRY * glCreateShader)(GLenum type);
        void (GL_APIENTRY * glDeleteProgram)(GLuint program);
        void (GL_APIENTRY * glDeleteObjectARB)(GLuint program);
        void (GL_APIENTRY * glDeleteShader)(GLuint shader);
        void (GL_APIENTRY * glDetachShader)(GLuint program, GLuint shader);
        void (GL_APIENTRY * glDisableVertexAttribArray)(GLuint index);
        void (GL_APIENTRY * glEnableVertexAttribArray)(GLuint index);
        void (GL_APIENTRY * glGetActiveAttrib)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
        void (GL_APIENTRY * glGetActiveUniform)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
        void (GL_APIENTRY * glGetAttachedShaders)(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *obj);
        GLint (GL_APIENTRY * glGetAttribLocation)(GLuint program, const GLchar *name);
        void (GL_APIENTRY * glGetProgramiv)(GLuint program, GLenum pname, GLint *params);
        void (GL_APIENTRY * glGetObjectParameterivARB)(GLuint program, GLenum pname, GLint *params);
        void (GL_APIENTRY * glGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
        void (GL_APIENTRY * glGetInfoLogARB)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
        void (GL_APIENTRY * glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
        void (GL_APIENTRY * glGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
        void (GL_APIENTRY * glGetShaderSource)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source);
        GLint (GL_APIENTRY * glGetUniformLocation)(GLuint program, const GLchar *name);
        void (GL_APIENTRY * glGetUniformfv)(GLuint program, GLint location, GLfloat *params);
        void (GL_APIENTRY * glGetUniformiv)(GLuint program, GLint location, GLint *params);
        void (GL_APIENTRY * glGetVertexAttribdv)(GLuint index, GLenum pname, GLdouble *params);
        void (GL_APIENTRY * glGetVertexAttribfv)(GLuint index, GLenum pname, GLfloat *params);
        void (GL_APIENTRY * glGetVertexAttribiv)(GLuint index, GLenum pname, GLint *params);
        void (GL_APIENTRY * glGetVertexAttribPointerv)(GLuint index, GLenum pname, GLvoid* *pointer);
        GLboolean (GL_APIENTRY * glIsProgram)(GLuint program);
        GLboolean (GL_APIENTRY * glIsShader)(GLuint shader);
        void (GL_APIENTRY * glLinkProgram)(GLuint program);
        void (GL_APIENTRY * glShaderSource)(GLuint shader, GLsizei count, const GLchar* *string, const GLint *length);
        void (GL_APIENTRY * glUseProgram)(GLuint program);
        void (GL_APIENTRY * glUniform1f)(GLint location, GLfloat v0);
        void (GL_APIENTRY * glUniform2f)(GLint location, GLfloat v0, GLfloat v1);
        void (GL_APIENTRY * glUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
        void (GL_APIENTRY * glUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
        void (GL_APIENTRY * glUniform1i)(GLint location, GLint v0);
        void (GL_APIENTRY * glUniform2i)(GLint location, GLint v0, GLint v1);
        void (GL_APIENTRY * glUniform3i)(GLint location, GLint v0, GLint v1, GLint v2);
        void (GL_APIENTRY * glUniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
        void (GL_APIENTRY * glUniform1fv)(GLint location, GLsizei count, const GLfloat *value);
        void (GL_APIENTRY * glUniform2fv)(GLint location, GLsizei count, const GLfloat *value);
        void (GL_APIENTRY * glUniform3fv)(GLint location, GLsizei count, const GLfloat *value);
        void (GL_APIENTRY * glUniform4fv)(GLint location, GLsizei count, const GLfloat *value);
        void (GL_APIENTRY * glUniform1iv)(GLint location, GLsizei count, const GLint *value);
        void (GL_APIENTRY * glUniform2iv)(GLint location, GLsizei count, const GLint *value);
        void (GL_APIENTRY * glUniform3iv)(GLint location, GLsizei count, const GLint *value);
        void (GL_APIENTRY * glUniform4iv)(GLint location, GLsizei count, const GLint *value);
        void (GL_APIENTRY * glUniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
        void (GL_APIENTRY * glUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
        void (GL_APIENTRY * glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
        void (GL_APIENTRY * glValidateProgram)(GLuint program);
        void (GL_APIENTRY * glVertexAttrib1d)(GLuint index, GLdouble x);
        void (GL_APIENTRY * glVertexAttrib1dv)(GLuint index, const GLdouble *v);
        void (GL_APIENTRY * glVertexAttrib1f)(GLuint index, GLfloat x);
        void (GL_APIENTRY * glVertexAttrib1fv)(GLuint index, const GLfloat *v);
        void (GL_APIENTRY * glVertexAttrib1s)(GLuint index, GLshort x);
        void (GL_APIENTRY * glVertexAttrib1sv)(GLuint index, const GLshort *v);
        void (GL_APIENTRY * glVertexAttrib2d)(GLuint index, GLdouble x, GLdouble y);
        void (GL_APIENTRY * glVertexAttrib2dv)(GLuint index, const GLdouble *v);
        void (GL_APIENTRY * glVertexAttrib2f)(GLuint index, GLfloat x, GLfloat y);
        void (GL_APIENTRY * glVertexAttrib2fv)(GLuint index, const GLfloat *v);
        void (GL_APIENTRY * glVertexAttrib2s)(GLuint index, GLshort x, GLshort y);
        void (GL_APIENTRY * glVertexAttrib2sv)(GLuint index, const GLshort *v);
        void (GL_APIENTRY * glVertexAttrib3d)(GLuint index, GLdouble x, GLdouble y, GLdouble z);
        void (GL_APIENTRY * glVertexAttrib3dv)(GLuint index, const GLdouble *v);
        void (GL_APIENTRY * glVertexAttrib3f)(GLuint index, GLfloat x, GLfloat y, GLfloat z);
        void (GL_APIENTRY * glVertexAttrib3fv)(GLuint index, const GLfloat *v);
        void (GL_APIENTRY * glVertexAttrib3s)(GLuint index, GLshort x, GLshort y, GLshort z);
        void (GL_APIENTRY * glVertexAttrib3sv)(GLuint index, const GLshort *v);
        void (GL_APIENTRY * glVertexAttrib4Nbv)(GLuint index, const GLbyte *v);
        void (GL_APIENTRY * glVertexAttrib4Niv)(GLuint index, const GLint *v);
        void (GL_APIENTRY * glVertexAttrib4Nsv)(GLuint index, const GLshort *v);
        void (GL_APIENTRY * glVertexAttrib4Nub)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
        void (GL_APIENTRY * glVertexAttrib4Nubv)(GLuint index, const GLubyte *v);
        void (GL_APIENTRY * glVertexAttrib4Nuiv)(GLuint index, const GLuint *v);
        void (GL_APIENTRY * glVertexAttrib4Nusv)(GLuint index, const GLushort *v);
        void (GL_APIENTRY * glVertexAttrib4bv)(GLuint index, const GLbyte *v);
        void (GL_APIENTRY * glVertexAttrib4d)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
        void (GL_APIENTRY * glVertexAttrib4dv)(GLuint index, const GLdouble *v);
        void (GL_APIENTRY * glVertexAttrib4f)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
        void (GL_APIENTRY * glVertexAttrib4fv)(GLuint index, const GLfloat *v);
        void (GL_APIENTRY * glVertexAttrib4iv)(GLuint index, const GLint *v);
        void (GL_APIENTRY * glVertexAttrib4s)(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
        void (GL_APIENTRY * glVertexAttrib4sv)(GLuint index, const GLshort *v);
        void (GL_APIENTRY * glVertexAttrib4ubv)(GLuint index, const GLubyte *v);
        void (GL_APIENTRY * glVertexAttrib4uiv)(GLuint index, const GLuint *v);
        void (GL_APIENTRY * glVertexAttrib4usv)(GLuint index, const GLushort *v);
        void (GL_APIENTRY * glVertexAttribPointer) (unsigned int, GLint, GLenum, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
        void (GL_APIENTRY * glVertexAttribIPointer) (unsigned int, GLint, GLenum, GLsizei stride, const GLvoid *pointer);
        void (GL_APIENTRY * glVertexAttribLPointer) (unsigned int, GLint, GLenum, GLsizei stride, const GLvoid *pointer);
        void (GL_APIENTRY * glVertexAttribDivisor)(GLuint index, GLuint divisor);

        void (GL_APIENTRY * glUniformMatrix2x3fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat* value );
        void (GL_APIENTRY * glUniformMatrix3x2fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat* value );
        void (GL_APIENTRY * glUniformMatrix2x4fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat* value );
        void (GL_APIENTRY * glUniformMatrix4x2fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat* value );
        void (GL_APIENTRY * glUniformMatrix3x4fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat* value );
        void (GL_APIENTRY * glUniformMatrix4x3fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat* value );
        void (GL_APIENTRY * glClipControl)( GLenum origin, GLenum depthMode );
        void (GL_APIENTRY * glProgramParameteri)( GLuint program, GLenum pname, GLint value );
        void (GL_APIENTRY * glPatchParameteri)( GLenum pname, GLint value );
        void (GL_APIENTRY * glPatchParameterfv)( GLenum pname, const GLfloat* values );
        void (GL_APIENTRY * glGetUniformuiv)( GLuint program, GLint location, GLuint* params );
        void (GL_APIENTRY * glBindFragDataLocation)( GLuint program, GLuint color, const GLchar* name );
        void (GL_APIENTRY * glBindFragDataLocationIndexed) (GLuint program, GLuint colorNumber, GLuint index, const GLchar * name);
        GLint (GL_APIENTRY * glGetFragDataIndex) (GLuint program,  const GLchar * name);
        GLint (GL_APIENTRY * glGetFragDataLocation)( GLuint program, const GLchar* name);
        void (GL_APIENTRY * glUniform1ui)( GLint location, GLuint v0 );
        void (GL_APIENTRY * glUniform2ui)( GLint location, GLuint v0, GLuint v1 );
        void (GL_APIENTRY * glUniform3ui)( GLint location, GLuint v0, GLuint v1, GLuint v2 );
        void (GL_APIENTRY * glUniform4ui)( GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 );
        void (GL_APIENTRY * glUniform1uiv)( GLint location, GLsizei count, const GLuint *value );
        void (GL_APIENTRY * glUniform2uiv)( GLint location, GLsizei count, const GLuint *value );
        void (GL_APIENTRY * glUniform3uiv)( GLint location, GLsizei count, const GLuint *value );
        void (GL_APIENTRY * glUniform4uiv)( GLint location, GLsizei count, const GLuint *value );
        void (GL_APIENTRY * glUniform1i64  )(GLint location, GLint64 x) ;
        void (GL_APIENTRY * glUniform1i64v )(GLint location, GLsizei count, const GLint64* value) ;
        void (GL_APIENTRY * glUniform1ui64 )(GLint location, GLuint64 x) ;
        void (GL_APIENTRY * glUniform1ui64v)(GLint location, GLsizei count, const GLuint64* value) ;
        void (GL_APIENTRY * glUniform2i64  )(GLint location, GLint64 x, GLint64 y) ;
        void (GL_APIENTRY * glUniform2i64v )(GLint location, GLsizei count, const GLint64* value) ;
        void (GL_APIENTRY * glUniform2ui64 )(GLint location, GLuint64 x, GLuint64 y) ;
        void (GL_APIENTRY * glUniform2ui64v)(GLint location, GLsizei count, const GLuint64* value) ;
        void (GL_APIENTRY * glUniform3i64  )(GLint location, GLint64 x, GLint64 y, GLint64 z) ;
        void (GL_APIENTRY * glUniform3i64v )(GLint location, GLsizei count, const GLint64* value) ;
        void (GL_APIENTRY * glUniform3ui64 )(GLint location, GLuint64 x, GLuint64 y, GLuint64 z) ;
        void (GL_APIENTRY * glUniform3ui64v)(GLint location, GLsizei count, const GLuint64* value) ;
        void (GL_APIENTRY * glUniform4i64  )(GLint location, GLint64 x, GLint64 y, GLint64 z, GLint64 w) ;
        void (GL_APIENTRY * glUniform4i64v )(GLint location, GLsizei count, const GLint64* value) ;
        void (GL_APIENTRY * glUniform4ui64 )(GLint location, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w) ;
        void (GL_APIENTRY * glUniform4ui64v)(GLint location, GLsizei count, const GLuint64* value) ;
        GLuint (GL_APIENTRY * glGetHandleARB) (GLenum pname);
        void (GL_APIENTRY * glGetUniformIndices)(GLuint program, GLsizei uniformCount, const GLchar* *uniformNames, GLuint *uniformIndices);
        void (GL_APIENTRY * glGetActiveUniformsiv)(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params);
        void (GL_APIENTRY * glGetActiveUniformName)(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName);
        GLuint (GL_APIENTRY * glGetUniformBlockIndex)(GLuint program, const GLchar *uniformBlockName);
        void (GL_APIENTRY * glGetActiveUniformBlockiv)(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params);
        void (GL_APIENTRY * glGetActiveUniformBlockName)(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName);
        void (GL_APIENTRY * glUniformBlockBinding)(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
        void (GL_APIENTRY * glGetProgramBinary)(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, GLvoid *binary);
        void (GL_APIENTRY * glProgramBinary)(GLuint program, GLenum binaryFormat, const GLvoid *binary, GLsizei length);
        void (GL_APIENTRY * glUniform1d)(GLint location, GLdouble v0);
        void (GL_APIENTRY * glUniform2d)(GLint location, GLdouble v0, GLdouble v1);
        void (GL_APIENTRY * glUniform3d)(GLint location, GLdouble v0, GLdouble v1, GLdouble v2);
        void (GL_APIENTRY * glUniform4d)(GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3);
        void (GL_APIENTRY * glUniform1dv)(GLint location, GLsizei count, const GLdouble *value);
        void (GL_APIENTRY * glUniform2dv)(GLint location, GLsizei count, const GLdouble *value);
        void (GL_APIENTRY * glUniform3dv)(GLint location, GLsizei count, const GLdouble *value);
        void (GL_APIENTRY * glUniform4dv)(GLint location, GLsizei count, const GLdouble *value);
        void (GL_APIENTRY * glUniformMatrix2dv)(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
        void (GL_APIENTRY * glUniformMatrix3dv)(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
        void (GL_APIENTRY * glUniformMatrix4dv)(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
        void (GL_APIENTRY * glUniformMatrix2x3dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble* value );
        void (GL_APIENTRY * glUniformMatrix3x2dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble* value );
        void (GL_APIENTRY * glUniformMatrix2x4dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble* value );
        void (GL_APIENTRY * glUniformMatrix4x2dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble* value );
        void (GL_APIENTRY * glUniformMatrix3x4dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble* value );
        void (GL_APIENTRY * glUniformMatrix4x3dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble* value );
        void (GL_APIENTRY * glGetActiveAtomicCounterBufferiv)( GLuint program, GLuint bufferIndex, GLenum pname, GLint* params );
        void (GL_APIENTRY * glDispatchCompute)( GLuint numGroupsX, GLuint numGroupsY, GLuint numGroupsZ );

        // ARB_bindless_texture
        GLuint64 (GL_APIENTRY* glGetTextureHandle)(GLuint texture);
        GLuint64 (GL_APIENTRY* glGetTextureSamplerHandle)(GLuint texture, GLuint sampler);
        void (GL_APIENTRY* glMakeTextureHandleResident)(GLuint64 handle);
        void (GL_APIENTRY* glMakeTextureHandleNonResident)(GLuint64 handle);
        GLboolean (GL_APIENTRY* glIsTextureHandleResident)(GLuint64 handle);
        GLuint64 (GL_APIENTRY* glGetImageHandle)(GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format);
        void (GL_APIENTRY* glMakeImageHandleResident)(GLuint64 handle);
        void  (GL_APIENTRY* glMakeImageHandleNonResident)(GLuint64 handle);
        GLboolean (GL_APIENTRY* glIsImageHandleResident)(GLuint64 handle);
        void  (GL_APIENTRY* glUniformHandleui64)(GLint location, GLuint64 handle);
        void  (GL_APIENTRY* glUniformHandleuiv64)(GLint location, GLsizei count, GLuint64 *handles);
        void  (GL_APIENTRY* glProgramUniformHandleui64)(GLuint program, GLint location, GLuint64 handle);
        void  (GL_APIENTRY* glProgramUniformHandleuiv64)(GLuint program, GLint location, GLsizei count, GLuint64 *handles);

        // Buffer Object extensions
        bool isBufferObjectSupported;
        bool isVBOSupported;
        bool isPBOSupported;
        bool isTBOSupported;
        bool isVAOSupported;
        bool isTransformFeedbackSupported;

        void (GL_APIENTRY * glGenBuffers) (GLsizei n, GLuint *buffers);
        void (GL_APIENTRY * glBindBuffer) (GLenum target, GLuint buffer);
        void (GL_APIENTRY * glBufferData) (GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
        void (GL_APIENTRY * glBufferSubData) (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
        void (GL_APIENTRY * glDeleteBuffers) (GLsizei n, const GLuint *buffers);
        GLboolean (GL_APIENTRY * glIsBuffer) (GLuint buffer);
        void (GL_APIENTRY * glGetBufferSubData) (GLenum target, GLintptr offset, GLsizeiptr size, GLvoid *data);
        GLvoid* (GL_APIENTRY * glBufferStorage) (GLenum target, GLintptr size, const GLvoid * data, GLbitfield flags);
        GLvoid* (GL_APIENTRY * glNamedBufferStorage) (GLuint buffer, GLsizei size, const void * data, GLbitfield flags);
        GLvoid* (GL_APIENTRY * glMapBuffer) (GLenum target, GLenum access);
        GLvoid* (GL_APIENTRY * glMapBufferRange)(GLenum target,  GLintptr offset,  GLsizeiptr length,  GLbitfield access);
        GLboolean (GL_APIENTRY * glUnmapBuffer) (GLenum target);
        void (GL_APIENTRY * glGetBufferParameteriv) (GLenum target, GLenum pname, GLint *params);
        void (GL_APIENTRY * glGetBufferPointerv) (GLenum target, GLenum pname, GLvoid* *params);
        void (GL_APIENTRY * glBindBufferRange) (GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
        void (GL_APIENTRY * glBindBufferBase) (GLenum target, GLuint index, GLuint buffer);
        void (GL_APIENTRY * glTexBuffer) (GLenum target, GLenum internalFormat, GLuint buffer);

        void (GL_APIENTRY * glMemoryBarrier)( GLbitfield barriers );

        // BlendFunc extensions
        bool                isBlendFuncSeparateSupported;
        void (GL_APIENTRY * glBlendFuncSeparate) (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) ;

        void (GL_APIENTRY * glBlendFunci) (GLuint buf, GLenum src, GLenum dst);
        void (GL_APIENTRY * glBlendFuncSeparatei) (GLuint buf, GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) ;


        // Vertex Array extensions
        bool isSecondaryColorSupported;
        bool isFogCoordSupported;
        bool isMultiTexSupported;
        bool isOcclusionQuerySupported;
        bool isARBOcclusionQuerySupported;
        bool isTimerQuerySupported;
        bool isARBTimerQuerySupported;



        void (GL_APIENTRY * glDrawArraysInstanced)( GLenum mode, GLint first, GLsizei count, GLsizei primcount );
        void (GL_APIENTRY * glDrawElementsInstanced)( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount );

        void (GL_APIENTRY * glSecondaryColor3ubv) (const GLubyte* coord);
        void (GL_APIENTRY * glSecondaryColor3fv) (const GLfloat* coord);

        void (GL_APIENTRY * glFogCoordfv) (const GLfloat* coord);

        void (GL_APIENTRY * glMultiTexCoord1f) (GLenum target,GLfloat coord);
        void (GL_APIENTRY * glMultiTexCoord4f) (GLenum target, GLfloat x, GLfloat y, GLfloat z, GLfloat w);

        void (GL_APIENTRY * glMultiTexCoord1fv) (GLenum target,const GLfloat* coord);
        void (GL_APIENTRY * glMultiTexCoord2fv) (GLenum target,const GLfloat* coord);
        void (GL_APIENTRY * glMultiTexCoord3fv) (GLenum target,const GLfloat* coord);
        void (GL_APIENTRY * glMultiTexCoord4fv) (GLenum target,const GLfloat* coord);

        void (GL_APIENTRY * glMultiTexCoord1d) (GLenum target,GLdouble coord);
        void (GL_APIENTRY * glMultiTexCoord1dv) (GLenum target,const GLdouble* coord);
        void (GL_APIENTRY * glMultiTexCoord2dv) (GLenum target,const GLdouble* coord);
        void (GL_APIENTRY * glMultiTexCoord3dv) (GLenum target,const GLdouble* coord);
        void (GL_APIENTRY * glMultiTexCoord4dv) (GLenum target,const GLdouble* coord);

        // Occlusion Query extensions
        void (GL_APIENTRY * glGenOcclusionQueries) ( GLsizei n, GLuint *ids );
        void (GL_APIENTRY * glDeleteOcclusionQueries) ( GLsizei n, const GLuint *ids );
        GLboolean (GL_APIENTRY * glIsOcclusionQuery) ( GLuint id );
        void (GL_APIENTRY * glBeginOcclusionQuery) ( GLuint id );
        void (GL_APIENTRY * glEndOcclusionQuery) ();
        void (GL_APIENTRY * glGetOcclusionQueryiv) ( GLuint id, GLenum pname, GLint *params );
        void (GL_APIENTRY * glGetOcclusionQueryuiv) ( GLuint id, GLenum pname, GLuint *params );

        void (GL_APIENTRY * glGetQueryiv) (GLenum target, GLenum pname, GLint *params);
        void (GL_APIENTRY * glGenQueries) (GLsizei n, GLuint *ids);
        void (GL_APIENTRY * glBeginQuery) (GLenum target, GLuint id);
        void (GL_APIENTRY * glEndQuery) (GLenum target);
        void (GL_APIENTRY * glBeginQueryIndexed) (GLenum target, GLuint index, GLuint id);
        void (GL_APIENTRY * glEndQueryIndexed) (GLenum target, GLuint index);
        void (GL_APIENTRY * glQueryCounter) (GLuint id, GLenum target);
        GLboolean (GL_APIENTRY * glIsQuery) (GLuint id);
        void (GL_APIENTRY * glDeleteQueries) (GLsizei n, const GLuint *ids);
        void (GL_APIENTRY * glGetQueryObjectiv) (GLuint id, GLenum pname, GLint *params);
        void (GL_APIENTRY * glGetQueryObjectuiv) (GLuint id, GLenum pname, GLuint *params);
        void (GL_APIENTRY * glGetQueryObjectui64v) (GLuint id, GLenum pname, GLuint64 *params);
        void (GL_APIENTRY * glGetInteger64v) (GLenum pname, GLint64 *params);


        // SampleMaski functionality
        bool isOpenGL32upported;
        bool isTextureMultisampleSupported;
        bool isSampleMaskiSupported;

        void (GL_APIENTRY * glSampleMaski) (GLuint maskNumber, GLbitfield mask);

        // Vertex/Fragment Programs
        bool isVertexProgramSupported;
        bool isFragmentProgramSupported;

        void (GL_APIENTRY * glBindProgram) (GLenum target, GLuint id);
        void (GL_APIENTRY * glGenPrograms) (GLsizei n, GLuint *programs);
        void (GL_APIENTRY * glDeletePrograms) (GLsizei n, GLuint *programs);
        void (GL_APIENTRY * glProgramString) (GLenum target, GLenum format, GLsizei len, const void *string);
        void (GL_APIENTRY * glProgramLocalParameter4fv) (GLenum target, GLuint index, const GLfloat *params);

        // Sample Extensions (OpenGL>=3.3)
        void (GL_APIENTRY * glSamplerParameteri)(GLuint sampler, GLenum param, GLint value);
        void (GL_APIENTRY * glSamplerParameterf)(GLuint sampler, GLenum param, GLfloat value);
        void (GL_APIENTRY * glSamplerParameteriv)(GLuint sampler, GLenum param, GLint *value);
        void (GL_APIENTRY * glSamplerParameterfv)(GLuint sampler, GLenum param, GLfloat *value);
        void (GL_APIENTRY * glSamplerParameterIiv)(GLuint sampler, GLenum param, GLint *value);
        void (GL_APIENTRY * glSamplerParameterIuiv)(GLuint sampler, GLenum param, GLuint *value);

        void (GL_APIENTRY * glGetSamplerParameteriv)(GLuint sampler, GLenum param, GLint *value);
        void (GL_APIENTRY * glGetSamplerParameterfv)(GLuint sampler, GLenum param, GLfloat *value);
        void (GL_APIENTRY * glGetSamplerParameterIiv)(GLuint sampler, GLenum param, GLint *value);
        void (GL_APIENTRY * glGetSamplerParameterIuiv)(GLuint sampler, GLenum param, GLuint *value);

        void (GL_APIENTRY * glGenSamplers)(GLsizei size,GLuint * samplers);
        void (GL_APIENTRY * glDeleteSamplers)(GLsizei size,const GLuint * samplers);
        void (GL_APIENTRY * glBindSampler)(GLuint tu, GLuint sampler);
        GLboolean (GL_APIENTRY * glIsSampler)(GLuint id);

        // Texture Extensions
        bool isMultiTexturingSupported;
        bool isTextureFilterAnisotropicSupported;
        bool isTextureSwizzleSupported;
        bool isTextureCompressionARBSupported;
        bool isTextureCompressionS3TCSupported;
        bool isTextureCompressionPVRTC2BPPSupported;
        bool isTextureCompressionPVRTC4BPPSupported;
        bool isTextureCompressionETCSupported;
        bool isTextureCompressionETC2Supported;
        bool isTextureCompressionRGTCSupported;
        bool isTextureCompressionPVRTCSupported;
        bool isTextureMirroredRepeatSupported;
        bool isTextureEdgeClampSupported;
        bool isTextureBorderClampSupported;
        bool isGenerateMipMapSupported;
        bool preferGenerateMipmapSGISForPowerOfTwo;
        bool isTextureMultisampledSupported;
        bool isShadowSupported;
        bool isShadowAmbientSupported;
        bool isTextureMaxLevelSupported;
        GLint maxTextureSize;
        bool isClientStorageSupported;
        bool isTextureIntegerEXTSupported;
        bool isTextureStorageEnabled;

        bool isTexStorage2DSupported() const { return glTexStorage2D != 0; }
        bool isCompressedTexImage2DSupported() const { return glCompressedTexImage2D!=0; }
        bool isCompressedTexSubImage2DSupported() const { return glCompressedTexSubImage2D!=0; }
        bool isBindImageTextureSupported() const { return glBindImageTexture!=0; }
        bool isNonPowerOfTwoTextureMipMappedSupported;
        bool isNonPowerOfTwoTextureNonMipMappedSupported;
        bool isNonPowerOfTwoTextureSupported(GLenum filter) const
        {
            return (filter==GL_LINEAR || filter==GL_NEAREST) ?
                    isNonPowerOfTwoTextureNonMipMappedSupported :
                    isNonPowerOfTwoTextureMipMappedSupported;
        }

        ///immutable texture storage and texture view
        void (GL_APIENTRY * glTexStorage1D) (GLenum target, GLsizei numMipmapLevels, GLenum internalformat, GLsizei width);
        void (GL_APIENTRY * glTextureStorage1D) (GLuint texture, GLsizei numMipmapLevels, GLenum internalformat, GLsizei width);
        void (GL_APIENTRY * glTexStorage2D) (GLenum target, GLsizei numMipmapLevels, GLenum internalformat, GLsizei width, GLsizei height);
        void (GL_APIENTRY * glTextureStorage2D) (GLuint texture, GLsizei numMipmapLevels, GLenum internalformat, GLsizei width, GLsizei height);
        void (GL_APIENTRY * glTexStorage3D) ( GLenum target, GLsizei numMipmapLevels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
        void (GL_APIENTRY * glTextureStorage3D) ( GLuint texture, GLsizei numMipmapLevels,GLenum internalformat,GLsizei width,GLsizei height,GLsizei depth);
        void (GL_APIENTRY * glTexStorage2DMultisample) ( GLenum target, GLsizei numSamples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
        void (GL_APIENTRY * glTexStorage3DMultisample) ( GLenum target, GLsizei numSamples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);
        void (GL_APIENTRY * glTextureView) ( GLuint texture, GLenum target, GLuint orig, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers);

        void (GL_APIENTRY * glCompressedTexImage2D) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
        void (GL_APIENTRY * glCompressedTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data);
        void (GL_APIENTRY * glGetCompressedTexImage) (GLenum target, GLint level, GLvoid *data);
        void (GL_APIENTRY * glTexImage2DMultisample) (GLenum target, GLsizei samples, GLint internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
        void (GL_APIENTRY * glTexParameterIiv) (GLenum target, GLenum pname, const GLint* data);
        void (GL_APIENTRY * glTexParameterIuiv) (GLenum target, GLenum pname, const GLuint* data);
        void (GL_APIENTRY * glBindImageTexture) (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);


        // Texture3D extensions
        bool isTexture3DSupported;
        bool isTexture3DFast;
        GLint maxTexture3DSize;
        bool isCompressedTexImage3DSupported() const { return glCompressedTexImage3D!=0; }
        bool isCompressedTexSubImage3DSupported() const { return glCompressedTexSubImage3D!=0; }

        void (GL_APIENTRY * glTexImage3D) ( GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
        void (GL_APIENTRY * glTexSubImage3D) ( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);

        void (GL_APIENTRY * glCopyTexSubImage3D) ( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
        void (GL_APIENTRY * glCompressedTexImage3D) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data);
        void (GL_APIENTRY * glCompressedTexSubImage3D) ( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data );
        void (GL_APIENTRY *glTexImage3DMultisample) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);
        void (GL_APIENTRY *glGetMultisamplefv) (GLenum pname, GLuint index, GLfloat *val);

        // Texture2DArray extensions
        bool isTexture2DArraySupported;
        GLint maxLayerCount;
        GLint max2DSize;


        // Blending
        bool isBlendColorSupported;
        bool isBlendEquationSupported;
        bool isBlendEquationSeparateSupported;
        bool isSGIXMinMaxSupported;
        bool isLogicOpSupported;

        void (GL_APIENTRY * glBlendColor) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
        void (GL_APIENTRY * glBlendEquation)(GLenum mode);
        void (GL_APIENTRY * glBlendEquationSeparate)(GLenum modeRGB, GLenum modeAlpha);
        void (GL_APIENTRY * glBlendEquationi)(GLuint buf,  GLenum mode);
        void (GL_APIENTRY * glBlendEquationSeparatei)(GLuint buf, GLenum modeRGB, GLenum modeAlpha);


        // glEnablei/glDisabeli
        void (GL_APIENTRY * glEnablei) (GLenum capability, GLuint buf);
        void (GL_APIENTRY * glDisablei) (GLenum capability, GLuint buf);


        // Stencil
        bool isStencilWrapSupported;
        bool isStencilTwoSidedSupported;
        bool isOpenGL20Supported;
        bool isSeparateStencilSupported;

        void (GL_APIENTRY * glActiveStencilFace) (GLenum face);
        void (GL_APIENTRY * glStencilOpSeparate) (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
        void (GL_APIENTRY * glStencilMaskSeparate) (GLenum face, GLuint mask);
        void (GL_APIENTRY * glStencilFuncSeparate) (GLenum face, GLenum func, GLint ref, GLuint mask);
        void (GL_APIENTRY * glStencilFuncSeparateATI) (GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);


        // ColorMask
        void (GL_APIENTRY * glColorMaski)(GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);


        // ClampColor
        bool isClampColorSupported;
        void (GL_APIENTRY * glClampColor) (GLenum target, GLenum mode);


        // PrimitiveRestartIndex
        void (GL_APIENTRY * glPrimitiveRestartIndex) ( GLuint index );


        // Mutlisample
        bool isMultisampleSupported;
        bool isMultisampleFilterHintSupported;

        void (GL_APIENTRY * glSampleCoverage) (GLfloat value, GLboolean invert);


        // Point
        bool isPointParametersSupported;
        bool isPointSpriteSupported;
        bool isPointSpriteModeSupported;
        bool isPointSpriteCoordOriginSupported;

        void (GL_APIENTRY * glPointParameteri) (GLenum pname, GLint param);
        void (GL_APIENTRY * glPointParameterf) (GLenum pname, GLfloat param);
        void (GL_APIENTRY * glPointParameterfv) (GLenum pname, const GLfloat *params);


        // FrameBuferObject
        bool isFrameBufferObjectSupported;
        bool isPackedDepthStencilSupported;
        bool isRenderbufferMultisampleSupported() const { return glRenderbufferStorageMultisample != 0; }
        bool isRenderbufferMultisampleCoverageSupported() const { return glRenderbufferStorageMultisampleCoverageNV != 0; }

        void (GL_APIENTRY * glBindRenderbuffer) (GLenum, GLuint);
        void (GL_APIENTRY * glDeleteRenderbuffers) (GLsizei n, const GLuint *renderbuffers);
        void (GL_APIENTRY * glGenRenderbuffers) (GLsizei, GLuint *);
        void (GL_APIENTRY * glRenderbufferStorage) (GLenum, GLenum, GLsizei, GLsizei);
        void (GL_APIENTRY * glRenderbufferStorageMultisample) (GLenum, GLsizei, GLenum, GLsizei, GLsizei);
        void (GL_APIENTRY * glRenderbufferStorageMultisampleCoverageNV) (GLenum, GLsizei, GLsizei, GLenum, GLsizei, GLsizei);
        void (GL_APIENTRY * glBindFramebuffer) (GLenum, GLuint);
        void (GL_APIENTRY * glDeleteFramebuffers) (GLsizei n, const GLuint *framebuffers);
        void (GL_APIENTRY * glGenFramebuffers) (GLsizei, GLuint *);
        GLenum (GL_APIENTRY * glCheckFramebufferStatus) (GLenum);

        void (GL_APIENTRY * glFramebufferTexture1D) (GLenum, GLenum, GLenum, GLuint, GLint);
        void (GL_APIENTRY * glFramebufferTexture2D) (GLenum, GLenum, GLenum, GLuint, GLint);
        void (GL_APIENTRY * glFramebufferTexture3D) (GLenum, GLenum, GLenum, GLuint, GLint, GLint);
        void (GL_APIENTRY * glFramebufferTexture) (GLenum, GLenum, GLint, GLint);
        void (GL_APIENTRY * glFramebufferTextureLayer) (GLenum, GLenum, GLuint, GLint, GLint);
        void (GL_APIENTRY * glFramebufferTextureFace)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face );
        void (GL_APIENTRY * glFramebufferRenderbuffer) (GLenum, GLenum, GLenum, GLuint);

        void (GL_APIENTRY * glGenerateMipmap) (GLenum);
        void (GL_APIENTRY * glBlitFramebuffer) (GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
        void (GL_APIENTRY * glGetRenderbufferParameteriv) (GLenum, GLenum, GLint*);

        //ARB_framebuffer_no_attachments
        void (GL_APIENTRY * glFramebufferParameteri)(GLenum target, GLenum pname, GLint param);
        void (GL_APIENTRY * glGetFramebufferParameteriv)(GLenum target, GLenum pname, GLint *params);
        void (GL_APIENTRY * glNamedFramebufferParameteri)(GLuint fbo, GLenum pname, GLint param);
        void (GL_APIENTRY * glGetNamedFramebufferParameteriv)(GLuint fbo, GLenum pname, GLint *params);

        //subroutine
        GLint(GL_APIENTRY* glGetSubroutineUniformLocation) (GLuint program, GLenum shadertype, const GLchar *name);
        void (GL_APIENTRY * glGetActiveSubroutineUniformName) (GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei *length, GLchar *name);
        void (GL_APIENTRY * glGetActiveSubroutineUniformiv) (GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint *values);
        GLuint (GL_APIENTRY *  glGetSubroutineIndex) (GLuint program, GLenum shadertype, const GLchar *name);
        void (GL_APIENTRY * glGetActiveSubroutineName) (GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei *length, GLchar *name);
        void (GL_APIENTRY * glGetProgramStageiv) (GLuint program, GLenum shadertype, GLenum pname, GLint *values);
        void (GL_APIENTRY *glUniformSubroutinesuiv) (GLenum shadertype, GLsizei count,const GLuint * indices);
        void (GL_APIENTRY * glGetUniformSubroutineuiv) (GLenum shadertype, GLint location, GLuint *params);

        // Sync
        GLsync (GL_APIENTRY * glFenceSync) (GLenum condition, GLbitfield flags);
        GLboolean (GL_APIENTRY * glIsSync) (GLsync sync);
        void (GL_APIENTRY * glDeleteSync) (GLsync sync);
        GLenum (GL_APIENTRY * glClientWaitSync) (GLsync sync, GLbitfield flags, GLuint64 timeout);
        void (GL_APIENTRY * glWaitSync) (GLsync sync, GLbitfield flags, GLuint64 timeout);
        void (GL_APIENTRY * glGetSynciv) (GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);

        // Indirect Rendering
        void (GL_APIENTRY * glDrawArraysIndirect) (GLenum  mode,  const void * indirect);
        void (GL_APIENTRY * glMultiDrawArraysIndirect) (GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride);
        void (GL_APIENTRY * glDrawElementsIndirect) (GLenum mode,  GLenum type,  const void *indirect);
        void (GL_APIENTRY * glMultiDrawElementsIndirect)(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride);

        // ARB_sparse_texture
        void (GL_APIENTRY * glTexPageCommitment) (GLenum target, GLint level,GLint xoffset,GLint yoffset,GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,GLboolean commit);

        // Transform feedback
        void (GL_APIENTRY * glBeginTransformFeedback) (GLenum primitiveMode);
        void (GL_APIENTRY * glEndTransformFeedback) (void);
        void (GL_APIENTRY * glTransformFeedbackVaryings) (GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode);
        void (GL_APIENTRY * glGetTransformFeedbackVarying) (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name);
        void (GL_APIENTRY * glBindTransformFeedback) (GLenum target, GLuint id);
        void (GL_APIENTRY * glDeleteTransformFeedbacks) (GLsizei n, const GLuint *ids);
        void (GL_APIENTRY * glGenTransformFeedbacks) (GLsizei n, GLuint *ids);
        GLboolean (GL_APIENTRY * glIsTransformFeedback) (GLuint id);
        void (GL_APIENTRY * glPauseTransformFeedback) (void);
        void (GL_APIENTRY * glResumeTransformFeedback) (void);
        void (GL_APIENTRY * glDrawTransformFeedback) (GLenum mode, GLuint id);
        void (GL_APIENTRY * glDrawTransformFeedbackStream) (GLenum mode, GLuint id, GLuint stream);
        void (GL_APIENTRY * glDrawTransformFeedbackInstanced) (GLenum mode, GLuint id, GLsizei instancecount);
        void (GL_APIENTRY * glDrawTransformFeedbackStreamInstanced) (GLenum mode, GLuint id, GLuint stream, GLsizei instancecount);
        void (GL_APIENTRY * glCreateTransformFeedbacks) (GLsizei n, GLuint *ids);
        void (GL_APIENTRY * glTransformFeedbackBufferBase) (GLuint xfb, GLuint index, GLuint buffer);
        void (GL_APIENTRY * glTransformFeedbackBufferRange) (GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizei size);
        void (GL_APIENTRY * glGetTransformFeedbackiv) (GLuint xfb, GLenum pname, GLint *param);
        void (GL_APIENTRY * glGetTransformFeedbacki_v) (GLuint xfb, GLenum pname, GLuint index, GLint *param);
        void (GL_APIENTRY * glGetTransformFeedbacki64_v) (GLuint xfb, GLenum pname, GLuint index, GLint64 *param);

        // Vertex Array Object
        void (GL_APIENTRY * glDeleteVertexArrays) (GLsizei size,const GLuint *handles);
        void (GL_APIENTRY * glGenVertexArrays) (GLsizei size, GLuint *handles);
        GLboolean (GL_APIENTRY * glIsVertexArray) (GLuint handle);
        void (GL_APIENTRY * glBindVertexArray) (GLuint handle);

        // OpenGL 4.3 / ARB_vertex_attrib_binding
        bool isVertexAttribBindingSupported;
        void (GL_APIENTRY * glBindVertexBuffer)(GLuint bindingindex, GLuint buffer, GLintptr offset, GLintptr stride);
        void (GL_APIENTRY * glVertexArrayVertexBuffer)(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride);
        void (GL_APIENTRY * glVertexAttribBinding)(GLuint attribindex, GLuint bindingindex);
        void (GL_APIENTRY * glVertexArrayAttribBinding)(GLuint vaobj, GLuint attribindex, GLuint bindingindex);

        void (GL_APIENTRY * glVertexAttribFormat)( GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset);
        void (GL_APIENTRY * glVertexAttribIFormat)( GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
        void (GL_APIENTRY * glVertexAttribLFormat)( GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
        void (GL_APIENTRY * glVertexArrayAttribFormat)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset);
        void (GL_APIENTRY * glVertexArrayAttribIFormat)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
        void (GL_APIENTRY * glVertexArrayAttribLFormat)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);

        // MultiDrawArrays
        void (GL_APIENTRY * glMultiDrawArrays) (GLenum mode, const GLint * first, const GLsizei * count, GLsizei primcount);
        void (GL_APIENTRY * glMultiDrawElements) (GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount);
        void (GL_APIENTRY * glMultiDrawElementsBaseVertex) (GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount, const GLint *basevertex);


        void (GL_APIENTRY * glDrawRangeElements) ( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices );
        void (GL_APIENTRY * glDrawElementsBaseVertex) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex);
        void (GL_APIENTRY * glDrawElementsInstancedBaseVertex) (GLenum mode, GLsizei count, GLenum type, const void *indices,GLsizei primcount, GLint basevertex);

        void (GL_APIENTRY * glDrawRangeElementsBaseVertex) (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex);
        void (GL_APIENTRY * glProvokingVertex) (GLenum mode);

        void (GL_APIENTRY * glBeginConditionalRender) (GLuint id, GLenum mode);
        void (GL_APIENTRY * glEndConditionalRender) (void);


        void (GL_APIENTRY *glDrawArraysInstancedBaseInstance) (GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance);
        void (GL_APIENTRY * glDrawElementsInstancedBaseInstance) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance);
        void (GL_APIENTRY *glDrawElementsInstancedBaseVertexBaseInstance) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance);


        // ViewportArray
        bool isViewportArraySupported;

        void (GL_APIENTRY * glViewportArrayv) (GLuint first, GLsizei count, const GLfloat * v);
        void (GL_APIENTRY * glViewportIndexedf) (GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h);
        void (GL_APIENTRY * glViewportIndexedfv) (GLuint index, const GLfloat * v);
        void (GL_APIENTRY * glScissorArrayv) (GLuint first, GLsizei count, const GLint * v);
        void (GL_APIENTRY * glScissorIndexed) (GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height);
        void (GL_APIENTRY * glScissorIndexedv) (GLuint index, const GLint * v);
        void (GL_APIENTRY * glDepthRangeArrayv) (GLuint first, GLsizei count, const GLdouble * v);
        void (GL_APIENTRY * glDepthRangeIndexed) (GLuint index, GLdouble n, GLdouble f);
        void (GL_APIENTRY * glDepthRangeIndexedf) (GLuint index, GLfloat n, GLfloat f);
        void (GL_APIENTRY * glGetFloati_v) (GLenum target, GLuint index, GLfloat *data);
        void (GL_APIENTRY * glGetDoublei_v) (GLenum target, GLuint index, GLdouble *data);

        //introduced by other OpenGL extensions such as EXT_draw_buffers2
        void (GL_APIENTRY * glGetIntegerIndexedvEXT) (GLenum target, GLuint index, int * v);
        void (GL_APIENTRY * glEnableIndexedEXT) (GLenum target, GLuint index);
        void (GL_APIENTRY * glDisableIndexedEXT) (GLenum target, GLuint index);
        GLboolean (GL_APIENTRY * glIsEnabledIndexedEXT) (GLenum target, GLuint index);

        void (GL_APIENTRY * glClientActiveTexture) (GLenum texture);
        void (GL_APIENTRY * glActiveTexture) (GLenum texture);
        void (GL_APIENTRY * glFogCoordPointer) (GLenum type, GLsizei stride, const GLvoid *pointer);
        void (GL_APIENTRY * glSecondaryColorPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);

        GLint  glMaxTextureCoords;
        GLint  glMaxTextureUnits;

        // debug extensions
        void (GL_APIENTRY * glObjectLabel) (GLenum identifier, GLuint name, GLsizei length, const GLchar* label);

        /** convenience wrapper around glObjectLabel that calls glObjectLabel if it's supported and using std::string as a label parameter.*/
        void debugObjectLabel(GLenum identifier, GLuint name, const std::string& label) const { if (glObjectLabel && !label.empty()) glObjectLabel(identifier, name, label.size(), label.c_str()); }
};


}

#endif
