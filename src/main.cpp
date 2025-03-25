#include "glw.hpp"
#include <vector>
#include <fstream>
#include <cassert>

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

int main() {
    glw::Context context("Voxel raytracer", WND_WIDTH, WND_HEIGHT);

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

    std::vector<glm::vec3> spheres = {
        { -1, 0, -2 },
        {  1, 0, -2 },
        {  0, 2, -2 }
    };
    shader.SetVec3Vec("uSpheres", spheres);

    glw::FPSCamera camera(80.0f, (float)WND_WIDTH / (float)WND_HEIGHT);

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
}

