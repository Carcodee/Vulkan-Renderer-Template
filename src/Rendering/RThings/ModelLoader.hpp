//



// Created by carlo on 2024-10-16.
//

#ifndef MODELLOADER_HPP
#define MODELLOADER_HPP


namespace Rendering
{
    class ModelLoader
    {

    public:
    	void LoadGLTF(std::string path,Model& model)
    	{

    		if (!std::filesystem::exists(path))
    		{

    			SYSTEMS::Logger::GetInstance()->SetLogPreferences(SYSTEMS::LogLevel::L_ERROR);
    			SYSTEMS::Logger::GetInstance()->LogMessage("Path do not exist: "+ path);
    			return;
    		}
    		tinygltf::Model gltfModel;
		    std::string err;
		    std::string warn = "warn";
		    tinygltf::TinyGLTF gltfContext;
		    gltfContext.LoadASCIIFromFile(&gltfModel, &err, &warn, path);
    		NodeMat* rootNode = new NodeMat();

    		model.meshCount = (int)gltfModel.meshes.size();
		    for (auto& scene : gltfModel.scenes)
		    {
			    for (auto& node : scene.nodes)
			    {
			    	LoadGLTFNode(gltfModel, gltfModel.nodes[node], rootNode, model);
			    }
			    
		    }
    		model.SetWorldMatrices();
    		LoadGLTFMaterials(gltfModel, path);
    	}
    	void LoadGLTFNode(tinygltf::Model& gltfModel, tinygltf::Node& node, NodeMat* parentNodeMat, Model& model)
    	{

    		glm::mat4 nodeMatrix = glm::mat4(1.0f);
    		if(node.scale.size()==3){
    			nodeMatrix = glm::scale(nodeMatrix,glm::vec3 (glm::make_vec3(node.scale.data())));
    		}
    		if(node.rotation.size()==4){
    			glm::quat rot = glm::make_quat(node.rotation.data());
    			nodeMatrix *= glm::mat4(rot);
    		}
    		if(node.translation.size()==3){
    			nodeMatrix = glm::translate(nodeMatrix,glm::vec3 (glm::make_vec3(node.translation.data())));
    		}
    		if(node.matrix.size()==16){
    			nodeMatrix = glm::make_mat4(node.matrix.data());
    		}
    		
    		NodeMat* nodeMat = new NodeMat();
		    nodeMat->parentNode = parentNodeMat;
    		nodeMat->matrix = nodeMatrix;

		    for (auto& child : node.children)
		    {
		    	LoadGLTFNode(gltfModel, gltfModel.nodes[child], nodeMat, model);
		    }
    		
    		if (node.mesh > -1)
    		{
    			model.nodeMats.push_back(nodeMat);
    			model.firstVertices.push_back(model.vertices.size());
    			model.firstIndices.push_back(model.indices.size());
    			tinygltf::Mesh& currMesh = gltfModel.meshes[node.mesh];
    			
    			if (currMesh.primitives[0].material > -1)
    			{
				    model.materials.push_back(
					    RenderingResManager::GetInstance()->materials.size() + currMesh.primitives[0].material);
    			}else
    			{
    				model.materials.push_back(0);
    			}
			    for (auto& primitive : currMesh.primitives)
			    {
			    	int verticesSize = model.vertices.size();
				    int vertexCount = 0;
				    int indexCount = 0;
			    	{
			    		const float* posBuff = nullptr;
			    		const float* normalsBuff = nullptr;
			    		const float* tangentsBuff = nullptr;
			    		const float* textCoordsBuff = nullptr;
			    		if (primitive.attributes.find("POSITION") != primitive.attributes.end())
			    		{
			    			tinygltf::Accessor& accessor = gltfModel.accessors[primitive.attributes.find("POSITION")->second];
			    			tinygltf::BufferView& view = gltfModel.bufferViews[accessor.bufferView];
			    			posBuff = reinterpret_cast<const float*> (&gltfModel.buffers[view.buffer].data[view.byteOffset + accessor.byteOffset]);
			    			vertexCount = accessor.count;
			    			
			    		}
					    if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
					    {
						    tinygltf::Accessor& accessor = gltfModel.accessors[primitive.attributes.find("NORMAL")->
							    second];
						    tinygltf::BufferView& view = gltfModel.bufferViews[accessor.bufferView];
						    normalsBuff = reinterpret_cast<const float*>(&gltfModel.buffers[view.buffer].data[view.
							    byteOffset + accessor.byteOffset]);
					    }
					    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
					    {
						    tinygltf::Accessor& accessor = gltfModel.accessors[primitive.attributes.find("TEXCOORD_0")->
							    second];
						    tinygltf::BufferView& view = gltfModel.bufferViews[accessor.bufferView];
						    textCoordsBuff = reinterpret_cast<const float*>(&gltfModel.buffers[view.buffer].data[view.
							    byteOffset + accessor.byteOffset]);
					    }
					    if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
					    {
						    tinygltf::Accessor& accessor = gltfModel.accessors[primitive.attributes.find("TANGENT")
							    ->
							    second];
						    tinygltf::BufferView& view = gltfModel.bufferViews[accessor.bufferView];
						    tangentsBuff = reinterpret_cast<const float*>(&gltfModel.buffers[view.buffer].data[view.
							    byteOffset + accessor.byteOffset]);
					    }

					    model.vertices.reserve(vertexCount);
				    	glm::mat4 worldSpaceMat = nodeMat->GetWorlMat(); 
					    for (int i = 0; i < vertexCount; ++i)
					    {
							M_Vertex3D vertex{};
					    	glm::vec3 pos =glm::make_vec3(&posBuff[i * 3]);
					    	glm::vec4 wsPos = worldSpaceMat * glm::vec4(pos, 1.0);
					    	vertex.pos = glm::vec3(wsPos.x, wsPos.y, wsPos.z);
							vertex.normal = normalsBuff ? glm::make_vec3(&normalsBuff[i * 3]) : glm::vec3(0.0f);
							//not passing vec4 tangents at the moment
							glm::vec4 tangent = tangentsBuff ? glm::make_vec4(&tangentsBuff[i * 4]) : glm::vec4(0.0f);
							vertex.tangent = tangentsBuff ? glm::vec3(tangent.x, tangent.y, tangent.z) * tangent.w: glm::vec3(0.0f);
							vertex.uv = textCoordsBuff? glm::make_vec2(&textCoordsBuff[i * 2]): glm::vec2(0.0f);
					    	vertex.id = node.mesh; 
					    	
							model.vertices.push_back(vertex);

					    }
			    	}
				    //indices
					{
						tinygltf::Accessor& accessor = gltfModel.accessors[primitive.indices];
						tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
						indexCount+= static_cast<uint32_t>(accessor.count);
						// meshIndexCount.push_back(indexCount);
						switch (accessor.componentType) {
							case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
								const uint32_t* buff = reinterpret_cast<const uint32_t*>(&(gltfModel.buffers[bufferView.buffer].data[bufferView.byteOffset + accessor.byteOffset]));
								for (size_t j = 0; j <accessor.count; ++j) {
									model.indices.push_back(verticesSize + buff[j]);
								}
								break;
							}
							case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
								const uint16_t* buff = reinterpret_cast<const uint16_t*>(&(gltfModel.buffers[bufferView.buffer].data[bufferView.byteOffset + accessor.byteOffset]));
								for (size_t j = 0; j <accessor.count; ++j) {
									model.indices.push_back(verticesSize +buff[j]);
								}
								break;
							}
							case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
								const uint8_t* buff = reinterpret_cast<const uint8_t*>(&(gltfModel.buffers[bufferView.buffer].data[bufferView.byteOffset + accessor.byteOffset]));
								for (size_t j = 0; j <accessor.count; ++j) {
									model.indices.push_back(verticesSize +buff[j]);
								}
								break;
							}
							default:
								std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
								return;
						}

					}
			    	
			    	model.indicesCount.push_back(indexCount);
			    	model.verticesCount.push_back(vertexCount);
			    }
    			
    		}
    	}

    	void LoadGLTFMaterials(tinygltf::Model &model, std::string modelPath, std::string modelPathAbs = "")
    	{
    		std::string modelFolder;
    		std::filesystem::path path (modelPath);
    		modelPath = path.parent_path().string();
		    modelFolder = modelPath;
    		if (!modelPathAbs.empty())
    		{
			    modelFolder = modelPathAbs;
    		}
    		if (!std::filesystem::exists(modelPath))
    		{
    			SYSTEMS::Logger::GetInstance()->SetLogPreferences(SYSTEMS::LogLevel::L_WARN);
    			SYSTEMS::Logger::GetInstance()->LogMessage("Texture path does not exist at: " + modelPath);
    			return;
    		};
	     
		    for (int i = 0; i < model.materials.size(); ++i)
		    {
		    	tinygltf::Material& gltfMat= model.materials[i];
		    	std::string materialName = "Material_"+ RenderingResManager::GetInstance()->materials.size();
		    	Material* material = RenderingResManager::GetInstance()->PushMaterial(materialName);
			    material->materialPackedData.diff = glm::vec4(gltfMat.pbrMetallicRoughness.baseColorFactor[0],
			                              gltfMat.pbrMetallicRoughness.baseColorFactor[1],
			                              gltfMat.pbrMetallicRoughness.baseColorFactor[2],
			                              gltfMat.pbrMetallicRoughness.baseColorFactor[3]);
		    	material->materialPackedData.roughnessFactor = static_cast<float>(gltfMat.pbrMetallicRoughness.roughnessFactor);
		    	material->materialPackedData.metallicFactor = static_cast<float>(gltfMat.pbrMetallicRoughness.metallicFactor);
		    	if (gltfMat.alphaMode =="OPAQUE"){
		    		material->materialPackedData.alphaCutoff = 1.0f;
		    	}
		    	if (gltfMat.alphaMode =="BLEND"){
		    		material->materialPackedData.alphaCutoff = gltfMat.alphaCutoff;
		    	}
			    if (gltfMat.pbrMetallicRoughness.baseColorTexture.index > -1)
			    {
				    std::string texturePath = GetGltfTexturePath(
				    	modelFolder,
					    model.images[model.textures[gltfMat.pbrMetallicRoughness.baseColorTexture.index].source].uri);
				    if (!texturePath.empty())
				    {
					    ENGINE::ImageShipper* texture = ENGINE::ResourcesManager::GetInstance()->BatchShipper(
						   texturePath, texturePath, 1, 1, ENGINE::g_ShipperFormat, ENGINE::GRAPHICS_READ);
					    material->materialPackedData.diff = glm::vec4(1.0f);
					    material->materialPackedData.albedoFactor = 1.0f;
				    	int id = ENGINE::ResourcesManager::GetInstance()->GetShipperID(texturePath);
				    	material->SetTexture(ALBEDO, id);
				    }
			    }
			    if (gltfMat.pbrMetallicRoughness.metallicRoughnessTexture.index > -1)
			    {
				    std::string texturePath = GetGltfTexturePath(
					    modelFolder,
					    model.images[model.textures[gltfMat.pbrMetallicRoughness.metallicRoughnessTexture.index].source].
					    uri);
				    if (!texturePath.empty())
				    {
					    ENGINE::ImageShipper* texture = ENGINE::ResourcesManager::GetInstance()->BatchShipper(
						    texturePath, texturePath, 1, 1, ENGINE::g_ShipperFormat, ENGINE::GRAPHICS_READ);
					    material->materialPackedData.diff = glm::vec4(1.0f);
					    material->materialPackedData.albedoFactor = 1.0f;
					    int id = ENGINE::ResourcesManager::GetInstance()->GetShipperID(texturePath);
					    material->SetTexture(METALLIC_ROUGHNESS, id);
				    }
			    	
			    }
			    if (gltfMat.emissiveTexture.index > -1)
			    {
				    std::string texturePath = GetGltfTexturePath(
						modelFolder,
						model.images[model.textures[gltfMat.emissiveTexture.index].source].
						uri);
			    	if (!texturePath.empty())
			    	{
			    		ENGINE::ImageShipper* texture = ENGINE::ResourcesManager::GetInstance()->BatchShipper(
							texturePath, texturePath, 1, 1, ENGINE::g_ShipperFormat, ENGINE::GRAPHICS_READ);
			    		material->materialPackedData.diff = glm::vec4(1.0f);
			    		material->materialPackedData.albedoFactor = 1.0f;
			    		int id = ENGINE::ResourcesManager::GetInstance()->GetShipperID(texturePath);
			    		material->SetTexture(EMISSION, id);
			    	}
			    }
			    if (gltfMat.normalTexture.index > -1)
			    {
				    std::string texturePath = GetGltfTexturePath(
						modelFolder,
						model.images[model.textures[gltfMat.normalTexture.index].source].
						uri);
			    	if (!texturePath.empty())
			    	{
			    		ENGINE::ImageShipper* texture = ENGINE::ResourcesManager::GetInstance()->BatchShipper(
							texturePath, texturePath, 1, 1, ENGINE::g_ShipperFormat, ENGINE::GRAPHICS_READ);
			    		material->materialPackedData.diff = glm::vec4(1.0f);
			    		material->materialPackedData.albedoFactor = 1.0f;
			    		int id = ENGINE::ResourcesManager::GetInstance()->GetShipperID(texturePath);
			    		material->SetTexture(NORMAL, id);
			    	}
			    }
		    }
    	}
    
    	std::string GetGltfTexturePath(std::string modelPath, std::string uriPath) {
    		if (std::filesystem::exists(uriPath)){
    			return uriPath;
    		}
    		std::string path =modelPath +"\\"+ uriPath;
    		if (std::filesystem::exists(path)){
    			return path;
    		}
    		std::cout<< "there is no valid texture path on the gltf path: "<<modelPath <<"\n"; 
    		return "";
    	}	
    	
    	ModelLoader(ENGINE::Core* core)
    	{
    		this->core = core; 
    	}
	    static ModelLoader* GetInstance(ENGINE::Core* core = nullptr)
	    {
		    if (instance == nullptr && core != nullptr)
		    {
			    instance = new ModelLoader(core);
		    }
		    return instance;
	    }
        static ModelLoader* instance;
    	ENGINE::Core* core;
    };

    ModelLoader* ModelLoader::instance = nullptr;
}


#endif //MODELLOADER_HPP
