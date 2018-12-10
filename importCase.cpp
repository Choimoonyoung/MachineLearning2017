#include <stdio.h>
#include <string.h>
#pragma warning (disable:4996)


///////////////////////////////////////////
///////////Vector 읽어오는 부분////////////
///////////////////////////////////////////

typedef struct vectorsequence {
	int T ;
	float x[700][39];
}Vec;


Vec input;

void readTestcase(char* dir) {
	FILE *fp;
	strcat(dir, ".txt");
	fp = fopen(dir, "r");
	if (fp == NULL) {
		printf("There's no file!\n");
		return;
	}
	int dim;
	fscanf(fp, "%d %d\n", &input.T, &dim);
	for (int i = 0; i < input.T; i++) {
		for (int j = 0; j < dim; j++) {
			fscanf(fp, "%e ", &input.x[i][j]);
		}
	}
	
	//check
	/*for (int i = 0; i < input.T; i++) {
		for (int j = 0; j < dim; j++) {
			printf("%e ", input.x[i][j]);
		}
		printf("\n");
	}*/
	fclose(fp);
}
