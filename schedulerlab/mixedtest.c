/*
	Author: Kevin Pickard
	November 8, 2015
	CSCI 3753: Operating Systems
	Programming Assignment 4: Scheduling
	Worked with Michelle Bray

About:
	Generates several mixed processes with a specified process scheduler for benchmarking purposes.

Usage:	./mixedtest <scheduler protocol> <number of processes> <input filename> <output filename>
			<scheduler protocol> :	Specifies which process scheduler to use (supports SHCED_FIFO, SCHED_RR, or SCHED_OTHER)
			<number of processes> :	The number of simultaneous threads to spawn. Default is 1,000,000.

Acknowledgements:
	Uses some code from the supplied rw.c to generate the I/O-bound process.
*/

// Dependencies
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sched.h>
#include <sys/types.h> 
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

// Variable Definitions
#define MAXFILENAMELENGTH 80
//      Defaults
#define DEFAULT_INPUTFILENAME "rwinput"
#define DEFAULT_OUTPUTFILENAMEBASE "rwoutput"
#define DEFAULT_BLOCKSIZE 1024
#define DEFAULT_TRANSFERSIZE 1024*100
//		Load scaling
#define LIGHT 				10
#define MEDIUM  			100
#define HEAVY				1000

int main(int argc, char* argv[]) {
	// Local variable definitions
	int rv, iteration;s
    int inputFD;
    int outputFD;
    char inputFilename[MAXFILENAMELENGTH];
    char outputFilename[MAXFILENAMELENGTH];
    char outputFilenameBase[MAXFILENAMELENGTH];

    ssize_t transfersize = 0;
    ssize_t blocksize = 0; 
    char* transferBuffer = NULL;
    ssize_t buffersize;

    ssize_t bytesRead = 0;
    ssize_t totalBytesRead = 0;
    int totalReads = 0;
    ssize_t bytesWritten = 0;
    ssize_t totalBytesWritten = 0;
    int totalWrites = 0;
    int inputFileResets = 0;

    // Processing input arguments
    // Setting the default number of iterations
    if(argc < 2) iterations = LIGHT;
    // Use the default scheudling policy if policy is not given
    if(argc < 3) policy = SCHED_OTHER;

    // Set the number of iterations, if given
    if(!strcmp(argv[2],light)) iterations = LIGHT;
    if(!strcmp(argv[2],medium)) iterations = MEDIUM;
    if(!strcmp(argv[2],heavy)) iterations = HEAVY;
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
    
    // Set supplied input filename or default if not supplied
    if(argc < 4){
        if(strnlen(DEFAULT_INPUTFILENAME, MAXFILENAMELENGTH) >= MAXFILENAMELENGTH){
            fprintf(stderr, "Default input filename too long\n");
            exit(EXIT_FAILURE);
        }
        strncpy(inputFilename, DEFAULT_INPUTFILENAME, MAXFILENAMELENGTH);
    }
    else{
        if(strnlen(argv[3], MAXFILENAMELENGTH) >= MAXFILENAMELENGTH){
            fprintf(stderr, "Input filename too long\n");
            exit(EXIT_FAILURE);
        }
        strncpy(inputFilename, argv[3], MAXFILENAMELENGTH);
    }
    // Set supplied output filename base or default if not supplied
    if(argc < 5){
        if(strnlen(DEFAULT_OUTPUTFILENAMEBASE, MAXFILENAMELENGTH) >= MAXFILENAMELENGTH){
            fprintf(stderr, "Default output filename base too long\n");
            exit(EXIT_FAILURE);
        }
    strncpy(outputFilenameBase, DEFAULT_OUTPUTFILENAMEBASE, MAXFILENAMELENGTH);
    }
    else{
        if(strnlen(argv[4], MAXFILENAMELENGTH) >= MAXFILENAMELENGTH){
            fprintf(stderr, "Output filename base is too long\n");
            exit(EXIT_FAILURE);
        }
    strncpy(outputFilenameBase, argv[4], MAXFILENAMELENGTH);
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
    fprintf(stdout, "Forking %d children processes\n", iterations);
    for(i = 0; i < iterations; i++){
        if((pid = fork()) == -1){
            fprintf(stderr, "Child process %d could not be forked, please try again\n", i);
            exit(EXIT_FAILURE);
        }
        else if(pid == 0){
            fprintf(stdout, "Process %d forked PID %d\n", i, pid);
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
            
            // Confirm blocksize is multiple of and less than transfersize
            if(blocksize > transfersize){
                fprintf(stderr, "blocksize can not exceed transfersize\n");
                exit(EXIT_FAILURE);
            }
            if(transfersize % blocksize){
                fprintf(stderr, "blocksize must be multiple of transfersize\n");
                exit(EXIT_FAILURE);
            }

            // Allocate buffer space
            buffersize = blocksize;
            if(!(transferBuffer = malloc(buffersize*sizeof(*transferBuffer)))){
                perror("Failed to allocate transfer buffer");
                exit(EXIT_FAILURE);
            }
    
            // Open Input File Descriptor in Read Only mode
            if((inputFD = open(inputFilename, O_RDONLY | O_SYNC)) < 0){
                perror("Failed to open input file");
                exit(EXIT_FAILURE);
            }

            // Open Output File Descriptor in Write Only mode with standard permissions
            rv = snprintf(outputFilename, MAXFILENAMELENGTH, "%s-%d",
            outputFilenameBase, getpid());    
            if(rv > MAXFILENAMELENGTH){
            fprintf(stderr, "Output filenmae length exceeds limit of %d characters.\n",
                MAXFILENAMELENGTH);
                exit(EXIT_FAILURE);
            }
            else if(rv < 0){
                perror("Failed to generate output filename");
                exit(EXIT_FAILURE);
            }
            if((outputFD =
                open(outputFilename,
                    O_WRONLY | O_CREAT | O_TRUNC | O_SYNC,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) < 0){
                perror("Failed to open output file");
                exit(EXIT_FAILURE);
            }

            // Print Status
            fprintf(stdout, "Reading from %s and writing to %s\n",
                inputFilename, outputFilename);

            // Read from input file and write to output file
            do{
                // Read transfersize bytes from input file
                bytesRead = read(inputFD, transferBuffer, buffersize);
                if(bytesRead < 0){
                    perror("Error reading input file");
                    exit(EXIT_FAILURE);
                }
                else{
                    totalBytesRead += bytesRead;
                    totalReads++;
                }
    
                // If all bytes were read, write to output file
                if(bytesRead == blocksize){
                    bytesWritten = write(outputFD, transferBuffer, bytesRead);
                    if(bytesWritten < 0){
                        perror("Error writing output file");
                        exit(EXIT_FAILURE);
                    }
                    else{
                        totalBytesWritten += bytesWritten;
                        totalWrites++;
                    }
                }
                // Otherwise assume we have reached the end of the input file and reset
                else{
                    if(lseek(inputFD, 0, SEEK_SET)){
                        perror("Error resetting to beginning of file");
                        exit(EXIT_FAILURE);
                    }
                    inputFileResets++;
                }
    
            }

            while(totalBytesWritten < transfersize);
                // Output some possibly helpfull info to make it seem like we were doing stuff
                fprintf(stdout, "Read:    %zd bytes in %d reads\n",
                    totalBytesRead, totalReads);
                fprintf(stdout, "Written: %zd bytes in %d writes\n",
                    totalBytesWritten, totalWrites);
                fprintf(stdout, "Read input file in %d pass%s\n",
                    (inputFileResets + 1), (inputFileResets ? "es" : ""));
                fprintf(stdout, "Processed %zd bytes in blocks of %zd bytes\n",
                    transfersize, blocksize);
    
                // Free Buffer
                free(transferBuffer);

                // Close Output File Descriptor
                if(close(outputFD)){
                    perror("Failed to close output file");
                    exit(EXIT_FAILURE);
                }

                // Close Input File Descriptor
                if(close(inputFD)){
                    perror("Failed to close input file");
                    exit(EXIT_FAILURE);
                }
            }

            // Exiting
            exit(0);
        }
    }

    // Reaping children
    fprintf(stdout, "\nWaiting for all children processes to exit\n");
    int reaped = 0;
    while((pid = wait(&status)) > 0){
        if(WIFEXITED(status)) {
            fprintf(stdout, "Child process %d exited normally.\n", pid);
            reaped ++;
        }
    }
    if(reaped == iterations){
        fprintf(stdout, "\n%d children forked, %d children reaped.\n", iterations, reaped);
        exit(EXIT_SUCCESS);
    }
    else{
        fprintf(stderr, "An error occured: The wrong number of children was reaped, please try again.\n");
        exit(EXIT_FAILURE);
    }
}