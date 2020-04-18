#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "loader.h"
#include <spdlog/spdlog.h>


std::tuple<std::vector<unsigned short>,
		   std::vector<glm::vec3>,
		   std::vector<glm::vec2>,
		   std::vector<glm::vec3>> load_asset(const char *path) {
	std::vector<unsigned short> indices;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	Assimp::Importer importer;

	const aiScene *scene = importer.ReadFile(path,
		aiProcess_MakeLeftHanded |
		aiProcess_FlipWindingOrder |
		aiProcess_FlipUVs |
		aiProcess_PreTransformVertices |
		aiProcess_CalcTangentSpace |
		aiProcess_GenSmoothNormals |
		aiProcess_Triangulate |
		aiProcess_FixInfacingNormals |
		aiProcess_FindInvalidData |
		aiProcess_ValidateDataStructure);
	if(!scene) {	
		throw asset_error(path);
	}

	const aiMesh* mesh = scene->mMeshes[0];

	vertices.reserve(mesh->mNumVertices);
	for( int i=0; i<mesh->mNumVertices; i++){
		aiVector3D pos = mesh->mVertices[i];
		vertices.push_back(glm::vec3(pos.x, pos.y, pos.z));
	}

	if (mesh->mNumUVComponents[0]) {
		uvs.reserve(mesh->mNumVertices);
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			aiVector3D UVW = mesh->mTextureCoords[0][i];
			uvs.push_back(glm::vec2(UVW.x, UVW.y));
		}
	} else {
		spdlog::warn("Loaded model {} does not contain uv coordinates", path);
	}

	normals.reserve(mesh->mNumVertices);
	for(unsigned int i=0; i<mesh->mNumVertices; i++){
		aiVector3D n = mesh->mNormals[i];
		normals.push_back(glm::vec3(n.x, n.y, n.z));
	}

	indices.reserve(3*mesh->mNumFaces);
	for (unsigned int i=0; i<mesh->mNumFaces; i++){
		// Assume the model has only triangles.
		indices.push_back(mesh->mFaces[i].mIndices[0]);
		indices.push_back(mesh->mFaces[i].mIndices[1]);
		indices.push_back(mesh->mFaces[i].mIndices[2]);
	}

	return std::make_tuple(std::move(indices), std::move(vertices), std::move(uvs), std::move(normals));
}
