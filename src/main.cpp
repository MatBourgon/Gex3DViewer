#include "shader.h"
#include "mapreader.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp> // glm::pi
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>

#include <fstream>
#include <string>

#ifdef _WIN32
std::string OpenLoadPrompt(const char* filter)
{
    OPENFILENAMEA ofn{ sizeof(OPENFILENAMEA) };
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = "Open File...";
    char buffer[MAX_PATH + 1]{ 0 };
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = MAX_PATH;
    if (GetOpenFileNameA(&ofn) == TRUE)
        return buffer;
    return "";
}
#else
std::string OpenLoadPrompt(const char* filter) {}
#endif

glm::vec3 g_CamPos = { 0, 0, 0 };
glm::vec2 g_CamRot = { 0, 0 };

struct Vertex
{
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 uv;
};

glm::vec3 GetUpVector()
{
    return { 0, 1, 0 };
}

glm::vec3 GetForwardVector()
{
    glm::mat4 RotationMatrix = glm::rotate(glm::mat4(1.f), glm::radians(g_CamRot.x), GetUpVector());
    RotationMatrix = glm::rotate(RotationMatrix, glm::radians(g_CamRot.y), glm::vec3(-1, 0, 0));
    return glm::vec3{ RotationMatrix * glm::vec4(0, 0, 1, 1) };
}

glm::vec3 GetRightVector()
{
    return glm::cross(GetForwardVector(), GetUpVector());
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    g_CamPos += GetForwardVector() * (float)yoffset;
}

struct sleveldata_t
{
    level_t level;
    std::vector<GLuint> vbos;
    std::vector<std::vector<Vertex>> vertices;
    GLuint texid = 0;
    bool open = false;
};

void setTexture(sleveldata_t& leveldata, int texId)
{
    // hack
    if (texId >= (int)leveldata.level.textures.size() - 1)
        texId = 0;

    texture_t& texImg = leveldata.level.textures[texId];

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, leveldata.texid);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texImg.w, texImg.h, 0, GL_RGBA, GL_FLOAT, texImg.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
};

static void mouse_callback(GLFWwindow* window, double x, double y)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_RELEASE)
    {
        int w, h;
        glfwGetWindowSize(window, &w, &h);
        double dX = x - w / 2.0, dY = y - h / 2.0;
        glfwSetCursorPos(window, w / 2.0, h / 2.0);

        g_CamRot += glm::vec2{glm::radians(-5 * dX), glm::radians(-5 * dY)};
        g_CamRot.y = glm::clamp<float>(g_CamRot.y, -75.f, 75.f);
    }
}

bool wireframe = false;
bool texturesVis = true;
bool vertexCols = true;
bool hideInvisible = true;

void SetWireframe(bool state)
{
    wireframe = state;
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    texturesVis = vertexCols = !wireframe;
}

static void mousebtn_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        int w, h;
        glfwGetWindowSize(window, &w, &h);
        glfwSetCursorPos(window, w / 2.0, h / 2.0);
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
    }
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    if (key == GLFW_KEY_X && action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL))
    {
        SetWireframe(!wireframe);
    }
}

GLFWwindow* g_Window = nullptr;

bool cameraInvalidated = true;

glm::mat4 camera(const glm::mat4& model)
{
    static glm::mat4 Projection;
    static glm::mat4 View;
    if (cameraInvalidated)
    {
        int w, h;
        glfwGetWindowSize(g_Window, &w, &h);
        Projection = glm::perspective(glm::pi<float>() * 0.25f, w / (float)h, 0.1f, 100.f);
        glm::mat4 RotationMatrix = glm::rotate(glm::mat4(1.f), glm::radians(g_CamRot.x), GetUpVector());
        RotationMatrix = glm::rotate(RotationMatrix, glm::radians(g_CamRot.y), glm::vec3(-1, 0, 0));
        View = glm::lookAt(g_CamPos, g_CamPos + GetForwardVector() * 10.f, GetUpVector());
        cameraInvalidated = false;
    }
    
    return Projection * View * model;
}

void CloseLevel(sleveldata_t& leveldata)
{
    glBindTexture(GL_TEXTURE_2D, 0);
    if (leveldata.texid != 0)
        glDeleteTextures(1, &leveldata.texid);
    leveldata.texid = 0;
    if (leveldata.vbos.size() > 0)
        glDeleteBuffers(leveldata.vbos.size(), leveldata.vbos.data());
    leveldata.vbos.clear();
    leveldata.vertices.clear();
    for (auto& tex : leveldata.level.textures)
        delete[] tex.pixels;
    leveldata.level.textures.clear();
    leveldata.level.models.clear();
    leveldata.open = false;
}

bool OpenLevel(const char* levelPath, sleveldata_t& leveldata)
{
    CloseLevel(leveldata);
    printf("Loading level \"%s\"\n", levelPath);
    if (!LoadLevel(levelPath, leveldata.level))
    {
        return false;
    }
    leveldata.open = true;

    for (size_t i = 0; i <= leveldata.level.textures.size(); ++i)
        leveldata.vertices.push_back({});

    for (auto& poly : leveldata.level.models[0]->polygons)
    {
        for (int i = 0; i < 3; ++i)
        {
            auto& vert = leveldata.level.models[0]->vertices[poly.vertex[i]];
            if (poly.materialID == 0xFFFFFFFF)
                poly.materialID = leveldata.vertices.size() - 1; // these have no textures
            else
                poly.materialID %= leveldata.vertices.size(); // todo: fix bug where material id is bigger than texture list?
            leveldata.vertices[poly.materialID].push_back(
                {
                    {vert.x / 1000.f, vert.y / 1000.f, vert.z / 1000.f},
                    {vert.r / 255.f, vert.g / 255.f, vert.b / 255.f, ((poly.flags & 0x80) == 0x80) ? 0.f : (vert.a / 255.f)},
                    poly.uvs[i]
                });
        }
    }

    leveldata.vbos.resize(leveldata.vertices.size());
    glGenBuffers(leveldata.vertices.size(), leveldata.vbos.data());
    for (size_t i = 0; i < leveldata.vertices.size(); ++i)
    {
        glBindBuffer(GL_ARRAY_BUFFER, leveldata.vbos[i]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * leveldata.vertices[i].size(), leveldata.vertices[i].data(), GL_STATIC_DRAW);
    }

    glGenTextures(1, &leveldata.texid);

    return true;
}

void DrawLevel(sleveldata_t& leveldata, GLuint program)
{
    if (!leveldata.open)
        return;

    for (size_t i = 0; i < leveldata.vertices.size() - hideInvisible; ++i)
    {
        if (texturesVis)
            setTexture(leveldata, i); // this is more of a hack than anything. we need to move this to a texture atlas

        glBindBuffer(GL_ARRAY_BUFFER, leveldata.vbos[i]);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)sizeof(glm::vec3));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3) + sizeof(glm::vec4)));
        glEnableVertexAttribArray(2);

        glm::mat4 Model = glm::translate(glm::mat4(1.0f), { 0, 0, 0 });
        glm::mat4 cam = camera(Model);
        glUniformMatrix4fv(glGetUniformLocation(program, "uCamera"), 1, false, glm::value_ptr(cam));
        glUniform1i(glGetUniformLocation(program, "uWireframe"), (wireframe << 0) | (vertexCols << 1) | (texturesVis << 2));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, leveldata.texid);
        glUniform1i(glGetUniformLocation(program, "uTexture"), 0);
        glUseProgram(program);

        glDrawArrays(GL_TRIANGLES, 0, leveldata.vertices[i].size());
    }
}

int main()
{

    glfwInit();
    g_Window = glfwCreateWindow(1024, 720, "Gex 2 Level Viewer", NULL, NULL);

    glfwMakeContextCurrent(g_Window);
    gladLoadGL();
    glfwSetScrollCallback(g_Window, scroll_callback);
    glfwSetCursorPosCallback(g_Window, mouse_callback);
    glfwSetMouseButtonCallback(g_Window, mousebtn_callback);
    glfwSetKeyCallback(g_Window, key_callback);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = NULL;
    ImGui::GetIO().LogFilename = NULL;

    ImGui_ImplGlfw_InitForOpenGL(g_Window, true);
    ImGui_ImplOpenGL3_Init();


    unsigned int program;
    if (!LoadShader(program, { "../data/shaders/basic.vert", "../data/shaders/basic.frag" }))
        return 1;

    sleveldata_t leveldata;

    while(!glfwWindowShouldClose(g_Window))
    {
        glfwPollEvents();
        int width, height;
        glfwGetFramebufferSize(g_Window, &width, &height);
        glViewport(0, 0, width, height);

        if (wireframe)
            glClearColor(0, 0, 0, 1.f);
        else
            glClearColor(0xBB / 255.f, 0xF6 / 255.f, 0xF7/255.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        // transparent textures bugged rn
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glEnable(GL_BLEND);

        // todo: use state instead
        //if (glfwGetInputMode(g_Window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        {
            bool front = glfwGetKey(g_Window, GLFW_KEY_W) == GLFW_PRESS;
            bool back = glfwGetKey(g_Window, GLFW_KEY_S) == GLFW_PRESS;
            bool right = glfwGetKey(g_Window, GLFW_KEY_D) == GLFW_PRESS;
            bool left = glfwGetKey(g_Window, GLFW_KEY_A) == GLFW_PRESS;

            int hori = right - left;
            int forward = front - back;

            g_CamPos += ((GetForwardVector() * (float)forward) + (GetRightVector() * (float)hori)) * 0.1f;
        }

        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        DrawLevel(leveldata, program);

        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ 240, (float)height });
        if (ImGui::Begin("Camera", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar))
        {
            ImGui::Text("Camera:");
            ImGui::Text("  Position (%.0f, %.0f, %.0f)", g_CamPos.x * 1000.f, g_CamPos.y * 1000.f, g_CamPos.z * -1000.f);
            ImGui::Text("  Rotation (>: %.1f, ^: %.1f)", g_CamRot.x, g_CamRot.y);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("Stats:");
            ImGui::Text("  Polygons: %d", leveldata.level.models.empty() ? 0 : leveldata.level.models[0]->polygons.size());
            ImGui::Text("  Textures: %d", leveldata.level.textures.size() - 1);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Temporary load button, +code to center it
            auto ImGui_CenteredButton = [](const char* label) -> bool
                {
                    ImGuiStyle& style = ImGui::GetStyle();

                    float size = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
                    float avail = ImGui::GetContentRegionAvail().x;

                    ImGui::Spacing();
                    float off = (avail - size) * 0.5f;
                    if (off > 0.0f)
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
                    return ImGui::Button(label);
                };

            ImGui::Checkbox("Hide Invisible Objects?", &hideInvisible);
            if (ImGui::Checkbox("Toggle Wireframe Mode?", &wireframe))
                SetWireframe(wireframe);
            ImGui::Checkbox("Toggle Vertex Color?", &vertexCols);
            ImGui::Checkbox("Toggle Textures?", &texturesVis);
            if (ImGui_CenteredButton("Open Level (*.dfx)"))
            {
                auto path = OpenLoadPrompt("Gex 3D Level File (*.dfx)\0*.dfx\0All files (*.*)\0*.*\0");
                if (!path.empty())
                    OpenLevel(path.c_str(), leveldata);
            }

            if (ImGui_CenteredButton("Reset Camera"))
            {
                g_CamPos = { 0, 0, 0 };
                g_CamRot = { 0, 0 };
            }

            ImGuiStyle& style = ImGui::GetStyle();

            const char* msg = "Shoutout to the /r/Gex Discord";
            float sizey = ImGui::CalcTextSize(msg).y + style.FramePadding.y * 2.0f;
            float availy = ImGui::GetContentRegionAvail().y;
            float sizex = ImGui::CalcTextSize(msg).x + style.FramePadding.x * 2.0f;
            float availx = ImGui::GetContentRegionAvail().x;

            ImGui::Spacing();
            float offy = availy - sizey;
            float offx = (availx - sizex) / 2.f;
            if (offy > 0.0f)
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offy);
            if (offx > 0.0f)
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offx);
            ImGui::Text(msg);

            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(g_Window);
        cameraInvalidated = true;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(g_Window);
    glfwTerminate();

    return 0;
}