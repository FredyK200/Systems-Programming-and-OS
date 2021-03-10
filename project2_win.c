//Project2.c - Multi-threaded Hybrid hybridsort
//
//Author: Kane Eder
//  Date: March 4, 2021
//Course: EECS 3540 (Operating Systems and Systems Programming)
//
//Purpose: 	This program demonstrates how to use threads to speed up the sorting process.
//			It uses a multi-threaded, hybridized quicksort implementation, which switches
//			from quicksort to Shell sort when the segment size is below a given threashold
//
//This version was built for fun to compare and contrast linux vs Windows thread implementations

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

int *LIST; 
int THRESHOLD;
int ARRAYSIZE;
int SEED = 0;
typedef struct 
{
	int lo;
	int hi;
	int size;
} param;
DWORD WINAPI sort(LPVOID threadarg);
double getTime();
void hybridsort(int lo, int hi, int size); 
int partition(int lo, int hi);
bool isSorted();

int main(int argc, char *argv[])
{
	clock_t cpu_start, cpu_end,
			create_start, create_end,
        	initialize_start, initialize_end,
            scramble_start, scramble_end,
			sort_cpu_s, sort_cpu_e,
			partition_start, partition_end;

	double 	create_time, initialize_time, scramble_time,
			partition_time, 
			win_cpu_start, win_cpu_end,
			sort_wall_total, sort_cpu_total, wall_total, cpu_total;
	
	cpu_start = clock();

	bool multithread = true;
	int pieces = 9;
	int maxthreads = 4;	
	/* ==================================
		   Argument Parsing
	 ==================================*/
	switch(argc)
	{
		case 7:	// Set # of Threads
			maxthreads = atoi(argv[6]);
			if(maxthreads < 0) {return -1;}
		case 6: // Set # of Pieces
			pieces = atoi(argv[5])-1;
			if (pieces < maxthreads) {return -1;}
		case 5: // turn Multithreading On/Off
			if(*argv[4] == 'n' || *argv[4] == 'N')
			{
				printf("multithreading off\n");
				multithread = false;
			}
		case 4: //Set Seed
			SEED = atoi(argv[3]);
			if(SEED == -1) 
			{
				srand(clock());
			}else{
				srand(SEED);
			}
		case 3: // Set size & threshold
			THRESHOLD = atoi(argv[2]);
			ARRAYSIZE = atoi(argv[1]);
			if (pieces >= ARRAYSIZE) {return -1;}
			break;
		default:	// Print Error if Wrong Number of Arguments
			printf("usage: ./project2 ARRAYSIZE THRESHOLD [SEED [MULTITHREAD [PIECES [THREADS]]]]\n");
			return -1;
	}
	/* ==================================
		 Make Array of Rands
	 ==================================*/
		// Create Array
	create_start = clock();
	LIST = (int *)malloc(ARRAYSIZE * sizeof(int));
	create_end = clock();
	create_time = ((double)create_end - (double)create_start)/CLOCKS_PER_SEC;
		// Initialize Array
	initialize_start = clock();
	for (int i = 0; i < ARRAYSIZE; i++) { 
		*(LIST + i) = i; 
	}
	initialize_end = clock();
	initialize_time = ((double)initialize_end - (double)initialize_start)/CLOCKS_PER_SEC;
		// Scramble Array
	scramble_start= clock();
	for (int i = 0; i < ARRAYSIZE; i++)
        {
            int r = rand() % ARRAYSIZE;
       		int temp = *(LIST+i);
       		*(LIST+i) = *(LIST+r);
       		*(LIST+r) = temp;
        }
	scramble_end = clock();
	scramble_time = ((double)scramble_end - (double)scramble_start)/CLOCKS_PER_SEC;

	cpu_start = clock();
	win_cpu_start = getTime();
	sort_cpu_s = clock();
	partition_start = clock();
	if(multithread)
	{
	/* ==================================
                 Partition Segments
    ==================================*/

	param *parts= (param *) malloc(pieces * sizeof(param));
	int lo, hi, size, i, count = 0;
	param *left = (param *) malloc(sizeof(param));
	param *right = (param *) malloc(sizeof(param));

	(parts + count)->lo = 0;		// L
	(parts + count)->hi = ARRAYSIZE-1; 	// R
	(parts + count)->size = ARRAYSIZE;	// part size
		while(count < pieces)
		{
			lo = (parts + count)->lo;
			hi = (parts + count)->hi;
			size= (parts + count)->size;
			count--;
			int p = partition(lo, hi);
			left->lo = lo;
			left->hi = p-1;
			left->size = ((p-1) - lo);
			right->lo = p+1;
			right->hi = hi;
			right->size = (hi - (p+1));
			i = count;
			while(left->size < (parts + i)->size && i >=0)
			{
				*(parts + i+1) = *(parts + i);
				i--;
			}
			*(parts + i+1) = *left;
			count++;
		
			i=count;
			while(right->size < (parts + i)->size && i >=0)
			{
				*(parts + i+1) = *(parts + i);
				i--;
			}
			*(parts + i+1) = *right;
			count++;
		}
		partition_end = clock();
		partition_time = ((double)partition_end - (double)partition_start)/CLOCKS_PER_SEC;
	 		/* ==================================
	                   Create Threads
	         ==================================*/
		int ret_val;
		DWORD *ThreadId;
		HANDLE *ThreadHandle;
		ThreadId = (DWORD*)malloc(maxthreads * sizeof(DWORD));
		ThreadHandle = (HANDLE*)malloc(maxthreads * sizeof(HANDLE));
		for (int i = 0; i < 4; i++)
		{
			printf("(%9i, %9i, %9i)\n", (parts + count)->lo, (parts + count)->hi, (parts + count)->size);
			*(ThreadHandle + i) = CreateThread(
				NULL,				/* default security attributes */
				0,					/* default stack aSIZE */
				sort,			/* thread function */
				(parts + count),	/* parameter to thread function */
				0,					/* default creation flags */
				(ThreadId + i));		/* returns the thread identifier */
			count--;
		}
		while (count >= 0)
		{
			for (int i = 0; i < maxthreads; i++)
			{
				ret_val = WaitForSingleObject(ThreadHandle[i], 0);
				if (ret_val == WAIT_OBJECT_0 && count >= 0)
				{
					printf("(%9i, %9i, %9i)\n", (parts + count)->lo, (parts + count)->hi, (parts + count)->size);
					*(ThreadHandle + i) = CreateThread(
						NULL,				/* default security attributes */
						0,					/* default stack aSIZE */
						sort,			/* thread function */
						(parts + count),	/* parameter to thread function */
						0,					/* default creation flags */
						(ThreadId + i));		/* returns the thread identifier */
					count--;
				}
			}
			Sleep(5000);
		}

		for (int i = 0; i < maxthreads; i++)
		{
			WaitForMultipleObjects(4, ThreadHandle, TRUE, INFINITE);
		}
	}
	else
	{
		partition_time = 0;
		hybridsort(0, ARRAYSIZE-1, ARRAYSIZE);
	}

/* ==================================
			Finish Up
 ==================================*/
	sort_cpu_e = clock();
	win_cpu_end = getTime();
	wall_total = ((double)sort_cpu_e - (double)cpu_start) / CLOCKS_PER_SEC;
	sort_wall_total = ((double)sort_cpu_e - (double)sort_cpu_s) / CLOCKS_PER_SEC;
	sort_cpu_total =  (win_cpu_end - win_cpu_start);
	cpu_total = (win_cpu_end - (double)cpu_start) / CLOCKS_PER_SEC;



	printf("  ARRAYSIZE THRESHOLD SD PC T CREATE   INIT  SHUFFLE   PART  SrtWall Srt CPU ALLWall ALL CPU\n");
	printf("  --------- --------- -- -- - ------ ------- ------- ------- ------- ------- ------- -------\n");
	printf("F:%9d %9d %2d %2d %1d %0.3f  %0.3f   %0.3f   %0.3f   %0.3f   %0.3f   %0.2f   %0.2f\n",ARRAYSIZE,THRESHOLD, SEED, pieces+1, maxthreads, create_time, initialize_time, scramble_time, partition_time, sort_wall_total, sort_cpu_total, wall_total, cpu_total);
	
	bool sorted = isSorted();
	if (!sorted)
	{
		fprintf(stderr, "Array Not Sorted\n");
		return -1;
	}
}	

/* ==================================
      	Thread Function
==================================*/
DWORD WINAPI sort(LPVOID threadarg)
{
	param* part = threadarg;
	int lo = part->lo;
	int hi = part->hi;
	int size = part->size;
	hybridsort(lo, hi, size);
	return 0;
}

void hybridsort(int lo, int hi, int size)
{
	int left_size, right_size;
	if (size < 1)
	{
		return;
	}
    else if (size == 1)
    {	// COMPARE & SWAP
    	if (*(LIST+lo) > *(LIST+hi))
		{
        	int temp = *(LIST+lo);
        	*(LIST+lo) = *(LIST+hi);
        	*(LIST+hi) = temp;
		}
       	return;
    }
    else if (size <= THRESHOLD)	
	{	// SHELL SORT
		int k=1;
		while (k <= size + 1) { k *= 2;} k = (k / 2) - 1;
		do{
       		for (int i=lo; i < (hi+1-k); i++)
          	{
                for(int j=i; j >= lo; j-=k)
                {
                   	if (*(LIST+j) <= *(LIST+j+k))
					{
						break;
					}
              		else
					{ 
        				int temp = *(LIST+j);
        				*(LIST+j) = *(LIST+j+k);
        				*(LIST+j+k) = temp;
					}
                }
            }
            k = k >> 1;
        }while(k > 0); 
		return;
	}
    else if (lo < hi) 
	{	// hybridsort
		int p = partition(lo,hi);
		left_size = (p-1) - lo;
		right_size = hi - (p+1);
		if (left_size < right_size) 
                {	
                    hybridsort(lo, p-1, left_size);
    	            hybridsort(p+1, hi, right_size);
                }
                else
                {
        	 	   hybridsort(p+1, hi, right_size);
                   hybridsort(lo, p-1, left_size);
                }
		return;
	}
}

int partition(int lo, int hi)
{
        int i = lo, j = hi+1;
        int x = *(LIST+lo);
        do{
                do i++; while (*(LIST+i) < x && i < hi);
                do j--; while (*(LIST+j) > x && j > lo);
                if (i < j)
		{
		        int temp = *(LIST+i);
		        *(LIST+i) = *(LIST+j);
		        *(LIST+j) = temp;
		}
                else{break;}
        }while(true);
	int temp = *(LIST+lo);
	*(LIST+lo) = *(LIST+j);
	*(LIST+j) = temp;
        return j;
}

bool isSorted()
{
        for(int i=0; i < ARRAYSIZE-1;i++)
        {
                if(*(LIST+i) != i)
				{
					return false;
				}
        }
        return true;
}

double getTime() 
{
	LARGE_INTEGER freq, val;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&val);
	return (double)val.QuadPart / (double)freq.QuadPart;
}