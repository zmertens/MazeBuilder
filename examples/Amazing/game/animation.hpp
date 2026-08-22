#pragma once

#include <cstddef>

namespace amazing::game
{
    class animation
    {
    public:
        void configure(float seconds_per_frame, std::size_t frame_count) noexcept;
        void reset() noexcept;
        void tick(float dt_seconds) noexcept;

        [[nodiscard]] std::size_t frame_index() const noexcept;

    private:
        float seconds_per_frame_{ 0.12f };
        float accumulator_{ 0.0f };
        std::size_t frame_count_{ 2u };
        std::size_t frame_index_{ 0u };
    };
}
