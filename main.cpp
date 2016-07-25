#include "stdafx.h"
#include "engine.h"
#include <sstream>

int main(int argc, char* argv[]){
	int aalevel;
	// No arguments -> no AA
	if (argc == 1) {
		aalevel = 0;
	}
	// Argument -> AA
	else if (argc > 1) {
		std::istringstream stream(argv[1]);
		if (!(stream >> aalevel)) {
			aalevel = 0;
		}
	}
	printf("Running at AA level %d\n", aalevel);
	Engine engine = Engine(aalevel);
	engine.run();
	return 0;
}
