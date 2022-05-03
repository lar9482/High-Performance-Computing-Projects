#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//NOTE: If you can't load in the 1000x1000 file, try running this code on hammer.
#define SIZE 1000

int main(int argc, char** argv) {
	printf("Testing test\n");

	double graph[SIZE][SIZE];
	read_matrix(graph);
	int i, j;
	for (i = 0; i < SIZE; i++) {
		for (j = 0; j < SIZE; j++) {
			printf("a[%d][%d]: %1f ", i, j, graph[i][j]);
		}
		printf("\n");
	}
	return 0;
}

void read_matrix(double graph[SIZE][SIZE]) {
	char f_name[55];
    
    //Create filename

    // sprintf(f_name,"/home/lar9482/Final_Project_2/DistanceMatrix%d_v2.csv", SIZE);
    sprintf(f_name,"DistanceMatrix%d_v2.csv", SIZE);
    printf("Reading array file %s of size %dx%d\n",f_name,SIZE,SIZE);
    //Open file
    FILE *datafile=fopen(f_name,"rb");

    
    //Read elements
    if (!datafile) {
    	printf("Can't open file\n");
    }
    else {
    	printf("Reading a file now\n");
    	int i,j = 0;
    	char buffer[SIZE*SIZE];
    	while (fgets(buffer, SIZE*SIZE, datafile)) {

    		j = 0;
    		char *value  = strtok(buffer, ", ");
    		while (value) {
    			double num = strtod(value, NULL);
    			graph[i][j] = num;
    			value = strtok(NULL, ", ");
    			j++;
    		}
    		i++;
    	}
    }

    printf("Matrix has been read.\n");
    return;
}