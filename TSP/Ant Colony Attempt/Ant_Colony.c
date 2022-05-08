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

int thread_count;
int num_agents;
double phi;
double beta;
double Q;

int *best_tour;
int best_length;

int main(int argc, char** argv) {
	thread_count = strtol(argv[1], NULL, 10);
	printf("Running Ant_Colony.c on %d threads\n", thread_count);
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
	initialize_transitions(transitions);
    best_tour = malloc((SIZE+1)*sizeof(int));
	best_length = INT_MAX;
	
	double start = omp_get_wtime();

	//This is the main driver loop. 
	int iteration = 0;
	while (iteration < iterations_max) {

		//For each agent, generate solutions using every possible starting_city and compare to the global global tour.
		for (i = 0; i < num_agents; i++) {
			int starting_city;
			for (starting_city = 0; starting_city < SIZE; starting_city++) {
				generate_solutions(graph, transitions, starting_city);
			}
		}

		//Make global updates to the pheromone table.
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
	free(transitions);
	
	return 0;
}

void generate_solutions(double graph[SIZE][SIZE], double* transitions[SIZE], int starting_city) {
	int current_city = starting_city;
	int remaining_cities = SIZE-1;

	//If visited[i] == -1, it indicates that the ith has been visited.
	int* visited_cities = malloc(SIZE*sizeof(int));
	initialize_visited_cities(visited_cities);
	visited_cities[starting_city] = -1;

	//Initializing a local tour.
	int *agent_tour = malloc((SIZE+1)*sizeof(int));
	agent_tour[0] = starting_city;
	int length = 0;
	int tour_city = 1;

	//This loop builds agent_tour by finding the best probability at every current city. 
	while (remaining_cities > 0) {
		int next_city;
		
		int best_city = -5;
		double best_prob = -5.00;
		double total_prob = 0.00;

		//Calculate the total probability for all legal cities in visited_cities
		#pragma omp parallel for num_threads(thread_count) private(next_city) reduction(+: total_prob)
		for (next_city = 0; next_city < SIZE; next_city++) {
			if (visited_cities[next_city] != -1 && next_city != current_city) {
				total_prob += (double)(transitions[current_city][next_city]*heuristic_function(graph, current_city, next_city, beta));
			}
		}

		//Calculate each local probability and select the next city based on the city that gives the greatest probability. 
		double local_prob = 0.00;
		#pragma omp parallel for num_threads(thread_count) private(next_city, local_prob) reduction(max: best_prob)
		for (next_city = 0; next_city < SIZE; next_city++) {
			if (visited_cities[next_city] != -1 && next_city != current_city) {
				local_prob = (double)(transitions[current_city][next_city]*heuristic_function(graph, current_city, next_city, beta)) / total_prob;
				if (local_prob > best_prob) {
					best_city = next_city;
				}
				best_prob = fmax(local_prob, best_prob);
			}
		}

		//Updating visited city list and tour length to reflect the best city choosen. 
		visited_cities[best_city] = -1;
		length += graph[current_city][best_city];

		//local update for transitions.
		local_update(transitions, current_city, best_city);

		//Officially adding the best city selected to the local tour. 
		current_city = best_city;
		agent_tour[tour_city] = best_city;

		tour_city++;
		remaining_cities--;
	}


	//Looping the tour back to the starting city. 
	length += graph[current_city][starting_city];
	agent_tour[SIZE] = starting_city;

	//Once a tour has been built, its length is compared to global tour. If the local tour's length is shorter, then the global tour
	//becomes this local tour. 
	if (length < best_length) {
		best_length = length;

		#pragma omp parallel for num_threads(thread_count) private(tour_city)
		for (tour_city = 0; tour_city < SIZE+1;tour_city++) {
			best_tour[tour_city]=agent_tour[tour_city];
		}
	}
	
	free(agent_tour);
	free(visited_cities);
}

//Local update to the pheromone table, which is called every time a best_city is selected based on the calculated probability. 
void local_update(double* transitions[SIZE], int current_city, int next_city) {
	transitions[current_city][next_city] = (1-phi)*transitions[current_city][next_city] + phi/(SIZE*best_length);
}


//Global updates to the pheromone table.
//Each pairing (current_city, next_city) is checked to see if it's in the global tour.
//Then, an decrement to the pheromone table at (current_city, next_city)
void update_transitions(double* transitions[SIZE]) {
	int current_city, next_city;

	#pragma omp parallel for num_threads(thread_count) private(current_city, next_city)
	for (current_city = 0; current_city < SIZE; current_city++) {
		for (next_city = 0; next_city < SIZE; next_city++) {
			double change_transition = 0.00;
			int test = pairing_global_tour(current_city, next_city);

			if (test == 1) {
				change_transition = (double) (Q) / (double) (best_length);
			}
			transitions[current_city][next_city] = ((1 - phi)*transitions[current_city][next_city]) + (phi*change_transition);
		}
	}
}

//Checking if current_city and next_city are in the best global tour. 
int pairing_global_tour(int current_city, int next_city) {
	int result = 0;
	for (int i = 0; i < SIZE; i++) {
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