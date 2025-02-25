#include "rendering.h"

#define SCREEN_WIDTH  1280 // 1920
#define SCREEN_HEIGHT 720  // 1080

// CONSIDER ADDING IMGUI WIDGETS
#define CAM_FOV 45.f
#define NUM_TEXTURES 13

bool rotate_cam = false;

int main()
{
	Window window = {};
	Keyboard keys = {};
	Mouse mouse = {};

	init_window(&window, SCREEN_WIDTH, SCREEN_HEIGHT, "GL Waves");
	init_keyboard(&keys);

	float radius = 75.0f;
	Camera camera = {};
	camera.position = vec3(sin(glfwGetTime() * 0.5) * radius, 50, cos(glfwGetTime() * 0.5) * radius);

	mat4 model = glm::scale(mat4(1.0f), vec3(100.0f));
	mat4 view = glm::lookAt(camera.position, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	mat4 projection = glm::perspective(ToRadians(CAM_FOV), (float)1280.f / (float)720.f, 0.1f, 500.f);

	// create shader program
	GLuint shader_program = {};
	shader_program = glCreateProgram();

	GLuint temp_shader = load_shader("assets/shaders/water.vert", GL_VERTEX_SHADER);
	glAttachShader(shader_program, temp_shader);
	glDeleteShader(temp_shader);

	temp_shader = load_shader("assets/shaders/water_tess_control.glsl", GL_TESS_CONTROL_SHADER);
	glAttachShader(shader_program, temp_shader);
	glDeleteShader(temp_shader);

	temp_shader = load_shader("assets/shaders/water_tess_eval.glsl", GL_TESS_EVALUATION_SHADER);
	glAttachShader(shader_program, temp_shader);
	glDeleteShader(temp_shader);

	temp_shader = load_shader("assets/shaders/water.frag", GL_FRAGMENT_SHADER);
	glAttachShader(shader_program, temp_shader);
	glDeleteShader(temp_shader);

	// link Shader Program
	int success = 0;
	glLinkProgram(shader_program);
	glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
	if (success == NULL) out("ERROR : COULD NOT LINK SHADER PROGRAM!");

	// render setup
	GLuint VAO = 0;
	GLuint heightMap[NUM_TEXTURES] = {};
	GLuint normalMap[NUM_TEXTURES] = {};

	glGenVertexArrays(1, &VAO);
	glGenTextures(NUM_TEXTURES, heightMap);
	glGenTextures(NUM_TEXTURES, normalMap);
	
	for (int i = 0; i < NUM_TEXTURES; ++i)
	{
		char heightmap_filename[256] = {};
		sprintf(heightmap_filename, "assets/textures/heights/%d.png", i + 1);
		load_texture(heightMap[i], std::string(heightmap_filename));

		char normalmap_filename[256] = {};
		sprintf(normalmap_filename, "assets/textures/normals/%d.png", i + 1);
		load_texture(normalMap[i], std::string(normalmap_filename));
	}

	GLuint waterTex;
	GLuint wavesNormalMap;
	GLuint wavesHeightMap;

	glGenTextures(1, &waterTex);
	glGenTextures(1, &wavesNormalMap);
	glGenTextures(1, &wavesHeightMap);

	load_texture(waterTex      , "assets/textures/water.jpg");
	load_texture(wavesNormalMap, "assets/textures/wavesNormal.jpg");
	load_texture(wavesHeightMap, "assets/textures/wavesHeight.jpg");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, heightMap[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, heightMap[1]);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, normalMap[0]);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, normalMap[1]);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, waterTex);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, wavesHeightMap);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, wavesNormalMap);

	static float interpolateFactor = 0.0f;
	static float offset = 0.0f;

	// set shader uniforms
	glUseProgram(shader_program);
	set_vec3 (shader_program, "light.direction"  , vec3(0.0 , -1.0, 0.0 ));
	set_vec3 (shader_program, "light.ambient"    , vec3(0.15, 0.15, 0.15));
	set_vec3 (shader_program, "light.diffuse"    , vec3(0.75, 0.75, 0.75));
	set_vec3 (shader_program, "light.specular"   , vec3(1.0 , 1.0 , 1.0 ));
	set_float(shader_program, "interpolateFactor", interpolateFactor);
	set_float(shader_program, "depth"            , 0.11f);
	set_int  (shader_program, "tessLevel"        , 1);

	set_int(shader_program, "heightMap1"    , 0);
	set_int(shader_program, "heightMap2"    , 1);
	set_int(shader_program, "normalMap1"    , 2);
	set_int(shader_program, "normalMap2"    , 3);
	set_int(shader_program, "water"         , 4);
	set_int(shader_program, "wavesHeightMap", 5);
	set_int(shader_program, "wavesNormalMap", 6);

	// frame timer
	float frame_time = 1.f / 60;
	int64 target_frame_milliseconds = frame_time * 1000.f;
	Timestamp frame_start = get_timestamp(), frame_end;

	while (!glfwWindowShouldClose(window.instance))
	{
		update_window(window);
		update_keyboard(&keys, window);
		update_mouse(&mouse, window);

		camera_update_dir(&camera, mouse.dx, mouse.dy);

		// controls
		if (keys.ESC.is_pressed) glfwSetWindowShouldClose(window.instance, true);
		if (keys.P.is_pressed) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		if (keys.O.is_pressed) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		if (keys.R.is_pressed) rotate_cam = true;
		if (keys.T.is_pressed) rotate_cam = false;

		if (rotate_cam)
		{
			camera.position = vec3(sin(glfwGetTime() * 0.5) * radius, 50, cos(glfwGetTime() * 0.5) * radius);
		}

		view = glm::lookAt(camera.position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		// blending(interpolating) between water textures
		interpolateFactor += 1.5f * frame_time;
		if (interpolateFactor > 1) // reset interpolation & use next texture in sequence
		{
			interpolateFactor = 0.0f;

			static uint texture_index = 0;
			if (++texture_index == NUM_TEXTURES - 1)
			{
				texture_index = 0;
			}

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, heightMap[texture_index]);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, heightMap[texture_index + 1]);

			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, normalMap[texture_index]);

			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, normalMap[texture_index + 1]);
		}
		
		offset += .25 * frame_time;
		if (offset >= 1000000)
		{
			offset = 0;
		}

		// --- rendering water

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shader_program);
		set_mat4 (shader_program, "model", model);
		set_mat4 (shader_program, "mvp", projection * view * model);
		set_vec3 (shader_program, "viewPos", camera.position);
		set_float(shader_program, "interpolateFactor", interpolateFactor);
		set_float(shader_program, "wavesOffset", offset);

		glBindVertexArray(VAO);
		glPatchParameteri(GL_PATCH_VERTICES, 4);
		glDrawArraysInstanced(GL_PATCHES, 0, 4, 64 * 64);

		//Frame Time
		frame_end = get_timestamp();
		int64 milliseconds_elapsed = calculate_milliseconds_elapsed(frame_start, frame_end);

		//print("frame time: %02d ms | fps: %06f\n", milliseconds_elapsed, 1000.f / milliseconds_elapsed);
		if (target_frame_milliseconds > milliseconds_elapsed) // frame finished early
		{
			os_sleep(target_frame_milliseconds - milliseconds_elapsed);
		}

		frame_start = frame_end;
	}

	shutdown_window();
	return 0;
}