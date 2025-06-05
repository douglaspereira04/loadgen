#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sstream>
#include "request_generation.h"
#include "types.h"

int
main(int argc, char const *argv[])
{
	if (argc < 2) {
		exit(1);
	}

	const auto config = toml::parse(argv[1]);

	auto export_path = toml::find<std::string>(
		config, "output", "requests", "export_path"
	);
	workload::create_requests(argv[1]);
	
	
}
