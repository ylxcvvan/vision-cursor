#pragma once

#include <memory>
#include <vector>
#include <cstdint>

namespace VisionCursor
{

class Frame;
using FramePtr = std::shared_ptr<Frame>;

class Frame
{
public:
    Frame() = default;
    virtual ~Frame() = default;

    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
    Frame(Frame&&) = delete;
    Frame& operator=(Frame&&) = delete;

    virtual FramePtr clone() const = 0;
    virtual uint8_t* getData() const = 0;

    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;

    virtual void setWidth(int width) = 0;
    virtual void setHeight(int height) = 0;
};

} // namespace VisionCursor