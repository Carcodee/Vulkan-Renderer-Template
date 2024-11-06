//
// Created by carlo on 2024-10-08.
//

#ifndef RENDERINGSTRUCTS_HPP
#define RENDERINGSTRUCTS_HPP

namespace Rendering
{
    struct Vertex2D
    {
        float pos[2];
        float uv[2];
    	
    	static ENGINE::VertexInput GetVertexInput()
		{
            ENGINE::VertexInput vertexInput;
            vertexInput.AddVertexAttrib(ENGINE::VertexInput::VEC2, 0, offsetof(Vertex2D, pos), 0);
            vertexInput.AddVertexInputBinding(0, sizeof(Vertex2D));

            vertexInput.AddVertexAttrib(ENGINE::VertexInput::VEC2, 0, offsetof(Vertex2D, uv), 1);
            vertexInput.AddVertexInputBinding(0, sizeof(Vertex2D));
			return vertexInput;
 		}

        static std::vector<Vertex2D> GetQuadVertices()
        {
	        std::vector<Vertex2D> quadVertices = {
		        {{-1.0f, 1.0f}, {0.0f, 1.0f}},
		        {{-1.0f, -1.0f}, {0.0f, 0.0f}},
		        {{1.0f, -1.0f}, {1.0f, 0.0f}},
		        {{1.0f, 1.0f}, {1.0f, 1.0f}}
	        };
	        return quadVertices;
        }

        static std::vector<uint32_t> GetQuadIndices()
        {
	        std::vector<uint32_t> indices = {
		        0, 1, 2, // First triangle (top-left, bottom-left, bottom-right)
		        2, 3, 0 // Second triangle (bottom-right, top-right, top-left)
	        };
	        return indices;
        };


    };

    struct M_Vertex3D
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec2 uv;
        
		bool operator==(const M_Vertex3D& other) const {
			return pos == other.pos && uv == other.uv && normal == other.normal && tangent == other.tangent;
		}

    	static ENGINE::VertexInput GetVertexInput()
		{
			
            ENGINE::VertexInput vertexInput;
            vertexInput.AddVertexAttrib(ENGINE::VertexInput::VEC3, 0, offsetof(M_Vertex3D, pos), 0);
            vertexInput.AddVertexInputBinding(0, sizeof(M_Vertex3D));

            vertexInput.AddVertexAttrib(ENGINE::VertexInput::VEC3, 0, offsetof(M_Vertex3D, normal), 1);
            vertexInput.AddVertexInputBinding(0, sizeof(M_Vertex3D));
			
            vertexInput.AddVertexAttrib(ENGINE::VertexInput::VEC3, 0, offsetof(M_Vertex3D, tangent), 2);
            vertexInput.AddVertexInputBinding(0, sizeof(M_Vertex3D));
			
            vertexInput.AddVertexAttrib(ENGINE::VertexInput::VEC2, 0, offsetof(M_Vertex3D, uv), 3);
            vertexInput.AddVertexInputBinding(0, sizeof(M_Vertex3D));

			return vertexInput;
 		}

    };

	struct ForwardPc
	{
		glm::mat4 model;
		glm::mat4 projView;
	};
	struct CPropsUbo
	{
		glm::mat4 invProj;
		glm::mat4 invView;
	};
	struct ArrayIndexer 
	{
		int offset = -1;
		int size = 0;
	};
	struct ScreenDataPc
	{
		int sWidth = -1;
		int sHeight = -1;
		int pointLightsCount = 0;
	};


    
}

namespace std
{
	template <>
	struct hash<Rendering::M_Vertex3D>
	{
		size_t operator()(Rendering::M_Vertex3D const& vertex) const
		{
			// Hash the position, color, normal, and texCoord
			size_t hash1 = hash<glm::vec3>()(vertex.pos);
			size_t hash2 = hash<glm::vec3>()(vertex.normal);
			size_t hash3 = hash<glm::vec3>()(vertex.tangent);
			size_t hash4 = hash<glm::vec2>()(vertex.uv);

			// Combine the hashes using bitwise operations
			size_t result = hash1;
			result = (result * 31) + hash2;
			result = (result * 31) + hash3;
			result = (result * 31) + hash4;

			return result;
		}
	};
}

#endif //RENDERINGSTRUCTS_HPP
