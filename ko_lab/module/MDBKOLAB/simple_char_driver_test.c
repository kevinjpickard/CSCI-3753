#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define DEVICE_PATH "/dev/simple_character_device"
#define BUFFER_SIZE 1024

int main () {
	char command, buffer[BUFFER_SIZE];
	int file = open(DEVICE_PATH, O_RDWR);
	printf("*** This is the test program for simple_char_driver.c ***\n\n");
	
	while (1) {	
	printf("    Usage:\n    -------------------------\n");		
	printf("    r/R: Read from device \n    w/W: Write from device \n    e/E: Exit (close) device \n\nEnter a command: ");		

		scanf("%c", &command);
		switch(command) {
			case 'r':
			case 'R':
				read(file, buffer, BUFFER_SIZE);
				printf("Output of device: %s\n", buffer);
				while (getchar() != '\n');
				break;			
			case 'w':
			case 'W':
				printf("Input data to be written to device (< to quit): ");
				scanf("%[^<]s", buffer);
				write(file, buffer, BUFFER_SIZE);
				while (getchar() != '\n');
				break;
			case 'e':
			case 'E':
				return 0;
			default:
				printf("Not a valid entry.  Please enter r/R, w/W, or e/E: ");
				while (getchar() != '\n');
				break;
	
		}
	}
	close(file);
	return 0;
}
