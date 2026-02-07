#include "request_generation.h"
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include "types.h"
namespace workload {
using namespace std;
using namespace rfunc;

static const size_t MAX_VALUE_LEN = 10240;

const char CharGenerator::__CHARSET[] ="     ,;:.!?0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

const size_t CharGenerator::__CHARSET_LEN = 73;




}
