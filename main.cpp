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

#include <windows.h>
#include <stdlib.h>
#include "core/gles_include.h"

#include <rendering/Camera.h>
#include <rendering/triangle.h>
#include <rendering/Cube.h>

#include <glm/gtc/matrix_transform.hpp>

Camera _camera;
Triangle _triangle;
Cube     _cube;

void Draw(ESContext *esContext)
{
	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	_triangle.draw(esContext);
	_cube.draw(esContext);

	eglSwapBuffers(esContext->eglDisplay, esContext->eglSurface);
	ValidateRect(esContext->eglNativeWindow, NULL);
}

void update(ESContext *esContext, float detlaTime)
{
	_camera.update(esContext, detlaTime);
	Draw(esContext);
}

void init(ESContext *esContext)
{
	_camera.lookAt(esContext, glm::vec3(0, 0, 4), glm::vec3(0, 0, -0.1), glm::vec3(0, 1, 0));
	_triangle.init();
	_cube.init();

	glEnable(GL_CULL_FACE);  // 不采用背面剔除
	glEnable(GL_DEPTH_TEST);

	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
}

int main(int argc, char *argv[])
{
	ESContext esContext;

	memset(&esContext, 0, sizeof (ESContext));

	esCreateWindow(&esContext, "gles_demo", 640, 480, ES_WINDOW_RGB | ES_WINDOW_DEPTH);

	init(&esContext);

	//esRegisterDrawFunc(&esContext, Draw);

	esRegisterUpdateFunc(&esContext, update);

	esStartLoop(&esContext);

	if (esContext.shutdownFunc != NULL)
	{
		esContext.shutdownFunc(&esContext);
	}

	return 0;
}
