#ifndef GLW_HPP
#define GLW_HPP

#include <vector>
#include <string>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

namespace glw {
    using i8 = int8_t;
    using u8 = uint8_t;
    using i16 = int16_t;
    using u16 = uint16_t;
    using i32 = int32_t;
    using u32 = uint32_t;
    using i64 = int64_t;
    using u64 = uint64_t;

    using EventHandlerCallback = void(*)(const SDL_Event& evt);

    class Context {
    public:
        Context(const char* window_name, u32 window_width, u32 window_height, bool fullscreen = false);
        ~Context();
        void Present();
        float UpdateDeltaTime();
        u32 GetWindowWidth();
        u32 GetWindowHeight();
        SDL_Window* GetWindow();
        SDL_GLContext GetContext();
    private:
        static void GLAPIENTRY MessageCallback(
            GLenum source, GLenum type, GLuint id, GLenum severity,
            GLsizei length, const GLchar* message, const void* userParam);
        u32 m_window_width, m_window_height;
        SDL_Window* m_window;
        SDL_GLContext m_context;
    };

    class GLObject {
    public:
        u32 GetID();
    protected:
        u32 m_ID;
    };

    template<GLenum BufferType, typename ElemType, GLenum Usage>
    struct Buffer : public GLObject {
    public:
        Buffer() {
            glGenBuffers(1, &m_ID);
        }
        Buffer(std::span<const ElemType> data) {
            glGenBuffers(1, &m_ID);
            Source(data);
        }
        Buffer(const void* data, u32 byte_size) {
            glGenBuffers(1, &m_ID);
            Source(data, byte_size);
        }
        ~Buffer() {
            glDeleteBuffers(1, &m_ID);
        }
        void Bind() const {
            glBindBuffer(BufferType, m_ID);
        }
        void Source(std::span<const ElemType> data) {
            Bind();
            m_length = data.size();
            glBufferData(BufferType, data.size_bytes(), data.data(), Usage);
        }
        void SubSource(u32 offset, std::span<const ElemType> data) { // TODO: test
            Bind();
            glBufferSubData(BufferType, offset, data.size_bytes(), data.data());
        }
        void Source(const void* data, u32 byte_size) {
            Bind();
            m_length = 1;
            glBufferData(BufferType, byte_size, data, Usage);
        }
        void SubSource(u32 offset, const void* data, u32 byte_size) { // TODO: test
            Bind();
            glBufferSubData(BufferType, offset, byte_size, data);
        }
        u32 GetLength() const { return m_length; }
    private:
        u32 m_length = 0;
    };

    template<typename TElem, GLenum Usage = GL_STATIC_DRAW>
    using VertexBuffer = Buffer<GL_ARRAY_BUFFER, TElem, Usage>;

    template<typename TElem, GLenum Usage = GL_STATIC_DRAW>
    using IndexBuffer = Buffer<GL_ELEMENT_ARRAY_BUFFER, TElem, Usage>;

    template<GLenum Usage = GL_STATIC_DRAW>
    class ShaderStorageBuffer : public Buffer<GL_SHADER_STORAGE_BUFFER, u8, Usage> {
    public:
        ShaderStorageBuffer(std::span<const u8> data, u32 bind_index = 0)
            : Buffer<GL_SHADER_STORAGE_BUFFER, u8, Usage>(data)
        {
            BindToIndex(bind_index);
        }
        ShaderStorageBuffer(const void* data, u32 byte_size, u32 bind_index = 0)
            : Buffer<GL_SHADER_STORAGE_BUFFER, u8, Usage>(data, byte_size)
        {
            BindToIndex(bind_index);
        }
        void BindToIndex(u32 idx) {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, idx, GLObject::m_ID);
        }
    private:
        u32 m_length;
    };

    template<
        typename VertexElemType, GLenum VertexUsage,
        typename IndexElemType, GLenum IndexUsage>
    class VertexArrayObject : public GLObject {
    public:
        struct Attribute {
            GLenum type;
            u32 num;
        };
        VertexArrayObject(
            const VertexBuffer<VertexElemType, VertexUsage> *vertex_buffer,
            const IndexBuffer<IndexElemType, IndexUsage> *index_buffer,
            const std::initializer_list<Attribute>& attributes)
            : m_vertex_buffer(vertex_buffer), m_index_buffer(index_buffer)
        {
            glGenVertexArrays(1, &m_ID);
            glBindVertexArray(m_ID);
            m_vertex_buffer->Bind();
            if (m_index_buffer != nullptr)
                m_index_buffer->Bind();
            InitializeAttributes(attributes);
            glBindVertexArray(0);
        }
        ~VertexArrayObject() {
            glDeleteVertexArrays(1, &m_ID);
        }
        void Draw(GLenum mode = GL_TRIANGLES, GLenum indices_elem_type = GL_UNSIGNED_INT) {
            glBindVertexArray(m_ID);
            if (m_index_buffer != nullptr)
                glDrawElements(mode, m_index_buffer->GetLength(), indices_elem_type, nullptr);
            else
                glDrawArrays(mode, 0, m_vertex_buffer->GetLength());
        }
    private:
        const VertexBuffer<VertexElemType, VertexUsage>* m_vertex_buffer;
        const IndexBuffer<IndexElemType, IndexUsage>* m_index_buffer;

        void InitializeAttributes(const std::initializer_list<Attribute>& attributes) {
            u32 idx = 0;
            u32 offset = 0;
            for (const Attribute& attrib : attributes) {
                glEnableVertexAttribArray(idx);
                glVertexAttribPointer(idx, attrib.num, attrib.type, GL_FALSE, sizeof(VertexElemType), reinterpret_cast<void*>(offset));
                u8 size = 0;
                switch(attrib.type) {
                    case GL_BYTE: case GL_UNSIGNED_BYTE: size = 1; break;
                    case GL_SHORT: case GL_UNSIGNED_SHORT: size = 2; break;
                    case GL_INT: case GL_UNSIGNED_INT: case GL_FLOAT: size = 4; break;
                }
                idx++;
                offset += attrib.num * size;
            }
        }
    };

    class Shader : public GLObject {
    public:
        Shader(const std::string& vert_source, const std::string& frag_source);
        ~Shader();
        void Bind();
        void Recompile(const std::string& vert_source, const std::string& frag_source);
        void SetInt(const std::string& name, i32 value);
        void SetIntVec(const std::string& name, const std::span<i32> value);
        void SetFloat(const std::string& name, float value);
        void SetFloatVec(const std::string& name, const std::span<float> value);
        void SetVec3(const std::string& name, const glm::vec3& value);
        void SetVec3Vec(const std::string& name, const std::span<glm::vec3> value);
        void SetVec4(const std::string& name, const glm::vec4& value);
        void SetVec4Vec(const std::string& name, const std::span<glm::vec4> value);
        void SetMat4(const std::string& name, const glm::mat4& value);
    private:
        void CheckErrors(u32 shader, GLenum type);
        u32 CreateShader(GLenum type, const std::string& filename);
        void Compile(const std::string& vert_source, const std::string& frag_source);
    };

    enum CameraMoveDir { CameraForward, CameraBackward, CameraLeft, CameraRight };
    class FPSCamera {
    public:
        static constexpr glm::vec3 DefaultPos = { 0, 0, 0 };
        static constexpr float DefaultSpeed = 0.01f;
        static constexpr float DefaultSensitivity = 1.0f;
        static constexpr float DefaultZNear = 0.1f;
        static constexpr float DefaultZFar = 100.0f;
        static constexpr glm::vec3 DefaultUpVec = { 0, 1, 0 };

        FPSCamera(float FOV, float w_h_ratio);
        glm::vec3 GetFrontVec();
        glm::mat4 GetViewMatrix();
        glm::mat4 GetProjection();
        void ProcessMouse(const Context& ctx);
        void Move(CameraMoveDir dir, float delta_time);
        glm::vec3 GetPos() const;
        void SetPos(const glm::vec3& pos);
        float GetSpeed() const;
        void SetSpeed(float speed);
        float GetYaw() const;
        void SetYaw(float yaw);
        float GetPitch() const;
        void SetPitch(float pitch);
         
        glm::vec3 GetFront() const { return m_front; }
    private:
        glm::vec3 m_pos = DefaultPos;
        float m_speed = DefaultSpeed;
        float m_sensitivity = DefaultSensitivity;
        const glm::vec3 m_up = DefaultUpVec;
        const float m_z_near = DefaultZNear;
        const float m_z_far = DefaultZFar;
        glm::vec3 m_front = glm::vec3(0, 0, 1);
        glm::mat4 m_projection;
        i32 m_last_mouse_x = 0, m_last_mouse_y = 0;
        float m_yaw = 0, m_pitch = 0;
    };
};

// Debugging
#define GLW_IMPLEMENTATION

#ifdef GLW_IMPLEMENTATION

#ifndef GLW_GL_MAJOR_VERSION
#define GLW_GL_MAJOR_VERSION 4
#endif
#ifndef GLW_GL_MINOR_VERSION
#define GLW_GL_MINOR_VERSION 3
#endif

namespace glw {
    Context::Context(const char* window_name, u32 window_width, u32 window_height, bool fullscreen)
        : m_window_width(window_width), m_window_height(window_height)
    {
        // SDL
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            SDL_LogCritical(
                SDL_LOG_CATEGORY_APPLICATION,
                "Failed to initialize SDL: %s", SDL_GetError()
            );
            exit(1);
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, GLW_GL_MAJOR_VERSION);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, GLW_GL_MINOR_VERSION);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

        m_window = SDL_CreateWindow(
            window_name,
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
            m_window_width, m_window_height,
            SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | (fullscreen * SDL_WINDOW_FULLSCREEN)
        );
        if (m_window == nullptr) {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "Failed to create window: %s\n", SDL_GetError()
            );
            SDL_Quit();
        }

        m_context = SDL_GL_CreateContext(m_window);
        if (m_context == nullptr) {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "Failed to create context: %s\n", SDL_GetError()
            );
            SDL_DestroyWindow(m_window);
            SDL_Quit();
        }

        // VSync
        SDL_GL_SetSwapInterval(-1);

        // GLEW
        GLenum glew_error = glewInit();
        if (glew_error != GLEW_OK) {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "Failed to initialize glew: %s\n", glewGetErrorString(glew_error)
            );
            SDL_GL_DeleteContext(m_context);
            SDL_DestroyWindow(m_window);
            SDL_Quit();
        }

        // Message callback
#ifdef DEBUG
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(MessageCallback, 0);
#endif
    }
    Context::~Context() {
        SDL_GL_DeleteContext(m_context);
        SDL_DestroyWindow(m_window);
    }
    void Context::Present() {
        SDL_GL_SwapWindow(m_window);
    }
    float Context::UpdateDeltaTime() {
        static u64 now = SDL_GetPerformanceCounter();
        static u64 then;
        then = now;
        now = SDL_GetPerformanceCounter();
        
        return (float)((now - then) * 1000 / (float)SDL_GetPerformanceFrequency());
    }
    u32 Context::GetWindowWidth() { return m_window_width; }
    u32 Context::GetWindowHeight() { return m_window_height; }
    SDL_Window* Context::GetWindow() { return m_window; }
    SDL_GLContext Context::GetContext() { return m_context; }
    void GLAPIENTRY Context::MessageCallback(
        GLenum source, GLenum type, GLuint id, GLenum severity,
        GLsizei length, const GLchar* message, const void* userParam)
    {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type, severity, message
        );
    }

    u32 GLObject::GetID() { return m_ID; }

    Shader::Shader(const std::string& vert_source, const std::string& frag_source) {
        Compile(vert_source, frag_source);
    }
    Shader::~Shader() {
        glDeleteProgram(m_ID);
    }
    void Shader::Bind() {
        glUseProgram(m_ID);
    }
    void Shader::Recompile(const std::string& vert_source, const std::string& frag_source) {
        glDeleteProgram(m_ID);
        Compile(vert_source, frag_source);
    }
    void Shader::SetInt(const std::string& name, i32 value) {
        glUniform1i(glGetUniformLocation(m_ID, name.c_str()), value);
    }
    void Shader::SetIntVec(const std::string& name, const std::span<i32> value) {
        glUniform1iv(glGetUniformLocation(m_ID, name.c_str()), value.size(), value.data());
    }
    void Shader::SetFloat(const std::string& name, float value) {
        glUniform1f(glGetUniformLocation(m_ID, name.c_str()), value);
    }
    void Shader::SetFloatVec(const std::string& name, const std::span<float> value) {
        glUniform1fv(glGetUniformLocation(m_ID, name.c_str()), value.size(), value.data());
    }
    void Shader::SetVec3(const std::string& name, const glm::vec3& value) {
        glUniform3fv(glGetUniformLocation(m_ID, name.c_str()), 1, &value[0]);
    }
    void Shader::SetVec3Vec(const std::string& name, const std::span<glm::vec3> value) {
        glUniform3fv(
            glGetUniformLocation(m_ID, name.c_str()),
            value.size(), reinterpret_cast<float*>(value.data())
        );
    }
    void Shader::SetVec4(const std::string& name, const glm::vec4& value) {
        glUniform3fv(glGetUniformLocation(m_ID, name.c_str()), 1, &value[0]);
    }
    void Shader::SetVec4Vec(const std::string& name, const std::span<glm::vec4> value) {
        glUniform4fv(
            glGetUniformLocation(m_ID, name.c_str()),
            value.size(), reinterpret_cast<float*>(value.data())
        );
    }
    void Shader::SetMat4(const std::string& name, const glm::mat4& value) {
        glUniformMatrix4fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, &value[0][0]);
    }
    void Shader::CheckErrors(u32 shader, GLenum type) {
        i32 success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE) {
            i32 error_len;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &error_len);
            char* error_msg = static_cast<char*>(malloc(error_len * sizeof(char)));
            glGetShaderInfoLog(shader, error_len, nullptr, error_msg);
            std::string type_str;
            switch(type) {
                case GL_VERTEX_SHADER: type_str = "GL_VERTEX_SHADER"; break;
                case GL_FRAGMENT_SHADER: type_str = "GL_FRAGMENT_SHADER"; break;
            }
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "Error in %s shader %d:\n%s\n", type_str.c_str(), shader, error_msg
            );
        }
    }
    u32 Shader::CreateShader(GLenum type, const std::string& source) {
        u32 shader = glCreateShader(type);
        const char* source_c_str = source.c_str();
        glShaderSource(shader, 1, &source_c_str, nullptr);
        glCompileShader(shader);
        CheckErrors(shader, type);
        return shader;
    }
    void Shader::Compile(const std::string& vert_source, const std::string& frag_source) {
        u32 vertex_shader = CreateShader(GL_VERTEX_SHADER, vert_source);
        u32 fragment_shader = CreateShader(GL_FRAGMENT_SHADER, frag_source);
        m_ID = glCreateProgram();
        glAttachShader(m_ID, vertex_shader);
        glAttachShader(m_ID, fragment_shader);
        glLinkProgram(m_ID);
        glDetachShader(m_ID, vertex_shader);
        glDeleteShader(vertex_shader);
        glDetachShader(m_ID, fragment_shader);
        glDeleteShader(fragment_shader);
    }

    FPSCamera::FPSCamera(float FOV, float w_h_ratio) {
        m_projection = glm::perspective(glm::radians(FOV), w_h_ratio, m_z_near, m_z_far);
    }
    glm::mat4 FPSCamera::GetViewMatrix() {
        return glm::lookAt(m_pos, m_pos + m_front, m_up);
    }
    glm::mat4 FPSCamera::GetProjection() {
        return m_projection;
    }
    void FPSCamera::ProcessMouse(const Context& ctx) {
        i32 mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);
        i32 delta_x = mouse_x - m_last_mouse_x;
        i32 delta_y = mouse_y - m_last_mouse_y;
        m_yaw += delta_x * m_sensitivity;
        m_pitch -= delta_y * m_sensitivity;
        m_last_mouse_x = mouse_x; m_last_mouse_y = mouse_y;

        m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);

        m_front.x = glm::cos(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch));
        m_front.y = glm::sin(glm::radians(m_pitch));
        m_front.z = glm::sin(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch));
        m_front = glm::normalize(m_front);
    }
    void FPSCamera::Move(CameraMoveDir dir, float delta_time) {
        switch(dir) {
            case CameraForward: m_pos += m_speed * m_front * delta_time; break;
            case CameraBackward: m_pos -= m_speed * m_front * delta_time; break;
            case CameraLeft: m_pos -= m_speed * glm::normalize(glm::cross(m_front, m_up)) * delta_time; break;
            case CameraRight: m_pos += m_speed * glm::normalize(glm::cross(m_front, m_up)) * delta_time; break;
        }
    }
    glm::vec3 FPSCamera::GetPos() const {
        return m_pos;
    }
    void FPSCamera::SetPos(const glm::vec3& pos) {
        m_pos = pos;
    }
    float FPSCamera::GetSpeed() const {
        return m_speed;
    }
    void FPSCamera::SetSpeed(float speed) {
        m_speed = speed;
    }
    float FPSCamera::GetYaw() const {
        return m_yaw;
    }
    void FPSCamera::SetYaw(float yaw) {
        m_yaw = yaw;
    }
    float FPSCamera::GetPitch() const {
        return m_pitch;
    }
    void FPSCamera::SetPitch(float pitch) {
        m_pitch = pitch;
    }
}

#endif

#endif // GLW_HPP
