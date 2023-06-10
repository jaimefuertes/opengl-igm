// Jaime Fuertes Agras
// Daniel Olañeta Fariña


// Copyright (C) 2020 Emilio J. Padrón
// Released as Free Software under the X11 License
// https://spdx.org/licenses/X11.html

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

// GLM library to deal with matrix operations
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::perspective
#include <glm/gtc/type_ptr.hpp>

#include "textfile_ALT.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int gl_width = 640;
int gl_height = 480;

void glfw_window_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void render(double);
unsigned int loadTexture(const char *path); 

GLuint shader_program = 0; // shader program to set render pipeline
GLuint cubeVao, tetraVao = 0; // Vertext Array Object to set input data
GLint model_location, view_location, proj_location, normal_location; // Uniforms for transformation matrices

// Shader names
const char *vertexFileName = "spinningcube_withlight_vs.glsl";
const char *fragmentFileName = "spinningcube_withlight_fs.glsl";

// Camera
glm::vec3 camera_pos(0.0f, 0.0f, 3.0f);

// Lighting
//Light 1
glm::vec3 light_pos(1.2f, 1.0f, 2.0f);
glm::vec3 light_ambient(0.2f, 0.2f, 0.2f);
glm::vec3 light_diffuse(0.5f, 0.5f, 0.5f);
glm::vec3 light_specular(1.0f, 1.0f, 1.0f);

//Light 2
glm::vec3 light_pos2(-1.2f, -1.0f, 3.0f);
glm::vec3 light_ambient2(0.4f, 0.4f, 0.4f);
glm::vec3 light_diffuse2(0.8f, 0.8f, 0.8f);
glm::vec3 light_specular2(1.0f, 1.0f, 1.0f);

// Material
glm::vec3 material_ambient(1.0f, 0.5f, 0.31f);
glm::vec3 material_diffuse(1.0f, 0.5f, 0.31f);
glm::vec3 material_specular(0.5f, 0.5f, 0.5f);
const GLfloat material_shininess = 32.0f;

unsigned int diffuseTexture;

int main() {
  // start GL context and O/S window using the GLFW helper library
  if (!glfwInit()) {
    fprintf(stderr, "ERROR: could not start GLFW3\n");
    return 1;
  }

  //  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  //  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  //  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  //  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window = glfwCreateWindow(gl_width, gl_height, "My spinning cube", NULL, NULL);
  if (!window) {
    fprintf(stderr, "ERROR: could not open window with GLFW3\n");
    glfwTerminate();
    return 1;
  }
  glfwSetWindowSizeCallback(window, glfw_window_size_callback);
  glfwMakeContextCurrent(window);

  // start GLEW extension handler
  // glewExperimental = GL_TRUE;
  glewInit();

  // get version info
  const GLubyte* vendor = glGetString(GL_VENDOR); // get vendor string
  const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
  const GLubyte* glversion = glGetString(GL_VERSION); // version as a string
  const GLubyte* glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION); // version as a string
  printf("Vendor: %s\n", vendor);
  printf("Renderer: %s\n", renderer);
  printf("OpenGL version supported %s\n", glversion);
  printf("GLSL version supported %s\n", glslversion);
  printf("Starting viewport: (width: %d, height: %d)\n", gl_width, gl_height);

  // Enable Depth test: only draw onto a pixel if fragment closer to viewer
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS); // set a smaller value as "closer"

  // Vertex Shader
  char* vertex_shader = textFileRead(vertexFileName);

  // Fragment Shader
  char* fragment_shader = textFileRead(fragmentFileName);

  // Shaders compilation
  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vertex_shader, NULL);
  free(vertex_shader);
  glCompileShader(vs);

  int  success;
  char infoLog[512];
  glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vs, 512, NULL, infoLog);
    printf("ERROR: Vertex Shader compilation failed!\n%s\n", infoLog);

    return(1);
  }

  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fragment_shader, NULL);
  free(fragment_shader);
  glCompileShader(fs);

  glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fs, 512, NULL, infoLog);
    printf("ERROR: Fragment Shader compilation failed!\n%s\n", infoLog);

    return(1);
  }

  // Create program, attach shaders to it and link it
  shader_program = glCreateProgram();
  glAttachShader(shader_program, fs);
  glAttachShader(shader_program, vs);
  glLinkProgram(shader_program);

  glValidateProgram(shader_program);
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if(!success) {
    glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
    printf("ERROR: Shader Program linking failed!\n%s\n", infoLog);

    return(1);
  }

  // Release shader objects
  glDeleteShader(vs);
  glDeleteShader(fs);

  // Vertex Array Object
  glGenVertexArrays(1, &cubeVao);
  glGenVertexArrays(1, &tetraVao);
  
  
  

  // Cube to be rendered
  //
  //          0        3
  //       7        4 <-- top-right-near
  // bottom
  // left
  // far ---> 1        2
  //       6        5
  //
  const GLfloat cube_vp[] = {
     -0.25f, -0.25f, -0.25f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // 1
      -0.25f, 0.25f, -0.25f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,  // 0
      0.25f, -0.25f, -0.25f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,  // 2

      0.25f, 0.25f, -0.25f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,  // 3
      0.25f, -0.25f, -0.25f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // 2
      -0.25f, 0.25f, -0.25f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // 0

      0.25f, -0.25f, -0.25f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 2
      0.25f, 0.25f, -0.25f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // 3
      0.25f, -0.25f, 0.25f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // 5

      0.25f, 0.25f, 0.25f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // 4
      0.25f, -0.25f, 0.25f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // 5
      0.25f, 0.25f, -0.25f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // 3

      0.25f, -0.25f, 0.25f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // 5
      0.25f, 0.25f, 0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,   // 4
      -0.25f, -0.25f, 0.25f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // 6

      -0.25f, 0.25f, 0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,  // 7
      -0.25f, -0.25f, 0.25f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // 6
      0.25f, 0.25f, 0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,   // 4

      -0.25f, -0.25f, 0.25f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // 6
      -0.25f, 0.25f, 0.25f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // 7
      -0.25f, -0.25f, -0.25f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // 1

      -0.25f, 0.25f, -0.25f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // 0
      -0.25f, -0.25f, -0.25f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // 1
      -0.25f, 0.25f, 0.25f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // 7

      0.25f, -0.25f, -0.25f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // 2
      0.25f, -0.25f, 0.25f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // 5
      -0.25f, -0.25f, -0.25f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // 1

      -0.25f, -0.25f, 0.25f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,  // 6
      -0.25f, -0.25f, -0.25f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // 1
      0.25f, -0.25f, 0.25f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // 5

      0.25f, 0.25f, 0.25f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,  // 4
      0.25f, 0.25f, -0.25f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // 3
      -0.25f, 0.25f, 0.25f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // 7

      -0.25f, 0.25f, -0.25f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // 0
      -0.25f, 0.25f, 0.25f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // 7
      0.25f, 0.25f, -0.25f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // 3
  };
  
GLfloat tetra_vp[] = {
    // Cara 1
    0.0f,  0.5f,  0.0f,  0.0f,  0.8165f,  0.5774f, 0.5f, 1.0f, // v1
   -0.5f, -0.5f,  0.5f,  0.0f,  0.8165f,  0.5774f, 0.0f, 0.0f, // v2
    0.5f, -0.5f,  0.5f,  0.0f,  0.8165f,  0.5774f, 1.0f, 0.0f, // v3

    // Cara 2
    0.0f,  0.5f,  0.0f,  0.8165f,  0.4082f, -0.5774f, 0.5f, 1.0f, // v1
    0.5f, -0.5f,  0.5f,  0.8165f,  0.4082f, -0.5774f, 1.0f, 0.0f, // v3
    0.0f, -0.5f, -0.5f,  0.8165f,  0.4082f, -0.5774f, 0.0f, 0.0f,// v4

    // Cara 3
    0.0f,  0.5f,  0.0f, -0.8165f,  0.4082f, -0.5774f, 0.5f, 1.0f, // v1
    0.0f, -0.5f, -0.5f, -0.8165f,  0.4082f, -0.5774f, 1.0f, 0.0f, // v4
   -0.5f, -0.5f,  0.5f, -0.8165f,  0.4082f, -0.5774f, 0.0f, 0.0f,// v2

    // Cara 4
    0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // v3
   -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // v2
    0.0f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.5f, 0.9f,// v4
    
};

// Vertex Buffer Object (for vertex coordinates)
  GLuint cubeVbo, tetraVbo = 0;
  
  
  //Cube
  glBindVertexArray(cubeVao);
  glGenBuffers(1, &cubeVbo);
  glBindBuffer(GL_ARRAY_BUFFER, cubeVbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vp), cube_vp, GL_STATIC_DRAW);
  
  // Vertex attributes
  // 0: vertex position (x, y, z)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), NULL);
  glEnableVertexAttribArray(0);

  // 1: vertex normals (x, y, z)
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  
  // 2: texture coords (u, v)
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  // Unbind vao & vbo (it was conveniently registered by VertexAttribPointer)
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  //Pyramid
  glBindVertexArray(tetraVao);
  glGenBuffers(1, &tetraVbo);
  glBindBuffer(GL_ARRAY_BUFFER, tetraVbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(tetra_vp), tetra_vp, GL_STATIC_DRAW);
  
  // Vertex attributes
  // 0: vertex position (x, y, z)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), NULL);
  glEnableVertexAttribArray(0);

  // 1: vertex normals (x, y, z)
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  
  // 2: texture coords (u, v)
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(8 * sizeof(float)));
  glEnableVertexAttribArray(2);
  
  // Unbind vao & vbo (it was conveniently registered by VertexAttribPointer)
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  
  //Load texture
  diffuseTexture = loadTexture("texture.png");

  glUseProgram(shader_program);
  
  // Uniforms
  // - Model matrix
  // - View matrix
  // - Projection matrix
  // - Normal matrix: normal vectors from local to world coordinates
  // - Camera position
  // - Light data
  // - Material data
  model_location = glGetUniformLocation(shader_program, "model");
  view_location = glGetUniformLocation(shader_program, "view");
  proj_location = glGetUniformLocation(shader_program, "projection");
  normal_location = glGetUniformLocation(shader_program, "normal_to_world");
  
  GLuint location;
  
  //Camera
  location = glGetUniformLocation(shader_program, "view_pos");
  glUniform3fv(location, 1, glm::value_ptr(camera_pos));
  
  
  //Lights
  //1 
  location = glGetUniformLocation(shader_program, "light1.position");
  glUniform3fv(location, 1, glm::value_ptr(light_pos));
  location = glGetUniformLocation(shader_program, "light1.ambient");
  glUniform3fv(location, 1, glm::value_ptr(light_ambient));
  location = glGetUniformLocation(shader_program, "light1.diffuse");
  glUniform3fv(location, 1, glm::value_ptr(light_diffuse));
  location = glGetUniformLocation(shader_program, "light1.specular");
  glUniform3fv(location, 1, glm::value_ptr(light_specular));
  //2
  location = glGetUniformLocation(shader_program, "light2.position");
  glUniform3fv(location, 1, glm::value_ptr(light_pos2));
  location = glGetUniformLocation(shader_program, "light2.ambient");
  glUniform3fv(location, 1, glm::value_ptr(light_ambient2));
  location = glGetUniformLocation(shader_program, "light2.diffuse");
  glUniform3fv(location, 1, glm::value_ptr(light_diffuse2));
  location = glGetUniformLocation(shader_program, "light2.specular");
  glUniform3fv(location, 1, glm::value_ptr(light_specular2));
  
  //Material
  location = glGetUniformLocation(shader_program, "material.ambient");
  glUniform3fv(location, 1, glm::value_ptr(material_ambient));
  location = glGetUniformLocation(shader_program, "material.diffuse");
  glUniform3fv(location, 1, glm::value_ptr(material_diffuse));
  location = glGetUniformLocation(shader_program, "material.specular");
  glUniform3fv(location, 1, glm::value_ptr(material_specular));
  location = glGetUniformLocation(shader_program, "material.shininess");
  glUniform1f(location, material_shininess);

// Render loop
  while(!glfwWindowShouldClose(window)) {

    processInput(window);

    render(glfwGetTime());

    glfwSwapBuffers(window);

    glfwPollEvents();
  }

  glfwTerminate();

  return 0;
}

void render(double currentTime) {
  float f = (float)currentTime * 0.3f;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glViewport(0, 0, gl_width, gl_height);

  
  glBindVertexArray(cubeVao);

  glm::mat4 model_matrix, view_matrix, proj_matrix;
  glm::mat3 normal_matrix;


  view_matrix = glm::lookAt(                 camera_pos,  // pos
                            glm::vec3(0.0f, 0.0f, -1.0f),  // target
                            glm::vec3(0.0f, 1.0f, 0.0f)); // up
                            
  glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(view_matrix));
  
  // Projection
  // proj_matrix = glm::perspective(glm::radians(50.0f),
  //   [...]
  proj_matrix = glm::perspective(glm::radians(50.0f),
                                 (float)gl_width / (float)gl_height,
                                 0.1f, 1000.0f);
  glUniformMatrix4fv(proj_location, 1, GL_FALSE, glm::value_ptr(proj_matrix));
  

  // Moving cube
  model_matrix = glm::translate(glm::mat4(1.f), glm::vec3(0.0f, 0.0f, -2.0f));
  //model_matrix = glm::translate(model_matrix,
  //                           glm::vec3(sinf(2.1f * f) * 0.5f,
  //                                     cosf(1.7f * f) * 0.5f,
  //                                     sinf(1.3f * f) * cosf(1.5f * f) * 2.0f));

  model_matrix = glm::rotate(model_matrix,
                          glm::radians((float)currentTime * 45.0f),
                          glm::vec3(0.0f, 1.0f, 0.0f));
  model_matrix = glm::rotate(model_matrix,
                          glm::radians((float)currentTime * 81.0f),
                          glm::vec3(1.0f, 0.0f, 0.0f));

  glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model_matrix));
  
  // Normal matrix: normal vectors to world coordinates
  normal_matrix = glm::transpose(glm::inverse(glm::mat3(model_matrix)));
  glUniformMatrix3fv(normal_location, 1, GL_FALSE, glm::value_ptr(normal_matrix));


  //Texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, diffuseTexture);

  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
  
  
  
  // Figura 2
  
  glBindVertexArray(tetraVao);
  
  model_matrix = glm::translate(glm::mat4(1.f), glm::vec3(-2.0f, 0.0f, -4.0f));
  model_matrix = glm::translate(model_matrix,
                             glm::vec3(sinf(2.1f * f) * 0.5f,
                                       cosf(1.7f * f) * 0.5f,
                                       sinf(1.3f * f) * cosf(1.5f * f) * 2.0f));

  model_matrix = glm::rotate(model_matrix,
                          glm::radians((float)currentTime * 45.0f),
                         glm::vec3(0.0f, 1.0f, 0.0f));
  model_matrix = glm::rotate(model_matrix,
                          glm::radians((float)currentTime * 81.0f),
                          glm::vec3(1.0f, 0.0f, 0.0f));

  glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model_matrix));
  
  
  
  // Normal matrix: normal vectors to world coordinates
  normal_matrix = glm::transpose(glm::inverse(glm::mat3(model_matrix)));
  glUniformMatrix3fv(normal_location, 1, GL_FALSE, glm::value_ptr(normal_matrix));

  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
}

void processInput(GLFWwindow *window) {
  if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, 1);
}

// Callback function to track window size and update viewport
void glfw_window_size_callback(GLFWwindow* window, int width, int height) {
  gl_width = width;
  gl_height = height;
  printf("New viewport: (width: %d, height: %d)\n", width, height);
}

unsigned int loadTexture(const char *path){
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  // load and generate the texture
  int width, height, nrChannels;
  unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
  
  if (data) {
    
    GLenum format;
    
    if (nrChannels == 1)
      format = GL_RED;
    else if (nrChannels == 3)
      format = GL_RGB;
    else if (nrChannels == 4)
      format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
  } else {

    printf("Texture failed to load");
    stbi_image_free(data);
  }
  return texture;
}
