#include "ne.h"
#include "router.h"


/* ----- GLOBAL VARIABLES ----- */
struct route_entry routingTable[MAX_ROUTERS];
int NumRoutes;
int find_router(int);


////////////////////////////////////////////////////////////////
void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID){
	/* ----- YOUR CODE HERE ----- */
	// Initialize router table
	routingTable[NumRoutes].dest_id = myID;
	routingTable[NumRoutes].next_hop = myID;
	routingTable[NumRoutes].cost = 0;
	routingTable[NumRoutes].path_len = 1;
	routingTable[NumRoutes].path[0] = myID;
	NumRoutes += 1;

	for (int i = 0; i < InitResponse->no_nbr; i++)
	{
		routingTable[NumRoutes].dest_id = InitResponse->nbrcost[i].nbr;
		routingTable[NumRoutes].next_hop = InitResponse->nbrcost[i].nbr;
		routingTable[NumRoutes].cost = InitResponse->nbrcost[i].cost;
		routingTable[NumRoutes].path_len = 2;
		routingTable[NumRoutes].path[0] = myID;
		routingTable[NumRoutes].path[1] = InitResponse->nbrcost[i].nbr;

		NumRoutes += 1;
	}

	return;
}


////////////////////////////////////////////////////////////////
int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID){
	/* ----- YOUR CODE HERE ----- */
	struct route_entry routeEntry;
	int routingTableChange = 0;
	int check1, check2, check3, check4;
	int rID;

	for (int i = 0; i < RecvdUpdatePacket->no_routes; i++)
	{
		routeEntry = RecvdUpdatePacket->route[i];

		rID = find_router(routeEntry.dest_id);

		// Split Horizon
		int split = 0;
		for (int i = 0; i < routeEntry.path_len; i++)
		{
			if (routeEntry.path[i] == myID)
				split = 1;
		}

		if (routeEntry.path_len == 0 || split)
			continue;
		if ((RecvdUpdatePacket->sender_id == routingTable[rID].next_hop) || (routeEntry.cost + costToNbr < routingTable[rID].cost) || (routingTable[rID].path_len == 0))
		{
			if (routingTable[rID].path_len == 0)
				NumRoutes++;

			check1 = routingTable[rID].dest_id - routeEntry.dest_id;
			routingTable[rID].dest_id = routeEntry.dest_id;

			check2 = routingTable[rID].next_hop - RecvdUpdatePacket->sender_id;
			routingTable[rID].next_hop = RecvdUpdatePacket->sender_id;

			check3 = routingTable[rID].cost - (routeEntry.cost + costToNbr);
			routingTable[rID].cost = routeEntry.cost + costToNbr;

			check4 = routingTable[rID].path_len - (routeEntry.path_len + 1);
			routingTable[rID].path_len = routeEntry.path_len + 1;

			routingTable[rID].path[0] = myID;
			for (int j = 0; j < routeEntry.path_len; j++)
				routingTable[rID].path[j + 1] = routeEntry.path[j];

			if (check1 || check2 || check3 || check4) {
				routingTableChange = 1;
			}
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
void UninstallRoutesOnNbrDeath(int DeadNbr)
{
	/* ----- YOUR CODE HERE ----- */
	for (int i = 0; i < MAX_ROUTERS; i++)
	{
		if (i == DeadNbr)
		{
			routingTable[DeadNbr].dest_id = 0;
			routingTable[DeadNbr].next_hop = 0;
			routingTable[DeadNbr].cost = 0;
			routingTable[DeadNbr].path_len = 0;
			continue;
		}
		for (int j = 0; j < routingTable[i].path_len; j++)
		{
			if (routingTable[i].path[j] == DeadNbr)
			{
				routingTable[i].dest_id = 0;
				routingTable[i].next_hop = 0;
				routingTable[i].cost = 0;
				routingTable[i].path_len = 0;
				break;
			}
		}
	}
}

int find_router(int rID) {
	for (int i = 0; i < NumRoutes; i++) {
		if (routingTable[i].dest_id == rID) {
			return i;
		}
	}
	return NumRoutes;

}


/*void UninstallRoutesOnNbrDeath(int DeadNbr)
{
	routingTable[DeadNbr].dest_id = 0;
	routingTable[DeadNbr].next_hop = 0;
	routingTable[DeadNbr].cost = 0;
	routingTable[DeadNbr].path_len = 0;
	for (int i = 0; i < MAX_ROUTERS; i++) {
		routingTable[DeadNbr].path[i] = 0;
	}
}*/
