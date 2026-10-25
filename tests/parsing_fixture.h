#ifndef TEST_PARSING_FIXTURE_H
#define TEST_PARSING_FIXTURE_H

#include <string>

#include <MazeBuilder/args.h>
#include <MazeBuilder/parsing_state.h>
#include <MazeBuilder/processed_text.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/runtime_stack.h>

namespace test_support
{
    // Drives parsing_state directly: RAW in, PARSED out
    class parsing_fixture
    {
    public:
        parsing_fixture()
            : stack{ctx.with_args_manager(args_mapper).with_text_manager(text_mapper).with_rng(rng)}
        {
            args_mapper.load<mazes::args>(mazes::args_identifier::RAW);
            args_mapper.load<mazes::args>(mazes::args_identifier::PARSED);
            text_mapper.load<mazes::processed_text>(mazes::processed_text_identifier::FINISHED);
            text_mapper.load<mazes::processed_text>(mazes::processed_text_identifier::PROCESSING);
            text_mapper.load<mazes::processed_text>(mazes::processed_text_identifier::UNKNOWN);
        }

        // Returns the result of update(); PARSED is filled on success
        bool run(const std::string &raw)
        {
            args_mapper.get(mazes::args_identifier::RAW).set({mazes::args::arg_map{{mazes::args::RAW_WORD_STR, raw}}});
            mazes::parsing_state state{ctx, &stack};
            return state.update(rng, 0.0);
        }

        mazes::args &parsed() { return args_mapper.get(mazes::args_identifier::PARSED); }
        mazes::args &raw() { return args_mapper.get(mazes::args_identifier::RAW); }

        mazes::args_manager args_mapper;
        mazes::processed_text_manager text_mapper;
        mazes::randomizer rng;
        mazes::runtime_app::context ctx;
        mazes::runtime_stack stack;
    };
} // namespace test_support

#endif // TEST_PARSING_FIXTURE_H
