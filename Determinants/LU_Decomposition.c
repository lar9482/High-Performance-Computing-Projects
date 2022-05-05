#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

//Change this variable to input different files.
//NOTE: Overflow is an issue for SIZE>=512. Solve this later cuz I'm tired right now.
#define SIZE 32

int count_digits();
void perform_LU_Decomposition(double *matrix[SIZE], int thread_count);
void read_matrix(double *matrix[SIZE]);
double calculate_det(double *matrix[SIZE]);
double calculate_log_det(double *matrix[SIZE]);

int main(int argc, char *argv[]) {

	int thread_count = strtol(argv[1], NULL, 10);
	printf("%d threads\n", thread_count);
	double *matrix[SIZE];
	int i, j;
	for (i = 0; i < SIZE; i++) {
		matrix[i] = (double*) malloc(SIZE * sizeof(double));
	}

	read_matrix(matrix);

	double start_time = omp_get_wtime();
	perform_LU_Decomposition(matrix, thread_count);
	double log_det = calculate_log_det(matrix);
	if (SIZE < 1000) {
		double det = calculate_det(matrix);
		printf("Det: %lf\n", det);
	}
	else {
		if (log_det < 0) {
			printf("Det: -");
		} 
		else {
			printf("Det: +");
		}
		printf("inf\n");
	}
	
	printf("Log Det: %lf", log_det);
	printf("Time: %1.16f\n\n", omp_get_wtime() - start_time);
	return 0;
}


void perform_LU_Decomposition(double *matrix[SIZE], int thread_count) {
	int i, j, k;
	double ratio;
	
	for (int i = 0; i < SIZE; i++) {

		//Unfortunately, due to crucial data dependency in the second loop, we can't parallelize the first loop.
		#pragma omp parallel for num_threads(thread_count) private(j, k, ratio)
		for (j = i+1; j < SIZE; j++) {
			ratio = (double) matrix[j][i] / (double) matrix[i][i];
			for (k = 0; k < SIZE; k++) {
				matrix[j][k] = (double) matrix[j][k] - ratio*matrix[i][k];
			}
		}
	}
}

double calculate_det(double *matrix[SIZE]) {
	int i;
	double det = 1;
	#pragma omp parallel for num_threads(thread_count) private(i) reduction(*: det)
	for (i = 0; i < SIZE; i++) {
		det *= (matrix[i][i]);
	}

	return det;
}

double calculate_log_det(double *matrix[SIZE]) {
	int i;
	double det = 0;
	#pragma omp parallel for num_threads(thread_count) private(i) reduction(+: det)

	for (i = 0; i < SIZE; i++) {
		det += log10(fabs(matrix[i][i]));
	}

	return det;
}
int count_digits() {
	int count = 0;
	int size = SIZE;
	while (size != 0) {
		size /= 10;
		count++;
	}
	return count;
}

void read_matrix(double *array[SIZE]) {
	char f_name[50];
    int i,j;
    //Create filename
    // int num_digits = count_digits();
    // if (num_digits == 2) {
    // 	sprintf(f_name,"m00%dx00%d.bin", SIZE, SIZE);
    // }
    // else if (num_digits == 3) {
    // 	sprintf(f_name,"m0%dx0%d.bin", SIZE, SIZE);
    // }
    // else if (num_digits == 4) {
    // 	sprintf(f_name,"m%dx%d.bin", SIZE, SIZE);
    // }


    if (num_digits == 2) {
    	sprintf(f_name,"/home/lar9482/Final_Project_1/m00%dx00%d.bin", SIZE, SIZE);
    }
    else if (num_digits == 3) {
    	sprintf(f_name,"/home/lar9482/Final_Project_1/m0%dx0%d.bin", SIZE, SIZE);
    }
    else if (num_digits == 4) {
    	sprintf(f_name,"/home/lar9482/Final_Project_1/m%dx%d.bin", SIZE, SIZE);
    }

    
    printf("File: %s\n", f_name);
    printf("Size: %d\n", SIZE);
    //Open file
    FILE *datafile=fopen(f_name,"rb");
    //Read elelements
    for (i=0; i< SIZE; i++) {
    	for (j=0; j< SIZE; j++) {
    		fread(&array[i][j],sizeof(double),1,datafile);
            //printf("a[%d][%d]=%f\n",i,j,array[i][j]);
    	}
    }

    //printf("Matrix has been read.\n");
}