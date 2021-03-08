//Project2.c - Multi-threaded Hybrid Quicksort
//
//Author: Kane Eder
//  Date: March 4, 2021
//Course: EECS 3540 (Operating Systems and Systems Programming)
//
//Purpose: 	This program demonstrates how to use threads to speed up the sorting process.
//			It uses a multi-threaded, hybridized quicksort implementation, which switches
//			from Quicksort to Shell sort when the segment size is below a given threashold
//
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>

int *LIST; 
int THRESHOLD;
int SIZE;
int SEED = 0;
typedef struct 
{
	int lo;
	int hi;
	int size;
} param;

void *quicksort(void *threadarg); 
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
	struct timeval 	wall_start, wall_end,
					sort_wall_s, sort_wall_e;
	double 	create_time, initialize_time, scramble_time,
			partition_time, 
			sort_wall_total, sort_cpu_total, wall_total, cpu_total;
	
	cpu_start = clock();
	gettimeofday(&wall_start, NULL);
	
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
			SIZE = atoi(argv[1]);
			if (pieces >= SIZE) {return -1;}
			break;
		default:	// Print Error if Wrong Number of Arguments
			printf("usage: ./project2 SIZE THRESHOLD [SEED [MULTITHREAD [PIECES [THREADS]]]]\n");
			return -1;
	}
	/* ==================================
		 Make Array of Rands
	 ==================================*/
		// Create Array
	create_start = clock();
	LIST = (int *)malloc(SIZE * sizeof(int));
	create_end = clock();
	create_time = ((double)create_end - (double)create_start)/CLOCKS_PER_SEC;
		// Initialize Array
	initialize_start = clock();
	for (int i = 0; i < SIZE; i++) { 
		*(LIST + i) = i; 
	}
	initialize_end = clock();
	initialize_time = ((double)initialize_end - (double)initialize_start)/CLOCKS_PER_SEC;
		// Scramble Array
	scramble_start= clock();
	for (int i = 0; i < SIZE; i++)
        {
            int r = rand() % SIZE;
       		int temp = *(LIST+i);
       		*(LIST+i) = *(LIST+r);
       		*(LIST+r) = temp;
        }
	scramble_end = clock();
	scramble_time = ((double)scramble_end - (double)scramble_start)/CLOCKS_PER_SEC;

	cpu_start = clock();
	gettimeofday(&sort_wall_s, NULL);
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
	(parts + count)->hi = SIZE-1; 	// R
	(parts + count)->size = SIZE;	// part size
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
		pthread_t tid[maxthreads];	
		pthread_attr_t attr[maxthreads];
		
		for (int i=0; i < maxthreads; i++) 
		{	
			pthread_attr_init(&attr[i]); 
			printf("(%9i, %9i, %9i)\n",(parts+count)->lo, (parts+count)->hi, (parts+count)->size);   
			pthread_create(&tid[i], &attr[i], quicksort, (parts + count)); 
			count--;
		}	
		while(count >= 0)
		{
			for(int i=0; i < maxthreads; i++)
			{
				ret_val = pthread_tryjoin_np(tid[i],NULL); 
				if (ret_val == 0 && count >= 0) 
				{
					printf("(%9i, %9i, %9i)\n",(parts+count)->lo, (parts+count)->hi, (parts+count)->size);   
					pthread_create(&tid[i], &attr[i], quicksort, (parts+count));
					count--;
				}
			}
			usleep(5000);
		}
	
		for (int i = 0; i < maxthreads; i++)
		{			
			pthread_join(tid[i], NULL);
		}
	}
	else
	{
		partition_time = 0;
		param *parts= (param *) malloc(pieces * sizeof(param));
		(parts)->lo = 0;		// L
		(parts)->hi = SIZE-1; 	// R
		(parts)->size = SIZE;	// part size
		quicksort(parts);
	}
	sort_cpu_e = clock();
	gettimeofday(&sort_wall_e, NULL);

		/* ==================================
                    Finish Up
         ==================================*/
	sort_wall_total = ((double)sort_wall_e.tv_sec-(double)sort_wall_s.tv_sec) + ((double)sort_wall_e.tv_usec-(double)sort_wall_s.tv_usec)/1000000;
	sort_cpu_total = ((double)sort_cpu_e - (double)sort_cpu_s)/CLOCKS_PER_SEC;
	wall_total = ((double)sort_wall_e.tv_sec-(double)wall_start.tv_sec) + ((double)sort_wall_e.tv_usec-(double)wall_start.tv_usec)/1000000;
	cpu_total = ((double)sort_cpu_e - (double)cpu_start)/CLOCKS_PER_SEC;
	printf("    SIZE    THRESHOLD SD PC T CREATE   INIT  SHUFFLE   PART  SrtWall Srt CPU ALLWall ALL CPU\n");
	printf("  --------- --------- -- -- - ------ ------- ------- ------- ------- ------- ------- -------\n");
	printf("F:%9d %9d %02d %2d %1d %0.3f  %0.3f   %0.3f   %0.3f   %0.3f   %0.3f   %0.3f   %0.3f\n",SIZE,THRESHOLD, SEED, pieces + 1, maxthreads, create_time, initialize_time, scramble_time, partition_time, sort_wall_total, sort_cpu_total, wall_total, cpu_total);
	
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
void *quicksort(void *threadarg)
{
  	param *part = threadarg;
    param *left = (param *) malloc(sizeof(param));
	param *right = (param *) malloc(sizeof(param));
	int lo = part->lo;
	int hi = part->hi;
	int size = part->size;
	if (size < 1)
	{
		return NULL;
	}
	else if (size == 1)
    {	// COMPARE & SWAP
    	if (*(LIST+lo) > *(LIST+hi))
		{
        	int temp = *(LIST+lo);
        	*(LIST+lo) = *(LIST+hi);
        	*(LIST+hi) = temp;
		}
       	return NULL;
    }
	else if (size <= THRESHOLD)	
	{	// SHELL SORT
		int k=1;
		while(k <= size+1){k *= 2;} k = (k / 2) - 1;
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
		return NULL;
	}
	else if (lo < hi)
	{	// QUICKSORT
		int p = partition(lo,hi);
		left->lo = lo;
		left->hi = p-1;
		left->size = (p-1) - lo;
		right->lo = p+1;
		right->hi = hi;
		right->size = hi - (p+1);
		if (left->size < right->size) 
                {	
                    quicksort(left);
    	            quicksort(right);
                }
                else
                {
        	 	   quicksort(right);
                   quicksort(left);
                }
		return NULL;
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
    for(int i=0; i < SIZE-1;i++)
    {
        if(*(LIST+i+1) != *(LIST+i) + 1)
		{
			return false;
		}
    }
	return true;
}
