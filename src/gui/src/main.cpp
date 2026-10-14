#include <pch.hpp>

#include <raylib.h>
#include <raymath.h>
#include <rlImGui.h>
#include <rlgl.h>

#include <imgui.h>

#include <ImGuizmo.h>
#include <implot.h>

#include <algorithm>
#include <cmath>

namespace {

ImVec4 adjust_hue_and_saturation(const ImVec4& color, const float hue_shift,
                                 const float saturation_scale) {
    ImVec4 result_color = color;
    ImGui::ColorConvertRGBtoHSV(result_color.x, result_color.y, result_color.z,
                                result_color.x, result_color.y, result_color.z);

    result_color.x += hue_shift;
    result_color.y *= saturation_scale;

    ImGui::ColorConvertHSVtoRGB(result_color.x, result_color.y, result_color.z,
                                result_color.x, result_color.y, result_color.z);
    return result_color;
}

void apply_style() {
    ImGuiStyle& style = ImGui::GetStyle();

    constexpr float HUE = 0.0F;                // [0,1] range.
    constexpr float SATURATION = 1.0F;         // [0,6] range.
    constexpr float SATURATION_ACCENT = 1.0F;  // [0,6] range.
    constexpr float TRANSPARENCY = 0.75f;
    constexpr float BORDER_SIZE = 0.0F;

    style.FrameBorderSize = BORDER_SIZE;
    style.ImageBorderSize = BORDER_SIZE;
    style.TabBorderSize = BORDER_SIZE;
    style.TabBarBorderSize = 3.0F;
    style.WindowRounding = 4.0F;
    style.ChildRounding = 4.0F;
    style.FrameRounding = 4.0F;
    style.GrabRounding = 4.0F;
    style.TabRounding = 4.0F;

    ImVec4 text_color{1.000F, 1.000F, 1.000F, 1.000F};
    ImVec4 text_dimmed_color{0.357F, 0.482F, 0.549F, 1.000F};
    ImVec4 backround_color{0.110F, 0.149F, 0.169F, 1.000F};
    ImVec4 backround_child_color{0.090F, 0.122F, 0.141F, 1.000F};
    ImVec4 backround_dimmed_color{0.000F, 0.000F, 0.000F, 0.600F};
    ImVec4 title_color{0.078F, 0.102F, 0.122F, 1.000F};
    ImVec4 header_color{0.184F, 0.247F, 0.286F, 1.000F};
    ImVec4 accent1_color{0.000F, 0.490F, 1.000F, 1.000F};
    ImVec4 accent2_color{0.000F, 0.412F, 0.824F, 1.000F};
    ImVec4 accent1_alternative_color{0.302F, 0.408F, 0.475f, 1.000F};
    ImVec4 accent2_alternative_color{0.251F, 0.337F, 0.392F, 1.000F};
    ImVec4 transparent_color{0.0F, 0.0F, 0.0F, 0.0F};

    text_color = adjust_hue_and_saturation(text_color, HUE, SATURATION);
    text_dimmed_color =
        adjust_hue_and_saturation(text_dimmed_color, HUE, SATURATION);
    backround_color =
        adjust_hue_and_saturation(backround_color, HUE, SATURATION);
    backround_child_color =
        adjust_hue_and_saturation(backround_child_color, HUE, SATURATION);
    backround_dimmed_color =
        adjust_hue_and_saturation(backround_dimmed_color, HUE, SATURATION);
    title_color = adjust_hue_and_saturation(title_color, HUE, SATURATION);
    header_color = adjust_hue_and_saturation(header_color, HUE, SATURATION);
    accent1_color =
        adjust_hue_and_saturation(accent1_color, HUE, SATURATION_ACCENT);
    accent2_color =
        adjust_hue_and_saturation(accent2_color, HUE, SATURATION_ACCENT);
    accent1_alternative_color = adjust_hue_and_saturation(
        accent1_alternative_color, HUE, SATURATION_ACCENT);
    accent2_alternative_color = adjust_hue_and_saturation(
        accent2_alternative_color, HUE, SATURATION_ACCENT);

    ImVec4 backround_transparent_color = backround_color;
    backround_transparent_color.w *= TRANSPARENCY;

    ImVec4 backround_child_transparent_color = backround_child_color;
    backround_child_transparent_color.w *= TRANSPARENCY;

    ImVec4 title_transparent_color = title_color;
    title_transparent_color.w *= TRANSPARENCY;

    ImVec4 header_transparent_color = header_color;
    header_transparent_color.w *= TRANSPARENCY;

    style.Colors[ImGuiCol_Text] = text_color;
    style.Colors[ImGuiCol_TextDisabled] = text_dimmed_color;
    style.Colors[ImGuiCol_WindowBg] = backround_transparent_color;
    style.Colors[ImGuiCol_ChildBg] = backround_child_transparent_color;
    style.Colors[ImGuiCol_PopupBg] = backround_child_transparent_color;
    style.Colors[ImGuiCol_Border] = title_color;
    style.Colors[ImGuiCol_BorderShadow] = transparent_color;
    style.Colors[ImGuiCol_FrameBg] = header_transparent_color;
    style.Colors[ImGuiCol_FrameBgHovered] = accent1_alternative_color;
    style.Colors[ImGuiCol_FrameBgActive] = accent2_alternative_color;
    style.Colors[ImGuiCol_TitleBg] = header_transparent_color;
    style.Colors[ImGuiCol_TitleBgActive] = title_transparent_color;
    style.Colors[ImGuiCol_TitleBgCollapsed] = header_transparent_color;
    style.Colors[ImGuiCol_MenuBarBg] = backround_child_transparent_color;
    style.Colors[ImGuiCol_ScrollbarBg] = title_transparent_color;
    style.Colors[ImGuiCol_ScrollbarGrab] = header_color;
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = accent1_alternative_color;
    style.Colors[ImGuiCol_ScrollbarGrabActive] = accent2_alternative_color;
    style.Colors[ImGuiCol_CheckMark] = accent1_color;
    style.Colors[ImGuiCol_SliderGrab] = accent1_color;
    style.Colors[ImGuiCol_SliderGrabActive] = accent2_color;
    style.Colors[ImGuiCol_Button] = header_transparent_color;
    style.Colors[ImGuiCol_ButtonHovered] = accent1_color;
    style.Colors[ImGuiCol_ButtonActive] = accent2_color;
    style.Colors[ImGuiCol_Header] = header_transparent_color;
    style.Colors[ImGuiCol_HeaderHovered] = accent1_color;
    style.Colors[ImGuiCol_HeaderActive] = accent2_color;
    style.Colors[ImGuiCol_Separator] = header_color;
    style.Colors[ImGuiCol_SeparatorHovered] = accent1_color;
    style.Colors[ImGuiCol_SeparatorActive] = accent2_color;
    style.Colors[ImGuiCol_ResizeGrip] = header_color;
    style.Colors[ImGuiCol_ResizeGripHovered] = accent1_color;
    style.Colors[ImGuiCol_ResizeGripActive] = accent2_color;
    style.Colors[ImGuiCol_InputTextCursor] = text_color;
    style.Colors[ImGuiCol_TabHovered] = accent1_color;
    style.Colors[ImGuiCol_Tab] = transparent_color;
    style.Colors[ImGuiCol_TabSelected] = header_transparent_color;
    style.Colors[ImGuiCol_TabSelectedOverline] = transparent_color;
    style.Colors[ImGuiCol_TabDimmed] = transparent_color;
    style.Colors[ImGuiCol_TabDimmedSelected] =
        backround_child_transparent_color;
    style.Colors[ImGuiCol_TabDimmedSelectedOverline] = transparent_color;
    style.Colors[ImGuiCol_DockingPreview] = header_color;
    style.Colors[ImGuiCol_DockingEmptyBg] = backround_transparent_color;
    style.Colors[ImGuiCol_PlotLines] = text_dimmed_color;
    style.Colors[ImGuiCol_PlotLinesHovered] = accent1_color;
    style.Colors[ImGuiCol_PlotHistogram] = text_dimmed_color;
    style.Colors[ImGuiCol_PlotHistogramHovered] = accent1_color;
    style.Colors[ImGuiCol_TableHeaderBg] = backround_child_transparent_color;
    style.Colors[ImGuiCol_TableBorderStrong] = title_color;
    style.Colors[ImGuiCol_TableBorderLight] = header_color;
    style.Colors[ImGuiCol_TableRowBg] = backround_transparent_color;
    style.Colors[ImGuiCol_TableRowBgAlt] = backround_child_transparent_color;
    style.Colors[ImGuiCol_TextLink] = accent1_color;
    style.Colors[ImGuiCol_TextSelectedBg] = accent1_color;
    style.Colors[ImGuiCol_TreeLines] = text_dimmed_color;
    style.Colors[ImGuiCol_DragDropTarget] = accent1_color;
    style.Colors[ImGuiCol_DragDropTargetBg] = transparent_color;
    style.Colors[ImGuiCol_UnsavedMarker] = accent1_color;
    style.Colors[ImGuiCol_NavCursor] = accent1_color;
    style.Colors[ImGuiCol_NavWindowingHighlight] = accent1_color;
    style.Colors[ImGuiCol_NavWindowingDimBg] = backround_dimmed_color;
    style.Colors[ImGuiCol_ModalWindowDimBg] = backround_dimmed_color;
}

void apply_font() {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    ImFontConfig cfg;
    cfg.OversampleH = 1;
    cfg.OversampleV = 1;
    cfg.PixelSnapH = true;

    ImFont* font =
        io.Fonts->AddFontFromFileTTF("assets/fonts/JetBrainsMono-Regular.ttf");
    if (font == nullptr) {
        TraceLog(LOG_ERROR,
                 "Failed to load assets/fonts/JetBrainsMono-Regular.ttf");
        return;
    }

    io.FontDefault = font;
    style.FontSizeBase = 24.0F;
    style.FontScaleDpi = std::round(GetWindowScaleDPI().x);
}

void init_imgui() {
    rlImGuiSetup(true);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImPlot::CreateContext();
    apply_style();
    apply_font();
}

void shutdown_imgui() {
    ImPlot::DestroyContext();
    rlImGuiShutdown();
}

struct Gizmo_State {
    ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE mode = ImGuizmo::WORLD;
    float snap_translate = 0.5F;  // world units
    float snap_rotate = 15.0F;    // degrees
    float snap_scale = 0.1F;
};

void apply_gizmo_style() {
    ImGuizmo::AllowAxisFlip(false);
    ImGuizmo::SetGizmoSizeClipSpace(0.14F);

    ImGuizmo::Style& style = ImGuizmo::GetStyle();
    style.TranslationLineThickness = 2.5F;
    style.TranslationLineArrowSize = 7.0F;
    style.RotationLineThickness = 2.0F;
    style.RotationOuterLineThickness = 2.5F;
    style.ScaleLineThickness = 2.5F;
    style.ScaleLineCircleSize = 6.0F;
    style.HatchedAxisLineThickness = 5.0F;
    style.CenterCircleSize = 5.0F;

    constexpr ImVec4 red{0.92F, 0.29F, 0.34F, 1.0F};
    constexpr ImVec4 green{0.51F, 0.80F, 0.27F, 1.0F};
    constexpr ImVec4 blue{0.25F, 0.55F, 0.95F, 1.0F};
    constexpr ImVec4 gold{1.00F, 0.84F, 0.20F, 1.0F};

    const auto faded = [](ImVec4 color, float alpha) {
        color.w = alpha;
        return color;
    };

    style.Colors[ImGuizmo::DIRECTION_X] = red;
    style.Colors[ImGuizmo::DIRECTION_Y] = green;
    style.Colors[ImGuizmo::DIRECTION_Z] = blue;
    style.Colors[ImGuizmo::PLANE_X] = faded(red, 0.55F);
    style.Colors[ImGuizmo::PLANE_Y] = faded(green, 0.55F);
    style.Colors[ImGuizmo::PLANE_Z] = faded(blue, 0.55F);
    style.Colors[ImGuizmo::SELECTION] = gold;
    style.Colors[ImGuizmo::INACTIVE] = {0.6F, 0.6F, 0.6F, 0.5F};
    style.Colors[ImGuizmo::TRANSLATION_LINE] = {0.85F, 0.85F, 0.85F, 0.6F};
    style.Colors[ImGuizmo::SCALE_LINE] = {0.35F, 0.35F, 0.35F, 1.0F};
    style.Colors[ImGuizmo::ROTATION_USING_BORDER] = gold;
    style.Colors[ImGuizmo::ROTATION_USING_FILL] = faded(gold, 0.35F);
    style.Colors[ImGuizmo::HATCHED_AXIS_LINES] = {0.0F, 0.0F, 0.0F, 0.5F};
    style.Colors[ImGuizmo::TEXT] = {1.0F, 1.0F, 1.0F, 1.0F};
    style.Colors[ImGuizmo::TEXT_SHADOW] = {0.0F, 0.0F, 0.0F, 1.0F};
}

void handle_gizmo_shortcuts(Gizmo_State& gizmo, const bool flying) {
    if (flying || ImGui::GetIO().WantTextInput) {
        return;
    }

    if (IsKeyPressed(KEY_W)) {
        gizmo.operation = ImGuizmo::TRANSLATE;
    }
    if (IsKeyPressed(KEY_E)) {
        gizmo.operation = ImGuizmo::ROTATE;
    }
    if (IsKeyPressed(KEY_R)) {
        gizmo.operation = ImGuizmo::SCALE;
    }
    if (IsKeyPressed(KEY_T)) {
        gizmo.operation = ImGuizmo::UNIVERSAL;
    }
    if (IsKeyPressed(KEY_X)) {
        gizmo.mode =
            (gizmo.mode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
    }
}

bool draw_gizmo(const Gizmo_State& gizmo, const float16& view,
                const float16& proj, float* object_matrix) {
    const ImGuiIO& io = ImGui::GetIO();

    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
    ImGuizmo::SetRect(0.0F, 0.0F, io.DisplaySize.x, io.DisplaySize.y);

    // Hold Ctrl to snap.
    float snap[3] = {0.0F, 0.0F, 0.0F};
    const float* snap_ptr = nullptr;
    if (io.KeyCtrl) {
        if (gizmo.operation == ImGuizmo::ROTATE) {
            snap[0] = snap[1] = snap[2] = gizmo.snap_rotate;
        } else if (gizmo.operation == ImGuizmo::SCALE) {
            snap[0] = snap[1] = snap[2] = gizmo.snap_scale;
        } else {
            snap[0] = snap[1] = snap[2] = gizmo.snap_translate;
        }
        snap_ptr = snap;
    }

    return ImGuizmo::Manipulate(view.v, proj.v, gizmo.operation, gizmo.mode,
                                object_matrix, nullptr, snap_ptr);
}

struct Fly_Camera {
    Vector3 position{10.0F, 10.0F, 10.0F};
    float yaw = 0.0F;    // radians, 0 = looking along +Z
    float pitch = 0.0F;  // radians, clamped to +-89 deg
    float move_speed = 8.0F;
    float boost_multiplier = 3.0F;
    float mouse_sensitivity = 0.0025F;
    float fovy = 45.0F;
    bool flying = false;
};

Vector3 camera_forward(const Fly_Camera& cam) {
    const float cp = std::cos(cam.pitch);
    return {cp * std::sin(cam.yaw), std::sin(cam.pitch),
            cp * std::cos(cam.yaw)};
}

void camera_look_at(Fly_Camera& cam, const Vector3 target) {
    const Vector3 dir = Vector3Normalize(Vector3Subtract(target, cam.position));
    cam.yaw = std::atan2(dir.x, dir.z);
    cam.pitch = std::asin(dir.y);
}

void update_fly_state(Fly_Camera& cam) {
    const ImGuiIO& io = ImGui::GetIO();

    if (!cam.flying && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) &&
        !io.WantCaptureMouse) {
        cam.flying = true;
        DisableCursor();
    } else if (cam.flying && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        cam.flying = false;
        EnableCursor();
    }
}

void update_camera_look(Fly_Camera& cam) {
    const Vector2 delta = GetMouseDelta();
    constexpr float max_pitch = 89.0F * DEG2RAD;

    cam.yaw -= delta.x * cam.mouse_sensitivity;
    cam.pitch = std::clamp(cam.pitch - delta.y * cam.mouse_sensitivity,
                           -max_pitch, max_pitch);
}

void update_camera_movement(Fly_Camera& cam, const float dt) {
    constexpr Vector3 up{0.0F, 1.0F, 0.0F};
    const Vector3 forward = camera_forward(cam);
    const Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, up));

    Vector3 move{0.0F, 0.0F, 0.0F};
    if (IsKeyDown(KEY_W)) move = Vector3Add(move, forward);
    if (IsKeyDown(KEY_S)) move = Vector3Subtract(move, forward);
    if (IsKeyDown(KEY_D)) move = Vector3Add(move, right);
    if (IsKeyDown(KEY_A)) move = Vector3Subtract(move, right);
    if (IsKeyDown(KEY_E)) move = Vector3Add(move, up);
    if (IsKeyDown(KEY_Q)) move = Vector3Subtract(move, up);

    if (Vector3LengthSqr(move) > 0.0F) {
        float speed = cam.move_speed;
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
            speed *= cam.boost_multiplier;
        }
        move = Vector3Scale(Vector3Normalize(move), speed * dt);
        cam.position = Vector3Add(cam.position, move);
    }

    // Wheel while flying = adjust speed (like Unity / Unreal).
    const float wheel = GetMouseWheelMove();
    if (wheel != 0.0F) {
        cam.move_speed =
            std::clamp(cam.move_speed * std::pow(1.15F, wheel), 0.5F, 200.0F);
    }
}

void update_fly_camera(Fly_Camera& cam, const float dt) {
    update_fly_state(cam);
    if (cam.flying) {
        update_camera_look(cam);
        update_camera_movement(cam, dt);
    }
}

Camera3D to_camera3d(const Fly_Camera& cam) {
    return {
        .position = cam.position,
        .target = Vector3Add(cam.position, camera_forward(cam)),
        .up = {.x = 0.0F, .y = 1.0F, .z = 0.0F},
        .fovy = cam.fovy,
        .projection = CAMERA_PERSPECTIVE,
    };
}

float16 camera_view_matrix(const Camera3D& camera) {
    return MatrixToFloatV(GetCameraMatrix(camera));
}

float16 camera_projection_matrix(const Camera3D& camera) {
    const float aspect = static_cast<float>(GetScreenWidth()) /
                         static_cast<float>(GetScreenHeight());
    return MatrixToFloatV(
        MatrixPerspective(static_cast<double>(camera.fovy) * DEG2RAD, aspect,
                          rlGetCullDistanceNear(), rlGetCullDistanceFar()));
}

void draw_gizmo_object(const float* object_matrix) {
    rlPushMatrix();
    rlMultMatrixf(object_matrix);
    DrawCube({0.0F, 0.0F, 0.0F}, 1.0F, 1.0F, 1.0F, ORANGE);
    DrawCubeWires({0.0F, 0.0F, 0.0F}, 1.0F, 1.0F, 1.0F, BLACK);
    rlPopMatrix();
}

void draw_scene(const Camera3D& camera, const float* object_matrix) {
    ClearBackground(DARKGRAY);
    BeginMode3D(camera);
    DrawGrid(20, 1.0F);
    draw_gizmo_object(object_matrix);
    EndMode3D();
}

void draw_dockspace_host() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    constexpr ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0F, 0.0F});

    ImGui::Begin("DockSpace Host", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    const ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, {0.0F, 0.0F},
                     ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();
}

// Engine-style inspector: edit translation / rotation / scale numerically.
void draw_inspector(Gizmo_State& gizmo, Fly_Camera& cam, float* object_matrix) {
    ImGui::Begin("Inspector");

    float t[3] = {};
    float r[3] = {};
    float s[3] = {};
    ImGuizmo::DecomposeMatrixToComponents(object_matrix, t, r, s);
    bool changed = false;
    changed |= ImGui::DragFloat3("Position", t, 0.05F);
    changed |= ImGui::DragFloat3("Rotation", r, 0.5F);
    changed |= ImGui::DragFloat3("Scale", s, 0.01F);
    if (changed) {
        ImGuizmo::RecomposeMatrixFromComponents(t, r, s, object_matrix);
    }

    ImGui::SeparatorText("Gizmo");
    int op = 0;
    if (gizmo.operation == ImGuizmo::TRANSLATE) {
        op = 0;
    } else if (gizmo.operation == ImGuizmo::ROTATE) {
        op = 1;
    } else if (gizmo.operation == ImGuizmo::SCALE) {
        op = 2;
    } else {
        op = 3;
    }

    bool op_changed = false;
    op_changed |= ImGui::RadioButton("Move (W)", &op, 0);
    ImGui::SameLine();
    op_changed |= ImGui::RadioButton("Rotate (E)", &op, 1);
    ImGui::SameLine();
    op_changed |= ImGui::RadioButton("Scale (R)", &op, 2);
    ImGui::SameLine();
    op_changed |= ImGui::RadioButton("All (T)", &op, 3);
    if (op_changed) {
        constexpr ImGuizmo::OPERATION ops[] = {
            ImGuizmo::TRANSLATE, ImGuizmo::ROTATE, ImGuizmo::SCALE,
            ImGuizmo::UNIVERSAL};
        gizmo.operation = ops[op];
    }

    bool local = gizmo.mode == ImGuizmo::LOCAL;
    if (ImGui::Checkbox("Local space (X)", &local)) {
        gizmo.mode = local ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
    }

    ImGui::DragFloat("Snap move", &gizmo.snap_translate, 0.05F, 0.01F, 100.0F);
    ImGui::DragFloat("Snap rotate", &gizmo.snap_rotate, 0.5F, 1.0F, 180.0F);
    ImGui::DragFloat("Snap scale", &gizmo.snap_scale, 0.01F, 0.01F, 10.0F);
    ImGui::TextDisabled("Hold Ctrl to snap");

    ImGui::SeparatorText("Camera");
    ImGui::DragFloat("Speed", &cam.move_speed, 0.1F, 0.5F, 200.0F);
    ImGui::DragFloat("FOV", &cam.fovy, 0.5F, 20.0F, 120.0F);
    ImGui::TextDisabled("Hold RMB + WASD/QE, Shift = fast, wheel = speed");

    ImGui::End();
}

}  // namespace

int main() {
    constexpr int width = 1280;
    constexpr int height = 720;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(width, height, "argus");

    init_imgui();
    apply_gizmo_style();

    Fly_Camera fly_camera;
    camera_look_at(fly_camera, {0.0F, 0.0F, 0.0F});

    Gizmo_State gizmo;

    float object_matrix[16] = {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
                               0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F};

    while (!WindowShouldClose()) {
        update_fly_camera(fly_camera, GetFrameTime());
        handle_gizmo_shortcuts(gizmo, fly_camera.flying);

        const Camera3D camera = to_camera3d(fly_camera);
        const float16 view_arr = camera_view_matrix(camera);
        const float16 proj_arr = camera_projection_matrix(camera);

        BeginDrawing();
        draw_scene(camera, object_matrix);

        rlImGuiBegin();

        draw_dockspace_host();
        draw_inspector(gizmo, fly_camera, object_matrix);
        // No gizmo interaction while flying (cursor is captured anyway).
        if (!fly_camera.flying) {
            draw_gizmo(gizmo, view_arr, proj_arr, object_matrix);
        }

        // ImGui::ShowDemoWindow();
        // ImPlot::ShowDemoWindow();

        rlImGuiEnd();
        EndDrawing();
    }

    shutdown_imgui();
    CloseWindow();
    return 0;
}
