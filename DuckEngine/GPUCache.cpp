#include "GPUCache.h"

GLuint GPUCache::loadTexture(const char* path)
{
	GLuint textureID;
	glGenTextures(1, &textureID);

	int width;
	int height;
	int numComponents;
	unsigned char* data = stbi_load(path, &width, &height, &numComponents, 0);
	if (data)
	{
		GLenum format;
		if (numComponents == 1) format = GL_RED;
		else if (numComponents == 3) format = GL_RGB;
		else if (numComponents == 4) format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

std::shared_ptr<GPUGeometry> GPUCache::createGeometry(const std::shared_ptr<ECS::MeshData>& md, const VertexLayoutKey& layout)
{
	auto geom = GPUGeometry();
	geom.layout = layout;

	glGenBuffers(1, &(geom.VBO));

	glBindBuffer(GL_ARRAY_BUFFER, geom.VBO);
	glBufferData(GL_ARRAY_BUFFER, md.get()->verts.size() * sizeof(float), md.get()->verts.data(), GL_STATIC_DRAW);

	geom.vertexCount = (GLsizei)(md.get()->verts.size() * sizeof(float) / geom.layout.stride);

	auto geomPointer = std::make_shared<GPUGeometry>(geom);
	geomCache[md] = geomPointer;
	return geomPointer;
}

std::shared_ptr<Shader> GPUCache::createShader(GPUMaterial::Type type, const char* vertexPath, const char* fragmentPath)
{
	auto shader = std::make_shared<Shader>(vertexPath, fragmentPath);

	switch (type)
	{
	case GPUMaterial::Type::Color:
	{
		shader.get()->CacheUniform("view");
		shader.get()->CacheUniform("projection");
		shader.get()->CacheUniform("model");
		shader.get()->CacheUniform("color");
	} break;
	case GPUMaterial::Type::Phong:
	{
		shader.get()->CacheUniform("view");
		shader.get()->CacheUniform("projection");
		shader.get()->CacheUniform("model");
		shader.get()->CacheUniform("transposeInverseModel");
		shader.get()->CacheUniform("viewPos");

		//can set once but since this same shader can be used for lots of objects with different vals its better to set each frame
		shader.get()->CacheUniform("material.shininess");
		shader.get()->CacheUniform("material.diffuse");
		shader.get()->CacheUniform("material.specular");

		//directional light
		shader.get()->CacheUniform("dirLight.direction");
		shader.get()->CacheUniform("dirLight.ambient");
		shader.get()->CacheUniform("dirLight.diffuse");
		shader.get()->CacheUniform("dirLight.specular");

		//point lights
		for (int i = 0; i < 4; i++)
		{
			std::string index = std::to_string(i);
			shader.get()->CacheUniform(("pointLights[" + index + "].position").c_str());
			shader.get()->CacheUniform(("pointLights[" + index + "].ambient").c_str());
			shader.get()->CacheUniform(("pointLights[" + index + "].diffuse").c_str());
			shader.get()->CacheUniform(("pointLights[" + index + "].specular").c_str());
			shader.get()->CacheUniform(("pointLights[" + index + "].constant").c_str());
			shader.get()->CacheUniform(("pointLights[" + index + "].linear").c_str());
			shader.get()->CacheUniform(("pointLights[" + index + "].quadratic").c_str());
		}

		//spotlight
		shader.get()->CacheUniform("spotLight.position");
		shader.get()->CacheUniform("spotLight.direction");
		shader.get()->CacheUniform("spotLight.ambient");
		shader.get()->CacheUniform("spotLight.diffuse");
		shader.get()->CacheUniform("spotLight.specular");
		shader.get()->CacheUniform("spotLight.constant");
		shader.get()->CacheUniform("spotLight.linear");
		shader.get()->CacheUniform("spotLight.quadratic");
		shader.get()->CacheUniform("spotLight.cutOff");
		shader.get()->CacheUniform("spotLight.outerCutOff");
	} break;
	}

	shaderCache[type] = shader;
	return shader;
}

std::shared_ptr<GLuint> GPUCache::createTexture(const std::string& path)
{
	auto texture = std::make_shared<GLuint>(loadTexture(path.c_str()));
	textureCache[path] = texture;

	return texture;
}

std::shared_ptr<GPUMaterial> GPUCache::createMaterial(const std::shared_ptr<ECS::MaterialData>& md)
{
	auto material = std::make_shared<GPUMaterial>();
	switch (md.get()->type)
	{
	case ECS::MaterialData::Type::Color:
	{
		material.get()->type = GPUMaterial::Type::Color;

		if (!shaderCache[material.get()->type])
		{
			createShader(GPUMaterial::Type::Color, "color.vert", "color.frag");
		}
		material.get()->shader = shaderCache[material.get()->type];

		auto& d = std::get<ECS::ColorData>(md.get()->data);
		material.get()->color = d.color;
	} break;
	case ECS::MaterialData::Type::Phong:
	{
		material.get()->type = GPUMaterial::Type::Phong;

		if (!shaderCache[material.get()->type])
		{
			createShader(GPUMaterial::Type::Phong, "lighting.vert", "lighting.frag");
		}
		material.get()->shader = shaderCache[material.get()->type];

		auto& d = std::get<ECS::PhongData>(md.get()->data);

		if (!textureCache[d.diffuseTexturePath])
		{
			createTexture(d.diffuseTexturePath);
		}
		material.get()->diffuseTexture = textureCache[d.diffuseTexturePath];
		if (!textureCache[d.specularTexturePath])
		{
			createTexture(d.specularTexturePath);
		}
		material.get()->specularTexture = textureCache[d.specularTexturePath];

		material.get()->shininess = d.shininess;
	}break;
	}

	materialCache[md] = material;
	return material;
}

std::shared_ptr<GLuint> GPUCache::createVAO(const GPUGeometry& geom, const VertexLayoutKey& layout, const ShaderSignature& signature)
{
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, geom.VBO);

	for (auto& req : signature.required)
	{
		auto it = std::find_if(
			layout.attrs.begin(), layout.attrs.end(),
			[&](const VertexAttrib& a) { return a.location == req.location; });

		if (it == layout.attrs.end())
		{
			throw std::runtime_error("Geometry missing attribute required by shader!");
		}

		const VertexAttrib& attr = *it;
		glEnableVertexAttribArray(attr.location);
		glVertexAttribPointer(attr.location, attr.size, attr.type, GL_FALSE, layout.stride, (void*)(uintptr_t)attr.offset);
	}

	vaoCache[{geom.VBO, signature}] = std::make_shared<GLuint>(vao);
	return vaoCache[{geom.VBO, signature}];
}

ShaderSignature GPUCache::GetRequiredSignature(const GPUMaterial& mat)
{
	ShaderSignature sig;
		switch (mat.type)
		{
		case GPUMaterial::Type::Color:
			sig =
			{
				{
					{0, 3, GL_FLOAT, 0}
				}
			};
			break;
		case GPUMaterial::Type::Phong:
			sig =
			{
				{
				{0, 3, GL_FLOAT, 0},
				{1, 3, GL_FLOAT, sizeof(float) * 3},
				{2, 2, GL_FLOAT, sizeof(float) * 6}
				}
			};
			break;
		default:
			break;
		}
	return sig;
}

void GPUCache::PreloadGPU(ECS::ECSRegistry& registry)
{
	for (auto& [entity, meshComp] : registry.view<ECS::Mesh>())
	{
		auto& matComp = registry.getComponent<ECS::Material>(entity);

		VertexLayoutKey layout
		{
			sizeof(float) * 8, //stride = pos(3) + norm(3) + text coords(2)
			{
			{0, 3, GL_FLOAT, 0},
			{1, 3, GL_FLOAT, sizeof(float) * 3},
			{2, 2, GL_FLOAT, sizeof(float) * 6}
			}
		};

		auto gpuMat = materialCache[matComp.data];
		if (!gpuMat)
		{
			gpuMat = createMaterial(matComp.data);
		}

		auto geom = geomCache[meshComp.data];
		if (!geom)
		{
			geom = createGeometry(meshComp.data, layout);
		}
		
		ShaderSignature sig = GetRequiredSignature(*gpuMat);
		if (!vaoCache[{geom.get()->VBO, sig}])
		{
			createVAO(*geom, layout, sig);
		}

		//auto vao = getOrCreateVAO(cache, geom, gpuMat.get()->shader);
	}
}
