#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <coroutine>
#include <cstdlib>
#include <exception>
#include <numeric>
#include <utility>
#include <vector>

#include <MazeBuilder/progress.h>
#include <MazeBuilder/randomizer.h>

#include <fmt/format.h>

namespace
{
    /// Minimal synchronous generator that yields ints.
    class generator final
    {
    public:
        struct promise_type
        {
            int current_value{};

            generator get_return_object() noexcept
            {
                return generator{std::coroutine_handle<promise_type>::from_promise(*this)};
            }
            // Lazy start and suspended at the end so the handle stays valid until destroyed by the owner
            std::suspend_always initial_suspend() noexcept { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            std::suspend_always yield_value(int value) noexcept
            {
                current_value = value;
                return {};
            }
            void return_void() noexcept {}
            void unhandled_exception() noexcept { std::terminate(); }
        };

        explicit generator(std::coroutine_handle<promise_type> h) noexcept : handle(h) {}
        generator(generator &&other) noexcept : handle(std::exchange(other.handle, {})) {}
        generator(const generator &) = delete;
        generator &operator=(const generator &) = delete;
        generator &operator=(generator &&) = delete;
        ~generator()
        {
            if (handle)
                handle.destroy();
        }

        /// Advances the coroutine; returns false once it has finished.
        bool next() noexcept
        {
            if (!handle || handle.done())
                return false;
            handle.resume();
            return !handle.done();
        }

        int value() const noexcept { return handle.promise().current_value; }

    private:
        std::coroutine_handle<promise_type> handle{};
    };

    /// Euclid's algorithm: yields each remainder step; the last yielded value is the gcd.
    generator gcd(int a, int b)
    {
        while (b != 0)
        {
            const int r = a % b;
            a = b;
            b = r;
            co_yield a;
        }
    }

    int run_gcd(int a, int b)
    {
        generator g = gcd(a, b);
        int last = std::abs(a);
        while (g.next())
            last = g.value();
        return last;
    }

    mazes::randomizer make_seeded(unsigned long long seed)
    {
        mazes::randomizer rng{};
        rng.seed(seed);
        return rng;
    }
} // namespace

TEST_CASE("gcd coroutine matches known values", "[randomizer][gcd]")
{
    REQUIRE(run_gcd(48, 18) == 6);
    REQUIRE(run_gcd(17, 5) == 1);
    REQUIRE(run_gcd(100, 10) == 10);
    REQUIRE(run_gcd(7, 0) == 7);
}

TEST_CASE("Random gcds agree with std::gcd", "[randomizer][gcd]")
{
    auto rng = make_seeded(3);

    for (int i = 0; i < 1'000; ++i)
    {
        const int a = rng.get_int(1, 100'000);
        const int b = rng.get_int(1, 100'000);
        const int g = run_gcd(a, b);

        REQUIRE(g == std::gcd(a, b));
        REQUIRE(a % g == 0);
        REQUIRE(b % g == 0);
    }
}

TEST_CASE("Random gcds are reproducible for a given seed", "[randomizer][gcd]")
{
    auto collect = [](unsigned long long seed)
    {
        auto rng = make_seeded(seed);
        std::vector<int> gcds;
        for (int i = 0; i < 100; ++i)
        {
            const int a = rng.get_int(1, 10'000);
            const int b = rng.get_int(1, 10'000);
            gcds.push_back(run_gcd(a, b));
        }
        return gcds;
    };

    REQUIRE(collect(42) == collect(42));
}

TEST_CASE("Randomizer ranges and vectors", "[randomizer]")
{
    auto rng = make_seeded(7);

    for (int i = 0; i < 500; ++i)
    {
        const int v = rng(-5, 5);
        REQUIRE(v >= -5);
        REQUIRE(v <= 5);

        const float f = rng.get_float(0.0f, 1.0f);
        REQUIRE(f >= 0.0f);
        REQUIRE(f <= 1.0f);
    }

    // Values are drawn with replacement, so only size and bounds are guaranteed
    const auto ints = rng.sample_int_n(0, 9, 50);
    REQUIRE(ints.size() == 50);
    REQUIRE(std::ranges::all_of(ints, [](int v)
                                { return v >= 0 && v <= 9; }));
    REQUIRE(rng.sample_int_n(5, 1, 3).empty());

    const auto floats = rng.sample_float_n(-1.0f, 1.0f, 50);
    REQUIRE(floats.size() == 50);
    REQUIRE(std::ranges::all_of(floats, [](float v)
                                { return v >= -1.0f && v <= 1.0f; }));
    REQUIRE(rng.sample_float_n(1.0f, 0.0f, 3).empty());
    REQUIRE(rng.sample_float_n(0.0f, 1.0f, 0).empty());
}

TEST_CASE("Sampling is reproducible and roughly uniform", "[randomizer][sample]")
{
    REQUIRE(make_seeded(5).sample_int_n(0, 99, 200) == make_seeded(5).sample_int_n(0, 99, 200));

    auto rng = make_seeded(9);
    const auto samples = rng.sample_int_n(0, 3, 40'000);
    std::array<int, 4> counts{};
    for (int v : samples)
        ++counts[static_cast<std::size_t>(v)];
    for (int c : counts)
    {
        REQUIRE(c > 9'000);
        REQUIRE(c < 11'000);
    }
}

TEST_CASE("Timing random gcds with progress", "[randomizer][gcd][progress]")
{
    auto rng = make_seeded(11);
    constexpr int ITERATIONS = 10'000;

    auto work = [&rng](int iterations)
    {
        long long sum = 0;
        for (int i = 0; i < iterations; ++i)
        {
            const int a = rng.get_int(1, 1'000'000);
            const int b = rng.get_int(1, 1'000'000);
            sum += run_gcd(a, b);
        }
        return sum > 0;
    };

    // duration() returns zero for a falsy result, so work must return true
    const auto elapsed = mazes::progress<>::duration(work, ITERATIONS);
    REQUIRE(elapsed.count() >= 0);
    fmt::print("{} random gcds took {} ms\n", ITERATIONS, elapsed.count());

    mazes::progress<> clock;
    clock.start();
    REQUIRE(work(100));
    REQUIRE(clock.elapsed<double>() >= 0.0);

    clock.reset();
    REQUIRE(clock.elapsed<double>() >= 0.0);
}
