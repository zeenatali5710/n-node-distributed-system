#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <thread>
#include <sys/socket.h>
#include <ctime>
#include <sys/shm.h>
#include <fcntl.h>
#include <semaphore.h>
#include <fstream>
#include <cstring>
#include <sstream>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#define PORT 5000
#define IP "225.0.0.37"
using namespace std;


int main(int argc, char *argv[])
{

    //(Bonus assignment) - Distributed Locking
	pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
	cout <<"------Read the file--------" << endl;
	//Acquire lock
	pthread_mutex_lock(&mut);
	int counterValue;
	FILE *counterRead = fopen("counter.txt","r+");
	if(counterRead == NULL)
	{
		cout << "File cannot be opened" << endl;
		return 0;
	}
	else
	{
		char buffer[256];
		fgets(buffer,sizeof(buffer),counterRead);
		counterValue = atoi(buffer);
		cout << "Current counter value: " << counterValue << endl;
		counterValue++;
	}
	fclose(counterRead);
	FILE *counterWrite = fopen("counter.txt","w+");
	if(counterWrite == NULL)
	{
		cout << "File cannot be opened" << endl;
		return 0;
	}
	else
	{
		fprintf(counterWrite, "%d\n",counterValue);
		cout << "Updated counter value: " << counterValue << endl;
	}
	fclose(counterWrite);
	//Release lock
	pthread_mutex_unlock(&mut);
	cout <<"-------File is updated-------" << endl;

    // Assignment#1 -  Clock Synchronization using Berkeley Algorithm
    //Variable declaration
	srand(time(NULL));
	int multicast_sock, ptp_sock;
	int logical_clock, process, p, m, send_clock, send_port;
	struct sockaddr_in p_addr, port_master, m_addr, m_r_addr;
	struct ip_mreq mreq;
	u_int on = 1;
	char message1[256];
	char message2[256];
	char message3[256];
	char message4[256];
	char buffer1[256];
	int m1, m2, m3;
	int initial_value, drift, send_drift;

	srand(time(NULL));
	logical_clock = rand()%100;

	// Multicast socket creation
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

	//Assigning destination address on receiver/client side
	memset((char*) &m_r_addr, 0, sizeof(m_r_addr));
	m_r_addr.sin_family = AF_INET;
	m_r_addr.sin_addr.s_addr =htonl(INADDR_ANY);
	m_r_addr.sin_port = htons(PORT);

	//Multiple sockets can use same PORT no.
	if(setsockopt(multicast_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
	{
		cout << "Multiple sockets using same PORT no. failed" << endl;
		return 0;
	}

	//Bind multicast socket to destination address
	m = bind(multicast_sock, (struct sockaddr *) &m_r_addr, sizeof(m_r_addr));
	if(m < 0)
	{
		cout << "Binding failed" << endl;
		return 0;
	}

	//setsockopt() allows kernel to join multicast group
	mreq.imr_multiaddr.s_addr = inet_addr(IP);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);

	if(setsockopt(multicast_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
	{
		cout << "setsockopt set up failed" << endl;
		return 0;
	}

	//Point to point socket creation
	ptp_sock = socket(AF_INET, SOCK_DGRAM, 0);

	//Assigning destination address
	p_addr.sin_family = AF_INET;
	p_addr.sin_addr.s_addr = INADDR_ANY;
	p_addr.sin_port = htons(INADDR_ANY);

	//Request to use same address
	if(setsockopt(ptp_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
	{
		cout << "Reusing address failed" << endl;
		return 0;
	}

	//Bind point to point socket
	p = bind(ptp_sock, (struct sockaddr *) &p_addr, sizeof(p_addr));
	if(p < 0)
	{
		cout << "Cannot bind point to point socket" << endl;
		return 0;
	}
	socklen_t sa_len;

	//Send the logical clock and port number of master
	if(argc == 2)
	{
		process = 0;
		cout << endl;
		cout << "-----Clock Synchronizing-----" << endl;
		cout << "Master Logical Clock: " << logical_clock << endl;

		//stores the int value in char
		char buffer[256];
		sprintf(buffer, "%d", logical_clock);
		sa_len = sizeof(port_master);
		int sockname = getsockname(ptp_sock,(struct sockaddr *)&port_master,&sa_len);

		//Send master clock time to all the processes
		send_clock = sendto(multicast_sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &m_addr, sizeof(m_addr));

		//store the port number of master process
		int master_port = (int)ntohs(port_master.sin_port);
		sprintf(buffer1, "%d", master_port);

		//Send master port number to all the processes
		send_clock = sendto(multicast_sock, buffer1, sizeof(buffer1), 0, (struct sockaddr *) &m_addr, sizeof(m_addr));
	}


	//Receive master clock
	socklen_t addr = sizeof(m_r_addr);
	if(argc == 3)
	{
		if((m1 = recvfrom(multicast_sock, message1, sizeof(message1), 0, (struct sockaddr *) &m_r_addr, &addr)))
		{
			cout << endl << endl;
			cout << "-----Clock Synchronizing-----" << endl;
			cout << "Master Logical Clock : " << message1 << endl;
			process = atoi(argv[2]);
			cout << "Logical Clock of process " << process << " is: " << logical_clock << endl;
		}
	}

	//Receive master port number
	if(argc == 3)
	{
		if((m1 = recvfrom(multicast_sock, message2, sizeof(message2), 0, (struct sockaddr *) &m_r_addr, &addr)))
		{
			cout << "Master Port number : "<< message2 << endl;
		}
	}

	if(argc == 2)
	{
		sprintf(message2, "%s", buffer1);
		cout << "Master Port number : " << message2 << endl;
	}
	//send drift to master
	if(argc == 3)
	{
		initial_value = atoi(message1);
		drift = logical_clock - initial_value;
		p_addr.sin_port = htons(atoi(message2));
		sprintf(message3, "%d", drift);
		send_drift = sendto(ptp_sock, message3, sizeof(message3), 0, (struct sockaddr *) &p_addr, sizeof(p_addr));
		cout << "Drift: " << message3 << endl;
	}

	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	if(setsockopt(ptp_sock,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(timeout))<0)
	{
	  cout << "Timeout error" << endl;
	}
    //Berkeley algorithm - take average of time difference
	char buffer2[256];
	int sum = 0, average = 0, NoOfProcesses = 0, send_updatedclock;
	socklen_t addr1 = sizeof(p_addr);
	if(argc == 2)
	{
		while((recvfrom(ptp_sock, buffer2, sizeof(buffer2), 0, (struct sockaddr *) &p_addr, &addr1)) >= 0)
		{
			sum = sum + atoi(buffer2);
			NoOfProcesses++;
		}
		average = sum/(NoOfProcesses + 1);
		cout << "Total number of processes: " << (NoOfProcesses + 1) << endl;
		logical_clock = logical_clock + average;
		cout << "Updated value of logical clock of master: " << logical_clock << endl;
		char buffer3[256];
		sprintf(buffer3, "%d", logical_clock);
		send_updatedclock = sendto(multicast_sock, buffer3, sizeof(buffer3), 0, (struct sockaddr *) &m_addr, sizeof(m_addr));
		cout << "----Sent the updated logical clock of master to all processes----" << endl;
	}

	//receive master updated logical clock value
	if(argc == 3)
	{
		if((m1 = recvfrom(multicast_sock, message4, sizeof(message4), 0, (struct sockaddr *) &m_r_addr, &addr)))
		{
			int drift1 = atoi(message4) - logical_clock;
			logical_clock = logical_clock + drift1;
			cout << "Updated Logical clock of process " << process << " is: " << logical_clock << endl;
			cout << "--------Clock synchronization done-------" << endl;
		}
	}
}
