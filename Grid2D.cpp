//
// Created by luc on 18/01/23.
//

#include "Grid2D.h"
#include <iostream>


void Grid2D::createGrid(uint32_t width, uint32_t height, uint32_t size) {
    uint32_t numW = width/size, numH = height/size;
    float pos[2] = {0.0f, 0.0f};
    auto fsize = (float) size;

    m_vertices.resize((numW+1) * (numH+1));
    for (uint32_t i = 0; i < m_vertices.size(); ++i) {
        auto& vertex = m_vertices[i];
        vertex = {{2*pos[0]/(float)width - 1.0f, 2*pos[1]/(float)height - 1.0f, 0.0f}, {0,0,float(i)/float(m_vertices.size())}};
        pos[0] += fsize;

        if (pos[0] > (float) width) {
            pos[0] = 0.0f;
            pos[1] += fsize;
        }
    }

    std::array<uint32_t, 6> defIndices{0, 1, numW+1, numW+1, 1, numW+2};
    m_indices.resize(numW * numH * defIndices.size());

    for (uint32_t i = 0, idx = 0; i < numW*numH; ++i, ++idx){
        if (idx % (numW + 1) == numW) idx++;
        for (uint32_t j = 0; j < defIndices.size(); ++j){
            m_indices[defIndices.size()*i + j] = idx + defIndices[j];
        }
    }


    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);


    glBufferData(GL_ARRAY_BUFFER, m_vertices.size()*sizeof(Vertex), m_vertices.data(), GL_DYNAMIC_DRAW);

    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    //color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, col));

    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size()*sizeof(unsigned short), m_indices.data(), GL_STATIC_DRAW);
}

void Grid2D::deleteBuffers() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &IBO);
}

void Grid2D::updateColor(uint32_t idx, float color) {
    m_vertices[idx].col[0] = color;
    m_vertices[idx].col[1] = color;
    m_vertices[idx].col[2] = color;
}

void Grid2D::updateBuffer() {
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size()*sizeof(Vertex), m_vertices.data(), GL_DYNAMIC_DRAW);
}

void Grid2D::render() {
    glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_SHORT, nullptr);
}


