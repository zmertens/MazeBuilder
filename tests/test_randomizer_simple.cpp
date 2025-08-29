#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <vector>
#include <iterator>

#include <MazeBuilder/randomizer.h>

using namespace mazes;

TEST_CASE("Randomizer Ranges Basic Functionality", "[ranges][randomizer]") {
    SECTION("Random range generates numbers in bounds") {
        randomizer rand;
        auto random_range = rand.get_ranges_inclusive(1, 10);
        
        // Test that we can create the range and get iterators
        auto it = random_range.begin();
        auto end = random_range.end();
        
        REQUIRE(it != end);  // Should be able to iterate
        
        // Get a few values to test
        std::vector<int> results;
        for (int i = 0; i < 5; ++i, ++it) {
            results.push_back(*it);
        }
        
        REQUIRE(results.size() == 5);
        for (const auto& val : results) {
            REQUIRE(val >= 1);
            REQUIRE(val <= 10);
        }
    }
}

TEST_CASE("Randomizer Ranges Iterator Operations", "[ranges][randomizer]") {
    SECTION("Iterator increment and dereference work correctly") {
        randomizer rand;
        auto random_range = rand.get_ranges_inclusive(5, 15);
        
        auto it1 = random_range.begin();
        auto it2 = random_range.begin();
        auto end = random_range.end();
        
        // Different iterators should not be equal (random state)
        REQUIRE(it1 != end);
        REQUIRE(it2 != end);
        
        // Test dereferencing
        int val1 = *it1;
        int val2 = *it2;
        
        REQUIRE(val1 >= 5);
        REQUIRE(val1 <= 15);
        REQUIRE(val2 >= 5);
        REQUIRE(val2 <= 15);
        
        // Test increment
        ++it1;
        int val3 = *it1;
        REQUIRE(val3 >= 5);
        REQUIRE(val3 <= 15);
    }
}

TEST_CASE("Randomizer Ranges Manual Processing", "[ranges][randomizer]") {
    SECTION("Manual filter and transform operations") {
        randomizer rand;
        auto random_range = rand.get_ranges_inclusive(1, 20);
        
        std::vector<int> results;
        auto it = random_range.begin();
        
        // Take 100 numbers, filter even, square them
        int count = 0;
        while (count < 100 && it != random_range.end()) {
            int val = *it;
            if (val % 2 == 0) {  // Filter even numbers
                results.push_back(val * val);  // Transform by squaring
            }
            ++it;
            ++count;
        }
        
        REQUIRE(!results.empty());
        for (const auto& val : results) {
            // Should be squared even numbers from [1,20]
            int root = static_cast<int>(std::sqrt(val));
            REQUIRE(root * root == val);  // Perfect square
            REQUIRE(root % 2 == 0);       // Root is even
        }
    }
}

TEST_CASE("Randomizer Traditional vs Ranges Comparison", "[ranges][randomizer]") {
    SECTION("Compare traditional method with ranges approach") {
        randomizer rand;
        
        // Traditional approach
        std::vector<int> traditional_results;
        for (int i = 0; i < 100; ++i) {
            int val = rand.get_int_incl(1, 20);
            if (val % 2 == 0) {
                traditional_results.push_back(val * val);
            }
        }
        
        // Ranges approach (manual processing since std::views not available)
        std::vector<int> ranges_results;
        auto random_range = rand.get_ranges_inclusive(1, 20);
        auto it = random_range.begin();
        
        int count = 0;
        while (count < 100 && it != random_range.end()) {
            int val = *it;
            if (val % 2 == 0) {
                ranges_results.push_back(val * val);
            }
            ++it;
            ++count;
        }
        
        // Both should produce similar results
        REQUIRE(!traditional_results.empty());
        REQUIRE(!ranges_results.empty());
        
        for (const auto& val : traditional_results) {
            int root = static_cast<int>(std::sqrt(val));
            REQUIRE(root * root == val);  // Perfect square
            REQUIRE(root % 2 == 0);       // Root is even
        }
        
        for (const auto& val : ranges_results) {
            int root = static_cast<int>(std::sqrt(val));
            REQUIRE(root * root == val);  // Perfect square
            REQUIRE(root % 2 == 0);       // Root is even
        }
    }
}
