#include "Graphics.h"
#include "CPU.h"
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <fstream>

std::string getSource(std::ifstream *file) {
  std::string source = "", line = ""; 
  while (getline(*file, line)) 
    source += line + "\n"; 
  return source;
}

void compileShader(GLuint shader, std::string file) {
  std::ifstream shadersrc(file); 
  if (shadersrc.is_open() == true) {
    std::string source = getSource(&shadersrc);
    shadersrc.close();

    char const *source_c = source.c_str();

    glShaderSource(shader, 1, &source_c, NULL);
    glCompileShader(shader);

    GLint status; 
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE) {
      GLint length; 
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

      char *log = new char[length+1];
      glGetShaderInfoLog(shader, length, NULL, log); 
      log[length] = 0;

      printf("%s\n", log);
    }
  }
}

void Graphics::plotPoint(int x, int y, unsigned char color[3]) {
  int offset = (x * 3) + (y * 160 * 3); 
  colors[offset+0] = color[0];
  colors[offset+1] = color[1];
  colors[offset+2] = color[2];  
} 

void Graphics::swapBuffers() {
  glBindTexture(GL_TEXTURE_2D, tex);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 160, 144, 0, GL_RGB, GL_UNSIGNED_BYTE, colors);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glBindTexture(GL_TEXTURE_2D, 0);
}

void Graphics::setKeyCB(GLFWkeyfun cb) {
  glfwSetKeyCallback(window, cb);
}

Graphics::Graphics() {
	if (!glfwInit()) exit(-1);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, false);

  window = glfwCreateWindow(160 * 5, 144 * 5, "GBCE", NULL, NULL);
  if (!window) {
  	glfwTerminate();
  	exit(-1);
  }

  glfwMakeContextCurrent(window); 
 
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  colors = new GLubyte[160 * 144 * 3]; 
  memset(colors, 0, 160 * 144 * 3);

  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 160, 144, 0, GL_RGB, GL_UNSIGNED_BYTE, colors);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glBindTexture(GL_TEXTURE_2D, 0);

  static const GLfloat pos[] = {
    -1, -1, 0,
    -1,  1, 0,
     1,  1, 0,
    -1, -1, 0,
     1,  1, 0,
     1, -1, 0
  };

  static const GLfloat tpos[] = {
     0, 1,
     0, 0,
     1, 0, 
     0, 1,
     1, 0, 
     1, 1 
  };

  glGenBuffers(1, &vbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(pos), pos, GL_STATIC_DRAW); 

  glGenBuffers(1, &tbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, tbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(tpos), tpos, GL_STATIC_DRAW); 

  vshader = glCreateShader(GL_VERTEX_SHADER);
  compileShader(vshader, "files/vertex_shader.vs"); 

  fshader = glCreateShader(GL_FRAGMENT_SHADER); 
  compileShader(fshader, "files/fragment_shader.fs");

  program = glCreateProgram();
  glAttachShader(program, vshader);
  glAttachShader(program, fshader);
  glLinkProgram(program);

  GLint status; 
  glGetProgramiv(program, GL_LINK_STATUS, &status);

  if (status == GL_FALSE)
    printf("Program failed to link.\n");

  texUniform = glGetUniformLocation(program, "tex");
}

const static float red[] = {0, 0, 0, 1};
    
void Graphics::run(std::function<void(void)> callback, std::function<void(void)> timercb) {
  struct timeval starttime;
  gettimeofday(&starttime, nullptr);

	while (!glfwWindowShouldClose(window) && running == true) {
		callback();

    if (running == false) break;

		glClearBufferfv(GL_COLOR, 0, red);

    glUseProgram(program);
    glEnable(GL_TEXTURE_2D);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, tbuffer);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);   

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(texUniform, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6); 

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    glfwSwapBuffers(window);
    glfwPollEvents();
    usleep(16666);

    struct timeval curtime;
    gettimeofday(&curtime, nullptr);

    if (curtime.tv_sec - starttime.tv_sec > 0) {
      starttime = curtime;
      timercb();
    }
	}	

	glfwTerminate();
}