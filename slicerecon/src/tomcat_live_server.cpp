#include <functional>
#include <numeric>

#include <spdlog/spdlog.h>

// not required, CLI args parser for testing server settings
#include "flags/flags.hpp"

// include the 'slicerecon' library headers
#include "slicerecon/slicerecon.hpp"

using namespace std::string_literals;

int main(int argc, char** argv)
{
    auto opts = flags::flags{argc, argv};
    opts.info(argv[0], "example of how to use `slicerecon` to host a "
                       "slice reconstruction server");

    auto host = opts.arg_or("--host", "localhost");
    auto port = opts.arg_as_or<int>("--port", 5558);

    auto rows = opts.arg_as_or<int32_t>("--rows", 1800);
    auto cols = opts.arg_as_or<int32_t>("--cols", 2016);

    auto darks = opts.arg_as_or<int32_t>("--darks", 10);
    auto flats = opts.arg_as_or<int32_t>("--flats", 10);
    auto projections = opts.arg_as_or<int32_t>("--projections", 128);

    // This is defined for the reconstruction
    auto slice_size = opts.arg_as_or<int32_t>("--slice-size", projections);
    auto preview_size = opts.arg_as_or<int32_t>("--preview-size", 128);
    auto group_size = opts.arg_as_or<int32_t>("--group-size", 32);
    auto filter_cores = opts.arg_as_or<int32_t>("--filter-cores", 8);
    if (slice_size < 0 || preview_size < 0 || group_size < 0 || filter_cores < 0) {
        std::cout << opts.usage();
        std::cout << "ERROR: Negative parameter passed\n";
        return -1;
    }

    auto mode = opts.passed("--continuous") 
                ? slicerecon::Mode::continuous : slicerecon::Mode::alternating;
    auto retrieve_phase = opts.passed("--phase");
    auto tilt = opts.passed("--tilt");
    auto gaussian_pass = opts.passed("--gaussian");
    auto filter = opts.arg_or("--filter", "shepp-logan");

    auto pixel_size = opts.arg_as_or<float>("--pixelsize", 1.0f);
    auto lambda = opts.arg_as_or<float>("--lambda", 1.23984193e-9);
    auto delta = opts.arg_as_or<float>("--delta", 1e-8);
    auto beta = opts.arg_as_or<float>("--beta", 1e-10);
    auto distance = opts.arg_as_or<float>("--distance", 40.0f);
    auto paganin = slicerecon::PaganinSettings{pixel_size, lambda, delta, beta, distance};

    auto recast_host = opts.arg_or("--recast-host", "localhost");

    auto bench = opts.passed("--bench");
    auto plugin = opts.passed("--plugin");
    auto py_plugin = opts.passed("--pyplugin");
    auto use_reqrep = opts.passed("--reqrep");

    if (opts.passed("-h") || !opts.sane()) {
        std::cout << opts.usage();
        return opts.passed("-h") ? 0 : -1;
    }

    auto params = slicerecon::Settings { slice_size, 
                                         preview_size, 
                                         group_size, 
                                         filter_cores, 
                                         darks, 
                                         flats, 
                                         mode, 
                                         false,
                                         retrieve_phase, 
                                         tilt, 
                                         paganin, 
                                         gaussian_pass, 
                                         filter };

    auto geom = slicerecon::Geometry { rows,
                                       cols,
                                       projections,
                                       {},
                                       true,
                                       false,
                                       {0.0f, 0.0f},
                                       {0.0f, 0.0f, 0.0f},
                                       {1.0f, 1.0f, 1.0f},
                                       0.0f,
                                       0.0f };

    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::info);

    // 1. setup reconstructor
    auto recon = std::make_unique<slicerecon::Reconstructor>(params, geom);

    // 2. listen to projection stream projection callback, push to projection stream all raw data
    auto proj = slicerecon::ProjectionServer(host, port, *recon);
    proj.serve();

    // 3. connect with (recast3d) visualization server
    auto viz = slicerecon::VisualizationServer("slicerecon test",
                                               "tcp://"s + recast_host + ":5555"s,
                                               "tcp://"s + recast_host + ":5556"s);
    viz.set_slice_callback([&](auto x, auto idx) { return recon->reconstructSlice(x); });
    recon->addListener(&viz);

    auto plugin_one = slicerecon::plugin("tcp://*:5650", "tcp://localhost:5651");
    plugin_one.set_slice_callback(
        [](auto shape, auto data, auto index) -> std::pair<std::array<int32_t, 2>, std::vector<float>> {
            for (auto& x : data) {
                if (x <= 3) {
                    x = 0;
                }
                else {
                    x = 17;
                }
            }

            return {shape, data};
        }
    );

    auto plugin_two = slicerecon::plugin("tcp://*:5651", "tcp://"s + recast_host + ":5555"s);
    plugin_two.set_slice_callback(
        [](auto shape, auto data,
        auto index) -> std::pair<std::array<int32_t, 2>, std::vector<float>> {
            for (auto& x : data) {
                (void)x;
            }

            return {shape, data};
        }
    );

    if (plugin) {
        viz.register_plugin("tcp://localhost:5650");
        plugin_one.serve();
        plugin_two.serve();
    }

    if (py_plugin) {
        std::cout << "Registering plugin at 5652\n";
        viz.register_plugin("tcp://localhost:5652");
    }

    if (bench) {
        slicerecon::util::bench.register_listener(&viz);
        slicerecon::util::bench.enable();
    }
    viz.serve();

    return 0;
}
