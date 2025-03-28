#ifndef COMMON_HPP
#define COMMON_HPP

#if __cplusplus < 202002L
#error "This file requires C++20 or newer."
#endif

#include <iostream>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <cstdint>
#include <format>
#include <cassert>
#include <cstring>

using u8 = uint8_t;
using i8 = int8_t;
using u16 = uint16_t;
using i16 = int16_t;
using u32 = uint32_t;
using i32 = int32_t;
using u64 = uint64_t;
using i64 = int64_t;

using string = std::string;
template<typename T, size_t size> using array = std::array<T, size>;
template<typename T> using vector = std::vector<T>;
template<typename T> using span = std::span<T>;

#define LOG(fmt, ...) \
    (std::cout << std::format(fmt, ##__VA_ARGS__) << std::endl)

#define LOG_FAILED_ASSERT(expr, fmt, ...) \
    (LOG("Assert failed at {}:{} -> {} -> {}", __FILE__, __LINE__, #expr, std::format(fmt, ##__VA_ARGS__)))

#define ASSERT(expr, fmt, ...) \
    (static_cast<bool>(expr) \
     ? void(0) \
     : (LOG_FAILED_ASSERT(expr, fmt, ##__VA_ARGS__), exit(1)))

class ByteBuffer {
public:
    template<typename T>
    void Add(const T* object) {
        m_data.reserve(m_data.size() + sizeof(T));
        const u8* insert_pos = m_data.data() + m_data.size();
        m_data.resize(m_data.size() + sizeof(T));
        memcpy((void*)insert_pos, object, sizeof(T));
    }
    template<typename T> // NOTE: This cannot be deducted for some reason (TODO)
    void Extend(const span<const T>& vec) {
        m_data.reserve(m_data.size() + vec.size_bytes());
        const u8* insert_pos = m_data.data() + m_data.size();
        m_data.resize(m_data.size() + vec.size_bytes());
        memcpy((void*)insert_pos, vec.data(), vec.size_bytes());
    }
    const vector<u8>& AsVec() { return m_data; }
private:
    vector<u8> m_data;
};

class File {
public:
    File() {}
    File(const string& filename, const string& options = "rb+") {
        Open(filename, options);
    }
    ~File() {
        Close();
    }
    bool Open(const string& filename, const string& options = "rb+") {
        m_handle = fopen(filename.c_str(), options.c_str());
        if (m_handle == nullptr) {
            return false;
        }
        return true;
    }
    void Close() {
        fclose(m_handle);
    }
    bool IsValid() {
        return m_handle != nullptr;
    }
    void MoveAt(i32 offset, i32 origin = SEEK_SET) {
        fseek(m_handle, offset, origin);
    }
    u32 Tell() {
        return ftell(m_handle);
    }
    void Read(void* data, u32 byte_size) {
        fread(data, 1, byte_size, m_handle);
    }
    void Write(const void* data, u32 byte_size) {
        fwrite(data, 1, byte_size, m_handle);
    }
    void ReadAt(u32 pos, void* data, u32 byte_size) {
        MoveAt(pos);
        Read(data, byte_size);
    }
    void WriteAt(u32 pos, const void* data, u32 byte_size) {
        MoveAt(pos);
        Write(data, byte_size);
    }
    void ReadAll(string* str) {
        MoveAt(0, SEEK_END);
        u32 size = Tell();
        str->resize(size);
        MoveAt(0);
        Read(str->data(), size);
    }
    FILE* GetHandle() { return m_handle; }
private:
    FILE* m_handle;
};

#endif
