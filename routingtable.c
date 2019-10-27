#include "ne.h"
#include "router.h"


/* ----- GLOBAL VARIABLES ----- */
struct route_entry routingTable[MAX_ROUTERS];
int NumRoutes;


////////////////////////////////////////////////////////////////
void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID){
	/* ----- YOUR CODE HERE ----- */
	// Set route to itself with cost 0
	InitResponse->nbrcost[InitResponse->no_nbr].nbr = myID;
	InitResponse->nbrcost[InitResponse->no_nbr].cost = 0;

	// Initialize dest_id to -1 in case it isnt used
	for (int i = 0; i < MAX_ROUTERS; i++)
	{
		routingTable[i].dest_id = -1;
	}

	// Initialize router table
	for (int i = 0; i <= InitResponse->no_nbr; i++)
	{
		int routerID = InitResponse->nbrcost[i].nbr;

		routingTable[routerID].dest_id = InitResponse->nbrcost[i].nbr;
		routingTable[routerID].next_hop = InitResponse->nbrcost[i].nbr;
		routingTable[routerID].cost = InitResponse->nbrcost[i].cost;
		routingTable[routerID].path_len = InitResponse->nbrcost[i].cost;
		routingTable[routerID].path[0] = InitResponse->nbrcost[i].nbr;
	}

	// Update NumRoutes
	NumRoutes = InitResponse->no_nbr;

	return;
}


////////////////////////////////////////////////////////////////
int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID){
	/* ----- YOUR CODE HERE ----- */
	int route_number = NumRoutes;
	int count = 0;
	struct route_entry re;
	int upd_id;


	for (int i = 0; i < MAX_ROUTERS; i++) {
		re = RecvdUpdatePacket->route_entry[i]
		if (re.dest_id != -1) { // Update packet has logical value for this ID.
			upd_id = re.dest_id;
			if (routingTable[upd_id].dest_id == -1)  {
				// Entry is new to router
				// So ... add to router
				memcpy(&routingTable[upd_id], re, sizeof(route_entry));
			}
			else {
				// check for shorter path
				continue ; // CHANGE THIS LATER
			}
		}
	}
	if (NumRoutes == route_number) { return 0; }
	return 1;


}
/*
struct pkt_RT_UPDATE {
  unsigned int sender_id; // id of router sending the message
  unsigned int dest_id; // id of neighbor router to which routing table is sent
  unsigned int no_routes; // number of routes in my routing table
  struct route_entry route[MAX_ROUTERS]; // array containing rows of routing table
};

struct route_entry {
  unsigned int dest_id; /* destination router id
  unsigned int next_hop; /* next hop on the shortest path to dest_id
  unsigned int cost; /* cost to desintation router
#ifdef PATHVECTOR
  unsigned int path_len; /* length of loop-free path to dest_id, eg: with path R1 -> R2 -> R3, the length is 3; self loop R0 -> R0 is length 1
  unsigned int path[MAX_PATH_LEN]; /* array containing id's of routers along the path, this includes the source node, all intermediate nodes, and the destination node; self loop R0 -> R0 should only contain one instance of R0 in path
#endif
};

*/

////////////////////////////////////////////////////////////////
void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID){
	/* ----- YOUR CODE HERE ----- */

	UpdatePacketToSend->sender_id = myID;
	UpdatePacketToSend->no_routes = NumRoutes;

	for (int i = 0; i < MAX_ROUTERS; i++)
		UpdatePacketToSend->route[i] = routingTable[i];

	return;
}


////////////////////////////////////////////////////////////////
//It is highly recommended that you do not change this function!
void PrintRoutes (FILE* Logfile, int myID){
	/* ----- PRINT ALL ROUTES TO LOG FILE ----- */
	int i;
	int j;
	for(i = 0; i < NumRoutes; i++){
		fprintf(Logfile, "<R%d -> R%d> Path: R%d", myID, routingTable[i].dest_id, myID);

		/* ----- PRINT PATH VECTOR ----- */
		for(j = 1; j < routingTable[i].path_len; j++){
			fprintf(Logfile, " -> R%d", routingTable[i].path[j]);
		}
		fprintf(Logfile, ", Cost: %d\n", routingTable[i].cost);
	}
	fprintf(Logfile, "\n");
	fflush(Logfile);
}


////////////////////////////////////////////////////////////////
void UninstallRoutesOnNbrDeath(int DeadNbr){
	/* ----- YOUR CODE HERE ----- */
	return;
}
