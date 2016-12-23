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

///
//  Includes
//
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "esUtil.h"

#include <libpng/png.h>

#define PI 3.1415926535897932384626433832795f

#ifdef ANDROID
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
typedef AAsset esFile;
#else
typedef FILE esFile;
#endif

#ifdef __APPLE__
#include "FileWrapper.h"
#endif

///
//  Macros
//
#define INVERTED_BIT            (1 << 5)

///
//  Types
//
#ifndef __APPLE__
#pragma pack(push,x1)                            // Byte alignment (8-bit)
#pragma pack(1)
#endif

typedef struct
#ifdef __APPLE__
__attribute__((packed))
#endif
{
	unsigned char  IdSize,
		MapType,
		ImageType;
	unsigned short PaletteStart,
		PaletteSize;
	unsigned char  PaletteEntryDepth;
	unsigned short X,
		Y,
		Width,
		Height;
	unsigned char  ColorDepth,
		Descriptor;

} TGA_HEADER;

#ifndef __APPLE__
#pragma pack(pop,x1)
#endif

#ifndef __APPLE__

///
// GetContextRenderableType()
//
//    Check whether EGL_KHR_create_context extension is supported.  If so,
//    return EGL_OPENGL_ES3_BIT_KHR instead of EGL_OPENGL_ES2_BIT
//
EGLint GetContextRenderableType(EGLDisplay eglDisplay)
{
#ifdef EGL_KHR_create_context
	const char *extensions = eglQueryString(eglDisplay, EGL_EXTENSIONS);

	// check whether EGL_KHR_create_context is in the extension string
	if (extensions != NULL && strstr(extensions, "EGL_KHR_create_context"))
	{
		// extension is supported
		return EGL_OPENGL_ES3_BIT_KHR;
	}
#endif
	// extension is not supported
	return EGL_OPENGL_ES2_BIT;
}
#endif

#ifdef WIN32_LEAN_AND_MEAN
LRESULT WINAPI ESWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT  lRet = 1;

	switch (uMsg)
	{
	case WM_CREATE:
		break;

	case WM_LBUTTONDOWN:
	{
		POINT      point;
		ESContext *esContext = (ESContext *)(LONG_PTR)GetWindowLongPtr(hWnd, GWL_USERDATA);

		GetCursorPos(&point);
		ScreenToClient(hWnd, &point);

		Input::getInstance()->updateKeys(LEFT_CLICK, true);

		break;
	}

	case WM_LBUTTONUP:
	{
		POINT      point;
		ESContext *esContext = (ESContext *)(LONG_PTR)GetWindowLongPtr(hWnd, GWL_USERDATA);

		GetCursorPos(&point);
		ScreenToClient(hWnd, &point);

		Input::getInstance()->updateKeys(LEFT_CLICK, false);

		break;
	}

	case WM_RBUTTONDOWN:
	{
		POINT      point;
		ESContext *esContext = (ESContext *)(LONG_PTR)GetWindowLongPtr(hWnd, GWL_USERDATA);

		GetCursorPos(&point);
		ScreenToClient(hWnd, &point);

		Input::getInstance()->updateKeys(RIGHT_CLICK, true);

		break;
	}

	case WM_RBUTTONUP:
	{
		POINT      point;
		ESContext *esContext = (ESContext *)(LONG_PTR)GetWindowLongPtr(hWnd, GWL_USERDATA);

		GetCursorPos(&point);
		ScreenToClient(hWnd, &point);

		Input::getInstance()->updateKeys(RIGHT_CLICK, false);

		break;
	}

	case WM_MOUSEMOVE:
	{
		POINT      point;
		ESContext *esContext = (ESContext *)(LONG_PTR)GetWindowLongPtr(hWnd, GWL_USERDATA);

		GetCursorPos(&point);
		ScreenToClient(hWnd, &point);

		Input::getInstance()->updateAxis((int)point.x, (int)point.y);

		break;
	}

	case WM_PAINT:
	{
	}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_KEYUP:
	{
		char ascii_code = wParam;        //�ɿ���ASCII��
		unsigned int key_state = lParam; //��ȡ���µļ�״̬

		if (ascii_code == 'w' || 
			ascii_code == 'W' ||
			ascii_code == 'S' ||
			ascii_code == 's' ||
			ascii_code == 'A' ||
			ascii_code == 'a' || 
			ascii_code == 'D' || 
			ascii_code == 'd' ||
			ascii_code == 'Z' || 
			ascii_code == 'z' ||
			ascii_code == 'X' || 
			ascii_code == 'x')
		{
			Input::getInstance()->updateMoveDirection(NOINPUT);
		}

		break;
	}

	case WM_CHAR:
	{
		POINT      point;
		ESContext *esContext = (ESContext *)(LONG_PTR)GetWindowLongPtr(hWnd, GWL_USERDATA);

		char ascii_code = wParam; //��ȡ���µ�ASCII��
		unsigned int key_state = lParam; //��ȡ���µļ�״̬

		//printf("%c ", ascii_code);

		if (ascii_code == 'w')
		{
			Input::getInstance()->updateMoveDirection(FORWARD);
		}
		else if (ascii_code == 's')
		{
			Input::getInstance()->updateMoveDirection(BACK);
		}
		else if (ascii_code == 'a')
		{
			Input::getInstance()->updateMoveDirection(LEFT);
		}
		else if (ascii_code == 'd')
		{
			Input::getInstance()->updateMoveDirection(RIGHT);
		}
		else if (ascii_code == 'z')
		{
			Input::getInstance()->updateMoveDirection(DOWN);
		}
		else if (ascii_code == 'x')
		{
			Input::getInstance()->updateMoveDirection(UP);
		}

		break;
	}

	default:
		lRet = DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	}

	return lRet;
}
#endif

///
//  WinLoop()
//
//      Start main windows loop
//
void esStartLoop(ESContext *esContext)
{
#ifdef WIN32_LEAN_AND_MEAN
	MSG msg = { 0 };
	int done = 0;
	DWORD lastTime = GetTickCount();

	while (!done)
	{
		int gotMsg = (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0);
		DWORD curTime = GetTickCount();
		float deltaTime = (float)(curTime - lastTime) / 1000.0f;
		lastTime = curTime;

		if (gotMsg)
		{
			if (msg.message == WM_QUIT)
			{
				done = 1;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		//else
		//{
			//SendMessage(esContext->eglNativeWindow, WM_PAINT, 0, 0);
		//}

		// Call update function if registered
		if (esContext->updateFunc != NULL)
		{
			esContext->updateFunc(esContext, deltaTime);
		}
	}
#endif
}

///
//  WinCreate()
//
//      Create Win32 instance and window
//
GLboolean WinCreate(ESContext *esContext, const char *title)
{
#ifdef ANDROID
#elif __APPLE__
#elif WIN32_LEAN_AND_MEAN
	WNDCLASS wndclass = { 0 };
	DWORD    wStyle = 0;
	RECT     windowRect;
	HINSTANCE hInstance = GetModuleHandle(NULL);


	wndclass.style = CS_OWNDC;
	wndclass.lpfnWndProc = (WNDPROC)ESWindowProc;
	wndclass.hInstance = hInstance;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclass.lpszClassName = "opengles3.0";

	if (!RegisterClass(&wndclass))
	{
		return FALSE;
	}

	wStyle = WS_VISIBLE | WS_POPUP | WS_BORDER | WS_SYSMENU | WS_CAPTION;

	// Adjust the window rectangle so that the client area has
	// the correct number of pixels
	windowRect.left = 0;
	windowRect.top = 0;
	windowRect.right = esContext->width;
	windowRect.bottom = esContext->height;

	AdjustWindowRect(&windowRect, wStyle, FALSE);



	esContext->eglNativeWindow = CreateWindow(
		"opengles3.0",
		title,
		wStyle,
		0,
		0,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		NULL,
		hInstance,
		NULL);

	// Set the ESContext* to the GWL_USERDATA so that it is available to the
	// ESWindowProc
	SetWindowLongPtr(esContext->eglNativeWindow, GWL_USERDATA, (LONG)(LONG_PTR)esContext);


	if (esContext->eglNativeWindow == NULL)
	{
		return GL_FALSE;
	}

	ShowWindow(esContext->eglNativeWindow, TRUE);

#endif

	return GL_TRUE;
}

//////////////////////////////////////////////////////////////////
//
//  Public Functions
//
//

///
//  esCreateWindow()
//
//      title - name for title bar of window
//      width - width of window to create
//      height - height of window to create
//      flags  - bitwise or of window creation flags
//          ES_WINDOW_ALPHA       - specifies that the framebuffer should have alpha
//          ES_WINDOW_DEPTH       - specifies that a depth buffer should be created
//          ES_WINDOW_STENCIL     - specifies that a stencil buffer should be created
//          ES_WINDOW_MULTISAMPLE - specifies that a multi-sample buffer should be created
//
GLboolean ESUTIL_API esCreateWindow(ESContext *esContext, const char *title, GLint width, GLint height, GLuint flags)
{
#ifndef __APPLE__
	EGLConfig config;
	EGLint majorVersion;
	EGLint minorVersion;
	EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };

	if (esContext == NULL)
	{
		return GL_FALSE;
	}

#ifdef ANDROID
	// For Android, get the width/height from the window rather than what the
	// application requested.
	esContext->width = ANativeWindow_getWidth(esContext->eglNativeWindow);
	esContext->height = ANativeWindow_getHeight(esContext->eglNativeWindow);
#else
	esContext->width = width;
	esContext->height = height;
#endif

	if (!WinCreate(esContext, title))
	{
		return GL_FALSE;
	}

	esContext->eglDisplay = eglGetDisplay(esContext->eglNativeDisplay);
	if (esContext->eglDisplay == EGL_NO_DISPLAY)
	{
		return GL_FALSE;
	}

	// Initialize EGL
	if (!eglInitialize(esContext->eglDisplay, &majorVersion, &minorVersion))
	{
		return GL_FALSE;
	}

	{
		EGLint numConfigs = 0;
		EGLint attribList[] =
		{
			EGL_RED_SIZE, 5,
			EGL_GREEN_SIZE, 6,
			EGL_BLUE_SIZE, 5,
			EGL_ALPHA_SIZE, (flags & ES_WINDOW_ALPHA) ? 8 : EGL_DONT_CARE,
			EGL_DEPTH_SIZE, (flags & ES_WINDOW_DEPTH) ? 8 : EGL_DONT_CARE,
			EGL_STENCIL_SIZE, (flags & ES_WINDOW_STENCIL) ? 8 : EGL_DONT_CARE,
			EGL_SAMPLE_BUFFERS, (flags & ES_WINDOW_MULTISAMPLE) ? 1 : 0,
			// if EGL_KHR_create_context extension is supported, then we will use
			// EGL_OPENGL_ES3_BIT_KHR instead of EGL_OPENGL_ES2_BIT in the attribute list
			EGL_RENDERABLE_TYPE, GetContextRenderableType(esContext->eglDisplay),
			EGL_NONE
		};

		// Choose config
		if (!eglChooseConfig(esContext->eglDisplay, attribList, &config, 1, &numConfigs))
		{
			return GL_FALSE;
		}

		if (numConfigs < 1)
		{
			return GL_FALSE;
		}
	}


#ifdef ANDROID
	// For Android, need to get the EGL_NATIVE_VISUAL_ID and set it using ANativeWindow_setBuffersGeometry
	{
		EGLint format = 0;
		eglGetConfigAttrib(esContext->eglDisplay, config, EGL_NATIVE_VISUAL_ID, &format);
		ANativeWindow_setBuffersGeometry(esContext->eglNativeWindow, 0, 0, format);
	}
#endif // ANDROID

	// Create a surface
	esContext->eglSurface = eglCreateWindowSurface(esContext->eglDisplay, config,
		esContext->eglNativeWindow, NULL);

	if (esContext->eglSurface == EGL_NO_SURFACE)
	{
		return GL_FALSE;
	}

	// Create a GL context
	esContext->eglContext = eglCreateContext(esContext->eglDisplay, config,
		EGL_NO_CONTEXT, contextAttribs);

	if (esContext->eglContext == EGL_NO_CONTEXT)
	{
		return GL_FALSE;
	}

	// Make the context current
	if (!eglMakeCurrent(esContext->eglDisplay, esContext->eglSurface,
		esContext->eglSurface, esContext->eglContext))
	{
		return GL_FALSE;
	}

#endif // #ifndef __APPLE__

	return GL_TRUE;
}

void ESUTIL_API
esFrustum(glm::mat4 &result, float left, float right, float bottom, float top, float nearZ, float farZ)
{
	float       deltaX = right - left;
	float       deltaY = top - bottom;
	float       deltaZ = farZ - nearZ;
	glm::mat4   frust;

	if ((nearZ <= 0.0f) || (farZ <= 0.0f) ||
		(deltaX <= 0.0f) || (deltaY <= 0.0f) || (deltaZ <= 0.0f))
	{
		return;
	}

	frust[0][0] = 2.0f * nearZ / deltaX;
	frust[0][1] = frust[0][2] = frust[0][3] = 0.0f;

	frust[1][1] = 2.0f * nearZ / deltaY;
	frust[1][0] = frust[1][2] = frust[1][3] = 0.0f;

	frust[2][0] = (right + left) / deltaX;
	frust[2][1] = (top + bottom) / deltaY;
	frust[2][2] = -(nearZ + farZ) / deltaZ;
	frust[2][3] = -1.0f;

	frust[3][2] = -2.0f * nearZ * farZ / deltaZ;
	frust[3][0] = frust[3][1] = frust[3][3] = 0.0f;

	result *= frust;
}


void ESUTIL_API
esPerspective(glm::mat4 &result, float fovy, float aspect, float nearZ, float farZ)
{
	GLfloat frustumW, frustumH;

	frustumH = tanf(fovy / 360.0f * PI) * nearZ;
	frustumW = frustumH * aspect;

	esFrustum(result, -frustumW, frustumW, -frustumH, frustumH, nearZ, farZ);
}

///
//  esRegisterDrawFunc()
//
void ESUTIL_API esRegisterDrawFunc(ESContext *esContext, void (ESCALLBACK *drawFunc) (ESContext *))
{
	esContext->drawFunc = drawFunc;
}

///
//  esRegisterShutdownFunc()
//
void ESUTIL_API esRegisterShutdownFunc(ESContext *esContext, void (ESCALLBACK *shutdownFunc) (ESContext *))
{
	esContext->shutdownFunc = shutdownFunc;
}

///
//  esRegisterUpdateFunc()
//
void ESUTIL_API esRegisterUpdateFunc(ESContext *esContext, void (ESCALLBACK *updateFunc) (ESContext *, float))
{
	esContext->updateFunc = updateFunc;
}


///
//  esRegisterKeyFunc()
//
void ESUTIL_API esRegisterKeyFunc(ESContext *esContext,
	void (ESCALLBACK *keyFunc) (ESContext *, unsigned char, int, int))
{
	esContext->keyFunc = keyFunc;
}

void ESUTIL_API esRegisterTouchEventFunc(ESContext *esContext,
	void (ESCALLBACK *touchFunc) (ESContext *, int, int, int))
{
	esContext->touchFunc = touchFunc;
}


///
// esLogMessage()
//
//    Log an error message to the debug output for the platform
//
void ESUTIL_API esLogMessage(const char *formatStr, ...)
{
	va_list params;
	char buf[BUFSIZ];

	va_start(params, formatStr);
	vsprintf(buf, formatStr, params);

#ifdef ANDROID
	__android_log_print(ANDROID_LOG_INFO, "esUtil", "%s", buf);
#else
	printf("%s", buf);
#endif

	va_end(params);
}

///
// esFileRead()
//
//    Wrapper for platform specific File open
//
static esFile *esFileOpen(void *ioContext, const char *fileName)
{
	esFile *pFile = NULL;

#ifdef ANDROID

	if (ioContext != NULL)
	{
		AAssetManager *assetManager = (AAssetManager *)ioContext;
		pFile = AAssetManager_open(assetManager, fileName, AASSET_MODE_BUFFER);
	}

#else
#ifdef __APPLE__
	// iOS: Remap the filename to a path that can be opened from the bundle.
	fileName = GetBundleFileName(fileName);
#endif

	pFile = fopen(fileName, "rb");
#endif

	return pFile;
}

///
// esFileRead()
//
//    Wrapper for platform specific File close
//
static void esFileClose(esFile *pFile)
{
	if (pFile != NULL)
	{
#ifdef ANDROID
		AAsset_close(pFile);
#else
		fclose(pFile);
		pFile = NULL;
#endif
	}
}

///
// esFileRead()
//
//    Wrapper for platform specific File read
//
static int esFileRead(esFile *pFile, int bytesToRead, void *buffer)
{
	int bytesRead = 0;

	if (pFile == NULL)
	{
		return bytesRead;
	}

#ifdef ANDROID
	bytesRead = AAsset_read(pFile, buffer, bytesToRead);
#else
	bytesRead = fread(buffer, bytesToRead, 1, pFile);
#endif

	return bytesRead;
}

///
// esLoadTGA()
//
//    Loads a 8-bit, 24-bit or 32-bit TGA image from a file
//
char *ESUTIL_API esLoadTGA(void *ioContext, const char *fileName, int *width, int *height)
{
	char        *buffer;
	esFile      *fp;
	TGA_HEADER   Header;
	int          bytesRead;

	// Open the file for reading
	fp = esFileOpen(ioContext, fileName);

	if (fp == NULL)
	{
		// Log error as 'error in opening the input file from apk'
		esLogMessage("esLoadTGA FAILED to load : { %s }\n", fileName);
		return NULL;
	}

	bytesRead = esFileRead(fp, sizeof (TGA_HEADER), &Header);

	*width = Header.Width;
	*height = Header.Height;

	if (Header.ColorDepth == 8 ||
		Header.ColorDepth == 24 || Header.ColorDepth == 32)
	{
		int bytesToRead = sizeof (char)* (*width) * (*height) * Header.ColorDepth / 8;

		// Allocate the image data buffer
		buffer = (char *)malloc(bytesToRead);

		if (buffer)
		{
			bytesRead = esFileRead(fp, bytesToRead, buffer);
			esFileClose(fp);

			return (buffer);
		}
	}

	return (NULL);
}

/// \brief Load a shader, check for compile errors, print error messages to output log
/// \param type Type of shader (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
/// \param shaderSrc Shader source string
/// \return A new shader object on success, 0 on failure
//
GLuint ESUTIL_API esLoadShader(GLenum type, const char *shaderSrc)
{
	GLuint shader;
	GLint compiled;

	// Create the shader object
	shader = glCreateShader(type);

	if (shader == 0)
	{
		return 0;
	}

	// Load the shader source
	glShaderSource(shader, 1, &shaderSrc, NULL);

	// Compile the shader
	glCompileShader(shader);

	// Check the compile status
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (!compiled)
	{
		GLint infoLen = 0;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1)
		{
			char *infoLog = (char *)malloc(sizeof (char)* infoLen);

			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			esLogMessage("Error compiling shader:\n%s\n", infoLog);

			free(infoLog);
		}

		glDeleteShader(shader);
		return 0;
	}

	return shader;

}

/// \brief Load a vertex and fragment shader, create a program object, link program.
//         Errors output to log.
/// \param vertShaderSrc Vertex shader source code
/// \param fragShaderSrc Fragment shader source code
/// \return A new program object linked with the vertex/fragment shader pair, 0 on failure
//
GLuint ESUTIL_API esLoadProgram(const char *vertShaderSrc, const char *fragShaderSrc)
{
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint programObject;
	GLint linked;

	// Load the vertex/fragment shaders
	vertexShader = esLoadShader(GL_VERTEX_SHADER, vertShaderSrc);

	if (vertexShader == 0)
	{
		return 0;
	}

	fragmentShader = esLoadShader(GL_FRAGMENT_SHADER, fragShaderSrc);

	if (fragmentShader == 0)
	{
		glDeleteShader(vertexShader);
		return 0;
	}

	// Create the program object
	programObject = glCreateProgram();

	if (programObject == 0)
	{
		return 0;
	}

	glAttachShader(programObject, vertexShader);
	glAttachShader(programObject, fragmentShader);

	// Link the program
	glLinkProgram(programObject);

	// Check the link status
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

	if (!linked)
	{
		GLint infoLen = 0;

		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1)
		{
			char *infoLog = (char *)malloc(sizeof (char)* infoLen);

			glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
			esLogMessage("Error linking program:\n%s\n", infoLog);

			free(infoLog);
		}

		glDeleteProgram(programObject);
		return 0;
	}

	// Free up no longer needed shader resources
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return programObject;
}

unsigned char *loadPng(const char *filename, int *width, int *height)
{
	FILE* file = 0;
	unsigned char* content = nullptr;
	file = fopen(filename, "rb");
	if (file) 
	{
		png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		png_infop info_ptr = png_create_info_struct(png_ptr);
		setjmp(png_jmpbuf(png_ptr));
		png_init_io(png_ptr, file);
		png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND, 0);

		int pic_width = png_get_image_width(png_ptr, info_ptr);
		*width = pic_width;
		int pic_height = png_get_image_height(png_ptr, info_ptr);
		*height = pic_height;

		int color_type = png_get_color_type(png_ptr, info_ptr);
		int size = pic_height * pic_width * 4;

		content = new unsigned char[size];
		png_bytep* row_pointers = png_get_rows(png_ptr, info_ptr);

		for (int i = 0; i<pic_height; ++i) {
			for (int j = 0; j<pic_width; ++j){
				content[i*pic_width * 4 + j * 4 + 0] = row_pointers[i][j * 4 + 0];
				content[i*pic_width * 4 + j * 4 + 1] = row_pointers[i][j * 4 + 1];
				content[i*pic_width * 4 + j * 4 + 2] = row_pointers[i][j * 4 + 2];
				content[i*pic_width * 4 + j * 4 + 3] = row_pointers[i][j * 4 + 3];
			}
		}

		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		fclose(file);
	}

	return content;
}

GLuint loadTexture(const char *filename, int *width, int *height)
{
	GLuint tex = 0;
	unsigned char * buffer = loadPng(filename, width, height);

	if (buffer)
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, *width, *height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

		delete[] buffer;
	}
	else
	{
		printf("not find picture %s", filename);
	}

	return tex;
}