
/************************************************************
An example of the classic Berkeley water molecule concurrency solution (provide link to example)
This is based heavily off of make_molecules, a similar program where ethynol radicals were made, but 
used POSIX threading in a Linux environment, and I wanted to try to make something similar in a win-32 environment
Make molecules was an assignment provided by Dr. Zastre at the University of Victoria where he provided the skeleton code, and as 
such some of the code was adapted from his work.
*/

#include "stdafx.h"
#include <stdio.h>
#include <Windows.h>
#include <process.h>
#include <assert.h>
/*Random below threshold indicates H, otherwise O */
#define ATOM_THRESHOLD 0.55
#define DEFAULT_NUM_ATOMS 50
/* bool defined, so not needed
#define TRUE 1 
#define FALSE 0 
*/

//define semaphores here
HANDLE mutex;// = CreateSemaphore(0,1,1,0);
HANDLE h_wait;// = CreateSemaphore(0,1,50,0);
HANDLE o_wait;// = CreateSemaphore(0,1,50,0);
HANDLE over_lock;// = CreateSemaphore(0,1,1,0);

//function prototypes
DWORD WINAPI hReady(void *arg);
DWORD WINAPI oReady(void *arg);
void makeMolecule(int, int );
void init(void);




//global variables here
int oNum = 0; 
int hNum = 0;
int oCount = 0; 
int hCount = 0;  //number of oxygens and hydrogens waiting on queue
int molNum = 0;
long numAtoms;
int count =0;
/*Needed to pass copy of integer to thread */

void init(){
	mutex = CreateSemaphore(0,1,1,0);
	h_wait = CreateSemaphore(0,0,50,0);
	o_wait = CreateSemaphore(0,0,50,0);
	over_lock = CreateSemaphore(0,0,1,0);
}
/*parameters are the atom number that forms the molecule and its type */
void makeMolecule(int atom, int type){
	if(oCount >1 && hCount > 0){
		molNum +=1;
		if(type == 0)fprintf(stdout, "water molecule number %d was made by the actions of h%d\n", molNum, atom);
		if(type == 1)fprintf(stdout, "water molecule number %d was made by the actions of o%d\n", molNum, atom);	
		oCount -=2;
		hCount -=1;
		//count++;
	}
}
DWORD WINAPI hReady(void *arg){
		
	
	WaitForSingleObject(mutex, INFINITE);
	int id = *((int *)arg);
	hNum++;
	ReleaseSemaphore(h_wait,1,NULL);
	printf("h%d is alive\n",id);
	
	hCount +=1;
	if(oCount >=2){//let two O's and an H go ahead
		ReleaseSemaphore(o_wait,1,NULL);
		ReleaseSemaphore(o_wait,1,NULL);
		ReleaseSemaphore(h_wait,1,NULL);
	}
	count +=1;
	WaitForSingleObject(h_wait, INFINITE);
	makeMolecule(id, 0); //atom's id and type
	if(count == numAtoms) ReleaseSemaphore(over_lock,1,NULL);
	ReleaseSemaphore(mutex, 1,0);
	return(0);
}
DWORD WINAPI oReady(void *arg){
			
	
	//printf("got here %d %d\n", count,hCount);
	WaitForSingleObject(mutex, INFINITE);
	int id = *((int *)arg);
	oNum++;
	ReleaseSemaphore(o_wait,1,NULL);
	printf("o%d is alive\n",id);
	
	oCount +=1;
	if(oCount >= 2 && hCount >0){  //two O's and and H go ahead
		ReleaseSemaphore(o_wait,1,NULL);
		ReleaseSemaphore(o_wait,1,NULL);
		ReleaseSemaphore(h_wait,1,NULL);
	}
	count++;
	WaitForSingleObject(o_wait, INFINITE);
	makeMolecule(id,1);
	if(count == numAtoms) ReleaseSemaphore(over_lock,1,NULL);
	ReleaseSemaphore(mutex, 1,NULL);
	return(0);
}

int _tmain(int argc, _TCHAR* argv[])
{
	init();
	long seed;
	numAtoms = DEFAULT_NUM_ATOMS;
	HANDLE **atom;
	DWORD id;
	int i;
	int status = 0;

	if(argc<2){
		fprintf(stderr,"usage: %s <seed>[<num atoms>]\n",argv[0]);
		exit(1);
	}
	if(argc >=2) seed = atoi(argv[1]);
	if(argc == 3){
		numAtoms = atoi(argv[2]);
		if (numAtoms <0){
			fprintf(stderr, "%Id is not a valid number of atoms\n",numAtoms);
			exit(1);
		}
	}
	srand(seed);
	atom = (HANDLE **)malloc(numAtoms * sizeof(HANDLE));
	assert (atom != NULL);
	for (i = 0; i<numAtoms; i++){
		atom[i] = (HANDLE *)malloc(sizeof(HANDLE));
		if((double)rand()/(double)RAND_MAX<ATOM_THRESHOLD){
			
			CreateThread(
				0,
				0,
				hReady,
				&hNum,//(void *)*dupint(hNum),
				0,
				&id);
			//printf("hydrogen\n");
		}else{
			
			//status = thread 
			CreateThread(
				0,
				0,
				oReady,
				&oNum,//(void *)dupint(oNum),
				0,
				&id);
		//	printf("oxygen\n");
		}
	}
		if(status != 0){
			fprintf(stderr, "Error creating atom thread\n");
			exit(1);
		}
	
	WaitForSingleObject(over_lock, INFINITE); //don't exit until all molecules have run
	//check for starvation here
	if(oCount >1 && hCount >0) printf("There is enough material to make at least one more molecule. Starvation has occurred\n");
	
	//release any waiting threads from the queue
	while(oCount > 0){
		ReleaseSemaphore(o_wait, 1,NULL);
		oCount--;
	}
	while(hCount > 0){
		ReleaseSemaphore(h_wait, 1,NULL);
		hCount--;
	}
	fprintf(stdout, "\n%d water molecules created from:\n",molNum);
	fprintf(stdout, "hydrogens: %d, oxygens: %d\n",hNum,oNum);

	return 0;
}

