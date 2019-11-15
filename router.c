#include "ne.h"
#include "router.h"
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <string.h>

/* ----- Function Declarations ----- */
void *timer_thread(void *arguments);
void *udp_thread(void *arguments);
int update_tim_last_update(int yourID);
void update_nbr_info(struct pkt_INIT_RESPONSE *InitResponse);

/* ----- GLOBAL VARIABLES ----- */
int nefd, rID, tim_update_interval, tim_converge_interval;
struct sockaddr_in serveraddr;
pthread_mutex_t lock;
int print_permission = 1;
FILE *fptr = NULL;

/* Create a struct to store the info of neighbors to the router.
   This will be used for sending update messages. */
struct neighbor_info {
	unsigned int nID;
	int cost;
	int tim_last_update;
};

int no_nbrs = 0;
struct neighbor_info neighbors[MAX_ROUTERS];


// NOTES - for personal use
/*
   1) Send pkt_INIT_REQUEST
   2) Waits until receiving pkt_INIT_RESPONSE
   3) Periodically, eery UPDATE_INTERVAL seconds, send RT_UPDATE to neighbors
   4) Use threads to monitor UDP file descriptor and implement time based functionality

   Instantiate threads after RESPONSE packet, UDP and timer

   in UDP
   wait to receive RT_UPDATE
   upon receiving, call UpdateRoutes
   when receiving routes, it is important to track the time of the last update

   in Timer
   monitor all time based constraints and implement timers to control frequency and convergence
   when UPDATE_INTERVAL expires, call ConvertTabletoPkt and send RT_UPDATE
   if neighbor does not send for x seconds, consider DeadNbr
   Check for convergence with no change after CONVERGE_TIMEOUT seconds

   data will be shared between threads, use mutex's

   Whenever table changes, must produce log file
   if it has converged, append converged

   ./router <router ID> <ne hostname> <ne UDP port> <router UDP port>


   UpdateRoutes returns 0 or 1 for a changed routingTable
   If 1, restart the clock
   If 0, don't

   What if multiple UDP messages come in at the same time?

*/

int open_listenfd_udp(int port)
{
	int n = 0;
	int listenfd, optval=1;
	struct sockaddr_in serveraddr_local;

	/* Create a socket descriptor */
	if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;

	/* Eliminates "Address already in use" error from bind. */
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
				(const void *)&optval , sizeof(int)) < 0)
		return -1;

	/* Listenfd will be an endpoint for all requests to port
	   on any IP address for this host */
	bzero((char *) &serveraddr_local, sizeof(serveraddr_local));
	serveraddr_local.sin_family = AF_INET;
	serveraddr_local.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr_local.sin_port = htons((unsigned short)port);
	if ((n=bind(listenfd, (struct sockaddr *)&serveraddr_local, sizeof(serveraddr_local))) < 0)
	{
		if (n == -1)
		{
			perror("Error binding to specified port!");
			return EXIT_FAILURE;
		}
		return -1;
	}

	return listenfd;
}


int main (int argc, char *argv[])
{
	if (argc != 5) {
		printf("\nERROR: ");
		printf("Please use the following format:\n\n");
		printf("./router <router ID> <ne hostname> <ne UDP port> <router UDP port>\n\n");
		return 0;
	}

	/* ----- Declare Variables ----- */
	int ne_udp_port, r_udp_port;
	char *hostname;
	int sendto_size, pkt_size;
	struct hostent *hp;
	socklen_t recvfrom_size;
	struct sockaddr_in recvaddr;
	struct pkt_INIT_RESPONSE init_response;
	struct pkt_INIT_REQUEST init_request;
	pthread_t udp_thread_id;
	pthread_t timer_thread_id;

	/* ----- Parse Input Arguments ----- */
	rID = atoi(argv[1]);          // Router ID.
	hostname = argv[2];           // Server name.
	ne_udp_port = atoi(argv[3]);  // UDP Port for Network Emulator.
	r_udp_port = atoi(argv[4]);   //  UDP Port for Router.


	/* ----- 1) Create socket ----- */
	/* ----- 2) Initialize contact with NE  ----- */
	/* ----- 3) Start threads ----- */


	// 1) Create UDP socket, bind router to given port number.
	nefd = open_listenfd_udp(r_udp_port);
	/* ----- END 1 ----- */


	// 2.a) Set up struct sockaddr_in with NE contact info.
	if ((hp = gethostbyname(hostname)) == NULL) {
		return -2;
	}
	bzero( &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(ne_udp_port);
	memcpy((void *) &serveraddr.sin_addr, hp->h_addr_list[0], hp->h_length);
	/* ----- END 2.a ----- */


	// 2.b) Set up packet for initial request.
	bzero(&init_request, sizeof(init_request));
	init_request.router_id = htonl(rID); //htons(rID);
	/* ----- END 2.b ----- */


	// 2.c) Contact NE.
	sendto_size = sizeof(serveraddr);
	pkt_size = sizeof(init_request);
	sendto(nefd, &init_request, pkt_size, 0, (struct sockaddr *) &serveraddr, sendto_size);
	/* ----- END 2.c ----- */


	// 2.d) Receive update packet from NE.  Interpret response.
	pkt_size = sizeof(init_response);
	bzero(&recvaddr, sizeof(recvaddr));
	recvfrom_size = sizeof(recvaddr);
	recvfrom(nefd, &init_response, pkt_size, 0, (struct sockaddr *) &recvaddr, &recvfrom_size);

	ntoh_pkt_INIT_RESPONSE(&init_response);
	InitRoutingTbl(&init_response, rID);

	update_nbr_info(&init_response);
	/* ----- END 2.d ----- */

	char filename[12] = "router0.log\0";
	filename[6] = (char) (filename[6] + rID);
	fptr = fopen(filename, "w");

	PrintRoutes(fptr, rID);

	// 3) Setup variables and begin multi-threading.
	tim_update_interval = time(NULL);
	tim_converge_interval = time(NULL);

	pthread_mutex_init(&lock, NULL);


	if(pthread_create(&udp_thread_id, NULL, udp_thread, NULL)){
		perror("Error creating thread for UDP monitoring!");
		return EXIT_FAILURE;
	}

	if(pthread_create(&timer_thread_id, NULL, timer_thread, NULL)){
		perror("Error creating thread for timer handling!");
		return EXIT_FAILURE;
	}

	pthread_join(udp_thread_id, NULL);
	pthread_join(timer_thread_id, NULL);

	fclose(fptr);
	return 1;
}


void *udp_thread(void *arguments) {

	int nbr_cost, update_flag;
	struct sockaddr_in recvaddr_local;
	socklen_t recvfrom_size_local;
	struct pkt_RT_UPDATE pkt_update_in_local;
	int update_size_local = sizeof(pkt_update_in_local);
	pthread_mutex_lock(&lock);
	int nefd_local = nefd;
	pthread_mutex_unlock(&lock);
	bzero(&recvaddr_local, sizeof(recvaddr_local));
	recvfrom_size_local = sizeof(recvaddr_local);

	while(1) {
		recvfrom(nefd_local, &pkt_update_in_local, update_size_local, 0, (struct sockaddr *) &recvaddr_local, &recvfrom_size_local);
		pthread_mutex_lock(&lock);
		ntoh_pkt_RT_UPDATE (&pkt_update_in_local);
		nbr_cost = update_tim_last_update(pkt_update_in_local.sender_id);
		update_flag = UpdateRoutes(&pkt_update_in_local, nbr_cost, rID);
		if (update_flag) {
			PrintRoutes(fptr, rID);
			fflush(fptr);
			tim_converge_interval = time(NULL);
			print_permission = 1;
		}

		pthread_mutex_unlock(&lock);
	}

	return NULL;
}


void *timer_thread(void *arguments) {
	// Need neighbors and costs
	// timer for each neighbor
	struct pkt_RT_UPDATE pkt_update_out;
	int update_size = sizeof(pkt_update_out);
	int sendto_size;
	sendto_size = sizeof(serveraddr);
	int DeadRouters[MAX_ROUTERS];
	int i = 0;
	for (i = 0; i < MAX_ROUTERS; i++)
		DeadRouters[i] = 0;

	while (1) {

		// Update Interval Checking
		pthread_mutex_lock(&lock);
		if ((time(NULL) - tim_update_interval) >= UPDATE_INTERVAL) {
			for (i = 0; i < no_nbrs; i++) {
				bzero(&pkt_update_out, sizeof(pkt_update_out));
				ConvertTabletoPkt(&pkt_update_out, rID);
				pkt_update_out.dest_id = neighbors[i].nID;
				hton_pkt_RT_UPDATE (&pkt_update_out);
				sendto(nefd, &pkt_update_out, update_size, 0, (struct sockaddr *) &serveraddr, sendto_size);
				tim_update_interval = time(NULL);
			}
		}
		pthread_mutex_unlock(&lock);

		// Neighbor Death Checking
		pthread_mutex_lock(&lock);
		for (i=0; i < no_nbrs; i++) {
			if ((time(NULL) - neighbors[i].tim_last_update) > FAILURE_DETECTION) {
				UninstallRoutesOnNbrDeath(neighbors[i].nID);
				if (DeadRouters[i] == 0)
				{
					PrintRoutes(fptr, rID);
					DeadRouters[i] = 1;
					print_permission = 1;
					tim_converge_interval = time(NULL);
				}
			}
			else
				DeadRouters[i] = 0;
			// Have to remove from neighbor table and add locks
		}
		pthread_mutex_unlock(&lock);

		pthread_mutex_lock(&lock);
		// Convergence Checking
		if (((time(NULL) - tim_converge_interval) > CONVERGE_TIMEOUT) && print_permission) {
			printf("Converged\n");
			//PrintRoutes(fptr, rID);
			fprintf(fptr, "%d:Converged\n", (int) time(NULL) - tim_converge_interval);
			fflush(fptr);
			tim_converge_interval = time(NULL);
			print_permission = 0;
		}
		pthread_mutex_unlock(&lock);
	}

	return NULL;
}


int update_tim_last_update(int yourID) {
	int i;
	for (i = 0; i < no_nbrs; i++) {
		if (yourID == neighbors[i].nID) {
			neighbors[i].tim_last_update = time(NULL);
			return neighbors[i].cost;
		}
	}
	return 0;
}


void update_nbr_info(struct pkt_INIT_RESPONSE *InitResponse) {
	int i;
	for (i = 0; i < InitResponse->no_nbr; i++) {
		int routerID = InitResponse->nbrcost[i].nbr;
		neighbors[i].tim_last_update = time(NULL);
		neighbors[i].nID = routerID;
		neighbors[i].cost = InitResponse->nbrcost[i].cost;
	}
	no_nbrs = InitResponse->no_nbr;
	return ;
}
