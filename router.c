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

    rID = atoi(argv[1]);
    hostname = argv[2];
  	ne_udp_port = atoi(argv[3]);
    r_udp_port = atoi(argv[4]);

    // Create UDP socket.
    int nefd, sendto_len, recvfrom_len, sendto_size, recvfrom_size, pkt_size;
    struct hostent *hp;
    struct sockaddr_in serveraddr;

    if ((nefd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      return -1;
    }
    if ((hp = gethostbyname(hostname)) == NULL) {
      return -2;
    }
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr, (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    //memcpy((void *)&serveraddr.sin_addr, hp->h_addr_list[0], hp->h_length);
    serveraddr.sin_port = htonl(ne_udp_port);

    struct pkt_INIT_REQUEST init_request;
    init_request.router_id = htonl(rID);

    sendto_size = sizeof(serveraddr);
    recvfrom_size = sendto_size;
    pkt_size = sizeof(init_request);
    //sigaction(SIGPIPE, &(struct sigaction){SIG_IGN}, NULL);
    sendto_len = sendto(nefd, &init_request, pkt_size, 0, (struct sockaddr *) &serveraddr, sendto_size);
    printf("This is test)");
    //recvfrom_len = recvfrom(sockfd, )

    return 1;
  }
