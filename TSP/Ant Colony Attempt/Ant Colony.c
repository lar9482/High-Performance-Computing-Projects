#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <omp.h>

//NOTE: If you can't load in the 1000x1000 file, try running this code on hammer.
#define SIZE 1000
void read_matrix(double graph[SIZE][SIZE]);
void initialize_visited_cities(int* list_cities);
void initialize_transitions(double* transitions[SIZE]);
double heuristic_function(double graph[SIZE][SIZE], int curr_city, int next_city, double beta);

void generate_solutions(double graph[SIZE][SIZE], double* transitions[SIZE], int starting_city);
void global_update(double* transitions[SIZE], int current_city, int next_city);
void local_update(double* transitions[SIZE], int current_city, int next_city);
void update_transitions(double* transitions[SIZE]);
int pairing_global_tour(int current_city, int next_city);

int num_agents;
int thread_count;
double phi;
double beta;
double Q;

int *best_tour;
int best_length;

int main(int argc, char** argv) {
	//printf("Testing test\n");
	thread_count = strtol(argv[1], NULL, 10);
	num_agents = 1;
	phi = 0.5;
	beta = 2;
	Q = 5.00;

	int iterations_max = 1;
	

	double graph[SIZE][SIZE];
	read_matrix(graph);
	int i, j;
	
	double* transitions[SIZE];
	for (i = 0; i < SIZE; i++) {
		transitions[i] = (double*) malloc(SIZE*sizeof(double));
	}
	
    best_tour = malloc((SIZE+1)*sizeof(int));
	best_length = INT_MAX;
	
	initialize_transitions(transitions);
	double start = omp_get_wtime();

	int iteration = 0;
	while (iteration < iterations_max) {
		for (i = 0; i < num_agents; i++) {
			int starting_city;
			for (starting_city = 0; starting_city < SIZE; starting_city++) {
				generate_solutions(graph, transitions, starting_city);
			}
		}

		update_transitions(transitions);
		iteration++;
	}

	printf("Length: %d\n", best_length);
	for (i = 0; i < SIZE+1; i++) {
		if (i % 20 == 0) {
			printf("\n");
		}
		printf("%d ->", best_tour[i]);
	}
	printf("\nTime: %1.16f\n\n", omp_get_wtime() - start);
	
	return 0;
}

void generate_solutions(double graph[SIZE][SIZE], double* transitions[SIZE], int starting_city) {
	int current_city = starting_city;
	int remaining_cities = SIZE-1;

	//If visited[i] == -1, it indicates that the ith has been visited.
	int* visited_cities = malloc(SIZE*sizeof(int));
	initialize_visited_cities(visited_cities);
	visited_cities[starting_city] = -1;


	int *agent_tour = malloc((SIZE+1)*sizeof(int));
	agent_tour[0] = starting_city;
	int length = 0;
	int k = 1;
	//printf("agent_tour_start %d\n", starting_city);
	while (remaining_cities > 0) {
		
		int i;
		
		int best_city = -5;
		double best_prob = -5.00;
		double total_prob = 0.00;

		//Calculate the total probability for all legal cities in visited_cities
		#pragma omp parallel for num_threads(thread_count) private(i) reduction(+: total_prob)
		for (i = 0; i < SIZE; i++) {
			if (visited_cities[i] != -1 && i != current_city) {
				total_prob += (double)(transitions[current_city][i]*heuristic_function(graph, current_city, i, beta));
			}
		}

		//Calculate the local probability.

		#pragma omp parallel for num_threads(thread_count) private(i) reduction(max: best_prob)
		for (i = 0; i < SIZE; i++) {
			if (visited_cities[i] != -1 && i != current_city) {
				double local_prob = (double)(transitions[current_city][i]*heuristic_function(graph, current_city, i, beta)) / total_prob;
				if (local_prob > best_prob) {
					best_city = i;
				}
				best_prob = fmax(local_prob, best_prob);
			}
		}

		visited_cities[best_city] = -1;
		length += graph[current_city][best_city];
		//local update for transitions.
		local_update(transitions, current_city, best_city);

		current_city = best_city;
		agent_tour[k] = best_city;

		k++;
		remaining_cities--;
	}

	length += graph[current_city][starting_city];
	agent_tour[SIZE] = starting_city;

	// for (k = 0; k < SIZE+1; k++) {
	// 	printf("%d ", agent_tour[k]);
	// }
	// printf("\n%d\n", length);

	if (length < best_length) {
		best_length = length;

		#pragma omp parallel for num_threads(thread_count) private(k)
		for (k = 0; k < SIZE+1;k++) {
			best_tour[k]=agent_tour[k];
		}
	}
	
	free(agent_tour);
	free(visited_cities);
}

void local_update(double* transitions[SIZE], int current_city, int next_city) {
	//printf("Updating pheromone from %d to %d\n", current_city, next_city);
	transitions[current_city][next_city] = (1-phi)*transitions[current_city][next_city] + phi/(SIZE*best_length);
}


void update_transitions(double* transitions[SIZE]) {
	int i, j;

	#pragma omp parallel for num_threads(thread_count) private(i, j)
	for (i = 0; i < SIZE; i++) {
		for (j = 0; j < SIZE; j++) {
			double change_transition = 0.00;
			int test = pairing_global_tour(i, j);

			if (test == 1) {
				change_transition = (double) (Q) / (double) (best_length);
			}
			//printf("Change happening %d", test);
			//printf("Change in transition[%d][%d]: %1f\n ", i, j, change_transition);
			transitions[i][j] = ((1 - phi)*transitions[i][j]) + (phi*change_transition);
		}
	}
}

int pairing_global_tour(int current_city, int next_city) {
	int result = 0;
	for (int i = 0; i <= (SIZE-1); i++) {
		if (best_tour[i] == current_city && best_tour[i+1] == next_city) {
			result = 1;
			break;
		}
	}
	return result;
}



void read_matrix(double graph[SIZE][SIZE]) {
	char f_name[55];
    
    //Create filename

    //sprintf(f_name,"/home/lar9482/Final_Project_2/DistanceMatrix%d_v2.csv", SIZE);
    sprintf(f_name,"DistanceMatrix%d_v2.csv", SIZE);
    //printf("Reading array file %s of size %dx%d\n",f_name,SIZE,SIZE);
    //Open file
    FILE *datafile=fopen(f_name,"rb");

    
    //Read elements
    if (!datafile) {
    	printf("Can't open file\n");
    }
    else {
    	//printf("Reading a file now\n");
    	int i,j = 0;
    	char buffer[SIZE*SIZE];
    	while (fgets(buffer, SIZE*SIZE, datafile)) {

    		j = 0;
    		char *value  = strtok(buffer, ", ");
    		while (value) {
    			double num = strtod(value, NULL);
    			graph[i][j] = num;
    			//printf("%1f\n", num);
    			value = strtok(NULL, ", ");
    			j++;
    		}
    		i++;
    		
    		free(value);
    	}
    }

    //printf("Matrix has been read.\n");
    return;
}

void initialize_visited_cities(int* list_cities) {
	int i;
	for (i = 0; i < SIZE; i++) {
		list_cities[i] = i;
	}
	return;
}

void initialize_transitions(double* transitions[SIZE]) {
	int i, j;
	//srand(5);
	for (i = 0; i < SIZE; i++) {
		for (j = 0; j < SIZE; j++) {
			//printf("%1f", (double) ((double)rand() / (double)RAND_MAX));
			transitions[i][j] = (double) ((double)rand() / (double)RAND_MAX);
		}
	}
	//printf("finished transitions\n");
}

double heuristic_function(double graph[SIZE][SIZE], int curr_city, int next_city, double beta) {
	return (double) pow((double) ( ((double) 1.00) / ((double) graph[curr_city][next_city]) ), beta);
}