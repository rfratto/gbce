#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#define GLFW_NO_GLU 1
#define GLFW_INCLUDE_GLCOREARB 1
#include <GLFW/glfw3.h>
#include <functional> 
#include "CPU.h"

class gameBoy;

class Graphics {
private:
	GLFWwindow *window; 

	GLuint VAO; 
	GLuint tex;
	GLuint vbuffer;
	GLuint tbuffer;

	GLuint program, vshader, fshader;
	GLuint texUniform;

	GLubyte *colors;

	bool running = true; 

public: 
	void setKeyCB(GLFWkeyfun cb);

	void run(std::function<void(void)> callback, std::function<void(void)> timercb); 
	void stop() { running = false; }

	void plotPoint(int x, int y, unsigned char color[3]);
	void swapBuffers(); 

	Graphics(); 
};

#endif 