# Contributing

Maze Builder is open source under the MIT License.

Issues are disabled on the GitHub repo. Please make a pull request summarizing the changes and improvements.
For example, visit the the [PRs](https://github.com/zmertens/MazeBuilder/pulls) page.
In the PR summary, add two sections, one for **Summary** where the maze-generating algorithm is explained.
Adding a picture of the output is encouraged. The second section is **Changes** where the changes (can use a checklist) are listed.

## Getting Started

Adding a new maze algorithm as a script is a good place to start...

  1. **Add the Algorithm Script**: Add a script implementing the algorithm under `scripts`.
  2. **Write a Test**: Add a Catch2 test to the "maze builder lib" tests under `tests`.
  3. **Ensure Consistency**: Ensure that the script's output and the test's output match the exepcted values.

## CPP Code

The codebase tries to follow modern C++ coding styles and conventions:

  * **Smart Pointers**: Include `<memory>` and use `std::unique_ptr<T>` or `std::shared_ptr<T>` over `malloc`, `free`, `new`, and `delete`. 
    * It is okay to do `MyClass* class_ptr = &MyClass::obj;`.
  * **Inline Functions**: No use of `inline` on functions.

## Shaders

Verify new shaders work on both OpenGL ES 3.0 and OpenGL Core 3.0 settings.

## TODOs

Use `@TODO` in code documentation sparingly. Instead, prefer implementing a fix or test instead.

## Need Help?

If you have questions or need help, feel free to reach out by opening a discussion on the [Discussions](https://github.com/zmertens/MazeBuilder/discussions).
