#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

void oom(uint64_t size)
{
	fprintf(stderr, "out of memory %ulld\n", size);
	exit(1);
}