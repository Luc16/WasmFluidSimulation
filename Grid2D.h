//
// Created by luc on 18/01/23.
//

#ifndef GRID2D_H
#define GRID2D_H

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#else
#include <glad/glad.h>
#endif
#include <GLFW/glfw3.h>
#include <vector>
#include <array>

class Grid2D {

public:
    struct Vertex {
        GLfloat pos[3];
        GLfloat col[3];
    };

    Grid2D() = default;
    Grid2D(const Grid2D &) = delete;
    Grid2D &operator=(const Grid2D &) = delete;
    ~Grid2D() = default;

    void createGrid(uint32_t width, uint32_t height, uint32_t size);
    void deleteBuffers();
    void updateColor(uint32_t idx, float color);
    void updateBuffer();
    void render();
    size_t size() { return m_vertices.size(); }

private:

    std::vector<Vertex> m_vertices;
    std::vector<unsigned short> m_indices;
    GLuint VBO{}, VAO{}, IBO{};
};




#endif //GRID2D_H
