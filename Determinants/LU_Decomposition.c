#include <stdio.h>
#include <stdlib.h>

//Change this variable to input different files.
//NOTE: Overflow is an issue for SIZE>512. Solve this later cuz I'm tired right now.
#define SIZE 128

int count_digits();
void read_matrix(double array[SIZE][SIZE]);
double calculate_det(double matrix[SIZE][SIZE]);

int main(int argc, char *argv[]) {
	
	double matrix[SIZE][SIZE];
	read_matrix(matrix);
	double det = calculate_det(matrix);
	printf("%lf\n", det);
	
	return 0;
}

double calculate_det(double matrix[SIZE][SIZE]) {
	int i, j, k;
	
	for (int i = 0; i < SIZE; i++) {
		for (j = i+1; j < SIZE; j++) {
			double ratio = (double) matrix[j][i] / (double) matrix[i][i];
			for (k = 0; k < SIZE; k++) {
				matrix[j][k] = (double) matrix[j][k] - ratio*matrix[i][k];
			}
		}
	}
	

	double det = 1;
	for (i = 0; i < SIZE; i++) {
		det *= (matrix[i][i]);
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

void read_matrix(double array[SIZE][SIZE]) {
	char f_name[50];
    int i,j;
    //Create filename
    int num_digits = count_digits();
    
    printf("num_digits: %d\n", num_digits);
    if (num_digits == 2) {
    	sprintf(f_name,"m00%dx00%d.bin", SIZE, SIZE);
    }
    else if (num_digits == 3) {
    	sprintf(f_name,"m0%dx0%d.bin", SIZE, SIZE);
    }
    else if (num_digits == 4) {
    	sprintf(f_name,"m%dx%d.bin", SIZE, SIZE);
    }
    
    printf("Reading array file %s of size %dx%d\n",f_name,SIZE,SIZE);
    //Open file
    FILE *datafile=fopen(f_name,"rb");
    //Read elelements
    for (i=0; i< SIZE; i++) {
    	for (j=0; j< SIZE; j++) {
    		fread(&array[i][j],sizeof(double),1,datafile);
            //printf("a[%d][%d]=%f\n",i,j,array[i][j]);
    	}
    }

    printf("Matrix has been read.\n");
}