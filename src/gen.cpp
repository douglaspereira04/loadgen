#include <stdlib.h>
#include "request/request_generator.h"

int
main(int argc, char const *argv[])
{
	if (argc < 2) {
		exit(1);
	}

	workload::RequestGenerator generator(argv[1]);
	generator.generate_to_file();

	return 0;
}
