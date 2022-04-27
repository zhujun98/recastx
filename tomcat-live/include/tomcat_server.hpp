#include <Eigen/Eigen>
#include <eigen3/unsupported/Eigen/FFT>

#include <cmath>
#include <memory>
#include <string>

#include <bulk/backends/thread/thread.hpp>
#include <bulk/bulk.hpp>

#include "nlohmann/json.hpp"
#include <zmq.hpp>
using json = nlohmann::json;

#include "processing.hpp"

#include "tomop/tomop.hpp"

#define ASTRA_CUDA
#include "astra/CudaBackProjectionAlgorithm3D.h"
#include "astra/CudaProjector3D.h"
#include "astra/Float32Data3DGPU.h"
#include "astra/Float32ProjectionData3DGPU.h"
#include "astra/Float32VolumeData3DGPU.h"
#include "astra/ParallelProjectionGeometry3D.h"
#include "astra/ParallelVecProjectionGeometry3D.h"
#include "astra/VolumeGeometry3D.h"

namespace tomcat {

template <typename T>
void minmaxoutput(std::string name, const std::vector<T>& xs) {
    return;
    std::cout << name << " min: " << *std::min_element(xs.begin(), xs.end())
              << ", ";
    std::cout << name << " max: " << *std::max_element(xs.begin(), xs.end())
              << "\n";
}

auto proj_to_vec(astra::CParallelProjectionGeometry3D* parallel_geom) {
    auto projs = parallel_geom->getProjectionCount();
    auto angles = parallel_geom->getProjectionAngles();
    auto rows = parallel_geom->getDetectorRowCount();
    auto cols = parallel_geom->getDetectorColCount();
    auto dx = parallel_geom->getDetectorSpacingX();
    auto dy = parallel_geom->getDetectorSpacingY();

    auto vectors = std::vector<astra::SPar3DProjection>(projs);
    auto i = 0;
    for (auto& v : vectors) {
        auto vrd =
            Eigen::Vector3f{std::sin(angles[i]), -std::cos(angles[i]), 0};
        auto vc = Eigen::Vector3f{0, 0, 0};
        auto vdx = Eigen::Vector3f{std::cos(angles[i]) * dx,
                                   std::sin(angles[i]) * dx, 0};
        auto vdy = Eigen::Vector3f{0, 0, dy};
        vc -= 0.5f * ((float)rows * vdy + (float)cols * vdx);
        v = {// ray dir
             vrd[0], vrd[1], vrd[2],
             // center
             vc[0], vc[1], vc[2],
             // pixel x dir
             vdx[0], vdx[1], vdx[2],
             // pixel y dir
             vdy[0], vdy[1], vdy[2]};
        ++i;
    }

    return std::make_unique<astra::CParallelVecProjectionGeometry3D>(
        projs, rows, cols, vectors.data());
}

std::tuple<Eigen::Vector3f, Eigen::Matrix3f, Eigen::Vector3f>
slice_transform(Eigen::Vector3f base, Eigen::Vector3f axis_1,
                Eigen::Vector3f axis_2, float k) {
    auto rotation_onto = [](Eigen::Vector3f x,
                            Eigen::Vector3f y) -> Eigen::Matrix3f {
        auto z = x.normalized();
        auto w = y.normalized();
        auto axis = z.cross(w);
        if (axis.norm() < 0.0001f) {
            return Eigen::Matrix3f::Identity();
        }
        auto alpha = z.dot(w);
        auto angle = std::acos(alpha);
        return Eigen::AngleAxis<float>(angle, axis.normalized()).matrix();
    };

    base = base * k;
    axis_1 = axis_1 * k;
    axis_2 = axis_2 * k;
    auto delta = base + 0.5f * (axis_1 + axis_2);

    auto rot = rotation_onto(axis_1, {2.0f * k, 0.0f, 0.0f});
    rot = rotation_onto(rot * axis_2, {0.0f, 2.0f * k, 0.0f}) * rot;

    return {-delta, rot, {1.0f, 1.0f, 1.0f}};
}

class server {
  public:
    enum class mode { continuous, single };

    using callback_type = std::function<void(const std::vector<float>&, int)>;

    server(std::string hostname, int rows, int cols, int proj_count, int darks,
           int flats, int group_size, int filter_processors,
           mode server_mode = mode::continuous, bool pull = false,
           bool hdfpub = false, std::vector<std::string> reqs = {},
           std::vector<std::string> subs = {})
        : context_(1), recast_sockets_(),
          socket_(context_, pull ? ZMQ_PULL : ZMQ_SUB), rows_(rows),
          cols_(cols), proj_count_(proj_count), darks_(darks), flats_(flats),
          dark_(rows_ * cols_), reciproc_(rows_ * cols_), buffer_(rows_ * cols),
          mode_(server_mode), hdfpub_(hdfpub), group_size_(group_size),
          processors_(filter_processors) {
        socket_.connect(hostname);
        if (!pull) {
            socket_.setsockopt(ZMQ_SUBSCRIBE, nullptr, 0);
        }

        initialize_astra_();
        initialize_recon_server_(reqs, subs);
    }

    void serve_single() {
        auto env = bulk::thread::environment();
        int output_cycle = 1000;

        std::vector<float> sino_buffer(rows_ * cols_ * group_size_);
        auto single_callback = [&](auto& projection_group, auto proj_id_min,
                                   auto proj_id_max, int proj_target) {
            std::cout << "Processing: " << proj_id_min << " up to "
                      << proj_id_max << "\n";
            env.spawn(processors_, [&](auto& world) {
                auto fft = Eigen::FFT<float>();
                auto freq_buffer = std::vector<std::complex<float>>(cols_, 0);

                auto dt = bulk::util::timer();
                for (int i = world.rank(); i < (proj_id_max - proj_id_min + 1);
                     i += world.active_processors()) {
                    process_projection_seq(
                        rows_, cols_,
                        &projection_group.data()[rows_ * cols_ * i],
                        dark_.data(), reciproc_.data(), fft, freq_buffer, filter_);
                }
                world.barrier();
                if (world.rank() == 0) {
                    auto t1 = dt.get();
                    transpose_sino_(projection_group, sino_buffer,
                                    proj_id_max - proj_id_min + 1);

                    auto t2 = dt.get();
                    std::cout << "upload to proj_target: " << proj_target
                              << "\n";
                    astra::uploadMultipleProjections(
                        proj_datas_[proj_target].get(), 
                        sino_buffer.data(),
                        proj_id_min, 
                        proj_id_max);

                    auto t3 = dt.get();
                    std::cout << "process: \t" << t1 << " ms\n";
                    std::cout << "sino: \t\t" << t2 - t1 << " ms\n";
                    std::cout << "upload: \t" << t3 - t2 << " ms\n";
                }
            });
        };
        bool kill = false;
        int frame = 0;
        std::vector<float> read_times(output_cycle, 0.0f);
        std::vector<float> recv_times(output_cycle, 0.0f);

        int idx = 0;
        int oidx = 0;
        int tidx_prev = -1;
        int grp_prev = -1;
        int buffer_idx = 0;

        assert(proj_count_ % group_size_ == 0);

        std::vector<std::vector<float>> projection_group(
            2, std::vector<float>(rows_ * cols_ * group_size_, 0.0f));
        while (true) {
            auto rt = bulk::util::timer();
            zmq::message_t update;
            if (!socket_.recv(&update)) {
                kill = true;
            } else {
                auto j = json::parse(
                    std::string((char*)update.data(), update.size()));
                frame = j["frame"];
                int scan_idx = 0;
                if (!hdfpub_) {
                    scan_idx = j["image_attributes"]["scan_index"];
                } else {
                    if (frame < darks_) {
                        scan_idx = 0;
                    } else if (frame < darks_ + flats_) {
                        scan_idx = 1;
                    } else {
                        scan_idx = 2;
                    }
                }

                if (frame % 50 == 0) {
                    std::cout << "Scan: " << scan_idx << "\n";
                    std::cout << "Frame: " << frame << "\n";
                    std::cout << "---\n";
                }

                // got projection data
                // depending on the frame, we light, dark, proj
                socket_.recv(&update);
                assert(update.size() == rows_ * cols_ * 2);
                recv_times[oidx] = rt.get();

                if (scan_idx == 0) {
                    read_(buffer_.data(), (uint16_t*)update.data());
                    all_darks_.push_back(buffer_);
                } else if (scan_idx == 1) {
                    read_(buffer_.data(), (uint16_t*)update.data());
                    all_flats_.push_back(buffer_);
                } else {
                    if (!has_darks_) {
                        dark_ = average_(all_darks_);
                        has_darks_ = true;
                    }
                    if (!has_flats_) {
                        auto flat = average_(all_flats_);
                        compute_recip_(dark_, flat);
                        has_flats_ = true;
                    }

                    int proj_idx = (frame - darks_ - flats_) % proj_count_;
                    int tidx = (frame - darks_ - flats_) / proj_count_;
                    if (!hdfpub_) {
                        proj_idx = frame % proj_count_;
                        tidx = frame / proj_count_;
                    }
                    int grp_idx = proj_idx % group_size_;
                    int grp_num = proj_idx / group_size_;

                    std::cout << idx << " " << frame << "\n";
                    if (grp_num != grp_prev && grp_prev >= 0) {
                        // if (callback_thread_.joinable()) {
                        //     callback_thread_.join();
                        // }
                        single_callback(projection_group[buffer_idx],
                                        grp_prev * group_size_, (grp_prev + 1) * group_size_ - 1,
                                        current_target_);
                        std::cout << idx << " " << proj_idx << "\n";
                        int buffer_idx = 1 - buffer_idx;

                        idx = 0;
                    }
                    grp_prev = grp_num;

                    if (tidx != tidx_prev && tidx_prev >= 0) {
                        // we finished a projection set (yay)
                        // make sure that data is uploaded to GPU
                        // if (callback_thread_.joinable()) {
                        //     callback_thread_.join();
                        // }
 
                        // we switch current targets.. all we have to do?
                        std::cout << "Projection buffer: " << current_target_ << " finished.\n";
                        current_target_ = 1 - current_target_;
                        // send recon packet? yeah lets
                        // FIXME ZMQ sockets are not thread safe, so is this?!
                        proj_datas_[1 - current_target_]->changeGeometry(
                            proj_geom_small_.get());
                        algs_small_[1 - current_target_]->run();

                        zmq::message_t reply;
                        auto small_recon_time = bulk::util::timer();
                        int n = this->small_size_;
                        float factor = (n / (float)cols_);
                        auto result = std::vector<float>(n * n * n, 0.0f);
                        auto pos = astraCUDA3d::SSubDimensions3D{n, n, n, n, n,
                                                                 n, n, 0, 0, 0};
                        astraCUDA3d::copyFromGPUMemory(result.data(),
                                                       vol_handle_small_, pos);

                        for (auto& x : result) {
                            x *= (factor * factor * factor);
                        }


                        std::cout << "small recon: \t" << small_recon_time.get() << " ms\n";
                        int s = 0;
                        for (auto& recast_socket_ : recast_sockets_) {
                            auto volprev = tomop::VolumeDataPacket(
                                recon_server_->scene_ids()[s], {n, n, n}, result);
                            volprev.send(recast_socket_);
                            recast_socket_.recv(&reply);
                            auto grsp = tomop::GroupRequestSlicesPacket(
                                recon_server_->scene_ids()[s], 1);
                            grsp.send(recast_socket_);
                            recast_socket_.recv(&reply);
                            ++s;
                        }
                        idx = 0;
                    }
                    tidx_prev = tidx;


                    read_(&projection_group[buffer_idx][rows_ * cols_ * grp_idx],
                          (uint16_t*)update.data());
                    idx++;
                }
            }
            read_times[oidx] = rt.get() - recv_times[oidx];
            oidx++;
            if (oidx == output_cycle) {
                std::cout << "recv: \t\t" << average_list_(recv_times)
                          << " ms\n";
                std::cout << "read: \t\t" << average_list_(read_times)
                          << " ms\n";
                oidx = 0;
            }

            if (kill) {
                std::cout << "Scene closed...\n";
                break;
            }
        }
    }

    void serve_continuous() {
        if (!callback_) {
            callback_ = [&](const auto& proj, auto proj_id) {
                assert(proj_id >= 0 && proj_id <= proj_count_);
                minmaxoutput("projection " + std::to_string(proj_id), proj);
                astra::uploadMultipleProjections(proj_data_.get(), 
                                                 proj.data(),
                                                 proj_id, 
                                                 proj_id);
            };
        }
        bool kill = false;
        int frame = 0;
        auto env = bulk::thread::environment();

        int output_cycle = 100;

        // benchmark data
        std::vector<float> total_times_(output_cycle, 0.0f);
        std::vector<float> read_times_(output_cycle, 0.0f);
        std::vector<float> process_times_(output_cycle, 0.0f);
        std::vector<float> cb_times_(output_cycle, 0.0f);
        int idx = 0;

        env.spawn(30, [&](auto& world) {
            auto s = world.rank();
            while (true) {
                auto total_time = bulk::util::timer();
                if (s == 0) {
                    auto rt = bulk::util::timer();
                    zmq::message_t update;
                    if (!socket_.recv(&update)) {
                        kill = true;
                    } else {
                        auto j = json::parse(
                            std::string((char*)update.data(), update.size()));
                        // std::cout << j.dump() << "\n";
                        frame = j["frame"];

                        int scan_idx = 0;
                        if (!hdfpub_) {
                            scan_idx = j["image_attributes"]["scan_index"];
                        } else {
                            if (frame < darks_) {
                                scan_idx = 0;
                            } else if (frame < darks_ + flats_) {
                                scan_idx = 1;
                            } else {
                                scan_idx = 2;
                            }
                        }

                        std::cout << "Scan: " << scan_idx << "\n";
                        std::cout << "Frame: " << frame << "\n";
                        std::cout << "---\n";

                        // got projection data
                        // depending on the frame, we light, dark, proj
                        socket_.recv(&update);
                        assert(update.size() == rows_ * cols_ * 2);
                        continue;

                        if (scan_idx == 0) {
                            read_(buffer_.data(), (uint16_t*)update.data());
                            all_darks_.push_back(buffer_);
                        } else if (scan_idx == 1) {
                            read_(buffer_.data(), (uint16_t*)update.data());
                            all_flats_.push_back(buffer_);
                        } else {
                            if (!has_darks_) {
                                dark_ = average_(all_darks_);
                                has_darks_ = true;
                                std::cout << "Received darks..\n";
                            }
                            if (!has_flats_) {
                                auto flat = average_(all_flats_);
                                compute_recip_(dark_, flat);
                                has_flats_ = true;
                                std::cout << "Received flats..\n";
                            }

                            auto pt = bulk::util::timer();
                            world.barrier();
                            process_projection(world, rows_, cols_,
                                               buffer_.data(), dark_.data(),
                                               reciproc_.data(), filter_);
                            // minmaxoutput(std::to_string(frame) + " buffer",
                            // buffer_);
                            if (s == 0) {
                                process_times_[idx % output_cycle] = pt.get();
                                auto cbt = bulk::util::timer();
                                callback_(buffer_, frame % proj_count_);
                                // minmaxoutput(std::to_string(frame) + "
                                // buffer",
                                // buffer_);
                                if ((frame + 1) % output_cycle == 0) {
                                    std::cout << "Handled " << frame + 1
                                              << "\n";
                                }
                                if ((frame % proj_count_) == proj_count_ - 1) {
                                    std::cout << "Requesting slices... "
                                              << frame << "\n";
                                    int s = 0;
                                    for (auto& recast_socket_ :
                                         recast_sockets_) {
                                        auto grsp =
                                            tomop::GroupRequestSlicesPacket(
                                                recon_server_->scene_ids()[s],
                                                1);
                                        recon_server_->send(grsp, s);
                                        ++s;
                                    }
                                }
                                cb_times_[idx % output_cycle] = cbt.get();
                            }
                        }
                    }

                    if (kill) {
                        if (s == 0) {
                            std::cout << "Scene closed...\n";
                        }
                        break;
                    }

                    if (s == 0) {
                        total_times_[idx % output_cycle] = total_time.get();
                        ++idx;

                        if (idx > 0 && idx % output_cycle == 0) {
                            // std::cout << "-----\n";
                            // std::cout << "read: " <<
                            // average_list_(read_times_)
                            //          << " ms\n";
                            // std::cout
                            //    << "process: " <<
                            //    average_list_(process_times_)
                            //    << " ms\n";
                            // std::cout << "callback: " <<
                            // average_list_(cb_times_)
                            //          << " ms\n";
                            // std::cout << "total: " <<
                            // average_list_(total_times_)
                            //          << " ms\n";
                        }
                    }
                }
            }
        });
    }

    void serve() {
        switch (mode_) {
        case mode::single:
            serve_single();
            break;
        case mode::continuous:
            serve_continuous();
            break;
        }
    }

    void set_callback(callback_type callback) { callback_ = callback; }

  private:
    void transpose_sino_(std::vector<float>& projection_group,
                         std::vector<float>& sino_buffer, int group_size) {
        auto di = cols_ * group_size;
        auto dig = rows_ * cols_;
        for (int i = 0; i < rows_; ++i) {
            for (int j = 0; j < group_size; ++j) {
                for (int k = 0; k < cols_; ++k) {
                    sino_buffer[i * di + j * cols_ + k] =
                        projection_group[j * dig + i * cols_ + k];
                }
            }
        }
    }

    void read_(float* dst, uint16_t* source) {
        for (int i = 0; i < rows_ * cols_; ++i) {
            dst[i] = (float)source[i];
        }
    }

    void compute_recip_(const std::vector<float>& dark_,
                        const std::vector<float>& flat) {
        for (int i = 0; i < rows_ * cols_; ++i) {
            if (dark_[i] == flat[i]) {
                reciproc_[i] = 1.0f;
            } else {
                reciproc_[i] = 1.0f / (flat[i] - dark_[i]);
            }
        }
    }

    std::vector<float> average_(std::vector<std::vector<float>> all) {
        auto result = std::vector<float>(rows_ * cols_);
        for (int i = 0; i < rows_ * cols_; ++i) {
            float total = 0.0f;
            for (auto j = 0u; j < all.size(); ++j) {
                total += all[j][i];
            }
            result[i] = total / all.size();
        }
        return result;
    }

    float average_list_(std::vector<float> xs) {
        float total = 0.0f;
        for (auto i = 0u; i < xs.size(); ++i) {
            total += xs[i];
        }
        return total / xs.size();
    }

    void initialize_recon_server_(std::vector<std::string> reqs, std::vector<std::string> subs) {
        recon_server_ = std::make_unique<tomop::multiserver>("TOMCAT recon", reqs, subs);
        // TODO multiple recast sockets
        for (int i = 0; i < reqs.size(); ++i) {
            recast_sockets_.emplace_back(context_, ZMQ_REQ);
            recast_sockets_[i].connect(reqs[i]);
        }

        auto k = vol_geom_->getWindowMaxX();

        recon_server_->set_slice_callback([&, k](std::array<float, 9> x,
                                                 int32_t)
                                              -> std::pair<
                                                  std::array<int32_t, 2>,
                                                  std::vector<float>> {
            auto[delta, rot, scale] = slice_transform(
                {x[6], x[7], x[8]}, {x[0], x[1], x[2]}, {x[3], x[4], x[5]}, k);

            // From the ASTRA geometry, get the vectors, modify
            // them,
            // and reset them
            int i = 0;
            for (auto[rx, ry, rz, dx, dy, dz, pxx, pxy, pxz, pyx, pyy, pyz] :
                 vectors_) {
                auto r = Eigen::Vector3f{rx, ry, rz};
                auto d = Eigen::Vector3f{dx, dy, dz};
                auto px = Eigen::Vector3f{pxx, pxy, pxz};
                auto py = Eigen::Vector3f{pyx, pyy, pyz};

                d += 0.5f * (cols_ * px + rows_ * py);
                r = scale.cwiseProduct(rot * r);
                d = scale.cwiseProduct(rot * (d + delta));
                px = scale.cwiseProduct(rot * px);
                py = scale.cwiseProduct(rot * py);
                d -= 0.5f * (cols_ * px + rows_ * py);

                vec_buf_[i] = {r[0],  r[1],  r[2],  d[0],  d[1],  d[2],
                               px[0], px[1], px[2], py[0], py[1], py[2]};
                ++i;
            }

            // 6) in callback, change geometry and run algorithm
            proj_geom_ =
                std::make_unique<astra::CParallelVecProjectionGeometry3D>(
                    proj_count_, rows_, cols_, vec_buf_.data());

            if (mode_ == mode::continuous) {
                proj_data_->changeGeometry(proj_geom_.get());
                alg_->run();
            } else {
                std::cout << "Reconstructing from: " << 1 - current_target_
                          << ".\n";
                proj_datas_[1 - current_target_]->changeGeometry(
                    proj_geom_.get());
                algs_[1 - current_target_]->run();
            }

            int n = this->slice_size_;
            auto result = std::vector<float>(n * n, 0.0f);
            auto pos =
                astraCUDA3d::SSubDimensions3D{n, n, 1, n, n, n, 1, 0, 0, 0};
            astraCUDA3d::copyFromGPUMemory(result.data(), vol_handle_, pos);

            minmaxoutput("slice", result);

            return {{this->slice_size_, this->slice_size_}, std::move(result)};
        });
        recon_server_->listen();
    }

    void initialize_astra_() {
        std::cout << "Initializing ASTRA...\n";
        // 1) create projection geometry
        std::vector<float> angles_(proj_count_);
        for (int i = 0; i < proj_count_; ++i) {
            angles_[i] = (i * M_PI) / proj_count_;
        }
        auto proj_geom = astra::CParallelProjectionGeometry3D(
            proj_count_, rows_, cols_, 1.0f, 1.0f, angles_.data());
        proj_geom_ = proj_to_vec(&proj_geom);
        proj_geom_small_ = proj_to_vec(&proj_geom);
        vectors_ = std::vector<astra::SPar3DProjection>(
            proj_geom_->getProjectionVectors(),
            proj_geom_->getProjectionVectors() + proj_count_);
        vec_buf_ = vectors_;

        // 1a) convert to par vec

        auto zeros = std::vector<float>(proj_count_ * cols_ * rows_, 0.0f);
        // 2) create projection data..

        if (mode_ == mode::continuous) {
            proj_handle_ = astraCUDA3d::createProjectionArrayHandle(
                zeros.data(), cols_, proj_count_, rows_);
            proj_data_ = std::make_unique<astra::CFloat32ProjectionData3DGPU>(
                proj_geom_.get(), proj_handle_);
        } else {
            for (int i = 0; i < 2; ++i) {
                proj_handles_.push_back(
                    astraCUDA3d::createProjectionArrayHandle(
                        zeros.data(), cols_, proj_count_, rows_));
                proj_datas_.push_back(
                    std::make_unique<astra::CFloat32ProjectionData3DGPU>(
                        proj_geom_.get(), proj_handles_[0]));
            }
        }

        // 3) create volume geometry
        vol_geom_ = std::make_unique<astra::CVolumeGeometry3D>(slice_size_,
                                                               slice_size_, 1);
        // 4) create volume data
        vol_handle_ = astraCUDA3d::allocateGPUMemory(slice_size_, slice_size_,
                                                     1, astraCUDA3d::INIT_ZERO);
        vol_data_ = std::make_unique<astra::CFloat32VolumeData3DGPU>(
            vol_geom_.get(), vol_handle_);

        // SMALL PREVIEW VOL
        vol_geom_small_ = std::make_unique<astra::CVolumeGeometry3D>(
            small_size_, small_size_, small_size_, vol_geom_->getWindowMinX(),
            vol_geom_->getWindowMinX(), vol_geom_->getWindowMinX(),
            vol_geom_->getWindowMaxX(), vol_geom_->getWindowMaxX(),
            vol_geom_->getWindowMaxX());
        vol_handle_small_ = astraCUDA3d::allocateGPUMemory(
            small_size_, small_size_, small_size_, astraCUDA3d::INIT_ZERO);
        vol_data_small_ = std::make_unique<astra::CFloat32VolumeData3DGPU>(
            vol_geom_small_.get(), vol_handle_small_);

        // 5) create back projection algorithm, link to previously
        // made objects
        projector_ = std::make_unique<astra::CCudaProjector3D>();
        if (mode_ == mode::continuous) {
            alg_ = std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
                projector_.get(), proj_data_.get(), vol_data_.get());
        } else {
            // make algorithm for two targets if mode is single
            for (int i = 0; i < 2; ++i) {
                algs_.push_back(
                    std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
                        projector_.get(), proj_datas_[i].get(),
                        vol_data_.get()));
                algs_small_.push_back(
                    std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
                        projector_.get(), proj_datas_[i].get(),
                        vol_data_small_.get()));
            }
        }

        // create filter
        auto mid = (cols_ + 1) / 2;
        filter_.resize(cols_);
        for (int i = 0; i < mid; ++i) {
            filter_[i] = i;
        }
        for (int j = mid; j < cols_; ++j) {
            filter_[j] = 2 * mid - j;
        }

        auto filter_weight = [=](auto i) {
            return std::sin(M_PI * (i / (float)mid)) / (M_PI * (i / (float)mid));
        };

        for (int i = 1; i < mid; ++i) {
            filter_[i] *= filter_weight(i);
        }
        for (int j = mid; j < cols_; ++j) {
            filter_[j] = filter_weight(2 * mid - j);
        }
    }

    std::unique_ptr<tomop::multiserver> recon_server_;

    zmq::context_t context_;
    zmq::socket_t socket_;
    std::vector<zmq::socket_t> recast_sockets_;
    callback_type callback_;

    int rows_;
    int cols_;
    int proj_count_;
    int darks_;
    int flats_;
    int slice_size_ = 768;

    std::vector<std::vector<float>> all_flats_;
    std::vector<std::vector<float>> all_darks_;

    std::vector<float> dark_;
    std::vector<float> reciproc_;
    std::vector<float> buffer_;

    // ASTRA stuff...
    std::unique_ptr<astra::CParallelVecProjectionGeometry3D> proj_geom_;
    astraCUDA3d::MemHandle3D proj_handle_;
    std::unique_ptr<astra::CFloat32ProjectionData3DGPU> proj_data_;
    std::unique_ptr<astra::CVolumeGeometry3D> vol_geom_;
    astraCUDA3d::MemHandle3D vol_handle_;
    std::unique_ptr<astra::CFloat32VolumeData3DGPU> vol_data_;
    std::unique_ptr<astra::CCudaProjector3D> projector_;
    std::unique_ptr<astra::CCudaBackProjectionAlgorithm3D> alg_;
    std::vector<astra::SPar3DProjection> vectors_;
    std::vector<astra::SPar3DProjection> vec_buf_;

    // small stuff
    std::unique_ptr<astra::CVolumeGeometry3D> vol_geom_small_;
    astraCUDA3d::MemHandle3D vol_handle_small_;
    std::unique_ptr<astra::CFloat32VolumeData3DGPU> vol_data_small_;
    std::vector<std::unique_ptr<astra::CCudaBackProjectionAlgorithm3D>>
        algs_small_;
    std::unique_ptr<astra::CParallelVecProjectionGeometry3D> proj_geom_small_;

    // Single stuff
    std::vector<std::unique_ptr<astra::CFloat32ProjectionData3DGPU>>
        proj_datas_;
    std::vector<std::unique_ptr<astra::CCudaBackProjectionAlgorithm3D>> algs_;
    std::vector<astraCUDA3d::MemHandle3D> proj_handles_;

    int current_target_ = 0;
    int small_size_ = 128;

    const mode mode_;
    std::thread callback_thread_;

    bool has_darks_ = false;
    bool has_flats_ = false;
    bool hdfpub_ = false;
    int group_size_ = -1;
    int processors_ = -1;

    std::vector<float> filter_;
};

} // namespace tomcat
