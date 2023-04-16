#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdint.h>
#include "packet.h"
#include "serial.h"
#include "serialize.h"
#include "constants.h"

#define PORT_NAME			"/dev/ttyACM0"
#define BAUD_RATE			B9600

bool COMMAND_MANUAL = false; 

int exitFlag=0;
sem_t _xmitSema;

/*
char getch() {
	char buf = 0;
	struct termios old = {0};
	if (tcgetattr(0, &old) < 0)
			perror("tcsetattr()");
	old.c_lflag &= ~ICANON;
	old.c_lflag &= ~ECHO;
	old.c_cc[VMIN] = 1;
	old.c_cc[VTIME] = 0;
	if (tcsetattr(0, TCSANOW, &old) < 0)
			perror("tcsetattr ICANON");
	if (read(0, &buf, 1) < 0)
			perror ("read()");
	old.c_lflag |= ICANON;
	old.c_lflag |= ECHO;
	if (tcsetattr(0, TCSADRAIN, &old) < 0)
			perror ("tcsetattr ~ICANON");
	return (buf);
} */

void handleError(TResult error)
{
	switch(error)
	{
		case PACKET_BAD:
			printf("ERROR: Bad Magic Number\n");
			break;

		case PACKET_CHECKSUM_BAD:
			printf("ERROR: Bad checksum\n");
			break;

		default:
			printf("ERROR: UNKNOWN ERROR\n");
	}
}

void handleStatus(TPacket *packet)
{
	printf("\n ------- ALEX STATUS REPORT ------- \n\n");
	printf("Left Forward Ticks:\t\t%d\n", packet->params[0]);
	printf("Right Forward Ticks:\t\t%d\n", packet->params[1]);
	printf("Left Reverse Ticks:\t\t%d\n", packet->params[2]);
	printf("Right Reverse Ticks:\t\t%d\n", packet->params[3]);
	printf("Left Forward Ticks Turns:\t%d\n", packet->params[4]);
	printf("Right Forward Ticks Turns:\t%d\n", packet->params[5]);
	printf("Left Reverse Ticks Turns:\t%d\n", packet->params[6]);
	printf("Right Reverse Ticks Turns:\t%d\n", packet->params[7]);
	printf("Forward Distance:\t\t%d\n", packet->params[8]);
	printf("Reverse Distance:\t\t%d\n", packet->params[9]);
	printf("\n---------------------------------------\n\n");
}

void handleColour(TPacket *packet)
{
	uint32_t red = packet->params[0];
	uint32_t green = packet->params[1];
	uint32_t blue = packet->params[2];
	uint32_t colour = packet->params[3];
	
	printf("\n ------- ALEX COLOUR REPORT ------- \n\n");
	printf("Red(R) freq: \t%d\n", red);
	printf("Green(G) freq: \t%d\n", green);
	printf("Blue(B) freq: \t%d\n", blue);
	printf("not detected / dummy=0, red=1, green=2, invalid=-1\n");
	printf("Colour detected:\t%d\n", colour);
	printf("\n---------------------------------------\n\n");
}

void handleDistance(TPacket *packet)
{
	uint32_t distance = packet->params[0];
	printf("Ultrasonic Distance:\t\t%d cm\n", distance);
	
	const int DIST_THRESHOLD = 25;
	if(distance < DIST_THRESHOLD) printf("WALL NEARBY! SLOW DOWN!\n");
}

void handleResponse(TPacket *packet)
{
	// The response code is stored in command
	switch(packet->command)
	{
		case RESP_OK:
			printf("Command OK\n");
			break;

		case RESP_STATUS:
			handleStatus(packet);
			break;
		
		case RESP_COLOUR:
			handleColour(packet);
			break;
		
		case RESP_DIST:
			handleDistance(packet);
			break;

		default:
			printf("Arduino is confused\n");
	}
}

void handleErrorResponse(TPacket *packet)
{
	// The error code is returned in command
	switch(packet->command)
	{
		case RESP_BAD_PACKET:
			printf("Arduino received bad magic number\n");
		break;

		case RESP_BAD_CHECKSUM:
			printf("Arduino received bad checksum\n");
		break;

		case RESP_BAD_COMMAND:
			printf("Arduino received bad command\n");
		break;

		case RESP_BAD_RESPONSE:
			printf("Arduino received unexpected response\n");
		break;

		default:
			printf("Arduino reports a weird error\n");
	}
}

void handleMessage(TPacket *packet)
{
	printf("Message from Alex: %s\n", packet->data);
}

void handlePacket(TPacket *packet)
{
	switch(packet->packetType)
	{
		case PACKET_TYPE_COMMAND:
				// Only we send command packets, so ignore
			break;

		case PACKET_TYPE_RESPONSE:
				handleResponse(packet);
			break;

		case PACKET_TYPE_ERROR:
				handleErrorResponse(packet);
			break;

		case PACKET_TYPE_MESSAGE:
				handleMessage(packet);
			break;
	}
}

void sendPacket(TPacket *packet)
{
	char buffer[PACKET_SIZE];
	int len = serialize(buffer, packet, sizeof(TPacket));

	serialWrite(buffer, len);
}

void *receiveThread(void *p)
{
	char buffer[PACKET_SIZE];
	int len;
	TPacket packet;
	TResult result;
	int counter=0;

	while(1)
	{
		len = serialRead(buffer);
		counter+=len;
		if(len > 0)
		{
			result = deserialize(buffer, len, &packet);

			if(result == PACKET_OK)
			{
				counter=0;
				handlePacket(&packet);
			}
			else 
				if(result != PACKET_INCOMPLETE)
				{
					printf("PACKET ERROR\n");
					handleError(result);
				}
		}
	}
}

void flushInput()
{
	char c;
	while((c = getchar()) != '\n' && c != EOF);
}

void getParams(TPacket *commandPacket)
{
	printf("Enter distance/angle in cm/degrees and power in %% separated by space.\n");
	scanf("%d %d", &commandPacket->params[0], &commandPacket->params[1]);
	flushInput();
}

void getColour(TPacket *commandPacket)
{
	printf("Enter 1 = redMusic, 2=greenMusic.\n");
	scanf("%d", &commandPacket->params[0]);
	flushInput();
}

void sendCommand(char command)
{
	TPacket commandPacket;
	commandPacket.packetType = PACKET_TYPE_COMMAND;

	switch(command)
	{
		case 'w':
			if(COMMAND_MANUAL) getParams(&commandPacket);
			else{
				commandPacket.params[0] = 5;
				commandPacket.params[1] = 100;
			}
			commandPacket.command = COMMAND_FORWARD;
			sendPacket(&commandPacket);
			break;

		case 's':
			if(COMMAND_MANUAL) getParams(&commandPacket);
			else{
				commandPacket.params[0] = 5;
				commandPacket.params[1] = 100;
			}
			commandPacket.command = COMMAND_REVERSE;
			sendPacket(&commandPacket);
			break;

		case 'a':
			if(COMMAND_MANUAL) getParams(&commandPacket);
			else{
				commandPacket.params[0] = 5;
				commandPacket.params[1] = 100;
			}
			commandPacket.command = COMMAND_TURN_LEFT;
			sendPacket(&commandPacket);
			break;

		case 'd':
			if(COMMAND_MANUAL) getParams(&commandPacket);
			else{
				commandPacket.params[0] = 5;
				commandPacket.params[1] = 100;
			}
			commandPacket.command = COMMAND_TURN_RIGHT;
			sendPacket(&commandPacket);
			break;

		case 'f':
			commandPacket.command = COMMAND_STOP;
			sendPacket(&commandPacket);
			break;

		case 'c':
			commandPacket.command = COMMAND_CLEAR_STATS;
			commandPacket.params[0] = 0;
			sendPacket(&commandPacket);
			break;

		case 'g':
			commandPacket.command = COMMAND_GET_STATS;
			sendPacket(&commandPacket);
			break;

		case 'q':
			exitFlag=1;
			break;
			
		case 'v':
			commandPacket.command = COMMAND_GET_COLOUR;
			sendPacket(&commandPacket);
			break;
		
		case 'b':
			commandPacket.command = COMMAND_DIST;
			sendPacket(&commandPacket);
			break;
		case 'n':
			getColour(&commandPacket);
			commandPacket.command = COMMAND_PLAY_MUSIC;
			sendPacket(&commandPacket);
			break;
			
		case 'r': // move forward for distance 50, power 100 to overcome the ramp
			commandPacket.params[0] = 30;
			commandPacket.params[1] = 100;
			commandPacket.command = COMMAND_FORWARD;
			sendPacket(&commandPacket);
			break;

		case 'm':
			COMMAND_MANUAL = !COMMAND_MANUAL; // toggle
			if(COMMAND_MANUAL) printf("Manual mode activated\n");
			else printf("Auto mode activated\n");	
			break;		

		default:
			printf("Bad command\n");
		// create a spearate command for speeding up the ramp

	}
}

int main()
{
	// Connect to the Arduino
	startSerial(PORT_NAME, BAUD_RATE, 8, 'N', 1, 5);

	// Sleep for two seconds
	printf("WAITING TWO SECONDS FOR ARDUINO TO REBOOT\n");
	sleep(2);
	printf("DONE\n");

	// Spawn receiver thread
	pthread_t recv;

	pthread_create(&recv, NULL, receiveThread, NULL);

	// Send a hello packet
	TPacket helloPacket;

	helloPacket.packetType = PACKET_TYPE_HELLO;
	sendPacket(&helloPacket);

	while(!exitFlag)
	{
		char ch;
		
		printf("Command (w=forward, s=reverse, a=turn left, d=turn right, f=stop, b=get distance, v=get colour, m=manual mode toggle, c=clear stats, g=get stats q=exit)\n");
		scanf("%c", &ch);
		
		// Purge extraneous characters from input stream
		flushInput();

		sendCommand(ch);
	}

	printf("Closing connection to Arduino.\n");
	endSerial();
}
