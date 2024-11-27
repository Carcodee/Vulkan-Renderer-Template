//
// Created by carlo on 2024-10-09.
//

#ifndef MODEL_HPP
#define MODEL_HPP

namespace Rendering
{
    struct NodeMat
    {
        NodeMat* parentNode = nullptr;
        glm::mat4 matrix = glm::mat4(1.0f);
        glm::mat4 GetWorlMat()
        {
            NodeMat* parent = parentNode;
            glm::mat4 mat = matrix;
            while (parent != nullptr)
            {
                mat = parent->matrix * mat;
                parent = parent->parentNode;
            }
            return mat;
        }
        ~NodeMat() = default;
    };
    struct Model
    {
        std::vector<M_Vertex3D> vertices;
        std::vector<uint32_t> indices;
        
        //map with mesh number
        int meshCount;
        std::vector<uint32_t> firstVertices;
        std::vector<uint32_t> firstIndices;
        std::vector<uint32_t> verticesCount;
        std::vector<uint32_t> indicesCount;
        std::vector<NodeMat*> nodeMats;
        std::vector<glm::mat4> modelsMat;
        std::vector<int> materials;
        void SetWorldMatrices(){
            modelsMat.reserve(meshCount);
            for (auto& node : nodeMats)
            {
                modelsMat.emplace_back(node->GetWorlMat());
            }
        }
        ~Model()
        {
            for (auto node : nodeMats)
            {
                delete node;
            }
        }
        void PackModel()
        {
            
        }
        
    };
}
#endif //MODEL_HPP
