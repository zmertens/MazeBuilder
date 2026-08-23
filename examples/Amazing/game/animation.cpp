#include "animation.hpp"

#include <algorithm>

namespace amazing::game
{
    void animation::configure(const float seconds_per_frame, const std::size_t frame_count) noexcept
    {
        seconds_per_frame_ = std::max(0.01f, seconds_per_frame);
        frame_count_ = std::max<std::size_t>(frame_count, 1u);
        reset();
    }

    void animation::reset() noexcept
    {
        accumulator_ = 0.0f;
        frame_index_ = 0u;
    }

    void animation::tick(const float dt_seconds) noexcept
    {
        accumulator_ += std::max(0.0f, dt_seconds);
        while (accumulator_ >= seconds_per_frame_)
        {
            accumulator_ -= seconds_per_frame_;
            frame_index_ = (frame_index_ + 1u) % frame_count_;
        }
    }

    std::size_t animation::frame_index() const noexcept
    {
        return frame_index_;
    }
}
