// Main file for the Maze Builder voxel editor application.

#include <iostream>
#include <stdexcept>
#include <memory>
#include <string>

#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/randomizer.h>

#include "craft.h"

// Run the SDL app
static constexpr auto window_w = 1200, window_h = 800;

const auto title{"Maze Builder - " + mazes::buildinfo::VERSION + " - " + mazes::buildinfo::COMMIT_SHA};

// Avoid function-local static initialization on wasm main thread.
// Eager init sidesteps __cxa_guard_acquire/pthread_cond_wait warnings.
std::shared_ptr<craft> g_voxel_engine = std::make_shared<craft>(title, window_w, window_h);

// Setup for Emscripten/WebAssembly
// Bind a getter method from C++ so that it can be accessed in the frontend with JS
#if defined(__EMSCRIPTEN__)
#include <emscripten/bind.h>

std::shared_ptr<craft> get()
{
    return g_voxel_engine;
}

EMSCRIPTEN_BINDINGS (craft_module)
{
    // Module.get() → returns the shared_ptr to the singleton engine instance.
    // Always call this first in onRuntimeInitialized before touching any other API.
    emscripten::function("get", &get, emscripten::allow_raw_pointers());

    emscripten::class_<craft>("craft")
        .smart_ptr<std::shared_ptr<craft>>("std::shared_ptr<craft>")
        // craft(title, width, height) – construction is handled by the global g_voxel_engine;
        // JS should use Module.get() rather than constructing a second instance.
        .constructor<const std::string&, int, int>()

        // ── Legacy synchronous download path ─────────────────────────────────
        // inst.artifacts() → full Wavefront OBJ string; may block if called before ready.
        .function("artifacts", &craft::artifacts)
        // inst.is_download_ready() → true once the OBJ data is available for download.
        .function("is_download_ready", &craft::is_download_ready)
        // inst.reset_download_flag() → call after consuming artifacts() to re-arm the flag.
        .function("reset_download_flag", &craft::reset_download_flag)

        // ── Async (non-blocking) export path — preferred for web ─────────────
        // inst.begin_export() → starts the background OBJ-generation worker; returns immediately.
        .function("begin_export", &craft::begin_export)
        // inst.is_export_ready() → non-blocking poll; returns true when the worker is done.
        .function("is_export_ready", &craft::is_export_ready)
        // inst.get_export() → retrieve the finished OBJ string; empty if worker not done yet.
        .function("get_export", &craft::get_export)
        // inst.get_export_status() → "idle" | "running" | "ready" for UI status overlays.
        .function("get_export_status", &craft::get_export_status)

        // ── Maze configuration (set before calling begin_export) ─────────────
        // inst.set_maze_rows(n) → number of rows for the maze grid (e.g. 10).
        .function("set_maze_rows", &craft::set_maze_rows)
        // inst.set_maze_columns(n) → number of columns for the maze grid (e.g. 10).
        .function("set_maze_columns", &craft::set_maze_columns)
        // inst.set_maze_algo("dfs") → algorithm name: "binary_tree" | "sidewinder" | "dfs".
        .function("set_maze_algo", &craft::set_maze_algo)
        // inst.set_maze_seed(n) → deterministic RNG seed; 0 = random.
        .function("set_maze_seed", &craft::set_maze_seed)

        // ── Engine meta ───────────────────────────────────────────────────────
        // inst.get_version() → MazeBuilder library version string (e.g. "8.2.1").
        .function("get_version", &craft::get_version);
}
#endif

int main()
{
    try
    {
        mazes::randomizer rng;

        if (!g_voxel_engine->run(nullptr, std::ref(rng)))
        {
            throw std::runtime_error("ERROR: Running SDL app failed.");
        }
    }
    catch (std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }

    return EXIT_SUCCESS;
}
