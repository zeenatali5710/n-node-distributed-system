#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#define PORT 12345
#define IP "225.0.0.37"
using namespace std;

//Variable declaration
void *sender(void *);
void *receiver(void *);
int multicast_sock, ptp_sock;
int logical_clock, process, p, m, send_clock, send_port;
struct sockaddr_in p_addr, port_master, m_addr, m_r_addr;
struct ip_mreq mr;

char * msg_input;
char mesg_bcast[256];

struct message_multicast{
	int processid;
	char rv_msg[256];
	int arr[3];
};

struct message_buffer{
	char msg[256];
	int buffer_arr[30];
};

struct message_multicast message_array[50];
int arr1[4] = {0,0,0,0};
int n = 4;


int main(int argc, char *argv[])
{
	u_int on = 1;
	char message1[256];
	char message2[256];
	char message3[256];
	char message4[256];
	char buffer1[256];
	int m1, m2, m3, m4;
	int initial_value, drift, send_drift;
	unsigned int rank = atoi(argv[1]);
	msg_input = argv[2];
	pthread_t sender_th, receiver_th;


	//multicast socket creation
	multicast_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(multicast_sock < 0)
	{
		cout << "Cannot create socket" << endl;
		return 0;
	}

	//Assigning destination address
	memset((char*) &m_addr, 0, sizeof(m_addr));
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = inet_addr(IP);
	m_addr.sin_port = htons(PORT);

	//Assigning destination address on receiver side
	memset((char*) &m_r_addr, 0, sizeof(m_r_addr));
	m_r_addr.sin_family = AF_INET;
	m_r_addr.sin_addr.s_addr =htonl(INADDR_ANY);
	m_r_addr.sin_port = htons(PORT);

	//Allowing multiple sockets to use the same PORT number
	if(setsockopt(multicast_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
	{
		cout << "Allowing multiple sockets to use the same PORT number failed" << endl;
		return 0;
	}

	//Binding to destination address
	m = bind(multicast_sock, (struct sockaddr *) &m_r_addr, sizeof(m_r_addr));
	if(m < 0)
	{
		cout << "Cannot bind" << endl;
		return 0;
	}

	//use setsockopt() to request the kernel to join the multicast group
	mr.imr_multiaddr.s_addr = inet_addr(IP);
	mr.imr_interface.s_addr = htonl(INADDR_ANY);

	if(setsockopt(multicast_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0)
	{
		cout << "Error in setsockopt" << endl;
		return 0;
	}

	//create point to point socket
	ptp_sock = socket(AF_INET, SOCK_DGRAM, 0);

	//set destination address
	p_addr.sin_family = AF_INET;
	p_addr.sin_addr.s_addr = INADDR_ANY;
	p_addr.sin_port = htons(INADDR_ANY);

	//Request to use same address
	if(setsockopt(ptp_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
	{
		cout << "Cannot reuse address" << endl;
		return 0;
	}

	//Bind point to point socket
	p = bind(ptp_sock, (struct sockaddr *) &p_addr, sizeof(p_addr));
	if(p < 0)
	{
		cout << "Cannot bind point to point socket" << endl;
		return 0;
	}

	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	if(setsockopt(ptp_sock,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(timeout))<0)
	{
	  cout << "Timeout error" << endl;
	}

	//Creating threads for sender and reciever
	if(pthread_create(&sender_th,NULL,sender,&rank)<0)
	{
	  cout << "Cannot create a thread for sender" << endl;
	  return 1;
	}
	if(pthread_create(&receiver_th,NULL,receiver,&rank)<0)
	{
	  cout << "Cannot create a thread for receiver" << endl;
	  return 1;
	}
	pthread_join(sender_th,NULL);

	pthread_join(receiver_th,NULL);
	return 0;
}

//thread for sending the messages to all the processes
void *sender(void *r)
{
	unsigned int rank1 = *((unsigned int *) r);
	int p1 = 0, p2 = 0, p3 = 0, p4 = 0;
	int process;
	cout << "Rank: " << rank1 << endl;
	cout << "Message Input: " << msg_input << endl;
	cout<<"The format of output: [SenderProcessId, Process1Clock, Process2Clock, Process3Clock, Process4Clock, Message]"<<endl;
	while(1)
	{
		for (int i = 0; i < 4; i++)
		{
			if(i == rank1)
			{
				if(i == 0)
				{
					p1++;
					process = 1;
					cout << "Process: " << process;
					arr1[i] = p1;
				}
				if(i == 1)
				{
					p2++;
					process = 2;
					cout << "Process: " << process;
					arr1[i] = p2;
				}
				if(i == 2)
				{
					p3++;
					process = 3;
					cout << "Process: " << process;
					arr1[i] = p3;
				}
				if(i == 3)
				{
					p4++;
					process = 4;
					cout << "Process: " << process;
					arr1[i] = p4;
				}
			}
		}
		char buffer1[256];
		sprintf(buffer1,"%d,%d,%d,%d,%d,%s", process, arr1[0], arr1[1], arr1[2], arr1[3], msg_input);
		cout <<endl<< "Message: " << buffer1 << endl;
		int send1 = sendto(multicast_sock, buffer1, sizeof(buffer1), 0, (struct sockaddr *) &m_addr, sizeof(m_addr));
		sleep(10);
	}
	return 0;
}

//thread for sending the messages from all the processes
void *receiver(void *r1)
{
	unsigned int rank2 = *((unsigned int *) r1);
	char buffer2[256];
	int count = 0;
	int processid;
	char rv_msg[256];
	message_buffer msg_buf;
	int arr[4];
	int num = 0;
	int varr1[4] = {0,0,0,0}, varr2[4] = {0,0,0,0}, varr3[4] = {0,0,0,0}, varr4[4] = {0,0,0,0};
	char *variable, *message, *arr2[4];
	socklen_t addr=sizeof(m_r_addr);
	while(recvfrom(multicast_sock, buffer2, sizeof(buffer2), 0, (struct sockaddr *) &m_r_addr, &addr) >= 0)
	{
		count++;
		cout << endl ;
		cout << "------Causal ordering---------" << endl;
		cout << "Message received: " << buffer2 << endl;
		cout << "Count: " << count << endl;

		message = strtok(buffer2, ",");

		if(message != NULL)
		{
			processid = atoi(message);
			cout << "Processid: " << processid << endl;
			message = strtok(NULL, ",");
			arr[0] = atoi(message);
			message = strtok(NULL, ",");
			arr[1] = atoi(message);
			message = strtok(NULL, ",");
			arr[2] = atoi(message);
			message = strtok(NULL, " ,");
			arr[3] = atoi(message);
			message = strtok(NULL, " ,");
			strcpy(rv_msg, message);
		}
		if(rank2 == 0)
		{
			if (processid == 1)
			{
				cout <<"Receiving Message from Process 2: " << endl;
				if (varr1[1] > arr[1])
				{
					cout << "Message has been buffered" << endl;
					msg_buf.buffer_arr[0] = arr[0];
					msg_buf.buffer_arr[1] = arr[1];
					msg_buf.buffer_arr[2] = arr[2];
					msg_buf.buffer_arr[3] = arr[3];
				}
				else
				{
					varr1[0] = varr1[0] + arr[0];
					varr1[1] = varr1[1] + arr[1];
					varr1[2] = varr1[2] + arr[2];
					varr1[3] = varr1[3] + arr[3];
					cout << endl ;
					cout << "Message Delivered" << endl;
					cout <<"******Vector value: " << varr1[0] << "," << varr1[1] <<"," << varr1[2]<<"," << varr1[3] << endl;
				}
			}
			else if (processid == 2)
			{
				cout <<"Receiving Message from Process 3: " << endl;
				if (varr1[2] > arr[2])
				{
					cout << "Message has been buffered" << endl;
					msg_buf.buffer_arr[0] = arr[0];
					msg_buf.buffer_arr[1] = arr[1];
					msg_buf.buffer_arr[2] = arr[2];
					msg_buf.buffer_arr[3] = arr[3];
				}
				else
				{
					varr1[0] = varr1[0] + arr[0];
					varr1[1] = varr1[1] + arr[1];
					varr1[2] = varr1[2] + arr[2];
					varr1[2] = varr1[2] + arr[2];
					cout << endl ;
					cout << "Message Delivered" << endl;
					cout <<"******Vector value: " << varr1[0] << "," << varr1[1] <<"," << varr1[2]<<"," << varr1[3] << endl;
				}
			}
			else if (processid == 3)
			{
				cout <<"Receiving Message from Process 4: " << endl;
				if (varr1[3] > arr[3])
				{
					cout << "Message has been buffered" << endl;
					msg_buf.buffer_arr[0] = arr[0];
					msg_buf.buffer_arr[1] = arr[1];
					msg_buf.buffer_arr[2] = arr[2];
					msg_buf.buffer_arr[3] = arr[3];
				}
				else
				{
					varr1[0] = varr1[0] + arr[0];
					varr1[1] = varr1[1] + arr[1];
					varr1[2] = varr1[2] + arr[2];
					varr1[2] = varr1[2] + arr[2];
					cout << endl ;
					cout << "Message Delivered" << endl;
					cout <<"******Vector value: " << varr1[0] << "," << varr1[1] <<"," << varr1[2]<<"," << varr1[3] << endl;
				}
			}
		}

		if(rank2 == 1)
		{
			if (processid == 0)
			{
				cout <<"Receiving Message from Process 1: " << endl;
				if (varr1[0] > arr[0])
				{
					cout << "Message has been buffered" << endl;
					msg_buf.buffer_arr[0] = arr[0];
					msg_buf.buffer_arr[1] = arr[1];
					msg_buf.buffer_arr[2] = arr[2];
					msg_buf.buffer_arr[3] = arr[3];
				}
				else
				{
					varr1[0] = varr1[0] + arr[0];
					varr1[1] = varr1[1] + arr[1];
					varr1[2] = varr1[2] + arr[2];
					varr1[3] = varr1[3] + arr[3];
					cout << endl ;
					cout << "Message Delivered" << endl;
					cout <<"******Vector value: " << varr1[0] << "," << varr1[1] <<"," << varr1[2]<<"," << varr1[3] << endl;
				}
			}
			else if (processid == 2)
			{
				cout <<"Receiving Message from Process 3: " << endl;
				if (varr1[2] > arr[2])
				{
					cout << "Message has been buffered" << endl;
					msg_buf.buffer_arr[0] = arr[0];
					msg_buf.buffer_arr[1] = arr[1];
					msg_buf.buffer_arr[2] = arr[2];
					msg_buf.buffer_arr[3] = arr[3];
				}
				else
				{
					varr1[0] = varr1[0] + arr[0];
					varr1[1] = varr1[1] + arr[1];
					varr1[2] = varr1[2] + arr[2];
					varr1[3] = varr1[3] + arr[3];
					cout << endl ;
					cout << "Message Delivered" << endl;
					cout <<"******Vector value: " << varr1[0] << "," << varr1[1] <<"," << varr1[2]<<"," << varr1[3] << endl;
				}
			}
			else if (processid == 3)
			{
				cout <<"Receiving Message from Process 4: " << endl;
				if (varr1[3] > arr[3])
				{
					cout << "Message has been buffered" << endl;
					msg_buf.buffer_arr[0] = arr[0];
					msg_buf.buffer_arr[1] = arr[1];
					msg_buf.buffer_arr[2] = arr[2];
					msg_buf.buffer_arr[3] = arr[3];
				}
				else
				{
					varr1[0] = varr1[0] + arr[0];
					varr1[1] = varr1[1] + arr[1];
					varr1[2] = varr1[2] + arr[2];
					varr1[3] = varr1[3] + arr[3];
					cout << endl ;
					cout << "Message Delivered" << endl;
					cout <<"******Vector value: " << varr1[0] << "," << varr1[1] <<"," << varr1[2]<<"," << varr1[3] << endl;
				}
			}
		}

		if(rank2 == 2)
		{
			if (processid == 0)
			{
				cout <<"Receiving Message from Process 1: " << endl;
				if (varr1[0] > arr[0])
				{
					cout << "Message has been buffered" << endl;
					msg_buf.buffer_arr[0] = arr[0];
					msg_buf.buffer_arr[1] = arr[1];
					msg_buf.buffer_arr[2] = arr[2];
					msg_buf.buffer_arr[3] = arr[3];
				}
				else
				{
					varr1[0] = varr1[0] + arr[0];
					varr1[1] = varr1[1] + arr[1];
					varr1[2] = varr1[2] + arr[2];
					varr1[3] = varr1[3] + arr[3];
					cout << endl ;
					cout << "Message Delivered" << endl;
					cout <<"******Vector value: " << varr1[0] << "," << varr1[1] <<"," << varr1[2]<<"," << varr1[3] << endl;
				}
			}
			else if (processid == 1)
			{
				cout <<"Receiving Message from Process 2: " << endl;
				if (varr1[1] > arr[1])
				{
					cout << "Message has been buffered" << endl;
					msg_buf.buffer_arr[0] = arr[0];
					msg_buf.buffer_arr[1] = arr[1];
					msg_buf.buffer_arr[2] = arr[2];
					msg_buf.buffer_arr[3] = arr[3];
				}
				else
				{
					varr1[0] = varr1[0] + arr[0];
					varr1[1] = varr1[1] + arr[1];
					varr1[2] = varr1[2] + arr[2];
					varr1[3] = varr1[3] + arr[3];
					cout << endl ;
					cout << "Message Delivered" << endl;
					cout <<"******Vector value: " << varr1[0] << "," << varr1[1] <<"," << varr1[2] <<"," << varr1[3] << endl;
				}
			}
			else if (processid == 3)
			{
				cout <<"Receiving Message from Process 4: " << endl;
				if (varr1[3] > arr[3])
				{
					cout << "Message has been buffered" << endl;
					msg_buf.buffer_arr[0] = arr[0];
					msg_buf.buffer_arr[1] = arr[1];
					msg_buf.buffer_arr[2] = arr[2];
					msg_buf.buffer_arr[3] = arr[3];
				}
				else
				{
					varr1[0] = varr1[0] + arr[0];
					varr1[1] = varr1[1] + arr[1];
					varr1[2] = varr1[2] + arr[2];
					varr1[3] = varr1[3] + arr[3];
					cout << endl ;
					cout << "Message Delivered" << endl;
					cout <<"******Vector value: " << varr1[0] << "," << varr1[1] <<"," << varr1[2] <<"," << varr1[3] << endl;
				}
			}
		}
		if(rank2 == 3)
		{
			if (processid == 0)
			{
				cout <<"Receiving Message from Process 1: " << endl;
				if (varr1[0] > arr[0])
				{
					cout << "Message has been buffered" << endl;
					msg_buf.buffer_arr[0] = arr[0];
					msg_buf.buffer_arr[1] = arr[1];
					msg_buf.buffer_arr[2] = arr[2];
					msg_buf.buffer_arr[3] = arr[3];
				}
				else
				{
					varr1[0] = varr1[0] + arr[0];
					varr1[1] = varr1[1] + arr[1];
					varr1[2] = varr1[2] + arr[2];
					varr1[3] = varr1[3] + arr[3];
					cout << endl ;
					cout << "Message Delivered" << endl;
					cout <<"******Vector value: " << varr1[0] << "," << varr1[1] <<"," << varr1[2]<<"," << varr1[3] << endl;
				}
			}
			else if (processid == 1)
			{
				cout <<"Receiving Message from Process 2: " << endl;
				if (varr1[1] > arr[1])
				{
					cout << "Message has been buffered" << endl;
					msg_buf.buffer_arr[0] = arr[0];
					msg_buf.buffer_arr[1] = arr[1];
					msg_buf.buffer_arr[2] = arr[2];
					msg_buf.buffer_arr[3] = arr[3];
				}
				else
				{
					varr1[0] = varr1[0] + arr[0];
					varr1[1] = varr1[1] + arr[1];
					varr1[2] = varr1[2] + arr[2];
					varr1[3] = varr1[3] + arr[3];
					cout << endl ;
					cout << "Message Delivered" << endl;
					cout <<"******Vector value: " << varr1[0] << "," << varr1[1] <<"," << varr1[2] <<"," << varr1[3] << endl;
				}
			}
			else if (processid == 2)
			{
				cout <<"Receiving Message from Process 3: " << endl;
				if (varr1[2] > arr[2])
				{
					cout << "Message has been buffered" << endl;
					msg_buf.buffer_arr[0] = arr[0];
					msg_buf.buffer_arr[1] = arr[1];
					msg_buf.buffer_arr[2] = arr[2];
					msg_buf.buffer_arr[3] = arr[3];
				}
				else
				{
					varr1[0] = varr1[0] + arr[0];
					varr1[1] = varr1[1] + arr[1];
					varr1[2] = varr1[2] + arr[2];
					varr1[3] = varr1[3] + arr[3];
					cout << endl ;
					cout << "Message Delivered" << endl;
					cout <<"******Vector value: " << varr1[0] << "," << varr1[1] <<"," << varr1[2] <<"," << varr1[3] << endl;
				}
			}
		}
		sleep(10);
		bzero(buffer2, 256);
	}
}


