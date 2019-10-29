#include "ne.h"
#include "router.h"


/* ----- GLOBAL VARIABLES ----- */
struct route_entry routingTable[MAX_ROUTERS];
int NumRoutes;


////////////////////////////////////////////////////////////////
void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID){
	/* ----- YOUR CODE HERE ----- */
	// Set route to itself with cost 0

	//InitResponse->nbrcost[InitResponse->no_nbr].nbr = myID;
	//InitResponse->nbrcost[InitResponse->no_nbr].cost = 0;

	// Initialize router table
	for (int i = 0; i < InitResponse->no_nbr; i++)
	{
		int routerID = InitResponse->nbrcost[i].nbr;

		routingTable[routerID].dest_id = InitResponse->nbrcost[i].nbr;
		routingTable[routerID].next_hop = InitResponse->nbrcost[i].nbr;
		routingTable[routerID].cost = InitResponse->nbrcost[i].cost;
		routingTable[routerID].path_len = 2;
		routingTable[routerID].path[0] = myID;
		routingTable[routerID].path[1] = InitResponse->nbrcost[i].nbr;
	}

	routingTable[myID].dest_id = myID;
	routingTable[myID].next_hop = myID;
	routingTable[myID].cost = 0;
	routingTable[myID].path_len = 1;
	routingTable[myID].path[0] = myID;

	// Update NumRoutes
	NumRoutes = InitResponse->no_nbr + 1;

	return;
}


////////////////////////////////////////////////////////////////
int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID){
	/* ----- YOUR CODE HERE ----- */
	//int route_number = NumRoutes;
	struct route_entry routeEntry;
	//int upd_id;
	int routingTableChange = 0;


	for (int i = 0; i < RecvdUpdatePacket->no_routes; i++)
	{
		routeEntry = RecvdUpdatePacket->route[i];

		// Split Horizon
		int split = 0;
		for (int i = 0; i < routeEntry.path_len; i++)
		{
			if (routeEntry.path[i] == myID)
				split = 1;
		}

		if (routeEntry.path_len == 0 || split)
			continue;
		if ((RecvdUpdatePacket->sender_id == routingTable[routeEntry.dest_id].next_hop) || (routeEntry.cost + costToNbr < routingTable[routeEntry.dest_id].cost) || (routingTable[routeEntry.dest_id].path_len == 0))
		{
			if (routingTable[routeEntry.dest_id].path_len == 0)
				NumRoutes++;
			routingTable[routeEntry.dest_id].dest_id = routeEntry.dest_id;
			routingTable[routeEntry.dest_id].next_hop = routeEntry.next_hop;
			routingTable[routeEntry.dest_id].cost = routeEntry.cost + costToNbr;
			routingTable[routeEntry.dest_id].path_len = routeEntry.path_len + 1;
			routingTable[routeEntry.dest_id].path[0] = myID;
			for (int j = 0; j < routeEntry.path_len; j++)
				routingTable[routeEntry.dest_id].path[j + 1] = routeEntry.path[j];
			routingTableChange = 1;
		}
	}

	return routingTableChange;
}

////////////////////////////////////////////////////////////////
void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID)
{
	/* ----- YOUR CODE HERE ----- */
	UpdatePacketToSend->sender_id = myID;
	UpdatePacketToSend->no_routes = NumRoutes;
	int j = 0;

	for (int i = 0; i < MAX_ROUTERS; i++)
	{
		if (routingTable[i].path_len != 0)
		{
			UpdatePacketToSend->route[j].dest_id = routingTable[i].dest_id;
			UpdatePacketToSend->route[j].next_hop = routingTable[i].next_hop;
			UpdatePacketToSend->route[j].cost = routingTable[i].cost;
			UpdatePacketToSend->route[j].path_len = routingTable[i].path_len;
			for (int k = 0; k < MAX_PATH_LEN; k++)
				UpdatePacketToSend->route[j].path[k] = routingTable[i].path[k];
			j++;
		}
	}

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
