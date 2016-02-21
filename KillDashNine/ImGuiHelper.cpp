#include "ImGuiHelper.hpp"

#include <sstream>
#include <cstddef> // offsetof

#include "engine/ResourceManager.hpp"

GLint ImGuiHelper::sShaderHandle = 0;
GLint ImGuiHelper::sVertHandle = 0;
GLint ImGuiHelper::sFragHandle = 0;
GLint ImGuiHelper::sAttribLocationTex = 0;
GLint ImGuiHelper::sAttribLocationProjMat = 0;
GLint ImGuiHelper::sAttribLocationPosition = 0;
GLint ImGuiHelper::sAttribLocationUv = 0;
GLint ImGuiHelper::sAttribLocationColor = 0;
GLuint ImGuiHelper::sVboHandle = 0;
GLuint ImGuiHelper::sVaoHandle = 0;
GLuint ImGuiHelper::sElementsHandle = 0;

/**
 * @brief ImGuiHelper::ImGuiHelper
 * @param sdl
 * @param rm
 */
ImGuiHelper::ImGuiHelper(const SdlManager& sdl, ResourceManager& rm)
: cSdlManager(sdl)
, mResourceManager(rm)
, mShowComboBoxWindow(true)
, mShow_Z_PositionWindow(true)
, mShow_Overlay(false)
, mBackgroundAlpha(0.65f)
, mComboWidgetHeight(8.25f)
, mSliderWidgetHeight(5.0f)
, mSliderGrabMinSize(30.0f)
, mZ_PositionSliderValue(7.5f)
, mImGuiWindowFlags(ImGuiWindowFlags_NoTitleBar
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoScrollbar
    | ImGuiWindowFlags_NoCollapse)
, mOverlayFlags(ImGuiWindowFlags_NoTitleBar
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoSavedSettings)
, g_FontTexture(0)
, g_Time(0.0f)
, g_MousePressed{false, false, false}
, g_MouseWheel(0.0f)
{
    ImGui_ImplSdlGL3_Init();

    ImGuiStyle& igStyle = ImGui::GetStyle();

    mDefaultStyle = igStyle.FramePadding;

    igStyle.WindowRounding = 0.0f;

    mDefaultPadding = igStyle.WindowPadding;

    igStyle.WindowPadding = ImVec2(0.0f, 0.0f);

    mDefaultItemSpacing = igStyle.ItemSpacing;

    // the slider in for the z position
    igStyle.GrabMinSize = mSliderGrabMinSize;

    igStyle.FramePadding = ImVec2(mDefaultStyle.x, mComboWidgetHeight);
} // constructor

/**
 * @brief ImGuiHelper::~ImGuiHelper
 */
ImGuiHelper::~ImGuiHelper()
{
    cleanUp();
}

/**
 * @brief ImGuiHelper::render
 */
void ImGuiHelper::render()
{
    ImGui_ImplSdlGL3_NewFrame();

//    ImGuiStyle& igStyle = ImGui::GetStyle();

    int windowWidth = cSdlManager.getDimensions().x;
    int windowHeight = cSdlManager.getDimensions().y;

//    ImGui::SetNextWindowPos(ImVec2(0, 0));

//    // window title (id), show or not, size, alpha blending, settings
//    ImGui::Begin("##ComboBoxWindow", &mShowComboBoxWindow,
//        ImVec2(windowWidth, static_cast<int>(static_cast<float>(windowHeight) * 0.10f)),
//        mBackgroundAlpha, mImGuiWindowFlags);

//    ImGui::PopItemWidth();

//    ImGui::End();


    mShow_Overlay = true;
    ImGui::SetNextWindowPos(ImVec2(windowWidth * 0.5f - 100.0f, windowHeight * 0.5f - 100.0f));
    if (!ImGui::Begin("OverLay Window", &mShow_Overlay, ImVec2(0, 0), 1.0f, mOverlayFlags))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("mOpenGL_Vendor.c_str(           as", "%s");
    ImGui::TextColored(ImColor(0, 255, 0, 255), "Another thing of text", "%s");
    ImGui::Text("Another text\n");
    ImGui::End();

    ImGui::Render();
}

/**
 * @brief ImGuiHelper::cleanUp
 */
void ImGuiHelper::cleanUp()
{
    // Cleanup
    ImGui_ImplSdlGL3_Shutdown();
}

/**
 * @brief ImGuiHelper::ImGui_ImplSdlGL3_RenderDrawLists
 * @param draw_data
 */
void ImGuiHelper::ImGui_ImplSdlGL3_RenderDrawLists(ImDrawData* draw_data)
{
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    // Handle cases of screen coordinates != from framebuffer coordinates (e.g. retina displays)
    ImGuiIO& io = ImGui::GetIO();
    float fb_height = io.DisplaySize.y * io.DisplayFramebufferScale.y;
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    const float ortho_projection[4][4] =
    {
    { 2.0f/io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
    { 0.0f,                  2.0f/-io.DisplaySize.y, 0.0f, 0.0f },
    { 0.0f,                  0.0f,                  -1.0f, 0.0f },
    {-1.0f,                  1.0f,                   0.0f, 1.0f },
    };
    glUseProgram(sShaderHandle);
    glUniform1i(sAttribLocationTex, 0);
    glUniformMatrix4fv(sAttribLocationProjMat, 1, GL_FALSE, &ortho_projection[0][0]);
    glBindVertexArray(sVaoHandle);

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        glBindBuffer(GL_ARRAY_BUFFER, sVboHandle);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.size() * sizeof(ImDrawVert), (GLvoid*)&cmd_list->VtxBuffer.front(), GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sElementsHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx), (GLvoid*)&cmd_list->IdxBuffer.front(), GL_STREAM_DRAW);

        for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); pcmd++)
        {
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

/**
 * @brief ImGuiHelper::ImGui_ImplSdlGL3_GetClipboardText
 * @return
 */
const char* ImGuiHelper::ImGui_ImplSdlGL3_GetClipboardText()
{
    return SDL_GetClipboardText();
}

/**
 * @brief ImGuiHelper::ImGui_ImplSdlGL3_SetClipboardText
 * @param text
 */
void ImGuiHelper::ImGui_ImplSdlGL3_SetClipboardText(const char* text)
{
    SDL_SetClipboardText(text);
}

/**
 * @brief ImGuiHelper::ImGui_ImplSdlGL3_ProcessEvent
 * @param event
 * @return
 */
bool ImGuiHelper::ImGui_ImplSdlGL3_ProcessEvent(SDL_Event* event)
{
    ImGuiIO& io = ImGui::GetIO();
    switch (event->type)
    {
        case SDL_MOUSEWHEEL:
        {
            if (event->wheel.y > 0)
                g_MouseWheel = 1;
            if (event->wheel.y < 0)
                g_MouseWheel = -1;
            return true;
        }
        case SDL_MOUSEBUTTONDOWN:
        {
            if (event->button.button == SDL_BUTTON_LEFT) g_MousePressed[0] = true;
            if (event->button.button == SDL_BUTTON_RIGHT) g_MousePressed[1] = true;
            if (event->button.button == SDL_BUTTON_MIDDLE) g_MousePressed[2] = true;
            return true;
        }
        case SDL_TEXTINPUT:
        {
            ImGuiIO& io = ImGui::GetIO();
            io.AddInputCharactersUTF8(event->text.text);
            return true;
        }
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            int key = event->key.keysym.sym & ~SDLK_SCANCODE_MASK;
            io.KeysDown[key] = (event->type == SDL_KEYDOWN);
            io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
            io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
            io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
            return true;
        }
    }
    return false;
}

/**
 * @brief ImGuiHelper::ImGui_ImplSdlGL3_CreateFontsTexture
 */
void ImGuiHelper::ImGui_ImplSdlGL3_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    // Load as RGBA 32-bits for OpenGL3 demo because it is more likely to be compatible with user's existing shader.

    // Upload texture to graphics system
    glGenTextures(1, &g_FontTexture);
    glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = reinterpret_cast<void*>(static_cast<intptr_t>(g_FontTexture));
}

/**
 * @brief ImGuiHelper::ImGui_ImplSdlGL3_CreateDeviceObjects
 * @return
 */
bool ImGuiHelper::ImGui_ImplSdlGL3_CreateDeviceObjects()
{
    const GLchar *vertex_shader =
    "#version 300 es\n"
    "precision mediump float;\n"
    "uniform mat4 ProjMtx;\n"
    "in vec2 Position;\n"
    "in vec2 UV;\n"
    "in vec4 Color;\n"
    "out vec2 Frag_UV;\n"
    "out vec4 Frag_Color;\n"
    "void main()\n"
    "{\n"
    "	Frag_UV = UV;\n"
    "	Frag_Color = Color;\n"
    "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
    "}\n";

    const GLchar* fragment_shader =
    "#version 300 es\n"
    "precision mediump float;\n"
    "uniform sampler2D Texture;\n"
    "in vec2 Frag_UV;\n"
    "in vec4 Frag_Color;\n"
    "out vec4 Out_Color;\n"
    "void main()\n"
    "{\n"
    "	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
    "}\n";

    sShaderHandle = glCreateProgram();
    sVertHandle = glCreateShader(GL_VERTEX_SHADER);
    sFragHandle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(sVertHandle, 1, &vertex_shader, 0);
    glShaderSource(sFragHandle, 1, &fragment_shader, 0);
    glCompileShader(sVertHandle);
    glCompileShader(sFragHandle);
    glAttachShader(sShaderHandle, sVertHandle);
    glAttachShader(sShaderHandle, sFragHandle);
    glLinkProgram(sShaderHandle);

    glDeleteShader(sVertHandle);
    glDeleteShader(sFragHandle);

    sAttribLocationTex = glGetUniformLocation(sShaderHandle, "Texture");
    sAttribLocationProjMat = glGetUniformLocation(sShaderHandle, "ProjMtx");
    sAttribLocationPosition = glGetAttribLocation(sShaderHandle, "Position");
    sAttribLocationUv = glGetAttribLocation(sShaderHandle, "UV");
    sAttribLocationColor = glGetAttribLocation(sShaderHandle, "Color");

    glGenBuffers(1, &sVboHandle);
    glGenBuffers(1, &sElementsHandle);

    glGenVertexArrays(1, &sVaoHandle);
    glBindVertexArray(sVaoHandle);
    glBindBuffer(GL_ARRAY_BUFFER, sVboHandle);
    glEnableVertexAttribArray(sAttribLocationPosition);
    glEnableVertexAttribArray(sAttribLocationUv);
    glEnableVertexAttribArray(sAttribLocationColor);

    glVertexAttribPointer(sAttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), reinterpret_cast<GLvoid*>(offsetof(ImDrawVert, pos)));
    glVertexAttribPointer(sAttribLocationUv, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), reinterpret_cast<GLvoid*>(offsetof(ImDrawVert, uv)));
    glVertexAttribPointer(sAttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), reinterpret_cast<GLvoid*>(offsetof(ImDrawVert, col)));

    ImGui_ImplSdlGL3_CreateFontsTexture();

    return true;
}

/**
 * @brief ImGuiHelper::ImGui_ImplSdlGL3_InvalidateDeviceObjects
 */
void ImGuiHelper::ImGui_ImplSdlGL3_InvalidateDeviceObjects()
{
    if (sVaoHandle)
        glDeleteVertexArrays(1, &sVaoHandle);
    if (sVboHandle)
        glDeleteBuffers(1, &sVboHandle);
    if (sElementsHandle)
        glDeleteBuffers(1, &sElementsHandle);

    sVaoHandle = sVboHandle = sElementsHandle = 0;

    glDetachShader(sShaderHandle, sVertHandle);
    glDeleteShader(sVertHandle);
    sVertHandle = 0;

    glDetachShader(sShaderHandle, sFragHandle);
    glDeleteShader(sFragHandle);
    sFragHandle = 0;

    glDeleteProgram(sShaderHandle);
    sShaderHandle = 0;

    if (g_FontTexture)
    {
        glDeleteTextures(1, &g_FontTexture);
        ImGui::GetIO().Fonts->TexID = 0;
        g_FontTexture = 0;
    }
}

/**
 * @brief ImGuiHelper::ImGui_ImplSdlGL3_Init
 * @return
 */
bool ImGuiHelper::ImGui_ImplSdlGL3_Init()
{
    // Keyboard mapping.
    // ImGui will use those indices to peek into the io.KeyDown[] array.
    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = SDLK_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
    io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
    io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
    io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
    io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
    io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = SDLK_a;
    io.KeyMap[ImGuiKey_C] = SDLK_c;
    io.KeyMap[ImGuiKey_V] = SDLK_v;
    io.KeyMap[ImGuiKey_X] = SDLK_x;
    io.KeyMap[ImGuiKey_Y] = SDLK_y;
    io.KeyMap[ImGuiKey_Z] = SDLK_z;

    io.RenderDrawListsFn = ImGui_ImplSdlGL3_RenderDrawLists;
    io.SetClipboardTextFn = ImGui_ImplSdlGL3_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplSdlGL3_GetClipboardText;

    return true;
}

/**
 * @brief ImGuiHelper::ImGui_ImplSdlGL3_Shutdown
 */
void ImGuiHelper::ImGui_ImplSdlGL3_Shutdown()
{
    ImGui_ImplSdlGL3_InvalidateDeviceObjects();
    ImGui::Shutdown();
}

/**
 * @brief ImGuiHelper::ImGui_ImplSdlGL3_NewFrame
 */
void ImGuiHelper::ImGui_ImplSdlGL3_NewFrame()
{
    if (!g_FontTexture)
        ImGui_ImplSdlGL3_CreateDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w = cSdlManager.getDimensions().x, h = cSdlManager.getDimensions().y;
    io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    // Setup time step
    Uint32	time = SDL_GetTicks();
    double current_time = static_cast<double>(time) / 1000.0;
    io.DeltaTime = g_Time > 0.0 ? static_cast<float>(current_time - g_Time) : 1.0f / 60.0f;
    g_Time = current_time;

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from SDL_PollEvent())
    int mx, my;
    Uint32 mouseMask = SDL_GetMouseState(&mx, &my);
    if (SDL_GetWindowFlags(cSdlManager.getSdlWindow()) & SDL_WINDOW_MOUSE_FOCUS)
        io.MousePos = ImVec2(static_cast<float>(mx), static_cast<float>(my));
    else
        io.MousePos = ImVec2(-1, -1);

    // If a mouse press event came, always pass it as "mouse held this frame",
    // so we don't miss click-release events that are shorter than 1 frame.
    io.MouseDown[0] = g_MousePressed[0] || (mouseMask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    io.MouseDown[1] = g_MousePressed[1] || (mouseMask & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    io.MouseDown[2] = g_MousePressed[2] || (mouseMask & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
    g_MousePressed[0] = g_MousePressed[1] = g_MousePressed[2] = false;

    io.MouseWheel = g_MouseWheel;
    g_MouseWheel = 0.0f;

    // Start the frame
    ImGui::NewFrame();
} // new frame
