# Contributing

Maze Builder is open source under the MIT License.

Issues are disabled on the GitHub repo. Please make a pull request summarizing the changes and improvements. For example, visit the the [PRs](https://github.com/zmertens/MazeBuilder/pulls) page. In the PR summary, add two sections, one for **Summary** and another for **Problems Solved** detailing the changes to the code.

## Getting Started

Adding a new maze algorithm as a script is a good place to start...

  1. **Add the Algorithm Script**: Add a script implementing the algorithm.
  2. **Write a Test**: Add a Catch2 test to the "maze builder lib" tests replicating the script's functionality
  3. **Ensure Consistency**: Ensure that the script's output and the test's output match the exepcted values.

## CPP Code

The codebase tries to follow modern C++ coding styles and conventions:

  * **Smart Pointers**: Include `<memory>` and use `std::unique_ptr<T>` or `std::shared_ptr<T>` over `malloc`, `free`, `new`, and `delete`. 
    * It is okay to do `MyClass* class_ptr = &MyClass::obj;`.
  * **Minimal Templates**: Rarely use templates to avoid complex debugging messages.
  * **Task-Based Programming**: Prefer task-based programming instead of threads. Use futures that return void, `future<void> foo = std::async(std::launch::async, _bar, _param1, _param2);`.

## Shaders

Verify new shaders work on both OpenGL ES 3.0 and OpenGL Core 3.0 settings.

## TODOs

Use `@TODO` in code documentation sparingly. Instead, prefer implementing a fix or test instead.

## Need Help?

If you have questions or need help, feel free to reach out by opening a discussion on the [Discussions](https://github.com/zmertens/MazeBuilder/discussions).
