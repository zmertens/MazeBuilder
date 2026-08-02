#include "sdl_gl_helper.h"

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <SDL3/SDL.h>

#include "geometries.h"
#include "shader.h"
#include "world.h"

bool sdl_gl_helper::initialize(std::string_view title, int width, int height) noexcept
{
    auto init_func = [this, title, width, height]()
    {
        if (!SDL_SetAppMetadata("Maze builder with voxels", title.data(), "voxels;maze;c++;sdl"))
        {
            return false;
        }

        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, title.data());
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "maze builder");
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING, "MIT License");
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "simulation;game;voxel");
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, title.data());

#if defined(MAZE_DEBUG)
        SDL_Log("OpenGL Setting :: SDL_GL_CONTEXT_DEBUG_FLAG\n");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
#endif

#if defined(__EMSCRIPTEN__)
        SDL_Log("OpenGL Setting :: SDL_GL_CONTEXT_PROFILE_ES\n");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
        SDL_Log("OpenGL Setting :: SDL_GL_CONTEXT_PROFILE_CORE\n");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

        constexpr auto window_flags = SDL_WINDOW_OPENGL
            | SDL_WINDOW_HIGH_PIXEL_DENSITY
            | SDL_WINDOW_RESIZABLE
            | SDL_WINDOW_INPUT_FOCUS;

        this->window = SDL_CreateWindow(title.data(), width, height, window_flags);

        if (!this->window)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateWindow failed (%s)\n", SDL_GetError());
            return false;
        }

        this->gl_context = SDL_GL_CreateContext(this->window);

        if (!this->gl_context)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_GL_CreateContext failed (%s)\n", SDL_GetError());
            return false;
        }

        SDL_GL_MakeCurrent(this->window, this->gl_context);

        SDL_GL_SetSwapInterval(1);

        SDL_ShowWindow(window);
        SDL_SetWindowRelativeMouseMode(window, false);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

#if !defined(__EMSCRIPTEN__)
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "OpenGL loader failed (%s)\n", SDL_GetError());
            SDL_Quit();
            return false;
        }
#endif

        print_opengl_info();

        return true;
    };

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        std::call_once(m_initialized_flag, init_func);
        return true;
    }
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_Init failed: %s\n", SDL_GetError());
    return false;
}

void sdl_gl_helper::destroy_and_quit() noexcept
{
    // Prevent double-destruction
    if (!this->window && !this->gl_context)
    {
#if defined(MAZE_DEBUG)
        SDL_Log("sdl_helper::destroy_and_quit() - Already destroyed, skipping\n");
#endif

        return;
    }

    if (this->gl_context)
    {
#if defined(MAZE_DEBUG)
        SDL_Log("sdl_helper::destroy_and_quit() - Destroying gl_context %p\n", static_cast<void*>(gl_context));
#endif

        // SDL_Des (this->gl_context);
        this->gl_context = nullptr;
    }

    if (window)
    {
#if defined(MAZE_DEBUG)
        SDL_Log("sdl_helper::destroy_and_quit() - Destroying window %p\n", static_cast<void*>(window));
#endif

        SDL_DestroyWindow(window);
        window = nullptr;
    }

    // Only call SDL_Quit() if we actually destroyed something
    if (SDL_WasInit(0) != 0)
    {
#if defined(MAZE_DEBUG)
        SDL_Log("sdl_helper::destroy_and_quit() - Calling SDL_Quit()\n");
#endif

        SDL_Quit();
    }
}

std::int32_t sdl_gl_helper::get_scale_factor() const noexcept
{
    int window_width, window_height;
    int buffer_width, buffer_height;
    SDL_GetWindowSize(this->window, &window_width, &window_height);
    SDL_GetWindowSizeInPixels(this->window, &buffer_width, &buffer_height);
    return buffer_width / window_width;
}

std::pair<std::int32_t, std::int32_t> sdl_gl_helper::get_window_size() const noexcept
{
    auto w = 0, h = 0;
    SDL_GetWindowSize(this->window, &w, &h);
    return {w, h};
}

std::pair<std::int32_t, std::int32_t> sdl_gl_helper::get_window_size_in_pixels() const noexcept
{
    auto w = 0, h = 0;
    SDL_GetWindowSizeInPixels(this->window, &w, &h);
    return {w, h};
}

void sdl_gl_helper::print_display_modes() noexcept
{
    const SDL_DisplayID display = SDL_GetPrimaryDisplay();
    int num_modes = 0;
    if (const SDL_DisplayMode* const * modes = SDL_GetFullscreenDisplayModes(display, &num_modes))
    {
        for (int i = 0; i < num_modes; ++i)
        {
            const SDL_DisplayMode* mode = modes[i];
            SDL_Log("Display %" SDL_PRIu32 " mode %d: %dx%d@%gx %gHz\n",
                    display, i, mode->w, mode->h, mode->pixel_density, mode->refresh_rate);
        }
    }
}

void sdl_gl_helper::print_opengl_info() noexcept
{
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);

    SDL_Log("-------------------------------------------------------------\n");
    SDL_Log("GL Vendor    : %s\n", vendor);
    SDL_Log("GL Renderer  : %s\n", renderer);
    SDL_Log("GL Version   : %s\n", version);
    SDL_Log("GL Version   : %d.%d\n", major, minor);
    SDL_Log("GLSL Version : %s\n", glslVersion);
    SDL_Log("-------------------------------------------------------------\n");
}

std::string sdl_gl_helper::load_file_to_string(std::string_view path) noexcept
{
    // Open binary file
    SDL_IOStream* io = SDL_IOFromFile(path.data(), "r");
    if (io == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_IOFromFile failed: %s", SDL_GetError());
        return "";
    }
    const auto data_size = SDL_GetIOSize(io);
    // Allocate memory for the file content + null terminator
    const auto data = static_cast<char*>(SDL_malloc(data_size + 1));

    if (data == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_malloc failed: %s", SDL_GetError());
        SDL_CloseIO(io);
        return "";
    }

    // Read file into memory
    // SDL_ReadIO returns the number of bytes read, or 0 on error or end of file
    int nb_read_total = 0, nb_read_size = 1;
    auto buf = data;
    while (nb_read_total < data_size && nb_read_size != 0)
    {
        nb_read_size = SDL_ReadIO(io, buf, (data_size - nb_read_total));
        nb_read_total += nb_read_size;
        buf += nb_read_size;
    }

    SDL_CloseIO(io);
    if (nb_read_total != data_size)
    {
        SDL_free(data);
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to read complete file: %s", SDL_GetError());
        return "";
    }
    data[nb_read_total] = '\0';
    std::string string_data(data, nb_read_total);
    SDL_free(data);
    return string_data;
}

std::vector<std::uint8_t> sdl_gl_helper::load_file_binary(std::string_view path) noexcept
{
    SDL_IOStream* io = SDL_IOFromFile(path.data(), "rb");
    if (!io)
    {
        return {};
    }

    auto size = SDL_GetIOSize(io);
    std::vector<std::uint8_t> buffer(size);
    SDL_ReadIO(io, buffer.data(), size);
    SDL_CloseIO(io);
    return buffer;
}

void sdl_gl_helper::set_window_icon(std::string_view icon_path) const noexcept
{
    SDL_Surface* icon_surface = SDL_LoadBMP_IO(SDL_IOFromFile(icon_path.data(), "rb"), true);
    if (icon_surface)
    {
        SDL_SetWindowIcon(this->window, icon_surface);
        SDL_DestroySurface(icon_surface);
    }
    else
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "(SDL) Couldn't load icon at %s\n", icon_path.data());
    }
}

void sdl_gl_helper::del_buffer(const std::uint32_t buffer) noexcept
{
    glDeleteBuffers(1, &buffer);
}

std::uint32_t sdl_gl_helper::gen_buffer(const std::size_t size, const float* data) noexcept
{
    if (size == 0)
    {
        return 0;
    }

    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return buffer;
}

float* sdl_gl_helper::malloc_faces(const std::size_t components, const std::size_t faces) noexcept
{
    return static_cast<GLfloat*>(SDL_malloc(sizeof(GLfloat) * 6 * components * faces));
}

std::uint32_t sdl_gl_helper::gen_faces(const std::size_t components, const std::size_t faces,
                                       const float* data) noexcept
{
    const GLuint buffer = gen_buffer(sizeof(GLfloat) * 6 * components * faces, data);
    return buffer;
}

std::uint32_t sdl_gl_helper::gen_crosshair_buffer() const noexcept
{
    auto [width, height] = get_window_size();
    const float x = static_cast<float>(width) / 2.0f;
    const float y = static_cast<float>(height) / 2.0f;
    const float p = 10.f * static_cast<float>(get_scale_factor());
    const float data[] = {
        x, y - p, x, y + p,
        x - p, y, x + p, y
    };
    return gen_buffer(sizeof(data), data);
}

std::uint32_t sdl_gl_helper::gen_wireframe_buffer(const float x, const float y, const float z, const float n) noexcept
{
    float data[72];
    geometries::make_cube_wireframe(data, x, y, z, n);
    return gen_buffer(sizeof(data), data);
}

std::uint32_t sdl_gl_helper::gen_line_buffer(const float x1, const float y1, const float z1,
                                             const float x2, const float y2, const float z2) noexcept
{
    // Create a simple line buffer with two 3D points
    float data[6] = {
        x1, y1, z1,  // Start point
        x2, y2, z2   // End point
    };
    return gen_buffer(sizeof(data), data);
}

std::uint32_t sdl_gl_helper::gen_cube_buffer(const float x, const float y, const float z, const float n,
                                             const int w) noexcept
{
    GLfloat* data = malloc_faces(10, 6);
    float ao[6][4] = {0};
    float light[6][4] = {
        {0.5, 0.5, 0.5, 0.5},
        {0.5, 0.5, 0.5, 0.5},
        {0.5, 0.5, 0.5, 0.5},
        {0.5, 0.5, 0.5, 0.5},
        {0.5, 0.5, 0.5, 0.5},
        {0.5, 0.5, 0.5, 0.5}
    };
    geometries::make_cube(data, ao, light, 1, 1, 1, 1, 1, 1, x, y, z, n, w);
    return gen_faces(10, 6, data);
}

std::uint32_t sdl_gl_helper::gen_plant_buffer(const float x, const float y, const float z, const float n,
                                              const int w) noexcept
{
    GLfloat* data = malloc_faces(10, 4);
    float ao = 0;
    float light = 1;
    geometries::make_plant(data, ao, light, x, y, z, n, w, 45);
    return gen_faces(10, 4, data);
}

std::uint32_t sdl_gl_helper::gen_player_buffer(const float x, const float y, const float z, const float rx,
                                               const float ry) noexcept
{
    GLfloat* data = malloc_faces(10, 6);
    geometries::make_player(data, x, y, z, rx, ry);
    return gen_faces(10, 6, data);
}

std::uint32_t sdl_gl_helper::gen_text_buffer(float x, const float y, const float n,
                                             const std::string_view text) noexcept
{
    const auto length = static_cast<GLsizei>(text.size());
    if (length <= 0)
    {
        return 0;
    }

    GLfloat* data = malloc_faces(4, length);
    if (!data)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to allocate text vertex buffer data\n");
        return 0;
    }

    for (int i = 0; i < length; i++)
    {
        geometries::make_character(data + i * 24, x, y, n / 2, n, text[i]);
        x += n;
    }
    return gen_faces(4, length, data);
}

int sdl_gl_helper::_gen_sign_buffer(float* data, const float x, const float y, const float z,
                                    const int face, const std::string_view text) noexcept
{
    auto tokenize = [](char* str, const char* delim, char** key)-> char*
    {
        if (str == nullptr)
        {
            str = *key;
        }
        str += strspn(str, delim);
        if (*str == '\0')
        {
            return nullptr;
        }
        char* result = str;
        str += strcspn(str, delim);
        if (*str)
        {
            *str++ = '\0';
        }
        *key = str;
        return result;
    };

    auto char_width = [](const char input)
    {
        static const int lookup[128] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            4, 2, 4, 7, 6, 9, 7, 2, 3, 3, 4, 6, 3, 5, 2, 7,
            6, 3, 6, 6, 6, 6, 6, 6, 6, 6, 2, 3, 5, 6, 5, 7,
            8, 6, 6, 6, 6, 6, 6, 6, 6, 4, 6, 6, 5, 8, 8, 6,
            6, 7, 6, 6, 6, 6, 8, 10, 8, 6, 6, 3, 6, 3, 6, 6,
            4, 7, 6, 6, 6, 6, 5, 6, 6, 2, 5, 5, 2, 9, 6, 6,
            6, 6, 6, 6, 5, 6, 6, 6, 6, 6, 6, 4, 2, 5, 7, 0
        };
        return lookup[input];
    };

    auto string_width = [=](const char* input)
    {
        int result = 0;
        const std::size_t length = SDL_strlen(input);
        for (int i = 0; i < length; i++)
        {
            result += char_width(input[i]);
        }
        return result;
    };

    auto wrap = [=](const char* input, const int max_width, char* output, const int max_length)
    {
        *output = '\0';
        auto* str = static_cast<char*>(SDL_malloc(sizeof(char) * (strlen(input) + 1)));
        SDL_strlcpy(str, input, SDL_strlen(input) + 1);
        const int space_width = char_width(' ');
        int line_number = 0;
        char *key1, *key2;
        char* line = tokenize(str, "\r\n", &key1);
        while (line)
        {
            int line_width = 0;
            const char* token = tokenize(line, " ", &key2);
            while (token)
            {
                int token_width = string_width(token);
                if (line_width)
                {
                    if (line_width + token_width > max_width)
                    {
                        line_width = 0;
                        line_number++;
                        SDL_strlcat(output, "\n", max_length - strlen(output) - 1);
                    }
                    else
                    {
                        SDL_strlcat(output, " ", max_length - strlen(output) - 1);
                    }
                }
                SDL_strlcat(output, token, max_length - strlen(output) - 1);
                line_width += token_width + space_width;
                token = tokenize(nullptr, " ", &key2);
            }
            line_number++;
            SDL_strlcat(output, "\n", max_length - strlen(output) - 1);
            line = tokenize(nullptr, "\r\n", &key1);
        }
        SDL_free(str);
        return line_number;
    };

    static constexpr int glyph_dx[8] = {0, 0, -1, 1, 1, 0, -1, 0};
    static constexpr int glyph_dz[8] = {1, -1, 0, 0, 0, -1, 0, 1};
    static constexpr int line_dx[8] = {0, 0, 0, 0, 0, 1, 0, -1};
    static constexpr int line_dy[8] = {-1, -1, -1, -1, 0, 0, 0, 0};
    static constexpr int line_dz[8] = {0, 0, 0, 0, 1, 0, -1, 0};
    if (face < 0 || face >= 8)
    {
        return 0;
    }
    int count = 0;
    constexpr float max_width = 64.f;
    constexpr float line_height = 1.25f;
    char lines[1024];
    int rows = wrap(text.data(), static_cast<int>(max_width), lines, 1024);
    rows = SDL_min(rows, 5);
    const int dx = glyph_dx[face];
    const int dz = glyph_dz[face];
    const int ldx = line_dx[face];
    const int ldy = line_dy[face];
    const int ldz = line_dz[face];
    constexpr float n = 1.0f / (max_width / 10.f);
    float sx = x - n * static_cast<float>(rows - 1) * (line_height / 2.f) * ldx;
    float sy = y - n * static_cast<float>(rows - 1) * (line_height / 2.f) * ldy;
    float sz = z - n * static_cast<float>(rows - 1) * (line_height / 2.f) * ldz;
    char* key;
    const char* line = tokenize(lines, "\n", &key);
    while (line)
    {
        const size_t length = SDL_strlen(line);
        int line_width = string_width(line);
        line_width = static_cast<int>(SDL_min(line_width, max_width));
        float rx = sx - dx * line_width / max_width / 2;
        const float ry = sy;
        float rz = sz - dz * line_width / max_width / 2;
        for (int i = 0; i < length; i++)
        {
            const int width = char_width(line[i]);
            line_width -= width;
            if (line_width < 0)
            {
                break;
            }
            rx += dx * width / max_width / 2;
            rz += dz * width / max_width / 2;
            if (line[i] != ' ')
            {
                geometries::make_character_3d(
                    data + count * 30, rx, ry, rz, n / 2, face, line[i]);
                count++;
            }
            rx += dx * width / max_width / 2;
            rz += dz * width / max_width / 2;
        }
        sx += n * line_height * ldx;
        sy += n * line_height * ldy;
        sz += n * line_height * ldz;
        line = tokenize(nullptr, "\n", &key);
        rows--;
        if (rows <= 0)
        {
            break;
        }
    }
    return count;
}

void sdl_gl_helper::gen_sign_buffer(scene_node* chunk) noexcept
{
    const sign_list* signs = &chunk->signs;

    // first pass - count characters
    std::size_t max_faces = 0;
    for (int i = 0; i < signs->size; i++)
    {
        const sign* e = signs->data + i;
        max_faces += SDL_strlen(e->text);
    }

    // second pass - generate geometry
    GLfloat* data = malloc_faces(5, max_faces);
    std::size_t faces = 0;
    for (int i = 0; i < signs->size; i++)
    {
        const sign* e = signs->data + i;
        faces += static_cast<std::size_t>(_gen_sign_buffer(data + static_cast<int>(faces) * 30,
                                                           static_cast<float>(e->x),
                                                           static_cast<float>(e->y),
                                                           static_cast<float>(e->z), e->face, e->text));
    }

    del_buffer(chunk->sign_buffer);
    chunk->sign_buffer = gen_faces(5, static_cast<GLsizei>(faces), data);
    chunk->sign_faces = static_cast<int>(faces);
}

std::uint32_t sdl_gl_helper::gen_sky_buffer() noexcept
{
    float data[12288];
    geometries::make_sphere(data, 1, 3);
    return gen_buffer(sizeof(data), data);
}

void sdl_gl_helper::draw_triangles_3d_ao(const attrib* a, const std::uint32_t buffer, const int count) noexcept
{
    if (!a || a->position < 0 || a->normal < 0 || a->uv < 0)
    {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(a->position);
    glEnableVertexAttribArray(a->normal);
    glEnableVertexAttribArray(a->uv);
    glVertexAttribPointer(a->position, 3, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 10, nullptr);
    glVertexAttribPointer(a->normal, 3, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 10, reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 3));
    glVertexAttribPointer(a->uv, 4, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 10, reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 6));
    glDrawArrays(GL_TRIANGLES, 0, count);
    glDisableVertexAttribArray(a->position);
    glDisableVertexAttribArray(a->normal);
    glDisableVertexAttribArray(a->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void sdl_gl_helper::draw_triangles_3d_text(const attrib* a, const std::uint32_t buffer, const int count) noexcept
{
    if (!a || a->position < 0 || a->uv < 0)
    {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(a->position);
    glEnableVertexAttribArray(a->uv);
    glVertexAttribPointer(a->position, 3, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 5, nullptr);
    glVertexAttribPointer(a->uv, 2, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 5, reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 3));
    glDrawArrays(GL_TRIANGLES, 0, count);
    glDisableVertexAttribArray(a->position);
    glDisableVertexAttribArray(a->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void sdl_gl_helper::draw_triangles_3d(const attrib* a, const std::uint32_t buffer, const int count) noexcept
{
    if (!a || a->position < 0 || a->normal < 0 || a->uv < 0)
    {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    glEnableVertexAttribArray(a->position);

    glEnableVertexAttribArray(a->normal);
    glEnableVertexAttribArray(a->uv);

    glVertexAttribPointer(a->position, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, nullptr);
    glVertexAttribPointer(a->normal, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8,
                          reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 3));
    glVertexAttribPointer(a->uv, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8,
                          reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 6));

    glDrawArrays(GL_TRIANGLES, 0, count);

    glDisableVertexAttribArray(a->position);
    glDisableVertexAttribArray(a->normal);
    glDisableVertexAttribArray(a->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void sdl_gl_helper::draw_triangles_2d(const attrib* a, const std::uint32_t buffer,
                                      const std::size_t count) noexcept
{
    if (!a || a->position < 0 || a->uv < 0)
    {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(a->position);
    glEnableVertexAttribArray(a->uv);
    glVertexAttribPointer(a->position, 2, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 4, 0);
    glVertexAttribPointer(a->uv, 2, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 4, reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 2));
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(count));
    glDisableVertexAttribArray(a->position);
    glDisableVertexAttribArray(a->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void sdl_gl_helper::draw_lines(const attrib* a, const std::uint32_t buffer, const int components,
                               const int count) noexcept
{
    if (!a || a->position < 0)
    {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(a->position);
    glVertexAttribPointer(
        a->position, components, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_LINES, 0, count);
    glDisableVertexAttribArray(a->position);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void sdl_gl_helper::draw_chunk(const attrib* a, const scene_node* chunk) noexcept
{
    draw_triangles_3d_ao(a, chunk->buffer, chunk->faces * 6);
}

void sdl_gl_helper::draw_item(const attrib* a, const GLuint buffer, const int count) noexcept
{
    draw_triangles_3d_ao(a, buffer, count);
}

void sdl_gl_helper::draw_text(const attrib* a, const std::uint32_t buffer, const std::size_t length) noexcept
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw_triangles_2d(a, buffer, length * 6);
    glDisable(GL_BLEND);
}

void sdl_gl_helper::draw_signs(const attrib* a, const scene_node* chunk) noexcept
{
    glDisable(GL_CULL_FACE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-8, -1024);
    draw_triangles_3d_text(a, chunk->sign_buffer, chunk->sign_faces * 6);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_CULL_FACE);
}

void sdl_gl_helper::draw_sign(const attrib* a, const std::uint32_t buffer, const int length) noexcept
{
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-8, -1024);
    draw_triangles_3d_text(a, buffer, length * 6);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void sdl_gl_helper::draw_cube(const attrib* a, const std::uint32_t buffer) noexcept
{
    draw_item(a, buffer, 36);
}

void sdl_gl_helper::draw_plant(const attrib* a, const std::uint32_t buffer) noexcept
{
    draw_item(a, buffer, 24);
}

void sdl_gl_helper::draw_player(const attrib* a, const std::uint32_t buffer) noexcept
{
    draw_item(a, buffer, 36);
}
