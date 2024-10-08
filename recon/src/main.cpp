/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <functional>
#include <memory>
#include <string>

#include <spdlog/spdlog.h>
#include <boost/program_options.hpp>

#include "common/version.hpp"
#include "recon/application.hpp"
#include "recon/ramp_filter.hpp"
#include "recon/reconstructor.hpp"
#include "recon/daq/daq_factory.hpp"

namespace po = boost::program_options;

std::pair<uint32_t, uint32_t> parseDownsampleFactor(
        const po::variable_value& downsample_row, 
        const po::variable_value& downsample_col, 
        const po::variable_value& downsample) {
    uint32_t row = downsample_row.empty() ? downsample.as<uint32_t>() : downsample_row.as<uint32_t>();
    uint32_t col = downsample_col.empty() ? downsample.as<uint32_t>() : downsample_col.as<uint32_t>();
    return {row, col};
}

recastx::AngleRange parseAngleRange(const po::variable_value& value) {
    int angle_range = value.as<int>();
    if (angle_range == 180) return recastx::AngleRange::HALF;
    if (angle_range == 360) return recastx::AngleRange::FULL;
    throw std::runtime_error("Angle range must be either 180 or 360");
}

int main(int argc, char** argv) {

    spdlog::info(std::string(80, '='));
    spdlog::info(std::string(30, ' ') + "RECASTX v{}", getRecastxVersion());
    spdlog::info(std::string(80, '='));

    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

    bool auto_acquiring = false;
    bool auto_processing = false;
    po::options_description general_desc("General options");
    general_desc.add_options()
        ("help,h", "print help message")
        ("auto-acquiring", po::bool_switch(&auto_acquiring),
         "start data acquisition automatically (without waiting for a trigger from the GUI client)")
        ("auto-processing", po::bool_switch(&auto_processing), 
         "start data processing automatically (without waiting for a trigger from the GUI client)")
    ;

    po::options_description communication_desc("Communication options");
    communication_desc.add_options()
        ("daq-protocol", po::value<std::string>()->default_value("tcp"), 
         "ZMQ transport protocol of the DAQ server. Options: tcp/ipc")
        ("daq-address", po::value<std::string>()->default_value("localhost:9667"), 
         "address (e.g. 127.0.0.1:5555) of the DAQ server")
        ("daq-socket", po::value<std::string>()->default_value("pull"),
         "ZMQ socket type of the DAQ server. Options: sub/pull")
        ("daq-data-protocol", po::value<std::string>()->default_value("default"),
         "DAQ data protocol")
        ("rpc-port", po::value<int>()->default_value(9971),
         "port of the gRPC server."
         "At TOMCAT, the valid port range is [9970, 9979]")
    ;

    po::options_description geometry_desc("Geometry options");
    geometry_desc.add_options()
        ("cols", po::value<size_t>()->default_value(0),
         "detector width in pixels")
        ("rows", po::value<size_t>()->default_value(0),
         "detector height in pixels")
        ("downsample", po::value<uint32_t>()->default_value(1),
         "downsampling factor along both the row and the column. It will be "
         "overwirtten if 'downsample-col' or 'downsample-row' is given")
        ("downsample-col", po::value<uint32_t>(),
         "downsampling factor along the column")
        ("downsample-row", po::value<uint32_t>(),
         "downsampling factor along the row")
        ("angles", po::value<size_t>()->default_value(0),
         "number of projections per scan")
        ("angle-range", po::value<int>()->default_value(180),
         "range of the scan angle (can only be 180 or 360)")
        ("minx", po::value<float>(),
         "minimal X-coordinate of the reconstructed volume")
        ("maxx", po::value<float>(),
         "maximal X-coordinate of the reconstructed volume")
        ("miny", po::value<float>(),
         "minimal Y-coordinate of the reconstructed volume")
        ("maxy", po::value<float>(),
         "maximal Y-coordinate of the reconstructed volume")
        ("minz", po::value<float>(),
         "minimal Z-coordinate of the reconstructed volume")
        ("maxz", po::value<float>(),
         "maximal Z-coordinate of the reconstructed volume")
    ;

    bool retrieve_phase = false;
    bool disable_minus_log = false;
    po::options_description preprocessing_desc("Preprocessing options");
    preprocessing_desc.add_options()
        ("retrieve-phase", po::bool_switch(&retrieve_phase),
         "switch to Paganin filter")
        ("ramp-filter", po::value<std::string>()->default_value("shepp"),
         "supported filters are: shepp (Shepp-Logan), ramlak (Ram-Lak)")
        ("disable-negative-log", po::bool_switch(&disable_minus_log),
         "Minus logarithm will not be applied to sinogram, "
         "e.g. when the raw data were generated from a phantom")
    ;

    po::options_description reconstruction_desc("Reconstruction options");
    reconstruction_desc.add_options()
        ("slice-size", po::value<uint32_t>(),
         "size of the square reconstructed slice in pixels. Default to detector columns.")
        ("volume-size", po::value<uint32_t>(),
         "size of the cubic reconstructed volume. Default to 128.")
        ("raw-buffer-size", po::value<size_t>()->default_value(2),
         "maximum number of projection groups to be cached in the memory buffer")
    ;

    po::options_description paganin_desc("Paganin options");
    paganin_desc.add_options()
        ("pixel-size", po::value<float>()->default_value(1.0f),
         "...")
        ("lambda", po::value<float>()->default_value(1.23984193e-9f),
         "...")
        ("delta", po::value<float>()->default_value(1.e-8f),
         "...")
        ("beta", po::value<float>()->default_value(1.e-10f),
         "...")
        ("distance", po::value<float>()->default_value(40.0f),
         "...")
    ;

    bool pipeline_wait_on_slowness = false;
    po::options_description pipeline_desc("Pipeline options");
    pipeline_desc.add_options()
        ("imageproc-threads", po::value<uint32_t>(),
         "number of threads used for image processing")
        ("daq-concurrency", po::value<uint32_t>(),
         "Concurrency for data acquisition")
        ("wait-on-slowness", po::bool_switch(&pipeline_wait_on_slowness),
         "false for dropping the unprocessed data when there is a mismatch on performance in"
         "different parts of the pipeline")
    ;

    po::options_description all_desc(
        "Allowed options for TOMCAT live 3D reconstruction server");
    all_desc.
        add(general_desc).
        add(communication_desc).
        add(geometry_desc).
        add(preprocessing_desc).
        add(reconstruction_desc).
        add(paganin_desc).
        add(pipeline_desc);

    po::variables_map opts;
    po::store(po::parse_command_line(argc, argv, all_desc), opts);
    po::notify(opts);

    if (opts.count("help")) {
        std::cout << all_desc << "\n";
        return 0;
    }

    auto daq_protocol = opts["daq-protocol"].as<std::string>();
    auto daq_address = opts["daq-address"].as<std::string>();
    auto daq_socket_type = opts["daq-socket"].as<std::string>();
    auto daq_data_protocol = opts["daq-data-protocol"].as<std::string>();
    auto rpc_port = opts["rpc-port"].as<int>();

    auto [downsampling_row, downsampling_col] = parseDownsampleFactor(
        opts["downsample-row"], opts["downsample-col"], opts["downsample"]);
    auto num_rows = opts["rows"].as<size_t>();
    auto num_cols = opts["cols"].as<size_t>();
    auto num_angles = opts["angles"].as<size_t>();
    auto angle_range = parseAngleRange(opts["angle-range"]);
    auto minx = opts["minx"].empty() ? std::nullopt : std::optional<float>{opts["minx"].as<float>()}; 
    auto maxx = opts["maxx"].empty() ? std::nullopt : std::optional<float>{opts["maxx"].as<float>()}; 
    auto miny = opts["miny"].empty() ? std::nullopt : std::optional<float>{opts["miny"].as<float>()}; 
    auto maxy = opts["maxy"].empty() ? std::nullopt : std::optional<float>{opts["maxy"].as<float>()}; 
    auto minz = opts["minz"].empty() ? std::nullopt : std::optional<float>{opts["minz"].as<float>()}; 
    auto maxz = opts["maxz"].empty() ? std::nullopt : std::optional<float>{opts["maxz"].as<float>()}; 

    auto slice_size = opts["slice-size"].empty()
        ? std::nullopt : std::optional<uint32_t>(opts["slice-size"].as<uint32_t>());
    auto volume_size = opts["volume-size"].empty()
        ? std::nullopt : std::optional<uint32_t>(opts["volume-size"].as<uint32_t>());
    auto raw_buffer_size = opts["raw-buffer-size"].as<size_t>();

    auto ramp_filter = opts["ramp-filter"].as<std::string>();

    auto pixel_size = opts["pixel-size"].as<float>();
    auto lambda = opts["lambda"].as<float>();
    auto delta = opts["delta"].as<float>();
    auto beta = opts["beta"].as<float>();
    auto distance = opts["distance"].as<float>();

    auto imageproc_threads = opts["imageproc-threads"].empty()
         ? recastx::recon::Application::defaultImageprocConcurrency() 
         : opts["imageproc-threads"].as<uint32_t>();
    auto daq_concurrency = opts["daq-concurrency"].empty()
         ? recastx::recon::Application::defaultDaqConcurrency()
         : opts["daq-concurrency"].as<uint32_t>();

    auto daq_client = recastx::recon::createDaqClient(
        daq_data_protocol,
        fmt::format("{}://{}", daq_protocol, daq_address),
        daq_socket_type,
        daq_concurrency);
    recastx::recon::RampFilterFactory ramp_filter_factory;
    recastx::recon::AstraReconstructorFactory recon_factory;
    recastx::RpcServerConfig rpc_server_cfg {rpc_port};
    recastx::ImageprocParams imageproc_params {
        imageproc_threads, downsampling_col, downsampling_row, 0, !disable_minus_log, { ramp_filter }
    };
    recastx::recon::Application app(raw_buffer_size, imageproc_params,
                                    daq_client.get(), &ramp_filter_factory, &recon_factory, rpc_server_cfg);

    if (retrieve_phase) app.setPaganinParams(pixel_size, lambda, delta, beta, distance);
    app.setProjectionGeometry(recastx::BeamShape::PARALELL,
                              num_cols, num_rows, 1.0f, 1.0f, 1.0f, 1.0f,
                              num_angles, angle_range);
    app.setReconGeometry(slice_size, volume_size, minx, maxx, miny, maxy, minz, maxz);

    app.setPipelinePolicy(pipeline_wait_on_slowness);

    if (auto_processing) {
        app.setScanMode(recastx::rpc::ScanMode_Mode_DYNAMIC);
        app.startProcessing();
    } else if (auto_acquiring) {
        app.startAcquiring();
    }

    app.spin();

    return 0;
}
