#include <argus/argus.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <numbers>
#include <string_view>
#include <vector>

namespace {

int fail(const char* what, argus::Error error) {
    const std::string_view name = argus::to_string(error);
    argus::log::error("{} failed ({})", what, name);
    std::printf("%s failed (%.*s)\n", what, static_cast<int>(name.size()),
                name.data());
    return 1;
}

void add_box(std::vector<argus::float3>* vertices,
             std::vector<argus::uint3>* triangles, argus::float3 lo,
             argus::float3 hi) {
    const auto base = static_cast<uint32_t>(vertices->size());
    const argus::float3 corner[] = {
        argus::float3{lo[0], lo[1], lo[2]}, argus::float3{hi[0], lo[1], lo[2]},
        argus::float3{hi[0], hi[1], lo[2]}, argus::float3{lo[0], hi[1], lo[2]},
        argus::float3{lo[0], lo[1], hi[2]}, argus::float3{hi[0], lo[1], hi[2]},
        argus::float3{hi[0], hi[1], hi[2]}, argus::float3{lo[0], hi[1], hi[2]},
    };
    vertices->insert(vertices->end(), corner, corner + 8);

    constexpr uint32_t face[][3] = {
        {0, 2, 1}, {0, 3, 2}, {4, 5, 6}, {4, 6, 7}, {0, 1, 5}, {0, 5, 4},
        {3, 6, 2}, {3, 7, 6}, {0, 4, 7}, {0, 7, 3}, {1, 2, 6}, {1, 6, 5},
    };
    for (const auto& index : face) {
        triangles->push_back(
            argus::uint3{base + index[0], base + index[1], base + index[2]});
    }
}

}  // namespace

int main() {
    [[maybe_unused]] argus::mem::Temporary_Thread_Scope temporary_storage;

    namespace sim = argus::sim;

    auto instance = sim::create_instance(sim::Backend::EMBREE);
    if (!instance) {
        if (instance.error() == argus::Error::UNSUPPORTED) {
            std::printf("embree unavailable\n");
            return 0;
        }
        return fail("instance", instance.error());
    }

    std::vector<argus::float3> vertices;
    std::vector<argus::uint3> triangles;
    add_box(&vertices, &triangles, {0.0F, 0.0F, 0.0F}, {10.0F, 3.0F, 10.0F});
    add_box(&vertices, &triangles, {4.0F, 0.0F, 4.0F}, {6.0F, 2.0F, 6.0F});

    argus::Mesh_View mesh;
    mesh.vertices = {vertices.data(), vertices.size()};
    mesh.triangles = {triangles.data(), triangles.size()};
    auto scene = sim::create_scene(*instance, mesh);
    if (!scene) {
        sim::destroy(*instance);
        return fail("scene", scene.error());
    }

    constexpr uint32_t columns = 64;
    constexpr uint32_t rows = 16;
    argus::Pattern pattern;
    pattern.directions.resize((static_cast<size_t>(columns) * rows));
    argus::fill_uniform_pattern(2.0F, 0.6F, columns, rows, &pattern);

    argus::Lidar lidars[2];
    lidars[0].pattern = argus::create_view(pattern);
    lidars[0].position = {1.5F, 1.4F, 1.5F};
    lidars[0].orientation.yaw = 0.3F;
    lidars[0].orientation.pitch = -0.15F;
    lidars[0].distance = 30.0F;
    lidars[1] = lidars[0];
    lidars[1].position = {8.5F, 1.4F, 1.5F};
    lidars[1].orientation.yaw = -0.3F;

    argus::Mirror mirrors[1];
    mirrors[0].position = {1.2F, 1.4F, 5.0F};
    mirrors[0].orientation.yaw = 1.2F;
    mirrors[0].extent.width = 1.5F;
    mirrors[0].extent.height = 1.0F;

    constexpr argus::Target targets[] = {
        {.min = -1.6F,
         .max = 1.6F,
         .index = 0,
         .subject = argus::Subject::MIRROR,
         .variable = argus::Variable::YAW,
         .bounded = true},
        {.min = -0.4F,
         .max = 0.4F,
         .index = 0,
         .subject = argus::Subject::MIRROR,
         .variable = argus::Variable::PITCH,
         .bounded = true},
        {.min = 0.4F,
         .max = 3.0F,
         .index = 0,
         .subject = argus::Subject::MIRROR,
         .variable = argus::Variable::X,
         .bounded = true},
    };
    if (const auto valid = argus::validate(targets, lidars, mirrors); !valid) {
        sim::destroy(*scene);
        sim::destroy(*instance);
        return fail("targets", valid.error());
    }

    argus::Grid grid;
    grid.min = {-0.25F, -0.25F, -0.25F};
    grid.voxel = 0.5F;
    grid.dims = {22, 8, 22};
    auto surface = argus::surface_voxels(argus::Context{}, mesh, grid);
    if (!surface || surface->voxels.empty()) {
        sim::destroy(*scene);
        sim::destroy(*instance);
        if (!surface) {
            return fail("surface", surface.error());
        }
        std::printf("no surface voxels\n");
        return 1;
    }
    float total_area = 0.0F;
    for (const float area : surface->area) {
        total_area += area;
    }
    std::printf("surface voxels %zu area %.3f\n", surface->voxels.size(),
                total_area);

    constexpr uint8_t together_groups[] = {0, 0};
    constexpr float together_weights[] = {1.0F};
    constexpr uint8_t separate_groups[] = {0, 1};
    constexpr float separate_weights[] = {0.5F, 0.5F};
    const float min_facing =
        std::cos(70.0F * std::numbers::pi_v<float> / 180.0F);
    argus::Facing_Limit facing;
    facing.min_facing = min_facing;

    argus::Coverage together;
    together.grid = grid;
    together.targets = surface->voxels;
    together.area = surface->area;
    together.groups = together_groups;
    together.group_weights = together_weights;
    together.accept = argus::accept_facing;
    together.accept_user = &facing;

    argus::Coverage separate = together;
    separate.groups = separate_groups;
    separate.group_weights = separate_weights;

    argus::Term terms[] = {
        {.metric = argus::coverage_metric, .user = &together, .weight = 1.0F},
    };

    argus::Problem problem;
    problem.targets = targets;
    problem.terms = terms;
    problem.scene = *scene;
    problem.max_bounces = 1;

    std::vector<argus::Hit> hits(sim::get_ray_count(lidars));
    std::vector<uint64_t> scratch(argus::scratch_words(
        surface->voxels.size(), std::size(separate_weights)));
    argus::Workspace workspace;
    workspace.problem = &problem;
    workspace.lidars = lidars;
    workspace.mirrors = mirrors;
    workspace.hits = hits;
    workspace.scratch = scratch;

    if (const auto cast =
            sim::cast(*scene, lidars, {}, problem.max_bounces, hits);
        !cast) {
        sim::destroy(*scene);
        sim::destroy(*instance);
        return fail("baseline", cast.error());
    }
    argus::Eval baseline_eval;
    baseline_eval.lidars = lidars;
    baseline_eval.hits = hits;
    baseline_eval.surface = &*surface;
    baseline_eval.scratch = scratch;
    const float baseline = argus::score(terms, baseline_eval);

    constexpr float yaws[] = {-1.2F, -0.4F, 0.4F, 1.2F};
    constexpr float pitches[] = {-0.3F, 0.0F, 0.3F};
    constexpr float xs[] = {0.6F, 1.2F, 2.0F, 2.8F};

    float best_together = -1.0F;
    float best_separate = -1.0F;
    float together_yaw = mirrors[0].orientation.yaw;
    float together_pitch = mirrors[0].orientation.pitch;
    float together_x = mirrors[0].position[0];
    float separate_yaw = together_yaw;
    float separate_pitch = together_pitch;
    float separate_x = together_x;
    for (const float yaw : yaws) {
        for (const float pitch : pitches) {
            for (const float x : xs) {
                const float params[] = {yaw, pitch, x};

                terms[0].user = &together;
                const float together_score =
                    argus::evaluate(params, &workspace);
                if (together_score > best_together) {
                    best_together = together_score;
                    together_yaw = yaw;
                    together_pitch = pitch;
                    together_x = x;
                }

                terms[0].user = &separate;
                const float separate_score =
                    argus::evaluate(params, &workspace);
                if (separate_score > best_separate) {
                    best_separate = separate_score;
                    separate_yaw = yaw;
                    separate_pitch = pitch;
                    separate_x = x;
                }
            }
        }
    }

    const float chosen[] = {together_yaw, together_pitch, together_x};
    terms[0].user = &together;
    best_together = argus::evaluate(chosen, &workspace);

    argus::Eval chosen_eval;
    chosen_eval.lidars = lidars;
    chosen_eval.hits = hits;
    chosen_eval.surface = &*surface;
    chosen_eval.scratch = scratch;
    const float blind = argus::blind_area_metric(chosen_eval, &together);
    const float density = argus::hit_density_cv_metric(chosen_eval, &together);

    size_t hit_count = 0;
    size_t bounced = 0;
    for (const argus::Hit& hit : hits) {
        if (!hit.hit) {
            continue;
        }
        ++hit_count;
        if (hit.bounces > 0) {
            ++bounced;
        }
    }

    std::printf("rays %zu hits %zu bounced %zu\n", hits.size(), hit_count,
                bounced);
    std::printf("baseline %.4f\n", baseline);
    std::printf("together %.4f gain %.4f at yaw %.2f pitch %.2f x %.2f\n",
                best_together, best_together - baseline, together_yaw,
                together_pitch, together_x);
    std::printf("separate %.4f at yaw %.2f pitch %.2f x %.2f\n", best_separate,
                separate_yaw, separate_pitch, separate_x);
    std::printf("blind area %.3f density cv %.4f\n", blind, density);

    sim::destroy(*scene);
    sim::destroy(*instance);
    return hit_count == 0 ? 1 : 0;
}
