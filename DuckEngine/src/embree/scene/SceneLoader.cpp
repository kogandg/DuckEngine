#include <embree/scene/SceneLoader.h>
#include <embree/lights/DirectionalLight.h>


SceneFileParsingOutput SceneLoader::ParseSceneFromTestFile(const char* fileName)
{
	Tokenizer* tokenizer = new Tokenizer();
	tokenizer->Open(fileName);

	auto scene = std::make_shared<Scene>();
	auto target = std::make_shared<RenderTarget>(0, 0);
	std::shared_ptr<Integrator> integrator = std::make_shared<RayTracerIntegrator>();
	integrator.get()->SetMaxDepth(5);

	std::vector<glm::vec3> verts;
	std::stack<glm::mat4> transformationStack;

	Material* currentMaterial = new RayTracingMaterial(glm::vec3(0.0f), glm::vec3(0.0f), 0, glm::vec3(0.0f), glm::vec3(0.2f));

	glm::vec3 currentAttenuation = glm::vec3(1, 0, 0);

	transformationStack.push(glm::mat4(1.0f));

	std::string command = "";
	do
	{
		command = tokenizer->GetToken();

		if (command == "#")
		{
			tokenizer->FindToken("\n");
			continue;
		}//line is a comment

		if (command == "size")
		{
			int width = tokenizer->GetInt();
			int height = tokenizer->GetInt();

			target = std::make_shared<RenderTarget>(width, height);
		}

		if (command == "integrator")
		{
			std::string type = tokenizer->GetToken();

			if (type == "raytracer")
			{
				integrator = std::make_shared<RayTracerIntegrator>();

				currentMaterial = new RayTracingMaterial(glm::vec3(0.0f), glm::vec3(0.0f), 0, glm::vec3(0.0f), glm::vec3(0.2f));
			}
		}

		if (command == "maxdepth")
		{
			int depth = tokenizer->GetInt();

			integrator.get()->SetMaxDepth(depth);
		}

		if (command == "camera")
		{
			float lookfromx = tokenizer->GetFloat();
			float lookfromy = tokenizer->GetFloat();
			float lookfromz = tokenizer->GetFloat();
			float lookatx = tokenizer->GetFloat();
			float lookaty = tokenizer->GetFloat();
			float lookatz = tokenizer->GetFloat();
			float upx = tokenizer->GetFloat();
			float upy = tokenizer->GetFloat();
			float upz = tokenizer->GetFloat();

			auto fovy = tokenizer->GetFloat();

			auto lookFrom = glm::vec3(lookfromx, lookfromy, lookfromz);
			auto lookAt = glm::vec3(lookatx, lookaty, lookatz);
			auto up = glm::vec3(upx, upy, upz);

			Camera camera = Camera(lookFrom, lookAt, up, fovy);
			scene.get()->SetCamera(std::make_shared<Camera>(camera));
		}

		if (command == "vertex")
		{
			float x = tokenizer->GetFloat();
			float y = tokenizer->GetFloat();
			float z = tokenizer->GetFloat();
			verts.push_back(glm::vec3(x, y, z));
		}
		if (command == "tri")
		{
			int v1 = tokenizer->GetInt();
			int v2 = tokenizer->GetInt();
			int v3 = tokenizer->GetInt();
			//triangles.push_back(glm::ivec3(v1, v2, v3));
			//triangles.push_back(tokenizer->GetInt());

			auto triangleGeom = Triangle(verts[v1], verts[v2], verts[v3]);

			auto mat = std::make_shared<RayTracingMaterial>(*dynamic_cast<RayTracingMaterial*>(currentMaterial));

			auto triangle = std::make_shared<RenderableObject>(RenderableObject("tri", std::make_shared<Triangle>(triangleGeom), mat, transformationStack.top()));

			scene.get()->AddObject(triangle);
		}

		if (command == "sphere")
		{
			float x = tokenizer->GetFloat();
			float y = tokenizer->GetFloat();
			float z = tokenizer->GetFloat();
			float r = tokenizer->GetFloat();

			glm::vec3 position = glm::vec3(x, y, z);

			auto sphereGeom = Sphere(position, r);

			auto mat = std::make_shared<RayTracingMaterial>(*dynamic_cast<RayTracingMaterial*>(currentMaterial));

			auto sphere = std::make_shared<RenderableObject>(RenderableObject("sphere", std::make_shared<Sphere>(sphereGeom), mat, transformationStack.top()));

			scene.get()->AddObject(sphere);
		}

		if (command == "background")
		{
			std::string type = tokenizer->GetToken();

			if (type == "none")
			{
				integrator->SetBackgroundType(None);
			}
			if (type == "color")
			{
				float r = tokenizer->GetFloat();
				float g = tokenizer->GetFloat();
				float b = tokenizer->GetFloat();

				integrator->SetBackgroundType(Color, glm::vec3(r, g, b));
			}
			if (type == "simpleSky")
			{
				integrator->SetBackgroundType(SimpleSky);
			}
		}

		if (command == "ambient")
		{
			float r = tokenizer->GetFloat();
			float g = tokenizer->GetFloat();
			float b = tokenizer->GetFloat();
			if (RayTracingMaterial* rtm = dynamic_cast<RayTracingMaterial*>(currentMaterial))
			{
				rtm->SetAmbient(glm::vec3(r, g, b));
			}
		}

		if (command == "diffuse")
		{
			float r = tokenizer->GetFloat();
			float g = tokenizer->GetFloat();
			float b = tokenizer->GetFloat();
			if (RayTracingMaterial* rtm = dynamic_cast<RayTracingMaterial*>(currentMaterial))
			{
				rtm->SetDiffuse(glm::vec3(r, g, b));
			}
		}
		if (command == "specular")
		{
			float r = tokenizer->GetFloat();
			float g = tokenizer->GetFloat();
			float b = tokenizer->GetFloat();
			if (RayTracingMaterial* rtm = dynamic_cast<RayTracingMaterial*>(currentMaterial))
			{
				rtm->SetSpecular(glm::vec3(r, g, b));
			}
		}
		if (command == "shininess")
		{
			float s = tokenizer->GetFloat();
			if (RayTracingMaterial* rtm = dynamic_cast<RayTracingMaterial*>(currentMaterial))
			{
				rtm->SetShininess(s);
			}
		}
		if (command == "emission")
		{
			float r = tokenizer->GetFloat();
			float g = tokenizer->GetFloat();
			float b = tokenizer->GetFloat();
			if (RayTracingMaterial* rtm = dynamic_cast<RayTracingMaterial*>(currentMaterial))
			{
				rtm->SetEmission(glm::vec3(r, g, b));
			}
		}

		if (command == "translate")
		{
			float x = tokenizer->GetFloat();
			float y = tokenizer->GetFloat();
			float z = tokenizer->GetFloat();
			transformationStack.top() = glm::translate(transformationStack.top(), glm::vec3(x, y, z));
		}
		if (command == "rotate")
		{
			float x = tokenizer->GetFloat();
			float y = tokenizer->GetFloat();
			float z = tokenizer->GetFloat();
			float angle = tokenizer->GetFloat();
			//auto bad = angle * 57.2957795;//to radians
			//auto test = glm::radians(angle);

			transformationStack.top() = glm::rotate(transformationStack.top(), glm::radians(angle), glm::vec3(x, y, z));
		}
		if (command == "scale")
		{
			float x = tokenizer->GetFloat();
			float y = tokenizer->GetFloat();
			float z = tokenizer->GetFloat();
			transformationStack.top() = glm::scale(transformationStack.top(), glm::vec3(x, y, z));
		}

		if (command == "pushTransform")
		{
			transformationStack.push(transformationStack.top());
		}
		if (command == "popTransform")
		{
			transformationStack.pop();
			//auto test = transformationStack.top();
		}

		if (command == "attenuation")
		{
			float x = tokenizer->GetFloat();
			float y = tokenizer->GetFloat();
			float z = tokenizer->GetFloat();
			currentAttenuation = glm::vec3(x, y, z);
		}

		if (command == "point")
		{
			float x = tokenizer->GetFloat();
			float y = tokenizer->GetFloat();
			float z = tokenizer->GetFloat();
			float r = tokenizer->GetFloat();
			float g = tokenizer->GetFloat();
			float b = tokenizer->GetFloat();

			auto pointLight = std::make_shared<PointLight>("pointLight", glm::vec3(x, y, z), glm::vec3(r, g, b), currentAttenuation);

			scene.get()->AddObject(pointLight);

			//light->attenuation = currentAttenuation;
		}
		if (command == "directional")
		{
			float x = tokenizer->GetFloat();
			float y = tokenizer->GetFloat();
			float z = tokenizer->GetFloat();
			float r = tokenizer->GetFloat();
			float g = tokenizer->GetFloat();
			float b = tokenizer->GetFloat();

			auto directionalLight = std::make_shared<DirectionalLight>("directionalLight", glm::vec3(x, y, z), glm::vec3(r, g, b));

			scene.get()->AddObject(directionalLight);
		}


	} while (command != "");

	SceneFileParsingOutput output = { scene, target, integrator};
	return output;
}
