#include "common.hpp"
#include <cstring>
#define OGT_VOX_IMPLEMENTATION
#include "../vendor/ogt_vox.h"
#define GLW_IMPLEMENTATION
#include "glw.hpp"

enum {
    WND_WIDTH = 1024,
    WND_HEIGHT = 768,
    VoxPaletteSize = 256
};

struct Scene {
    struct Metadata {
        glm::ivec3 size;
        array<glm::vec4, VoxPaletteSize> palette;
    };
    Metadata metadata;
    vector<u8> voxels;
};

class Raytracer {
public:
    Raytracer(const string& vert_path, const string& frag_path)
        : m_shader(), m_ssbo(0),
        m_vertex_array_object(&m_vertex_buffer, {{GL_FLOAT, 2}}, &m_index_buffer) {}
    void LoadScene(const string& path) {
        string vox_file_contents;
        File file(path);
        ASSERT(file.IsValid(), "Could not open file!");
        file.ReadAll(&vox_file_contents);

        const ogt_vox_scene* scene_data = ogt_vox_read_scene(
            (uint8_t*)vox_file_contents.data(), vox_file_contents.size()
        );
        ASSERT(scene_data->num_models >= 1, "File has no models!");

        // Scene dimensions
        m_scene.metadata.size.x = scene_data->models[0]->size_x;
        m_scene.metadata.size.y = scene_data->models[0]->size_y;
        m_scene.metadata.size.z = scene_data->models[0]->size_z;

        // Scene palette
        for (u32 i = 0; i < m_scene.metadata.palette.size(); i++) {
            m_scene.metadata.palette[i] = glm::vec4(
                scene_data->palette.color[i].r / 255.0f,
                scene_data->palette.color[i].g / 255.0f,
                scene_data->palette.color[i].b / 255.0f,
                scene_data->palette.color[i].a / 255.0f
            );
        }

        // Voxel data (indices to palette)
        const u32 voxel_data_byte_size =
            m_scene.metadata.size.x *
            m_scene.metadata.size.y *
            m_scene.metadata.size.z;
        m_scene.voxels.resize(voxel_data_byte_size);
        memcpy(m_scene.voxels.data(), scene_data->models[0]->voxel_data, voxel_data_byte_size);

        ogt_vox_destroy_scene(scene_data);

        // Shader
        string vert_source, frag_source;
        File("src/shaders/rt.vert.glsl").ReadAll(&vert_source);
        File("src/shaders/rt.frag.glsl").ReadAll(&frag_source);
        m_shader.Compile(vert_source, frag_source);
        m_shader.Bind();
        m_shader.SetFloat("uRatio", (float)WND_WIDTH / (float)WND_HEIGHT);

        // Shader storage buffer
        ByteBuffer ssbo_data;
        ssbo_data.Add(&m_scene.metadata);
        ssbo_data.Extend<u8>(m_scene.voxels);
        m_ssbo.Source(ssbo_data.AsVec());

        m_vertex_buffer.Source(m_vertices);
        m_index_buffer.Source(m_indices);
    }
    void Render(const glw::FPSCamera& camera) {
        m_shader.Bind();
        m_shader.SetVec3("uCamPos", camera.GetPos());
        m_shader.SetMat4("uInvProj", glm::inverse(camera.GetProjection()));
        m_shader.SetMat4("uInvView", glm::inverse(camera.GetViewMatrix()));
        m_vertex_array_object.Draw();
    }
private:
    Scene m_scene;
    constexpr static array<glm::vec2, 4> m_vertices = {
        glm::vec2(1.0f,  1.0f),
        glm::vec2(1.0f, -1.0f),
        glm::vec2(-1.0f, -1.0f),
        glm::vec2(-1.0f,  1.0f)
    };
    constexpr static array<u32, 6> m_indices = { 0, 1, 3, 1, 2, 3 };
    glw::Shader m_shader;
    glw::ShaderStorageBuffer m_ssbo;
    glw::VertexBuffer<glm::vec2> m_vertex_buffer;
    glw::IndexBuffer<u32> m_index_buffer;
    glw::VertexArrayObject<glm::vec2, u32> m_vertex_array_object;
};

void UpdateCamera(glw::FPSCamera* camera, float delta_time) {
    const glw::u8* keys = SDL_GetKeyboardState(nullptr);

    if (keys[SDL_SCANCODE_W])
        camera->Move(glw::CameraForward, delta_time);
    else if (keys[SDL_SCANCODE_S])
        camera->Move(glw::CameraBackward, delta_time);
    if (keys[SDL_SCANCODE_A])
        camera->Move(glw::CameraLeft, delta_time);
    else if (keys[SDL_SCANCODE_D])
        camera->Move(glw::CameraRight, delta_time);

    camera->ProcessMouse();
}

int main() {
    glw::Context context("Voxel raytracer", WND_WIDTH, WND_HEIGHT);

    Raytracer raytracer("src/shaders/rt.vert.glsl", "src/shaders/rt.frag.glsl");
    raytracer.LoadScene("res/spellbook.vox");

    glw::FPSCamera camera(80.0f, (float)WND_WIDTH / (float)WND_HEIGHT);
    camera.SetPos(glm::vec3(60, 60, 60));
    camera.SetSpeed(glw::FPSCamera::DefaultSpeed * 3);

    const std::vector<glm::vec3> cube_vertices = {
        { -1.0f,-1.0f,-1.0f }, { -1.0f,-1.0f, 1.0f }, { -1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f,-1.0f }, { -1.0f,-1.0f,-1.0f }, { -1.0f, 1.0f,-1.0f },
        { 1.0f,-1.0f, 1.0f }, { -1.0f,-1.0f,-1.0f }, { 1.0f,-1.0f,-1.0f },
        { 1.0f, 1.0f,-1.0f }, { 1.0f,-1.0f,-1.0f }, { -1.0f,-1.0f,-1.0f },
        { -1.0f,-1.0f,-1.0f }, { -1.0f, 1.0f, 1.0f }, { -1.0f, 1.0f,-1.0f },
        { 1.0f,-1.0f, 1.0f }, { -1.0f,-1.0f, 1.0f }, { -1.0f,-1.0f,-1.0f },
        { -1.0f, 1.0f, 1.0f }, { -1.0f,-1.0f, 1.0f }, { 1.0f,-1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f }, { 1.0f,-1.0f,-1.0f }, { 1.0f, 1.0f,-1.0f },
        { 1.0f,-1.0f,-1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f,-1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f,-1.0f }, { -1.0f, 1.0f,-1.0f },
        { 1.0f, 1.0f, 1.0f }, { -1.0f, 1.0f,-1.0f }, { -1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f }, { -1.0f, 1.0f, 1.0f }, { 1.0f,-1.0f, 1.0f }
    };
    glw::VertexBuffer<glm::vec3> bounds_vbo;
    glw::VertexArrayObject bounds_vao(&bounds_vbo, { { GL_FLOAT, 3 } });

    bool should_quit = false;
    SDL_Event evt;
    while (!should_quit) {
        const float delta_time = context.UpdateDeltaTime();
        SDL_SetWindowTitle(context.GetWindow(), std::to_string(1000.0f / delta_time).c_str());

        while (SDL_PollEvent(&evt)) {
            switch (evt.type) {
                case SDL_QUIT:
                    should_quit = true;
                    break;
            }
        }

        UpdateCamera(&camera, delta_time);
        
        raytracer.Render(camera);
        context.Present();
    }
}

