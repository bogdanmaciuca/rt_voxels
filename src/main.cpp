#include <vector>
#include <array>
#include <fstream>
#include <cassert>
#include <span>
#include <cstring>
#define OGT_VOX_IMPLEMENTATION
#include "../vendor/ogt_vox.h"
#define GLW_IMPLEMENTATION
#include "glw.hpp"

std::string ReadFile(const std::string& filename) {
    std::ifstream file(filename);
    assert(file.is_open());
    std::string result;
    std::string line;
    while (std::getline(file, line)) {
        result += line + '\n';
    }
    return result;
}

enum {
    WND_WIDTH = 1024,
    WND_HEIGHT = 768
};

struct SceneMetadata {
    glw::i32 size_x, size_y, size_z;
    std::array<glm::vec4, 256> palette;
};

int main() {
    const std::string vox_data = ReadFile("res/spellbook.vox");

    const ogt_vox_scene* scene_data = ogt_vox_read_scene((uint8_t*)vox_data.data(), vox_data.size());
    assert(scene_data->num_models != 0);
 
    SceneMetadata scene_metadata;
    scene_metadata.size_x = scene_data->models[0]->size_x;
    scene_metadata.size_y = scene_data->models[0]->size_y;
    scene_metadata.size_z = scene_data->models[0]->size_z;
    for (glw::u32 i = 0; i < scene_metadata.palette.size(); i++) {
        scene_metadata.palette[i] = glm::vec4(
            scene_data->palette.color[i].r / 255.0f,
            scene_data->palette.color[i].g / 255.0f,
            scene_data->palette.color[i].b / 255.0f,
            scene_data->palette.color[i].a / 255.0f
        );
    }

    std::vector<glw::u8> ssbo_data;
    const glw::u32 voxel_data_size = sizeof(glw::u8) * scene_metadata.size_x * scene_metadata.size_y * scene_metadata.size_z;
    ssbo_data.resize(sizeof(SceneMetadata) + scene_metadata.size_x * scene_metadata.size_y * scene_metadata.size_z);
    std::memcpy(ssbo_data.data(), &scene_metadata, sizeof(SceneMetadata));
    std::memcpy(ssbo_data.data() + sizeof(SceneMetadata), scene_data->models[0]->voxel_data, voxel_data_size);

    glw::Context context("Voxel raytracer", WND_WIDTH, WND_HEIGHT);

    glw::ShaderStorageBuffer<GL_DYNAMIC_DRAW> ssbo(ssbo_data);

    std::vector<glm::vec2> vertices = {
        { 1.0f,  1.0f },
        { 1.0f, -1.0f },
        {-1.0f, -1.0f },
        {-1.0f,  1.0f }
    };
    glw::VertexBuffer<glm::vec2> vertex_buffer(vertices);

    const std::vector<glw::u32> indices = { 0, 1, 3, 1, 2, 3 };
    glw::IndexBuffer<glw::u32> index_buffer(indices);

    glw::VertexArrayObject vertex_array_object(
        &vertex_buffer, &index_buffer, { {GL_FLOAT, 2} }
    );

    glw::Shader shader(
        ReadFile("src/shaders/rt.vert.glsl"),
        ReadFile("src/shaders/rt.frag.glsl")
    );
    shader.Bind();
    shader.SetFloat("uRatio", (float)WND_WIDTH/(float)WND_HEIGHT);

    glw::FPSCamera camera(80.0f, (float)WND_WIDTH / (float)WND_HEIGHT);
    camera.SetPos(glm::vec3(60, 60, 60));

    bool should_quit = false;
    SDL_Event evt;
    while (!should_quit) {
        const float delta_time = context.UpdateDeltaTime();
        while (SDL_PollEvent(&evt)) {
            switch (evt.type) {
                case SDL_QUIT:
                    should_quit = true;
                    break;
            }
        }
        const glw::u8* keys = SDL_GetKeyboardState(nullptr);

        if (keys[SDL_SCANCODE_W])
            camera.Move(glw::CameraForward, delta_time);
        else if (keys[SDL_SCANCODE_S])
            camera.Move(glw::CameraBackward, delta_time);
        if (keys[SDL_SCANCODE_A])
            camera.Move(glw::CameraLeft, delta_time);
        else if (keys[SDL_SCANCODE_D])
            camera.Move(glw::CameraRight, delta_time);


        camera.ProcessMouse(context);
        
        shader.SetVec3("uCamPos", camera.GetPos());
        shader.SetMat4("uInvProj", glm::inverse(camera.GetProjection()));
        shader.SetMat4("uInvView", glm::inverse(camera.GetViewMatrix()));
        vertex_array_object.Draw();
        context.Present();
    }

    ogt_vox_destroy_scene(scene_data);
}

