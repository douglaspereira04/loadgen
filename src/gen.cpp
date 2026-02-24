#include <stdlib.h>
#include "request/request_generator.h"

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        exit(1);
    }
    int num_threads = 1;
    if (argc == 3) {
        num_threads = std::stoi(argv[2]);
    }
    workload::RequestGenerator generator(argv[1]);
    generator.generate_to_file(num_threads);

    return 0;
}
