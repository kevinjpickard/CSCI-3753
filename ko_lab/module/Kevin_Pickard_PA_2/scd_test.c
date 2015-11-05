

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define PATH "/dev/simple_character_device"
#define BUFFER_SIZE 1024

int main() {
	char command, buffer[BUFFER_SIZE];
	int file = open(PATH, O_RDWR);
	printf("\n\n-------TEST FUNCTION FOR SIMPLE CHARACTER DEIVCE DRIVER-------\n");
	printf("\nThis program tests the functionality of the simple character\n\
device driver for CSCI 3753: Operating Systems. Please choose\n\
one of the following options (not case-sensitive) to continue:\n\n");

	while(1){

		printf("	----------------------------------------------\n");
		printf("	R) Read from the simple character device.\n");
		printf("	W) Write to the simple character device.\n");
		printf("	E) Exit this test program, closing the device.\n");
		printf("	----------------------------------------------\n\n");
		printf("Choice:");
		scanf("%c", &command);
		switch(command){
			case 'R' :
			case 'r' :
				read(file, buffer, BUFFER_SIZE);
				printf("Device Contents:\n");
				printf("%s\n\n", buffer);
				while(getchar() != '\n');
					break;
			case 'W' :
			case 'w' :
				printf("Data to store (> exits):");
				scanf("%[^>]s", buffer);
				printf("\n");
				write(file, buffer, BUFFER_SIZE);
				while(getchar() != '\n');
					break;
			case 'E' :
			case 'e' :
				return 0;
			default :
				while(getchar() != '\n');
					printf("Invalid entry, please choose one of the following options.\n\n");
					break;
		}
	}
	close(file);
	return 0;
}
