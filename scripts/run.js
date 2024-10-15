// Example script performing sanity-checks on C++ => JS transpilation

import Module from "./sparks.js";

let libptr = null;

const run = async () => {
	const activeModule = await Module();
	if (activeModule) {
		libptr = activeModule.Lib.get_instance("Calling from JS", 1.2);
		if (libptr) {
			console.log(`libptr: ${libptr}`);
			console.log(`libptr.to_str(): ${libptr.to_str()}`);
			libptr.description = "Updated from JS";
			libptr.version = 2.2;
			console.log(`libptr.description: ${libptr.description}\nlibptr.version: ${libptr.version}`);
			libptr.delete();
		} else {
			console.error("No lib ptr");
		}
	} else {
		console.error("Failed to create Lib instance");
	}
}

run();

