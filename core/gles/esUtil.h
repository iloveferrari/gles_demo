// The MIT License (MIT)
//
// Copyright (c) 2013 Dan Ginsburg, Budirijanto Purnomo
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


/// \file ESUtil.h
/// \brief A utility library for OpenGL ES.  This library provides a
///        basic common framework for the example applications in the
///        OpenGL ES 3.0 Programming Guide.
//
#ifndef ESUTIL_H
#define ESUTIL_H

///
//  Includes
//
#include <Input.h>

#include <stdlib.h>
#include <glm/glm.hpp>

#ifdef __APPLE__
#include <OpenGLES/ES3/gl.h>
#else
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif
#ifdef __cplusplus

extern "C" {
#endif


	///
	//  Macros
	//
#ifdef WIN32
#define ESUTIL_API  __cdecl
#define ESCALLBACK  __cdecl
#else
#define ESUTIL_API
#define ESCALLBACK
#endif

#ifdef _WIN64
#define GWL_USERDATA GWLP_USERDATA
#endif


	/// esCreateWindow flag - RGB color buffer
#define ES_WINDOW_RGB           0
	/// esCreateWindow flag - ALPHA color buffer
#define ES_WINDOW_ALPHA         1
	/// esCreateWindow flag - depth buffer
#define ES_WINDOW_DEPTH         2
	/// esCreateWindow flag - stencil buffer
#define ES_WINDOW_STENCIL       4
	/// esCreateWindow flat - multi-sample buffer
#define ES_WINDOW_MULTISAMPLE   8


	///
	// Types
	//
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

	typedef struct
	{
		GLfloat   m[4][4];
	} ESMatrix;

	typedef struct
	{
		GLfloat   m[16];
	} Matrix;

	typedef struct ESContext ESContext;

	struct ESContext
	{
		/// Put platform specific data here
		void       *platformData;

		/// Put your user data here...
		void       *userData;

		/// Window width
		GLint       width;

		/// Window height
		GLint       height;

		glm::mat4   camera_matrix;
		glm::mat4   mode_view_matrix;
		glm::mat4   perspective_matrix;
		glm::mat4   mvp_matrix;

#ifndef __APPLE__
		/// Display handle
		EGLNativeDisplayType eglNativeDisplay;

		/// Window handle
		EGLNativeWindowType  eglNativeWindow;

		/// EGL display
		EGLDisplay  eglDisplay;

		/// EGL context
		EGLContext  eglContext;

		/// EGL surface
		EGLSurface  eglSurface;
#endif

		/// Callbacks
		void (ESCALLBACK *drawFunc) (ESContext *);
		void (ESCALLBACK *shutdownFunc) (ESContext *);
		void (ESCALLBACK *keyFunc) (ESContext *, unsigned char, int, int);
		void (ESCALLBACK *updateFunc) (ESContext *, float deltaTime);
		void (ESCALLBACK *touchFunc) (ESContext *, int, int, int);
	};


	///
	//  Public Functions
	//

	//
	/// \brief Create a window with the specified parameters
	/// \param esContext Application context
	/// \param title Name for title bar of window
	/// \param width Width in pixels of window to create
	/// \param height Height in pixels of window to create
	/// \param flags Bitfield for the window creation flags
	///         ES_WINDOW_RGB     - specifies that the color buffer should have R,G,B channels
	///         ES_WINDOW_ALPHA   - specifies that the color buffer should have alpha
	///         ES_WINDOW_DEPTH   - specifies that a depth buffer should be created
	///         ES_WINDOW_STENCIL - specifies that a stencil buffer should be created
	///         ES_WINDOW_MULTISAMPLE - specifies that a multi-sample buffer should be created
	/// \return GL_TRUE if window creation is succesful, GL_FALSE otherwise
	GLboolean ESUTIL_API esCreateWindow(ESContext *esContext, const char *title, GLint width, GLint height, GLuint flags);

	//
	/// \brief Register a draw callback function to be used to render each frame
	/// \param esContext Application context
	/// \param drawFunc Draw callback function that will be used to render the scene
	//
	void ESUTIL_API esRegisterDrawFunc(ESContext *esContext, void (ESCALLBACK *drawFunc) (ESContext *));

	//
	/// \brief Register a callback function to be called on shutdown
	/// \param esContext Application context
	/// \param shutdownFunc Shutdown callback function
	//
	void ESUTIL_API esRegisterShutdownFunc(ESContext *esContext, void (ESCALLBACK *shutdownFunc) (ESContext *));

	//
	/// \brief Register an update callback function to be used to update on each time step
	/// \param esContext Application context
	/// \param updateFunc Update callback function that will be used to render the scene
	//
	void ESUTIL_API esRegisterUpdateFunc(ESContext *esContext, void (ESCALLBACK *updateFunc) (ESContext *, float));

	//
	/// \brief Register an keyboard input processing callback function
	/// \param esContext Application context
	/// \param keyFunc Key callback function for application processing of keyboard input
	//
	void ESUTIL_API esRegisterKeyFunc(ESContext *esContext,
		void (ESCALLBACK *drawFunc) (ESContext *, unsigned char, int, int));

	void ESUTIL_API esRegisterTouchEventFunc(ESContext *esContext,
		void (ESCALLBACK *touchFunc) (ESContext *, int, int, int));
	//
	/// \brief Log a message to the debug output for the platform
	/// \param formatStr Format string for error log.
	//
	void ESUTIL_API esLogMessage(const char *formatStr, ...);

#ifdef WIN32_LEAN_AND_MEAN
	LRESULT WINAPI ESWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
	GLboolean WinCreate(ESContext *esContext, const char *title);
	void esStartLoop(ESContext *esContext);
	//
	///
	/// \brief Load a shader, check for compile errors, print error messages to output log
	/// \param type Type of shader (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
	/// \param shaderSrc Shader source string
	/// \return A new shader object on success, 0 on failure
	//
	GLuint ESUTIL_API esLoadShader(GLenum type, const char *shaderSrc);

	//
	///
	/// \brief Load a vertex and fragment shader, create a program object, link program.
	///        Errors output to log.
	/// \param vertShaderSrc Vertex shader source code
	/// \param fragShaderSrc Fragment shader source code
	/// \return A new program object linked with the vertex/fragment shader pair, 0 on failure
	//
	GLuint ESUTIL_API esLoadProgram(const char *vertShaderSrc, const char *fragShaderSrc);

	//
	/// \brief Loads a 8-bit, 24-bit or 32-bit TGA image from a file
	/// \param ioContext Context related to IO facility on the platform
	/// \param fileName Name of the file on disk
	/// \param width Width of loaded image in pixels
	/// \param height Height of loaded image in pixels
	///  \return Pointer to loaded image.  NULL on failure.
	//
	char *ESUTIL_API esLoadTGA(void *ioContext, const char *fileName, int *width, int *height);

	//
	/// \brief multiply matrix specified by result with a perspective matrix and return new matrix in result
	/// \param result Specifies the input matrix.  new matrix is returned in result.
	/// \param left, right Coordinates for the left and right vertical clipping planes
	/// \param bottom, top Coordinates for the bottom and top horizontal clipping planes
	/// \param nearZ, farZ Distances to the near and far depth clipping planes.  These values are negative if plane is behind the viewer
	//
	void ESUTIL_API esOrtho(ESMatrix *result, float left, float right, float bottom, float top, float nearZ, float farZ);


	GLuint loadTexture(const char *filename, int *width, int *height);
	unsigned char *loadPng(const char *filename, int *width, int *height);

#ifdef __cplusplus
}
#endif

#endif // ESUTIL_H
