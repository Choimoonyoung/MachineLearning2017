#include <stdio.h>
#include <string.h>
#pragma warning (disable:4996)

float unigram[13];
float bigram[13][13] = { 0, };

//zero one two three four five six seven eight nine <s> oh zero 순서로 받아온다. (zero는 구성 음소가 2개이므로)
//Language model만 다루는 영역

void readUnigram() {
	FILE* fp;
	char buffer[10];
	float prob;

	fp = fopen("unigram.txt", "r");
	if (fp == NULL) {
		printf("'unigram.txt' doesn't exist\n");
		return;
	}
	for (int i = 0; i < 12; i++) {
		fscanf(fp, "%s %f\n", buffer, &prob);
		if (strcmp(buffer, "zero") == 0) unigram[0] = unigram[12] = prob;
		else if (strcmp(buffer, "one") == 0) unigram[1] = prob;
		else if (strcmp(buffer, "two") == 0) unigram[2] = prob;
		else if (strcmp(buffer, "three") == 0) unigram[3] = prob;
		else if (strcmp(buffer, "four") == 0) unigram[4] = prob;
		else if (strcmp(buffer, "five") == 0) unigram[5] = prob;
		else if (strcmp(buffer, "six") == 0) unigram[6] = prob;
		else if (strcmp(buffer, "seven") == 0) unigram[7] = prob;
		else if (strcmp(buffer, "eight") == 0) unigram[8] = prob;
		else if (strcmp(buffer, "nine") == 0) unigram[9] = prob;
		else if (strcmp(buffer, "<s>") == 0) unigram[10] = prob;
		else if (strcmp(buffer, "oh") == 0) unigram[11] = prob;
		else printf("Error!");
	}
	
	//check unigram array
	for (int i = 0; i < 13; i++)
		printf("unigram[%d] : %.6f\n", i, unigram[i]);

	fclose(fp);
	return;
}

void readBigram() {
	FILE * fp;
	char buffer[2][10];
	float prob;
	int idx[2];

	fp = fopen("bigram.txt", "r");
	if (fp == NULL) {
		printf("'bigram.txt' doesn't exist!\n");
		return;
	}
	for (int i = 0; i < 141; i++) {
		fscanf(fp, "%s %s %f\n", buffer[0], buffer[1], &prob);
		for (int j = 0; j < 2; j++) {
			if (strcmp(buffer[j], "zero") == 0) idx[j] = 0;
			else if (strcmp(buffer[j], "one") == 0) idx[j] = 1;
			else if (strcmp(buffer[j], "two") == 0) idx[j] = 2;
			else if (strcmp(buffer[j], "three") == 0)  idx[j] = 3;
			else if (strcmp(buffer[j], "four") == 0) idx[j] = 4;
			else if (strcmp(buffer[j], "five") == 0)  idx[j] = 5;
			else if (strcmp(buffer[j], "six") == 0) idx[j] = 6;
			else if (strcmp(buffer[j], "seven") == 0) idx[j] = 7;
			else if (strcmp(buffer[j], "eight") == 0) idx[j] = 8;
			else if (strcmp(buffer[j], "nine") == 0)idx[j] = 9;
			else if (strcmp(buffer[j], "<s>") == 0) idx[j] = 10;
			else if (strcmp(buffer[j], "oh") == 0) idx[j] = 11;
			else printf("Error!");
		}
		if (idx[0] == 0)
			bigram[12][idx[1]] = prob;
		if (idx[1] == 0)
			bigram[idx[0]][12] = prob;
		if (idx[1] == 0 && idx[0] == 0) bigram[12][12] = prob;
		bigram[idx[0]][idx[1]] = prob;
	}

	//check bigram
	for (int i = 0; i < 13; i++) {
		for (int j = 0; j < 13; j++) {
			printf("%f\n",bigram[i][j]);
		}
		printf("\n");
	}

	fclose(fp);
	return;
}
