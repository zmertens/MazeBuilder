/**
 * @brief Main entry for the Generator
 * @file Main.cpp
 *
 * Create a singleton instance of the Generator and run it
 *
 */

#include <iostream>
#include <exception>
#include <string>

#include "Generator.hpp"

int main(int argc, char* argv[]) {
	using namespace std;

    static constexpr auto MESSAGE = R"msg(
        --- WELCOME TO THE MAZE GENERATOR ---
        |   1. Press 'B' on keyboard to generate a 2D maze   |
        -------------------------------------------
    )msg";

    cout << MESSAGE << endl;

    try {
        string title = "generator";
        string version = "0.1.0";
        auto myGameInstance = Generator::get_instance(cref(title), cref(version), 1280, 720);
        bool res = myGameInstance->run();
        if (!res) {
            cerr << "Generator failed to run" << endl;
        }
    } catch (exception ex) {
        cerr << ex.what() << endl;
    }

	return 0;
}
