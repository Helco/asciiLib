#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include "asciiLib.h"

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 41
#define CONSOLE_LINE 40

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

typedef int bool;
#define true 1
#define false 0

typedef struct _sample {
	asciiPoint pos;
	int cluster;
} sample;

int clusterCount=3;
int populationCount=16;
sample* population=0;
asciiPoint* middlePoints=0;

double distance (asciiPoint p1,asciiPoint p2) {
	double x=p1.x-p2.x;
	double y=p1.y-p2.y;
	return sqrt(x*x+y*y);
}
double distance2 (asciiPoint p,double mx,double my) {
	mx-=p.x;
	my-=p.y;
	return sqrt(mx*mx+my*my);
}
void createNewPopulation () {
	int i;
	if (middlePoints!=0) {
		free(middlePoints);
		middlePoints=0;
	}
	population=(sample*)realloc(population,sizeof(sample)*populationCount);
	for (i=0;i<populationCount;i++) {
		population[i].pos.x=rand()%SCREEN_WIDTH;
		population[i].pos.y=rand()%(SCREEN_HEIGHT-1);
		population[i].cluster=ASCII_COLOR_WHITE-1;
	}
}
void iterateKmeans () {
	int i,j,c,r;
	double dist,dist2,mx,my;
	if (middlePoints==0) {
		middlePoints=(asciiPoint*)malloc(sizeof(asciiPoint)*clusterCount);
		for (i=0;i<clusterCount;i++)
			middlePoints[i]=population[rand()%populationCount].pos;
	}
	else {
		for (i=0;i<clusterCount;i++) {
			c=-1;
			for (j=0;j<populationCount;j++) {
				if (population[j].cluster==i) {
					if (c<0) {
						c=0;
						mx=population[j].pos.x;
						my=population[j].pos.y;
					}
					else {
						mx=(mx+population[j].pos.x)*0.5;
						my=(my+population[j].pos.y)*0.5;
					}
				}
			}
			c=-1;
			for (j=0;j<populationCount;j++) {
				if (population[j].cluster==i) {
					if (c<0) {
						c=0;
						r=j;
						dist=distance2(population[j].pos,mx,my);
					}
					else {
						dist2=distance2(population[j].pos,mx,my);
						if (dist2<dist) {
							r=j;
							dist=dist2;
						}
					}
				}
			}
			middlePoints[i]=population[r].pos;
		}
	}
	for (i=0;i<populationCount;i++) {
		dist=distance(population[i].pos,middlePoints[0]);
		c=0;
		for (j=1;j<clusterCount;j++) {
			dist2=distance(population[i].pos,middlePoints[j]);
			if (dist2<dist) {
				dist=dist2;
				c=j;
			}
		}
		population[i].cluster=c;
	}
}
void createOptimalClusters () {
	int len=sizeof(asciiPoint)*clusterCount;
	asciiPoint* oldMiddlePoints;
	if (middlePoints==0)
		iterateKmeans ();
	oldMiddlePoints=(asciiPoint*)malloc(len);
	do {
		memcpy(oldMiddlePoints,middlePoints,len);
		iterateKmeans ();
	} while (memcmp(oldMiddlePoints,middlePoints,len)!=0);
}

void draw () {
	static char numberBuffer [SCREEN_WIDTH]={0};
	int i;
	asciiClearRect(asciiRect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT));
	if (population!=0) {
		for (i=0;i<populationCount;i++)
			asciiDrawCharColored('x',population[i].pos,ASCII_COLOR_BLACK,population[i].cluster+1);
	}
	if (middlePoints!=0) {
		for (i=0;i<clusterCount;i++) 
			asciiDrawCharColored('M',middlePoints[i],ASCII_COLOR_BLACK,i+1);
	}
	sprintf(numberBuffer,"(t)Pop(z):%i (g)Cluster(h):%i New pop:(b) Iterate:(space) Optimal:(n)",populationCount,clusterCount);
	asciiDrawTextColored(numberBuffer,asciiPoint(0,CONSOLE_LINE),ASCII_COLOR_BLACK,ASCII_COLOR_WHITE);
	asciiFlip ();
}

void keyEventCallback (uint8_t key,uint8_t pressed,void* context) {
	if (pressed) {
		bool redraw=false;
		bool newPop=false;
		switch (key) {
		case(ASCII_KEY_T):{populationCount=max(populationCount-1,0);newPop=true;redraw=true;}break;
		case(ASCII_KEY_Z):{populationCount++;newPop=true;redraw=true;}break;
		//case(ASCII_KEY_G):{clusterCount=max(clusterCount-1,1);iterateKmeans ();redraw=true;}break;
		//case(ASCII_KEY_H):{clusterCount++;iterateKmeans ();redraw=true;}break;
		case(ASCII_KEY_B):{newPop=true;redraw=true;}break;
		case(ASCII_KEY_N):{createOptimalClusters ();redraw=true;}break;
		case(ASCII_KEY_SPACE):{iterateKmeans ();redraw=true;}break;
		}//switch
		if (newPop)
			createNewPopulation();
		if (redraw)
			draw ();
	}
}

int main (int argc,char* argv[]) {
	srand((unsigned int)time(0));
	asciiInit (SCREEN_WIDTH,SCREEN_HEIGHT);
	asciiSetKeyEventCallback (keyEventCallback,0);
	createNewPopulation ();
	draw ();
	asciiFlip ();
	asciiRun ();
	return 0;
}
