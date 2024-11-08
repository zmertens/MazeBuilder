/**
 * @brief Main entry for the game
 * @file Main.cpp
 *
 * Create a singleton instance of the Game and run it
 *
 */

#include <iostream>
#include <exception>
#include <string>

#include "Game.hpp"

int main(int argc, char* argv[]) {
	using namespace std;
    try {
        string title = "simultageous";
        string version = "0.1.0";
        auto myGameInstance = Game::get_instance(cref(title), cref(version), 1280, 720);
        bool res = myGameInstance->run("https://worker", "lastSave.png");
        if (!res) {
            cerr << "Game failed to run" << endl;
        }
    } catch (exception ex) {
        cerr << ex.what() << endl;
    }

	return 0;
}
