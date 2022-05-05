/* Traveling Salesman Problem
 * - Implements basic Heuristics as a search method, with added randomness
 * - Multiple Nodes with multiple threads
 * 		- Nodes handle different starting cities
 * 		- Threads handle different random paths from the same starting city
 * - Returns the shortest path found in 60ish seconds
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <mpi.h>
#include <omp.h>

#define ODDS 50
#define FILENAME "DistanceMatrix1000_v2.csv"

int main(int argc, char* argv[]){
	// MPI Initializing
	MPI_Init(NULL, NULL);
    int commSz;
    MPI_Comm_size(MPI_COMM_WORLD, &commSz);
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    time_t start;
    srand((unsigned) time(&start));
    // Start Timer
    int (*distMat)[1000] = malloc(sizeof(int[1000][1000]));
    if(distMat==NULL){
        printf("Memory Allocation Issue: Matrix");
        exit(1);
    }
	printf("Rank: %d\n", rank);
    if(rank==0){
	    // Read Input
	    FILE* filePointer;
	    filePointer = fopen(FILENAME,"r");
	    for(int i = 0; i<1000;i++){
	        for(int j=0;j<1000;j++){
	            fscanf(filePointer, "%d,", &distMat[i][j]);
		    //printf("Reading %d, %d = %d\r", i, j, distMat[i][j]);
	        }
	        fscanf(filePointer, "\n");
	    }
	}
    // Search Loop
	MPI_Bcast(&(distMat[0][0]),1000*1000, MPI_INT, 0, MPI_COMM_WORLD);
	// Randomly pick a starting city
	int startCity = rand() % 1000;
	// distMat[A][B] = distance from A->B
	int* bestPath;
	int bestCost = INT_MAX;
	printf("Starting Value: %d ; Starting City: %d\n", bestCost, startCity);
    #pragma omp parallel private(rank, startCity) //Reduction of what exactly?? That could actually be kinda painful to figure out
	{
		time_t tim;
		int *currPath = calloc(1000, sizeof(int));
		currPath[0] = startCity;
		int currCost = 0;
		time(&tim);
		while(tim-start < 60){
			int *visited = calloc(1000, sizeof(int)); //Visited array
			int prev = startCity;
			int next;
			for(int i =1;i<1000;i++){
				//Each Node on the path
				if(rand()%100<ODDS){
					// IF RANDOM
					do{
						next = rand()%1000;
					} while(visited[next]);
				} else {
					for(int j = 0;j<1000;j++){
						if(visited[j]) continue;
						printf("distMat[%d][%d] < distMat[%d][%d]\n", prev, j, prev, next);
						if(distMat[prev][j] < distMat[prev][next]){ // Nearest Neighbor
							next = j;
						}
					}
					// ELSE HEURISTIC
				}
				// Count up stats, update visited
				//printf("CurrCost: %d ; distMat[%d][%d]=%d\n", currCost, prev, next, distMat[prev][next]);
				fflush(stdout);
				currCost += distMat[prev][next];
				currPath[i] = next;
				visited[next] = 1;
				prev = next;
			}
			//Tack on final step
			currCost += distMat[prev][startCity];
			// Check path against our past paths
			#pragma omp critical
			{
				if(currCost<bestCost){
					bestCost = currCost;
					bestPath = currPath;
				}
			}
			time(&tim); //Time check before looking at another path
		}
	}//PARRALLEL
	//Argmin from MPI: MINLOC then SEND/RECV path to root
	printf("%d\n", bestCost);
	//MINLOC
	// IF me == loc, send path to root
	// IF me == root, get path from loc
	// Report output
	MPI_Finalize();
	return 0;
}
