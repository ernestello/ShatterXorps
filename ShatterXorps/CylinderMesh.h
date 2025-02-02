#ifndef CYLINDER_MESH_H
#define CYLINDER_MESH_H

#include "Vertex.h"
#include <vector>
#include <cmath>

namespace CylinderMesh {

    // Generate side vertices for a cylinder oriented along the Y axis.
    // The bottom circle is at y=0 and the top circle at y=height.
    // 'segmentCount' is the number of segments around the circle.
    inline std::vector<Vertex> createCylinder(float radius, float height, uint32_t segmentCount) {
        std::vector<Vertex> vertices;
        // For each segment, add a bottom and a top vertex.
        for (uint32_t i = 0; i < segmentCount; i++) {
            float theta = (float)i / segmentCount * 2.f * 3.14159265f;
            float x = radius * std::cos(theta);
            float z = radius * std::sin(theta);
            glm::vec3 normal = glm::normalize(glm::vec3(x, 0.f, z));
            // Color chosen arbitrarily (yellowish)
            glm::vec3 color(1.f, 1.f, 0.f);
            // Bottom vertex
            vertices.push_back({ glm::vec3(x, 0.f, z), color, normal });
            // Top vertex
            vertices.push_back({ glm::vec3(x, height, z), color, normal });
        }
        return vertices;
    }

    // Generate indices for the cylinder sides.
    // It creates two triangles per segment.
    inline std::vector<uint16_t> createCylinderIndices(uint32_t segmentCount) {
        std::vector<uint16_t> indices;
        for (uint16_t i = 0; i < segmentCount; i++) {
            uint16_t next = (i + 1) % segmentCount;
            // Each segment has two triangles:
            // Triangle 1: bottom(i), top(i), bottom(next)
            indices.push_back(i * 2);
            indices.push_back(i * 2 + 1);
            indices.push_back(next * 2);
            // Triangle 2: bottom(next), top(i), top(next)
            indices.push_back(next * 2);
            indices.push_back(i * 2 + 1);
            indices.push_back(next * 2 + 1);
        }
        return indices;
    }

} // namespace CylinderMesh

#endif // CYLINDER_MESH_H
