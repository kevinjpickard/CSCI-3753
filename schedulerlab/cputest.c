/*
	Author: Kevin Pickard
	November 8, 2015
	CSCI 3753: Operating Systems
	Programming Assignment 4: Scheduling
	Worked with Michelle Bray

About:
	Generates several CPU-bound processes with a specified process scheduler for benchmarking purposes.

Usage:	./cputest <scheduler protocol> <number of processes>
			<scheduler protocol> :	Specifies which process scheduler to use (supports SHCED_FIFO, SCHED_RR, or SCHED_OTHER)
			<number of processes> :	The number of simultaneous threads to spawn. Default is 1,000,000.

Acknowledgements:
	Uses some code from the supplied pi-sched.c to generate the CPU-bound process.
*/

// Dependencies
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sched.h>
#include <sys/types.h> 
#include <unistd.h>
#include <sys/wait.h>

// Variable Definitions
//		Defaults
#define DEFAULT_ITERATIONS 	1000000
#define RADIUS 				(RAND_MAX / 2)
//		Load scaling
#define LIGHT 				10
#define MEDIUM  			100
#define HEAVY				1000

inline double dist(double x0, double y0, double x1, double y1){
    return sqrt(pow((x1-x0),2) + pow((y1-y0),2));
}

inline double zeroDist(double x, double y){
    return dist(0, 0, x, y);
}

int main(int argc, char* argv[]) {
// Local variable definitions
    long i;
    long iterations;
    struct sched_param param;
    int policy;
    double x, y;
    double inCircle = 0.0;
    double inSquare = 0.0;
    double pCircle = 0.0;
    double piCalc = 0.0;
    pid_t pid;
    int status;

    // Processing input arguments
    if(argc < 2) iterations = DEFAULT_ITERATIONS;
    // Use the default scheudling policy if policy is not given
    if(argc < 3) policy = SCHED_OTHER;
    
    // Set the number of iterations, if given
    if(!strcmp(argv[2],"light")) iterations = LIGHT;
    if(!strcmp(argv[2],"medium")) iterations = MEDIUM;
    if(!strcmp(argv[2],"heavy")) iterations = HEAVY;
	if(iterations < 1){
	    fprintf(stderr, "Bad iterations value\n");
	    exit(EXIT_FAILURE);
	}
	if(iterations < 1){
	    fprintf(stderr, "Bad iterations value\n");
	    exit(EXIT_FAILURE);
	}

    // Parsing the desired scheduler
    if(argc > 2){
		if(!strcmp(argv[2], "SCHED_OTHER")){
	    	policy = SCHED_OTHER;
		}
		else if(!strcmp(argv[2], "SCHED_FIFO")) policy = SCHED_FIFO;
	
		else if(!strcmp(argv[2], "SCHED_RR")) policy = SCHED_RR;
		
		else{
	    	fprintf(stderr, "Unhandeled scheduling policy\n");
	    	exit(EXIT_FAILURE);
		}
    }
    
    // Set process to max prioty for given scheduler
    param.sched_priority = sched_get_priority_max(policy);
    
    // Set new scheduler policy
    fprintf(stdout, "Current Scheduling Policy: %d\n", sched_getscheduler(0));
    fprintf(stdout, "Setting Scheduling Policy to: %d\n", policy);
    
    if(sched_setscheduler(0, policy, &param)){
		perror("Error setting scheduler policy");
		exit(EXIT_FAILURE);
    }
    
    fprintf(stdout, "New Scheduling Policy: %d\n", sched_getscheduler(0));

    // Begin testing
    fprintf(stdout, "Forking %ld children processes\n", iterations);
    for(i = 0; i < iterations; i++){
    	if((pid = fork()) == -1){
    		fprintf(stderr, "Child process %ld could not be forked, please try again\n", i);
    		exit(EXIT_FAILURE);
    	}
    	else if(pid == 0){
    		fprintf(stdout, "Process %ld forked PID %d\n", i, pid);
    		// Calculate pi using statistical methode across all iterations
    		for(i=0; i<iterations; i++){
				x = (random() % (RADIUS * 2)) - RADIUS;
				y = (random() % (RADIUS * 2)) - RADIUS;
				if(zeroDist(x,y) < RADIUS){
	    			inCircle++;
				}
				inSquare++;
    		}

    		// Finishing
    		pCircle = inCircle/inSquare;
    		piCalc = pCircle * 4.0;

    		// Display the result
   			fprintf(stdout, "pi = %f\n", piCalc);

   			// Exiting
    		exit(0);
    	}
    }

    // Reaping children
    fprintf(stdout, "\nWaiting for all children processes to exit\n");
    long reaped = 0;
    while((pid = wait(&status)) > 0){
    	if(WIFEXITED(status)) {
    		fprintf(stdout, "Child process %d exited normally.\n", pid);
    		reaped ++;
    	}
    }
    if(reaped == iterations){
    	fprintf(stdout, "\n%ld children forked, %ld children reaped.\n", iterations, reaped);
    	exit(EXIT_SUCCESS);
    }
    else{
    	fprintf(stderr, "An error occured: The wrong number of children was reaped, please try again.\n");
    	exit(EXIT_FAILURE);
    }
}
