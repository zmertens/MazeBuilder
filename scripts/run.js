// Example script performing sanity-checks on C++ => JS transpilation

import Module from "./maze_builder_js.js";

class Maze {
	constructor(rows, cols, depth, seed, algorithm, output) {
		this.rows = rows;
		this.cols = cols;
		this.depth = depth;
		this.seed = seed;
		this.algorithm = algorithm;
		this.output = output;
	}
}

let libptr = null;

const run = async () => {
	const activeModule = await Module();
	if (activeModule) {
		libptr = activeModule.maze.get_instance(10, 10, 1, 0, "binary_tree", "Maze");
		if (libptr) {
			const my_maze = new Maze(libptr.rows, libptr.cols, libptr.depth,
				libptr.seed, libptr.algorithm, libptr.output);
			console.log(my_maze);
			const s = JSON.stringify(my_maze);
			console.log(s);
			libptr.delete();
		} else {
			console.error("No lib ptr");
		}
	} else {
		console.error("Failed to create Lib instance");
	}
}

run();

