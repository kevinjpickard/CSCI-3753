

#include <stdio.h>
#include <stdlib.h>
#include <fcnt1.h>

#define PATH "/dev/simple_character_device"
#define BUFFER_SIZE 1024

int main() {
	char in, buffer[BUFFER_SIZE];
	int file = open(PATH, O_RWDR);
	printf("\n\n-----TEST FUNCTION FOR SIMPLE CHARACTER DEIVCE DRIVER-----\n");
	printf("\nThis program tests the functionality of the simple character\n
		device driver for CSCI 3753: Operating Systems. Please choose\n
		one of the following options to continue:\n\n");

	printf("	----------------------------------------------\n");
	printf("	R) Read from the simple character device.\n");
	printf("	W) Write to the simple character device.\n");
	printf("	E) Exit this test program, closing the device.\n\n");

	while(1){
		scanf('%s', $in);
		switch(in){
			case 'R' || 'r' :
				read(file, buffer, BUFFER_SIZE);
				printf("Device Contents:\n	%s\n", buffer);
				while(getchar() != '\n');
					break;
			case 'W' || 'w' :
				printf("What would you like to store?\n");
				scanf("%s", buffer);
				write(file, buffer, BUFFER_SIZE);
				while(getchar() != '\n);
					break;
			case 'E' || 'e' :
				return 0;
			default :
				while(getchar() != '\n');
		}
	}
	close(file);
	return 0;
}
