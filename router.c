#include "ne.h"
#include "router.h"

//#include <signal.h>


/* ----- GLOBAL VARIABLES ----- */

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

  data will be shared between threads, use mutex's+

  Whenever table changes, must produce log file
  if it has converged, append converged

  ./router <router ID> <ne hostname> <ne UDP port> <router UDP port>
  */

  int open_listenfd_udp(int port)
  {
  	int n = 0;
  	int listenfd, optval=1;
  	struct sockaddr_in serveraddr;

  	/* Create a socket descriptor */
  	if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  		return -1;

  	/* Eliminates "Address already in use" error from bind. */
  	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
  				(const void *)&optval , sizeof(int)) < 0)
  		return -1;

  	/* Listenfd will be an endpoint for all requests to port
  	   on any IP address for this host */
  	bzero((char *) &serveraddr, sizeof(serveraddr));
  	serveraddr.sin_family = AF_INET;
  	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  	serveraddr.sin_port = htons((unsigned short)port);
  	if ((n=bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0)
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

    int rID, ne_udp_port, r_udp_port;
    char *hostname;

    rID = atoi(argv[1]);          // Router ID.
    hostname = argv[2];           // Server name.
  	ne_udp_port = atoi(argv[3]);  // UDP Port for Network Emulator.
    r_udp_port = atoi(argv[4]);   //  UDP Port for Router.

    // Create UDP socket, and send pkt_INIT_REQUEST
    int nefd, sendto_len, recvfrom_len, sendto_size, pkt_size;
    socklen_t recvfrom_size;
    struct hostent *hp;
    struct sockaddr_in serveraddr, recvaddr;

    // Create socket.
    /*if ((nefd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      return -1;
    }*/
    // Get server IP by name (localhost).
    if ((hp = gethostbyname(hostname)) == NULL) {
      return -2;
    }

    // Prepare UDP sendto().

    nefd = open_listenfd_udp(r_udp_port);

    bzero( &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(ne_udp_port);
    memcpy((void *) &serveraddr.sin_addr, hp->h_addr_list[0], hp->h_length);

    struct pkt_INIT_REQUEST init_request;
    init_request.router_id = htonl(rID); //htons(rID);

    sendto_size = sizeof(serveraddr);
    recvfrom_size = sendto_size;
    pkt_size = sizeof(init_request);

    sendto_len = sendto(nefd, &init_request, pkt_size, 0, (struct sockaddr *) &serveraddr, sendto_size);

    struct pkt_INIT_RESPONSE init_response;
    pkt_size = sizeof(init_response);
    recvfrom_len = recvfrom(nefd, &init_response, pkt_size, 0, (struct sockaddr *) &recvaddr, &recvfrom_size);

    ntoh_pkt_INIT_RESPONSE(&init_response);
    InitRoutingTbl(&init_response, rID);
    printf("This is a test");

    return 1;
  }
