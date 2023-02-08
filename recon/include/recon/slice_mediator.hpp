#include <map>
#include <unordered_set>


#include "buffer.hpp"


namespace tomcat::recon {

class Reconstructor;


class SliceMediator {

public:

    using DataType = typename SliceBuffer<float>::DataType;
    using ValueType = typename SliceBuffer<float>::ValueType;

protected:

    std::map<size_t, std::pair<size_t, Orientation>> params_;
    SliceBuffer<float> all_slices_;
    SliceBuffer<float> ondemand_slices_;
    std::unordered_set<size_t> updated_;

    std::mutex mtx_;

public:

    SliceMediator();

    ~SliceMediator();

    void resize(const std::array<size_t, 2>& shape);

    void insert(size_t timestamp, const Orientation& orientation);

    void reconAll(Reconstructor* recon, int gpu_buffer_index);

    void reconOnDemand(Reconstructor* recon, int gpu_buffer_index);

    SliceBuffer<float>& allSlices() { return all_slices_; }

    SliceBuffer<float>& onDemandSlices() { return ondemand_slices_; }

};

} // tomcat::recon