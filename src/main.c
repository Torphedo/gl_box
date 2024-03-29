#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

#include <cglm/struct.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "gl_debug.h"
#include "gl_setup.h"
#include "allocator.h"
#include "camera.h"
#include "types.h"
#include "shader.h"
#include "image.h"
#include "logging.h"
#include "input.h"

vertex quad_vertices[] = {
    { .position = {0.5f, 0.5f, 0.5f},
      .tex_coord = {1.0f, 1.0f}
    },
    {
      .position = {0.5f, -0.5f, 0.5f},
      .tex_coord = {1.0f, 0.0f}
    },
    {
      .position = {-0.5f, -0.5f, 0.5f},
      .tex_coord = {0.0f, 0.0f}
    },
    {
      .position = {-0.5f,  0.5f, 0.5f},
      .tex_coord = {0.0f, 1.0f}
    },
    { .position = {0.5f, 0.5f, -0.5f},
      .tex_coord = {1.0f, 1.0f}
    },
    {
      .position = {0.5f, -0.5f, -0.5f},
      .tex_coord = {1.0f, 0.0f}
    },
    {
      .position = {-0.5f, -0.5f, -0.5f},
      .tex_coord = {0.0f, 0.0f}
    },
    {
      .position = {-0.5f,  0.5f, -0.5f},
      .tex_coord = {0.0f, 1.0f}
    }
};

u32 quad_indices[] = {
    0, 1, 3,
    1, 2, 3,
    
    0, 1, 4,
    1, 4, 5,
    
    1, 2, 5,
    2, 5, 6,
    
    2, 3, 6,
    3, 6, 7,
    
    3, 4, 7,
    4, 3, 0,
    
    4, 5, 7,
    5, 6, 7
};

int main() {
    // Remove the console window on startup on Windows
    // FreeConsole();

    static const s32 width = 800;
    static const s32 height = 600;
    GLFWwindow* window = setup_opengl(width, height, "gl_box", ENABLE_DEBUG);
    if (window == NULL) {
        LOG_MSG(error, "Failed to create a valid window & OpenGL context for rendering\n");
        return 1;
    }

    glfwSetKeyCallback(window, input_update);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
      glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    
    // Setup VAO to store our state
    gl_obj vertex_array = 0;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);
    
    // Setup vertex buffer
    gl_obj vertex_buffer = 0;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
    
    // Element buffer
    gl_obj element_buffer = 0;
    glGenBuffers(1, &element_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), &quad_indices, GL_STATIC_DRAW);
    
    // Create vertex layout
    glVertexAttribPointer(0, sizeof(vec3f) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, sizeof(vec2f) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, tex_coord));
    glEnableVertexAttribArray(1);
    
    // Load and compile shaders
    char vert_path[256] = SRC_ROOT;
    strncat(vert_path, "/src/gl/vertex.glsl", sizeof(vert_path) -1);
    gl_obj vertex_shader = shader_compile(allocator_default, vert_path, GL_VERTEX_SHADER);
    
    char frag_path[256] = SRC_ROOT;
    strncat(frag_path, "/src/gl/fragment.glsl", sizeof(frag_path) - 1);
    gl_obj fragment_shader = shader_compile(allocator_default, frag_path, GL_FRAGMENT_SHADER);
    
    if (vertex_shader == 0 || fragment_shader == 0) {
        // All needed error information should already be given by the shader
        // compiler function and our debug callbacks.
        LOG_MSG(error, "failed to compile shaders.\n");
        glfwTerminate();
        return 0;
    }

    // Link the compiled shaders
    gl_obj shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);


    // Check if linking succeeded and print log if not
    shader_link_check(shader_program);
    glUseProgram(shader_program);

    // Delete the individual shader objects
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    glEnable(GL_DEPTH_TEST);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Wireframe mode.

    // Load texture(s)
    texture perlin = image_dds_load(allocator_default, "data/perlin.dds");
    u32 gl_perlin = 0;
    glGenTextures(1, &gl_perlin);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gl_perlin);

    // Wrapping & filtering settings
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (perlin.data != NULL) {
        LOG_MSG(info, "Binding loaded image data.\n");
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, perlin.width, perlin.height, 0, GL_BGR, GL_UNSIGNED_BYTE, perlin.data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    // Keep window alive and updated
    while (!glfwWindowShouldClose(window)) {
        // Clear framebuffer
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Quad transforms (updated each frame)
        mat4 model = {0};
        glm_mat4_identity(model); // Create identity matrix
        glm_rotate(model, glm_rad(-55.0f), (vec3){1.0f, 0.0f, 0.0f});
        glm_rotate(model, glfwGetTime(), (vec3){0.0f, 0.0f, 1.0f});
        gl_obj u_model = glGetUniformLocation(shader_program, "model");
        glUniformMatrix4fv(u_model, 1, GL_FALSE, (const float*)&model);


        mat4 view = {0};
        glm_mat4_identity(view); // Create identity matrix

        camera_update(&view);

        gl_obj u_view = glGetUniformLocation(shader_program, "view");
        glUniformMatrix4fv(u_view, 1, GL_FALSE, (const float*)&view);

        // Create & upload projection matrix
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        mat4 projection = {0};
        glm_perspective_rh_no(glm_rad(45), (float)mode->width / (float)mode->height, 0.1f, 1000.0f, projection);
        gl_obj u_projection = glGetUniformLocation(shader_program, "projection");
        glUniformMatrix4fv(u_projection, 1, GL_FALSE, (const float*)&projection);

        // Draw
        glUseProgram(shader_program);
        glBindTexture(GL_TEXTURE_2D, gl_perlin);
        glBindVertexArray(vertex_array);
        glDrawElements(GL_TRIANGLES, sizeof(quad_indices) / sizeof(*quad_indices), GL_UNSIGNED_INT, 0);

        // Quad transforms (updated each frame)
        glm_mat4_identity(model); // Create identity matrix
        glm_rotate(model, glm_rad(29.0f), (vec3){0.7f, 1.0f, 0.2f});
        glm_translate(model, (vec3){1.2f, 0.2f, 0.5f});
        glUniformMatrix4fv(u_model, 1, GL_FALSE, (const float*)&model);
        glDrawElements(GL_TRIANGLES, sizeof(quad_indices) / sizeof(*quad_indices), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // Clean up
    glDeleteProgram(shader_program);
    glDeleteVertexArrays(1, &vertex_array);
    glDeleteBuffers(1, &vertex_buffer);

    if (perlin.data != NULL) {
        free(perlin.data);
    }
    
    // Shut down
    glfwTerminate();
    return 0;
}

