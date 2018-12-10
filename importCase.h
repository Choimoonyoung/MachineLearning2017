#pragma once
#include <stdio.h>

typedef struct vectorsequence {
	int T;
	float x[500][39];
}Vec;

void readTestcase(char* dir);