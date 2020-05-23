#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "loader.h"
#include <spdlog/spdlog.h>


namespace loader {

std::pair<std::vector<vertex>, std::vector<unsigned int>> load_asset(const char* path) {
	std::vector<unsigned int> indices;
	std::vector<vertex> vertices;

	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(path,
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
	if (!scene) {
		throw asset_error(path);
	}

	const aiMesh* mesh = scene->mMeshes[0];
	if (!mesh->mNumUVComponents[0]) {
		throw std::exception("Mesh does not contain uv coordinates");
	}

	vertices.reserve(mesh->mNumVertices);
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		aiVector3D pos = mesh->mVertices[i];
		aiVector3D norm = mesh->mNormals[i];
		aiVector3D uv = mesh->mTextureCoords[0][i];

		vertices.push_back({
			glm::vec3{pos.x, pos.y, pos.z},
			glm::vec3{norm.x, norm.y, norm.z},
			glm::vec2{uv.x, uv.y}
		});
	}

	indices.reserve(3 * mesh->mNumFaces);
	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		// AssumewW the model has only triangles.
		indices.push_back(mesh->mFaces[i].mIndices[0]);
		indices.push_back(mesh->mFaces[i].mIndices[1]);
		indices.push_back(mesh->mFaces[i].mIndices[2]);
	}

	return std::make_pair(std::move(vertices), std::move(indices));
}

}