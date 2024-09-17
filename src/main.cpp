#include "shader.h"
#include "mapreader.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

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
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
        return;

    g_CamPos += GetForwardVector() * (float)yoffset;
}

struct sleveldata_t
{
    level_t level;
    GLuint texid = 0;
    bool open = false;
};

static void mouse_callback(GLFWwindow* window, double x, double y)
{
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
        return;

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
bool noObjects = false;
bool enableBillboarding = true;

void SetWireframe(bool state)
{
    wireframe = state;
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    texturesVis = vertexCols = !wireframe;
}

static void mousebtn_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
        return;

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
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
        return;

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
        View = glm::lookAt(g_CamPos, g_CamPos + GetForwardVector() * 10.f, GetUpVector());
        cameraInvalidated = false;
    }
    
    return Projection * View * model;
}

bool IsBillboardObject(const std::string& name)
{
    const char* BillboardObjectNames[] = {
        "nflame__",
        "mflame__",

        "charger_",
        "steam___",

        /// COLLECTIBLES
        // Aztec 2 Step
        "gem_____",

        // I Got the Reruns
        "coltv___",

        // Trouble in Uranus
        "saucer__",

        // In Drag Net
        "badge___",

        // The Spy Who Loved Himself
        "case____",

        // Circuit Central
        "batt____", // Chips and Dips
        "led_____",
        "atom____",

        // Scream TV
        "skull___", // Thursday the 12th
        "tomb____",
        "jason___",

        // Kung-Fu Theatre
        "takeout_", // Lizard in a China Shop
        "yinyang_",
        "kabuki__",

        // Toon TV
        "carrot__",
        "spinach_",
        "plunge__",

        // Prehistory Channel
        "drum____",
        "cowhead_",
        "dino____",

        // Space Channel
        "ship____",
        "phaser__",
        "robot___"

        // Mazed and Confused (Rezop 1)
        "cd______",
        "radiate_", // Bugged Out
        "camera__",

        // No Weddings and a Funeral (Rezop 3)
        "gear____",
        "toolbox_",
        "oilcan__",
    };
    const char** pENDPTR = BillboardObjectNames + sizeof(BillboardObjectNames) / sizeof(const char*);

    return std::find_if(BillboardObjectNames, pENDPTR,
        [&name](const char* objName) {
            return name == objName;
        }) != pENDPTR;
}

struct globj_t
{
    GLuint vbo = 0;
    std::vector<Vertex> vertices;
    void draw(GLuint program, sleveldata_t& leveldata, objinstance_t& inst, const std::string& name)
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)sizeof(glm::vec3));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3) + sizeof(glm::vec4)));
        glEnableVertexAttribArray(2);

        glm::mat4 Model = glm::translate(glm::mat4(1.f), -inst.position);
        bool doBillboarding = enableBillboarding && IsBillboardObject(name);
        if (doBillboarding)
        {
            Model = glm::rotate(Model, glm::radians(g_CamRot.x-180), {0, 1, 0});
        }
        else
        {
            //Model = glm::rotate(Model, -inst.rotation.x, { 1, 0, 0 });
            Model = glm::rotate(Model, -inst.rotation.y, { 0, 1, 0 });
            //Model = glm::rotate(Model, -inst.rotation.z, { 0, 0, 1 });
        }
        glm::mat4 cam = camera(Model);
        glUniformMatrix4fv(glGetUniformLocation(program, "uCamera"), 1, false, glm::value_ptr(cam));
        glUniform1i(glGetUniformLocation(program, "uWireframe"), (wireframe << 0) | (vertexCols << 1) | (texturesVis << 2));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, leveldata.texid);
        glUniform1i(glGetUniformLocation(program, "uTexture"), 0);
        // extra hack for billboarding transparency
        // todo: hopefully remove with fixed textures
        glUniform1i(glGetUniformLocation(program, "uBillboard"), (int)doBillboarding);
        glUseProgram(program);

        glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    }
};
std::vector<std::shared_ptr<globj_t>> mdls;

void CloseLevel(sleveldata_t& leveldata)
{
    glBindTexture(GL_TEXTURE_2D, 0);
    if (leveldata.texid != 0)
        glDeleteTextures(1, &leveldata.texid);
    leveldata.texid = 0;
    for (auto& tex : leveldata.level.textures)
        delete[] tex.pixels;
    leveldata.level.textures.clear();
    leveldata.level.models.clear();
    leveldata.open = false;
    mdls.clear();
}

std::shared_ptr<globj_t> createobj(std::shared_ptr<Model> model)
{
    auto ptr = std::make_shared<globj_t>();

    for (auto& p : model->polygons)
    {
        for (int i = 0; i < 3; ++i)
        {
            auto& v = model->vertices[p.vertex[i]];
            ptr->vertices.push_back({ {v.x / 1000.f, v.y / 1000.f, v.z / 1000.f}, {v.r / 255.f, v.g / 255.f, v.b / 255.f, v.a / 255.f}, p.uvs[i] });
            if (p.materialID == 0xFFFF'FFFF)
                ptr->vertices.rbegin()->color.a = 0.f;
        }
    }

    glGenBuffers(1, &ptr->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ptr->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * ptr->vertices.size(), ptr->vertices.data(), GL_STATIC_DRAW);

    return ptr;
}

std::string levelPath, levelName;
bool OpenLevel(const char* levelPath, sleveldata_t& leveldata)
{
    CloseLevel(leveldata);
    printf("Loading level \"%s\"\n", levelPath);
    if (!LoadLevel(levelPath, leveldata.level))
    {
        ::levelPath = levelName = "";
        return false;
    }
    ::levelPath = levelPath;
    size_t fsi = ::levelPath.find_last_of("/");
    size_t bsi = ::levelPath.find_last_of("\\");
    if (fsi != std::string::npos || bsi != std::string::npos)
    {
        size_t pos = 0;
        if (fsi != std::string::npos && bsi != std::string::npos)
        {
            pos = fsi < bsi ? bsi : fsi;
        }
        else
        {
            if (fsi != std::string::npos)
                pos = fsi;
            else
                pos = bsi;
        }

        ::levelPath = ::levelPath.substr(pos + 1);
    }
    levelName = leveldata.level.name;
    leveldata.open = true;

    for (auto& m : leveldata.level.models)
        mdls.push_back(createobj(m));

    glGenTextures(1, &leveldata.texid);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, leveldata.texid);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, leveldata.level.sheet.w, leveldata.level.sheet.h, 0, GL_RGBA, GL_FLOAT, leveldata.level.sheet.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
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
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplGlfw_InitForOpenGL(g_Window, true);
    ImGui_ImplOpenGL3_Init();

    unsigned int program;
    if (!LoadShader(program, { "../data/shaders/basic.vert", "../data/shaders/basic.frag" }))
        return 1;

    sleveldata_t leveldata;

    std::vector<Vertex> vertices = {
        {{-10, -10, 50}, {0.5f, 0.5f, 0.5f, 0.5f}, {1, 1}},
        {{10, -10, 50}, {0.5f, 0.5f, 0.5f, 0.5f}, {0, 1}},
        {{10, 10, 50}, {0.5f, 0.5f, 0.5f, 0.5f}, {0, 0}},
        {{-10, -10, 50}, {0.5f, 0.5f, 0.5f, 0.5f}, {1, 1}},
        {{10, 10, 50}, {0.5f, 0.5f, 0.5f, 0.5f}, {0, 0}},
        {{-10, 10, 50}, {0.5f, 0.5f, 0.5f, 0.5f}, {1, 0}},
    };

    OpenLevel(R"(C:\Users\Matt\Desktop\level\Map5.dfx)", leveldata);

    bool showTexturePanel = false;
    float textureZoomScale = 1.f;

    bool toggleObjectsMenu = false;

    ImVec4 bgColor = { 0xBB / 255.f, 0xF6 / 255.f, 0xF7 / 255.f, 255 };

    while(!glfwWindowShouldClose(g_Window))
    {
        glfwPollEvents();
        int width, height;
        glfwGetFramebufferSize(g_Window, &width, &height);
        glViewport(0, 0, width, height);

        if (wireframe)
            glClearColor(0, 0, 0, 1.f);
        else
            glClearColor(bgColor.x, bgColor.y, bgColor.z, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        // transparent textures bugged rn
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glEnable(GL_BLEND);

        // todo: use state instead
        //if (glfwGetInputMode(g_Window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        {
            if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
            {
                bool front = glfwGetKey(g_Window, GLFW_KEY_W) == GLFW_PRESS;
                bool back = glfwGetKey(g_Window, GLFW_KEY_S) == GLFW_PRESS;
                bool right = glfwGetKey(g_Window, GLFW_KEY_D) == GLFW_PRESS;
                bool left = glfwGetKey(g_Window, GLFW_KEY_A) == GLFW_PRESS;

                int hori = right - left;
                int forward = front - back;

                g_CamPos += ((GetForwardVector() * (float)forward) + (GetRightVector() * (float)hori)) * 0.1f;
            }
        }

        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        for (size_t i = 0; i < leveldata.level.models.size(); ++i)
        {
            if (leveldata.level.models[i]->objectVisibility)
                for (auto& inst : leveldata.level.models[i]->instances)
                {
                    if (inst.isVisible)
                        mdls[i]->draw(program, leveldata, inst, leveldata.level.models[i]->name);
                }
            if (noObjects)
                break;
        }

        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ 240, (float)height });
        if (ImGui::Begin("Camera", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysVerticalScrollbar))
        {
            ImGui::Text("Level: %s", levelName.c_str());
            ImGui::Text("File name: %s", levelPath.c_str());
            ImGui::Separator();
            ImGui::Spacing();
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

            if (ImGui::Checkbox("Toggle Wireframe Mode?", &wireframe))
                SetWireframe(wireframe);
            ImGui::Checkbox("Toggle Vertex Color?", &vertexCols);
            ImGui::Checkbox("Toggle Textures?", &texturesVis);
            ImGui::Checkbox("Toggle Objects?", &noObjects);
            ImGui::Checkbox("Toggle Billboarding?", &enableBillboarding);
            if (ImGui_CenteredButton("Open Level (*.dfx)"))
            {
                auto path = OpenLoadPrompt("Gex 3D Level File (*.dfx)\0*.dfx\0All files (*.*)\0*.*\0");
                if (!path.empty())
                    OpenLevel(path.c_str(), leveldata);
            }

            if (ImGui_CenteredButton("Open Objects Panel"))
            {
                toggleObjectsMenu = true;
            }

            if (ImGui_CenteredButton("Reset Camera"))
            {
                g_CamPos = { 0, 0, 0 };
                g_CamRot = { 0, 0 };
            }

            ImGui::Spacing();
            ImGuiStyle& style = ImGui::GetStyle();
            float ratio = (ImGui::GetContentRegionAvail().x - style.FramePadding.x * 2.f) / leveldata.level.sheet.w;
            auto [cx, cy] = ImGui::GetCursorPos();
            //if (ImGui::ImageButton((ImTextureID)leveldata.texid, { ratio * leveldata.level.sheet.w, (float)leveldata.level.sheet.h * ratio }))
            //{
            //    showTexturePanel = true;
            //}

            ImGui::ColorPicker3("Skybox\nColor", &bgColor.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoOptions);

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

        if (showTexturePanel)
        {
            ImGui::SetNextWindowSize({ 512, 512 }, ImGuiCond_Appearing);
            ImGui::SetNextWindowPos({ ImGui::GetWindowWidth() / 2.f, ImGui::GetWindowHeight() / 2.f }, ImGuiCond_Appearing);
            if (ImGui::Begin("Texture Atlas", &showTexturePanel, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar))
            {
                ImGui::Text("Atlas Size: %lux%lu", leveldata.level.sheet.w, leveldata.level.sheet.h);
                ImGui::SliderFloat("Texture Zoom", &textureZoomScale, 1.f, 8.f, "%.0f");
                ImGuiStyle& style = ImGui::GetStyle();
                float ratio = 1.f;
                if (ImGui::GetContentRegionAvail().x < ImGui::GetContentRegionAvail().y)
                {
                    ratio = (ImGui::GetContentRegionAvail().x - style.FramePadding.x * 2.f) / leveldata.level.sheet.w;
                }
                else
                {
                    ratio = (ImGui::GetContentRegionAvail().y - style.FramePadding.y * 2.f) / leveldata.level.sheet.h;
                }
                auto [cx, cy] = ImGui::GetCursorPos();
                ImGui::Image((ImTextureID)leveldata.texid, { ratio * leveldata.level.sheet.w * textureZoomScale, (float)leveldata.level.sheet.h * ratio * textureZoomScale });
                ImGui::End();
            }
        }

        if (toggleObjectsMenu)
        {
            ImGui::SetNextWindowSize({ 512, 512 }, ImGuiCond_Appearing);
            ImGui::SetNextWindowPos({ ImGui::GetWindowWidth() / 2.f, ImGui::GetWindowHeight() / 2.f }, ImGuiCond_Appearing);
            if (ImGui::Begin("Objects", &toggleObjectsMenu, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysVerticalScrollbar))
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
                if (ImGui::Button("Show All"))
                {
                    for (auto& mdl : leveldata.level.models)
                        mdl->objectVisibility = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Hide All"))
                {
                    for (auto& mdl : leveldata.level.models)
                        mdl->objectVisibility = false;
                }
                for (auto& mdl : leveldata.level.models)
                {
                    if (mdl->addr == 0xFFFF'FFFF)
                        ImGui::Text("Level");
                    else if (mdl->addr == 0x0000'0000)
                        ImGui::Text("Misc.");
                    else
                        ImGui::Text(mdl->name.c_str());

                    ImGui::BeginGroup();
                    ImGui::Indent(8.f);
                    ImGui::Checkbox(("Visible?##" + std::to_string(mdl->addr)).c_str(), &mdl->objectVisibility);
                    ImGui::Text("Instances: %u", mdl->instances.size());
                    ImGui::Checkbox(("Show Instances List##" + std::to_string(mdl->addr)).c_str(), &mdl->showInstances);
                    if (ImGui::Button(("Show All##" + std::to_string(mdl->addr)).c_str()))
                    {
                        for (auto& inst : mdl->instances)
                            inst.isVisible = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(("Hide All##" + std::to_string(mdl->addr)).c_str()))
                    {
                        for (auto& inst : mdl->instances)
                            inst.isVisible = false;
                    }
                    if (mdl->showInstances)
                    {
                        ImGui::Indent(8.f);
                        int __i = 0;
                        for (auto& inst : mdl->instances)
                        {
                            ImGui::Text("Pos: (%.0f, %.0f, %.0f) Rot: (%.0f, %.0f, %.0f)", -inst.position.x * 1000.f, inst.position.y * 1000.f, inst.position.z * 1000.f, inst.rotation.x * 180 / glm::pi<float>(), inst.rotation.y * 180 / glm::pi<float>(), inst.rotation.z * 180 / glm::pi<float>());
                            ImGui::Checkbox(("Visible?##" + std::to_string(mdl->addr) + "_" + std::to_string(__i++)).c_str(), &inst.isVisible);
                            ImGui::Separator();
                        }
                        ImGui::Unindent(8.f);
                    }
                    ImGui::Unindent(8.f);
                    ImGui::EndGroup();

                    ImGui::Separator();
                }
                ImGui::End();
            }
        }

        ImGui::Render();
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
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