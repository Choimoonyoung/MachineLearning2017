#include <stdio.h>
#include <algorithm>
#include <math.h>
#include <limits.h>
#include "LanguageModel.h"
#include "hmm.h"
#include "importCase.h"
#define PI 3.141592
#define INIT  -1000000000000000000
#pragma warning (disable:4996)

using namespace std;

typedef struct wordhmm {
	int statenum;
	float trans[18][18] = { 0, };
}wmm; //�ܾ� hmm ���鶧 ���

typedef struct pair {
	int word;
	int nth;
}Pair; //phi���� ���

extern float unigram[13];
extern float bigram[13][13]; 
extern Vec input; // Vector sequence�� ������ ����
char* dictionary[13][7] = { //���� �������� �����Ǵ°�?
	{"z","iy","r","ow","sp","0"},
	{"w","ah","n","sp","0"},
	{"t","uw","sp","0"},
	{"th","r","iy","sp","0" },
	{"f","ao","r","sp","0" },
	{"f","ay","v","sp","0" },
	{"s","ih","k","s","sp","0" },
	{"s","eh","v","ah","n","sp","0" },
	{"ey","t","sp","0" },
	{"n","ay","n","sp","0" },
	{"sil","0" },
	{"ow","sp","0" },
	{"z","ih","r","ow","sp","0" }
};
struct wordhmm wlist[13]; //word�� ����Ǿ� �ְ�, �׿� �ش��ϴ� transposition matrix�� statenumber�� ����ִ�.
float vit[700][13][17]; //���ʴ�� T, word, �׸��� word�� state. likelihood�� �����ϴ� �迭
Pair phi[700][13][17]; //�� �ܰ� likelihood�� �ִ��� index�� �����س��� �迭
float precompute[21][3][10]; //B �Լ����� ���̴� ������ dp �迭


int findLocation(char* a) {
	for (int i = 0; i < 21; i++) {
		if (strcmp(phones[i].name, a) == 0)
			return i;
	}
	return -1;
}

void makeWMM() {
	//���Ҹ��� state�� ���� 13���� word�� ���ؼ� transposition matrix�� ���� ��, �׸� wlist�� �����Ѵ�.
	//one -> w,ah,n,sp �� ��� ��ħ.

	for (int w = 0; w < 13; w++) { //word ����
		for (int i = 0; i < 7; i++) { //���� �ִ� ����
			if (strcmp(dictionary[w][i], "0") == 0) {
				if (i == 1) //sil�� isolated word �󿡼��� �����Ѵ�.
					wlist[w].statenum = 3;
				else
					wlist[w].statenum = (i - 1) * 3 + 1; //sp �� ����
				break;
			}
		}

		//findLocation�� phones �迭���� �ش��ϴ� ���ڿ��� ���� index�� �����մϴ�.
		int arr[10];
		for (int i = 0; i < (wlist[w].statenum - 1) / 3 + 1; i++) {
			int tmp = findLocation(dictionary[w][i]);
			if (tmp == -1) {
				printf("error!\n");
				break;
			}
			arr[i] = tmp;
		}

		if (wlist[w].statenum == 3) {
			//sil�� ���� �״�� �ִ´�.

			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 5; j++) {
					wlist[w].trans[i][j] = phones[arr[0]].tp[i][j];
					//printf("w : %e %e\n", wlist[w].trans[i][j], phones[arr[0]].tp[i][j]);
				}
			}
			printf("%d : %e\n",w, wlist[w].trans[3][4]);
			continue;
		}

		
		//sil �� ������ �������� matrix�� ���ļ� �ִ´�.
		//sp�� ��� optional �̹Ƿ� �׸� ����ؼ� matrix�� �־�� �Ѵ�.
		wlist[w].trans[0][1] = 1.0;
	
		for (int i = 1; i <= wlist[w].statenum; i++) {
			int idx = ((i - 1) / 3);

			wlist[w].trans[i][i] = phones[arr[idx]].tp[i - idx * 3][i - idx * 3];
			/*if (i == wlist[w].statenum - 1 ) {
				wlist[w].trans[i][i + 2] = phones[arr[idx]].tp[i - (idx * 3)][i - (idx * 3) + 1] * phones[20].tp[0][2];
				wlist[w].trans[i][i + 1] = phones[arr[idx]].tp[i - (idx * 3)][i - (idx * 3) + 1] * phones[20].tp[0][1];
				continue;
			}*/
			wlist[w].trans[i][i + 1] = phones[arr[idx]].tp[i - idx * 3][i - (idx * 3) + 1];
		}

	}

	//check
	for (int i = 0; i < 13; i++) {
		printf("%d\n", i);
		for (int j = 0; j <= wlist[i].statenum + 1; j++) {
			for (int k = 0; k <= wlist[i].statenum + 1; k++) {
				printf("%e ", wlist[i].trans[j][k]);
			}
			printf("\n");
		}
		printf("\n");
	}

	//	float tmp = 1; // �򰥷��� Ȯ������ ���δ� ������
	/*for (int i = 1; i <= zero1.statenum; i++) {
		int idx = ((i -1)/ 3);

		zero1.trans[i][i] = phones[arr[idx]].tp[i - idx * 3][i - idx * 3] * tmp;
		if (i % 3 == 0 && i != zero1.statenum - 1) {
			tmp *= phones[arr[idx]].tp[i - (idx * 3)][i - (idx * 3) + 1];
			zero1.trans[i][i + 1] = tmp;
			continue;
		}
		if (i == zero1.statenum - 1) {
			tmp *= phones[arr[idx]].tp[i - (idx * 3)][i - (idx * 3) + 1];
			zero1.trans[i][i + 2] = tmp * phones[arr[idx] + 1].tp[0][2];
			tmp *= phones[arr[idx] + 1].tp[0][1];
			zero1.trans[i][i + 1] = tmp;
			continue;
		}
		zero1.trans[i][i + 1] = phones[arr[idx]].tp[i - idx * 3][i - (idx * 3) + 1] * tmp;
	}*/
	return;
}

float B(int t, int word, int nth){
	//state�� x^t �� ������ ��, �̿� �ش��ϴ� Ȯ���� �α׸� ���� �� �����Ѵ�.
	//Gaussian Mixture Distribution (10��)
	//precompute�� ����ؼ� ���Ͱ��� �ʿ���� ��� �̸� ����س��´� (dp) ������ ������ �� ���� ���� ����
	//�� Normal ���� li�� ���� ���� �� �׸� ������� ���� �Լ��� ����Ѵ�.

	float l[N_PDF];
	for (int i = 0; i < N_PDF; i++) {
		int idx = findLocation(dictionary[word][(nth - 1) / 3]); //�ش��ϴ� phones idxã��
		int order = (nth + 2) % 3;

		pdfType now = phones[idx].state[order].pdf[i]; //�ش��ϴ� state ã��! (g=1�϶�)
		if (precompute[idx][order][i] == 0) {
			precompute[idx][order][i] = logf(now.weight) - ((float)N_DIMENSION / 2.0)*logf(2.0 * PI);
			for (int j = 0; j < N_DIMENSION; j++) {
				precompute[idx][order][i] -= logf(sqrtf(now.var[j]));
			}
		}
		l[i] = precompute[idx][order][i];
		for (int j = 0; j < N_DIMENSION; j++) {
			l[i] -= (1.0 / 2.0)*((input.x[t][j] - now.mean[j])*(input.x[t][j] - now.mean[j]) / now.var[j]);
		}
	} //l1~l10���� ���� ������

	float tmp = INIT;
	int idx = 0;
	for (int i = 0; i < N_PDF; i++) {
		if (tmp != max(tmp, l[i])) {
			tmp = max(tmp, l[i]);
			idx = i;
		}
	}

	double sum = 0.0;
	for (int i = 0; i < N_PDF; i++) {
		if (i == idx) continue;
		double tmp = l[i] - l[idx];
		tmp = exp(tmp);
		sum +=tmp;
	}
	sum += logf(sum+1.0) + l[idx];
	return sum;
}

void update(int t, int i, int j, int to) {
	//�� �ܾ� ������ ���� state�� �Ѿ �� Ȯ���� ������ִ� �Լ��̴�.
	//index�� ���� �����Ѵ�.
	float tmp = vit[t][i][to];
	vit[t][i][to] = max(vit[t][i][to], (vit[t - 1][i][j] + logf(wlist[i].trans[j][to]) + B(t, i, to)));
	if (tmp != vit[t][i][to]) {
		phi[t][i][to].word = i, phi[t][i][to].nth = j;
	}
}

void biupdate(int t, int i, int j, int to){
	//�� �ܾ�� ���� �ܾ�� �Ѿ�� ��� update�ϴ� �Լ��̴�.
	//update�� �ٸ� ���� prior���� �����شٴ� ���̴�.

	for (int k = 0; k < 13; k++) {
		if (i ==10 && k == 10) continue;
		float tmp = vit[t][k][1];
		vit[t][k][1] = max(vit[t][k][1], vit[t - 1][i][j] + logf(wlist[i].trans[j][to]) + B(t, k, 1) + logf(unigram[k])/*+logf(bigram[i][k])*/);
		if (tmp != vit[t][k][1]) {
			phi[t][k][1].word = i, phi[t][k][1].nth = j;
		}
	}
}

void viterbi(FILE *resfp) {
	//HMM�� ����� ���ؼ��� �ش��ϴ� ������ HMM���� ����
	//�� �Ŀ� Transition Graph�� ���� ���ͺ� �˰����� �����ϴ� ��� �ش��ϴ� sequence�� likelihood�� ���� �� �ִ�.
	//vi �� �α� �Լ��� �迭

	vit[0][10][1] = B(0, 10, 1);
	/*for (int t = 0; t < 13; t++) {
		vit[0][t][1] = logf(unigram[t]) + B(0,t,1);
	} //initialize */
	for (int t = 1; t < input.T; t++) {
		for (int i = 0; i < 13; i++) {
			for (int j = 1; j <= wlist[i].statenum; j++) { //��� state�� ���ؼ� dp (state�� 1~statenum idx)
				if (vit[t - 1][i][j] != INIT) {
					//j update
					update(t, i, j,j);
					//��� ������
					if (i == 10) { //sil
						if (j == 1) {// ù��° state
							update(t, i, j, j + 1);
							update(t, i, j, j + 2);
						}
						else if (j == 2) {//sil�� 2��° state
							update(t, i, j, j + 1);
						}
						else {//sil�� 3��° state
							update(t, i, j, j - 2);
							biupdate(t, i, j,j+1);
						}
						continue;
					}// sil ��

					if (j == wlist[i].statenum) {//sp�� ���
						biupdate(t, i, j, j+1); //�ʱ� state update
						continue;
					}
					if (j == wlist[i].statenum - 1) { //sp���ܰ��� ���
						biupdate(t, i, j,j+2);//�ʱ� state update
						update(t, i, j, j + 1);
						continue;
					}
					update(t, i, j, j + 1);
				}
			}
		}
	}

	//maximum sequence ���ϱ�

	float likelihood = INIT;
	int seq[700][2] = { 0, };
	for (int i = 0; i < 13; i++) {
		for (int j = 0; j <=0; j++) {
			float tmp = likelihood;
			likelihood = max(likelihood, vit[input.T-1][i][wlist[i].statenum-j]);
			if (tmp != likelihood) {
				seq[input.T-1][0] = i, seq[input.T-1][1] = wlist[i].statenum -j;
			}
		}
	}
	
	//backtracking

	float tmp = likelihood;
	likelihood = max(likelihood, vit[input.T - 1][10][1]);
	if(tmp!= likelihood)
		seq[input.T - 1][0] = 10, seq[input.T - 1][1] = 1;

	for (int t = input.T - 2; t >= 0; t--) {
		Pair tmp = phi[t+1][seq[t + 1][0]][seq[t + 1][1]];
		seq[t][0] = tmp.word, seq[t][1] = tmp.nth;
	}

	/*for (int i = 0; i < input.T; i++) {
		printf("%d %d\n", seq[i][0], seq[i][1]);
	} //��ü check
	*/ 

	//char* l[10];

	//file�� ���� ���缭 ����
	for (int i = 0,idx = 0; i < input.T; i++) {
		if (seq[i][0] != seq[i - 1][0]||(seq[i][0] == seq[i - 1][0] && seq[i][1] - seq[i - 1][1] != 1 && seq[i][1] - seq[i - 1][1] != 0 && seq[i][1] == 1)) {
			idx++;
			switch (seq[i][0]%12) {
			case 0:
				//l[idx++] = "zero";
				fprintf(resfp,"zero\n");
				break;
			case 1:
				//l[idx++] = "one";
				fprintf(resfp, "one\n");
				break;
			case 2:
				//l[idx++] = "two";
				fprintf(resfp, "two\n");
				break;
			case 3:
				//l[idx++] = "three";
				fprintf(resfp, "three\n");
				break;
			case 4:
				//l[idx++] = "four";
				fprintf(resfp, "four\n");
				break;
			case 5:
				//l[idx++] = "five";
				fprintf(resfp, "five\n");
				break;
			case 6:
				//l[idx++] = "six";
				fprintf(resfp, "six\n");
				break;
			case 7:
				//l[idx++] = "seven";
				fprintf(resfp, "seven\n");
				break;
			case 8:
				//l[idx++] = "eight";
				fprintf(resfp, "eight\n");
				break;
			case 9:
				//l[idx++] = "nine";
				fprintf(resfp, "nine\n");
				break;
			case 10:
				break;
			case 11:
				//l[idx++] = "zero";
				//if(idx == 1 || idx >= 8) continue;
				fprintf(resfp, "oh\n");
				break;
			}
			//printf("%d\n", seq[i][0]);
		}
	}
}

void initializeVit() {
	//viterbi�� ���̴� �迭�� MIN �� ������ �ʱ�ȭ�Ѵ� (max �Լ��� ���ؼ�)
	for (int i = 0; i < input.T; i++) {
		for (int j = 0; j < 13; j++) {
			for (int k = 1; k <= 16; k++)
				vit[i][j][k] = INIT;
		}
	}
}

void makeOutput(FILE *fp, FILE *resfp) {
	//viterbi algorithm�� ���� recognized.txt�� ����� �Լ�
	//��ü�� ���밡 �Ǵ� �Լ��̴�.

	char dir[25];
	char buf[20];
	char tmp;
	fscanf(fp, "%s\n", dir);
	fprintf(resfp, "%s\n", dir);
	for (int i = 1; i < 11179; i++) {
		fscanf(fp, "%c%16s%s\n", &tmp,dir,buf);
		for (int j = 0; j < 8; j++)
			fscanf(fp, "%s\n");
		fprintf(resfp, "\"%s.rec\"\n", dir);
		readTestcase(dir);
		initializeVit();
		viterbi(resfp);
		fprintf(resfp, ".\n");
	}
	fclose(fp);
	fclose(resfp);
}


int main() {
	readUnigram();
	readBigram();
	//read case
	makeWMM();
	//make word HMM

	FILE *fp, *resfp;
	fp = fopen("reference.txt", "r");
	resfp = fopen("recognized.txt", "w");
	if (fp == NULL || resfp == NULL)
		return -1;

	makeOutput(fp, resfp);
	//��� ����

	return 0;
}