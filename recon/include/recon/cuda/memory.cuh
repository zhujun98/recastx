/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_CUDA_UPLOADER_H
#define RECON_CUDA_UPLOADER_H

#include "astra/Float32ProjectionData3DGPU.h"


namespace recastx::recon {

class AstraMemHandleBase {

protected:

    astraCUDA3d::MemHandle3D handle_;

public:

    AstraMemHandleBase();

    virtual ~AstraMemHandleBase();

    AstraMemHandleBase(AstraMemHandleBase&&) noexcept;
    AstraMemHandleBase& operator=(AstraMemHandleBase&&) noexcept;

    [[nodiscard]] astraCUDA3d::MemHandle3D& handle() { return handle_; }
    [[nodiscard]] const astraCUDA3d::MemHandle3D& handle() const { return handle_; }

    [[nodiscard]] unsigned int x() const;
    [[nodiscard]] unsigned int y() const;
    [[nodiscard]] unsigned int z() const;
    [[nodiscard]] unsigned int size() const;
};

class AstraMemHandle : public AstraMemHandleBase {

public:

    AstraMemHandle(unsigned int x, unsigned int y, unsigned int z);

    ~AstraMemHandle() override;

    AstraMemHandle(AstraMemHandle&&) noexcept;
    AstraMemHandle& operator=(AstraMemHandle&&) noexcept;
};

class AstraMemHandleArray : public AstraMemHandleBase {

public:

    AstraMemHandleArray(unsigned int x, unsigned int y, unsigned int z);

    ~AstraMemHandleArray() override;

    AstraMemHandleArray(AstraMemHandleArray&&) noexcept;
    AstraMemHandleArray& operator=(AstraMemHandleArray&&) noexcept;
};

} // recastx::recon

#endif // RECON_CUDA_UPLOADER_H