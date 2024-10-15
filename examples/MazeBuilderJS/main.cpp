#include <string>
#include <memory>
#include <exception>
#include <iostream>
#include <vector>

#include "my_lib.hpp"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <emscripten/bind.h>

// Bind C++ functions so that it can be transpiled to JS
EMSCRIPTEN_BINDINGS(maze_builder_module) {
    emscripten::class_<Lib>("Lib")
        .smart_ptr<std::shared_ptr<Lib>>("std::shared_ptr<Lib>")
        .constructor<const std::string&, double>()
        .property("description", &Lib::get_description, &Lib::set_description)
        .property("version", &Lib::get_version, &Lib::set_version)
        .function("to_str", &Lib::to_str)
        .class_function("get_instance", &Lib::get_instance, emscripten::allow_raw_pointers());
}
#endif

void loop() {
    int counter = 0;
    while (true) {
        if (counter % 300000 == 0) {
            std::cout << "Counter: " << counter << std::endl;
        }
		counter++;
    }

#if defined(__EMSCRIPTEN__)
    emscripten_cancel_main_loop();
#endif

}

int main(int argc, char* argv[]) {

    using namespace std;

    vector<string> args_vec{ argv, argv + argc };;

    try {
        bool success = false;
		auto my_lib = Lib::get_instance("My Amazing C++ Lib", 1.0);
		
#if defined(__EMSCRIPTEN__)
		emscripten_set_main_loop(&loop, 0, 1);
#endif
        
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl; 
    }

    return EXIT_SUCCESS;
}
