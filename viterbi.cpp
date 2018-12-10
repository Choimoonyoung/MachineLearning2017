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
}wmm; //단어 hmm 만들때 사용

typedef struct pair {
	int word;
	int nth;
}Pair; //phi에서 사용

extern float unigram[13];
extern float bigram[13][13]; 
extern Vec input; // Vector sequence와 개수로 구성
char* dictionary[13][7] = { //음소 무엇으로 구성되는가?
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
struct wordhmm wlist[13]; //word가 저장되어 있고, 그에 해당하는 transposition matrix와 statenumber가 들어있다.
float vit[700][13][17]; //차례대로 T, word, 그리고 word당 state. likelihood를 저장하는 배열
Pair phi[700][13][17]; //전 단계 likelihood가 최대인 index를 저장해놓는 배열
float precompute[21][3][10]; //B 함수에서 쓰이는 일종의 dp 배열


int findLocation(char* a) {
	for (int i = 0; i < 21; i++) {
		if (strcmp(phones[i].name, a) == 0)
			return i;
	}
	return -1;
}

void makeWMM() {
	//음소마다 state를 합쳐 13개의 word에 대해서 transposition matrix를 만든 후, 그를 wlist에 저장한다.
	//one -> w,ah,n,sp 를 모두 합침.

	for (int w = 0; w < 13; w++) { //word 개수
		for (int i = 0; i < 7; i++) { //음소 최대 개수
			if (strcmp(dictionary[w][i], "0") == 0) {
				if (i == 1) //sil은 isolated word 상에서는 제외한다.
					wlist[w].statenum = 3;
				else
					wlist[w].statenum = (i - 1) * 3 + 1; //sp 뒤 포함
				break;
			}
		}

		//findLocation은 phones 배열에서 해당하는 문자열이 속한 index를 리턴합니다.
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
			//sil의 경우는 그대로 넣는다.

			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 5; j++) {
					wlist[w].trans[i][j] = phones[arr[0]].tp[i][j];
					//printf("w : %e %e\n", wlist[w].trans[i][j], phones[arr[0]].tp[i][j]);
				}
			}
			printf("%d : %e\n",w, wlist[w].trans[3][4]);
			continue;
		}

		
		//sil 을 제외한 나머지는 matrix를 합쳐서 넣는다.
		//sp의 경우 optional 이므로 그를 계산해서 matrix에 넣어야 한다.
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

	//	float tmp = 1; // 헷갈려서 확률곱을 전부다 했을때
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
	//state와 x^t 가 들어왔을 때, 이에 해당하는 확률을 로그를 취한 후 리턴한다.
	//Gaussian Mixture Distribution (10개)
	//precompute를 사용해서 벡터값이 필요없는 경우 미리 계산해놓는다 (dp) 다음에 접근할 때 쉽게 접근 가능
	//각 Normal 마다 li를 각각 구한 후 그를 기반으로 방출 함수를 계산한다.

	float l[N_PDF];
	for (int i = 0; i < N_PDF; i++) {
		int idx = findLocation(dictionary[word][(nth - 1) / 3]); //해당하는 phones idx찾음
		int order = (nth + 2) % 3;

		pdfType now = phones[idx].state[order].pdf[i]; //해당하는 state 찾음! (g=1일때)
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
	} //l1~l10까지 전부 구했음

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
	//한 단어 내에서 다음 state로 넘어갈 때 확률을 계산해주는 함수이다.
	//index도 같이 저장한다.
	float tmp = vit[t][i][to];
	vit[t][i][to] = max(vit[t][i][to], (vit[t - 1][i][j] + logf(wlist[i].trans[j][to]) + B(t, i, to)));
	if (tmp != vit[t][i][to]) {
		phi[t][i][to].word = i, phi[t][i][to].nth = j;
	}
}

void biupdate(int t, int i, int j, int to){
	//한 단어에서 다음 단어로 넘어가는 경우 update하는 함수이다.
	//update와 다른 점은 prior값을 곱해준다는 점이다.

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
	//HMM을 만들기 위해서는 해당하는 음소의 HMM들을 연결
	//그 후에 Transition Graph를 통해 비터비 알고리즘을 구현하는 경우 해당하는 sequence와 likelihood를 구할 수 있다.
	//vi 는 로그 함수의 배열

	vit[0][10][1] = B(0, 10, 1);
	/*for (int t = 0; t < 13; t++) {
		vit[0][t][1] = logf(unigram[t]) + B(0,t,1);
	} //initialize */
	for (int t = 1; t < input.T; t++) {
		for (int i = 0; i < 13; i++) {
			for (int j = 1; j <= wlist[i].statenum; j++) { //모든 state에 대해서 dp (state는 1~statenum idx)
				if (vit[t - 1][i][j] != INIT) {
					//j update
					update(t, i, j,j);
					//경우 나누기
					if (i == 10) { //sil
						if (j == 1) {// 첫번째 state
							update(t, i, j, j + 1);
							update(t, i, j, j + 2);
						}
						else if (j == 2) {//sil의 2번째 state
							update(t, i, j, j + 1);
						}
						else {//sil의 3번째 state
							update(t, i, j, j - 2);
							biupdate(t, i, j,j+1);
						}
						continue;
					}// sil 끝

					if (j == wlist[i].statenum) {//sp인 경우
						biupdate(t, i, j, j+1); //초기 state update
						continue;
					}
					if (j == wlist[i].statenum - 1) { //sp전단계인 경우
						biupdate(t, i, j,j+2);//초기 state update
						update(t, i, j, j + 1);
						continue;
					}
					update(t, i, j, j + 1);
				}
			}
		}
	}

	//maximum sequence 구하기

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
	} //전체 check
	*/ 

	//char* l[10];

	//file에 형식 맞춰서 쓰기
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
	//viterbi에 쓰이는 배열을 MIN 한 값으로 초기화한다 (max 함수를 위해서)
	for (int i = 0; i < input.T; i++) {
		for (int j = 0; j < 13; j++) {
			for (int k = 1; k <= 16; k++)
				vit[i][j][k] = INIT;
		}
	}
}

void makeOutput(FILE *fp, FILE *resfp) {
	//viterbi algorithm을 통해 recognized.txt를 만드는 함수
	//전체의 뼈대가 되는 함수이다.

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
	//결과 적용

	return 0;
}