#include "seek.h"
//#include "communication.h"

SourceRoutingTableSlot SR_Table[MAX_SOURCE_ROUTING_DESTINATIONS];

int net_addr;
extern unsigned int 	net_address;

int LOCAL_left_low_corner = -1;
int LOCAL_right_high_corner = -1;
int wrapper_value = 0;

/*--------------------------------------------------------------------
* initSRstructs
*
* DESCRIPTION:
*    Initialize structures for executing source routing (SR_Table)
*
*    parameters: none
*    
*    return: 1
*--------------------------------------------------------------------*/
int initSRstructs(){
	int i;

	net_addr = MemoryRead(NI_CONFIG);

	// puts("\ninitSRstructs");
	for (i=0; i<MAX_SOURCE_ROUTING_DESTINATIONS; i++){
		SR_Table[i].tableSlotStatus = SR_LIVRE;
		// puts("\n:tableSlotStatus:");puts(itoh(SR_Table[i].tableSlotStatus));
	}
	return 1;
}

/*--------------------------------------------------------------------
* GetFreeSlotSourceRouting
*
* DESCRIPTION:
*    Searches SR_Table structure for a free space (SR_LIVRE status)
*
*    parameters: none
*    
*    return: the position or -1 if there is no free position
*--------------------------------------------------------------------*/
int GetFreeSlotSourceRouting(int target){
	int i;
	for (i=0; i<MAX_TASK_COMMUNICATION; i++){
		if(target == SR_Table[i].target && SR_Table[i].tableSlotStatus == SR_USADO){
			return -1;
		}
	}
	for (i=0; i<MAX_SOURCE_ROUTING_DESTINATIONS; i++){
		if(SR_Table[i].tableSlotStatus == SR_LIVRE){
			return i;
		}
	}
	puts("There is no space for storing Source Routing destination\n");
	return -1;
}


// int GetFreeSlotSourceRouting(){
// 	int i;
// 	for (i=0; i<MAX_SOURCE_ROUTING_DESTINATIONS; i++){
// 		if(SR_Table[i].tableSlotStatus == SR_LIVRE){
// 			return i;
// 		}
// 	}
// 	puts("There is no space for storing Source Routing destination\n");
// 	return -1;
// }
// int GetFreeSlotSourceRouting(int target){
// 	int i;
// 	int value;

// 	value = -1;

// 	for (i=0; i<MAX_SOURCE_ROUTING_DESTINATIONS; i++){
// 		if(SR_Table[i].tableSlotStatus == SR_LIVRE){
// 			value = i;
// 		}
// 		else{
// 			if(SR_Table[i].target == target){//&& SR_Table[i].tableSlotStatus == SR_USADO
// 			return i;
// 			}
// 		}
// 	}
// 	if(value == -1){
// 		puts("ERROR: There is no space for storing Source Routing destination\n");
// 	}
// 	return value;
// }

/*--------------------------------------------------------------------
* SearchSourceRoutingDestination
*
* DESCRIPTION:
*    Searches PE target in SR_Table (SR_USE status)
*	 If it is stored, then it should use the stored Source Routing
*
*    parameters: target PE address
*    
*    return: the position of the SR_Table or -1 if target is not Source
*	 Routing
*--------------------------------------------------------------------*/
int SearchSourceRoutingDestination(int target){
	int i;
	for (i=0; i<MAX_TASK_COMMUNICATION; i++){
		if(target == SR_Table[i].target && SR_Table[i].tableSlotStatus == SR_USADO){
			return i;
		}
	}
	return -1;	
}

/*--------------------------------------------------------------------
* ClearSlotSourceRouting
*
* DESCRIPTION:
*    Searches PE target in SR_Table (SR_USE status)
*	 If it is stored, then it clear setting to SR_LIVRE
*
*    parameters: target PE address
*    
*    return: the position of the SR_Table or -1 if target is not Source
*	 Routing
*--------------------------------------------------------------------*/
int ClearSlotSourceRouting(int target){
	int i;
	for (i=0; i<MAX_TASK_COMMUNICATION; i++){
		if(target == SR_Table[i].target && SR_Table[i].tableSlotStatus == SR_USADO){
			SR_Table[i].tableSlotStatus = SR_LIVRE;
			//puts("Clear\n");
			return i;
		}
	}
	return -1;	
}


/*--------------------------------------------------------------------
* ClearAllSourceRouting
*
* DESCRIPTION:
*    clear SR_Table (SR_USE status) setting to SR_LIVRE
*
*--------------------------------------------------------------------*/
void ClearAllSourceRouting(){
	int i;
	for (i=0; i<MAX_TASK_COMMUNICATION; i++){
		if(SR_Table[i].tableSlotStatus == SR_USADO){
			SR_Table[i].tableSlotStatus = SR_LIVRE;
		}
	}
}


void print_port(unsigned int port){
	switch(port){
		case 0x0: seek_puts("E0"); break;
		case 0x4: seek_puts("E1"); break;
		case 0x1: seek_puts("W0"); break;
		case 0x5: seek_puts("W1"); break;
		case 0x2: seek_puts("N0"); break;
		case 0x6: seek_puts("N1"); break;
		case 0x3: seek_puts("S0"); break;
		case 0x7: seek_puts("S1"); break;
		default: seek_puts("-"); break;
	}
}



void Seek(unsigned int service, unsigned int source, unsigned int target, unsigned int payload){
		MemoryWrite(SEEK_SERVICE_REGISTER,service);
		MemoryWrite(SEEK_SOURCE_REGISTER,source);
		MemoryWrite(SEEK_PAYLOAD_REGISTER, payload);
		switch(service){
			case PACKET_RESEND_SERVICE:
			case CLEAR_SERVICE:
			case END_TASK_SERVICE:
			case END_TASK_OTHER_CLUSTER_SERVICE:
			case START_APP_SERVICE:
			case TASK_ALLOCATED_SERVICE:
			case SET_SECURE_ZONE_SERVICE:
			case SET_SZ_RECEIVED_SERVICE:
			case SET_EXCESS_SZ_SERVICE:
			case OPEN_SECURE_ZONE_SERVICE:
			case SECURE_ZONE_CLOSED_SERVICE:
			case SECURE_ZONE_OPENED_SERVICE:
			case UNFREEZE_TASK_SERVICE:
			case FREEZE_TASK_SERVICE:
			case RCV_FREEZE_TASK_SERVICE:
			case LOAN_PROCESSOR_REQUEST_SERVICE:
			case LOAN_PROCESSOR_RELEASE_SERVICE:
			case INITIALIZE_SLAVE_SERVICE:
			case INITIALIZE_CLUSTER_SERVICE:
			case GMV_READY_SERVICE:
				 MemoryWrite(SEEK_OPMODE_REGISTER,GLOBAL_MODE);
			break;

			case TARGET_UNREACHABLE_SERVICE:
			case BACKTRACK_SERVICE:
			case SEARCHPATH_SERVICE:
			case WARD_SERVICE:
				 MemoryWrite(SEEK_OPMODE_REGISTER,RESTRICT_MODE);
			break;

			default:
				 MemoryWrite(SEEK_OPMODE_REGISTER,GLOBAL_MODE);

			break;
		}
		// Must be last value writed because target value fires the seek_send function

		MemoryWrite(SEEK_TARGET_REGISTER,target);  

		//puts("\nTask id: "); puts(itoa(current->id)); putsv(" seek service: ", service);
}

void Set_Secure_Zone(unsigned int left_low_corner, unsigned int right_high_corner, unsigned int master_PE){
	unsigned int my_X_addr, my_Y_addr;

	unsigned int RH_X_addr, RH_Y_addr;
	unsigned int LL_X_addr, LL_Y_addr;
	int isolated_ports = 0;

	my_X_addr = (get_net_address() & 0xF00) >> 8;
	my_Y_addr = get_net_address() & 0x00F;

	RH_X_addr = (right_high_corner & 0xF0) >> 4;
	RH_Y_addr = right_high_corner & 0x0F;

	LL_X_addr = (left_low_corner & 0xF0) >> 4;
	LL_Y_addr = left_low_corner & 0x0F;

	if((my_X_addr == RH_X_addr) && (my_Y_addr == RH_Y_addr)){
		Seek(CLEAR_SERVICE, master_PE, master_PE, 0);
	}

	// puts("X");puts(itoh(my_X_addr));puts(" ");
	// puts("Y");puts(itoh(my_Y_addr));puts("\n");

	// puts("LL_X");puts(itoh(LL_X_addr));puts(" ");
	// puts("LL_Y");puts(itoh(LL_Y_addr));puts("\n");

	// puts("RH_X");puts(itoh(RH_X_addr));puts(" ");
	// puts("RH_Y");puts(itoh(RH_Y_addr));puts("\n");

	//WEST or EAST test
	if(my_Y_addr <= RH_Y_addr && my_Y_addr >= LL_Y_addr ){
		if(my_X_addr == RH_X_addr){//EAST
			// puts("E border\n");
			isolated_ports = 0x3;
		}
		if(my_X_addr == LL_X_addr){//WEST
			// puts("W border\n");
			isolated_ports = 0xC;
		}
	}

	//NORTH or SOUTH test
	if(my_X_addr >= LL_X_addr && my_X_addr <= RH_X_addr){
		if(my_Y_addr == RH_Y_addr){//NORTH
			// puts("N border\n");
			isolated_ports = 0x30 + isolated_ports;
		}
		if(my_Y_addr == LL_Y_addr){//SOUTH
			// puts("S border\n");
			isolated_ports = 0xC0 + isolated_ports;
		}
	}
	if(isolated_ports != 0){
		wrapper_value = isolated_ports;
		//MemoryWrite(WRAPPER_REGISTER,isolated_ports);
		LOCAL_left_low_corner = left_low_corner;
		LOCAL_right_high_corner = right_high_corner;
		if((my_X_addr == RH_X_addr) && (my_Y_addr == RH_Y_addr)){
			Seek(SET_SZ_RECEIVED_SERVICE, get_net_address(), master_PE, right_high_corner);
			puts("SET SZ RH: ");puts(itoh(LOCAL_right_high_corner));puts("\n");	
		}
		seek_puts("wrapper: ");seek_puts(itoh(isolated_ports));seek_puts("\n");
	}
}


void Unset_Secure_Zone(unsigned int left_low_corner, unsigned int right_high_corner, unsigned int master_PE){
  unsigned int my_X_addr, my_Y_addr, master_X_addr, master_Y_addr;

  unsigned int RH_X_addr, RH_Y_addr, LOCAL_RH_X_addr, LOCAL_RH_Y_addr;
  unsigned int LL_X_addr, LL_Y_addr;
  int isolated_ports, previous_isolated;

  my_X_addr = (get_net_address() & 0xF00) >> 8;
  my_Y_addr = get_net_address() & 0x00F;

  master_X_addr = (master_PE & 0xF0) >> 4;
  master_Y_addr = master_PE & 0x00F;

  LOCAL_RH_X_addr = (LOCAL_right_high_corner & 0xF0) >> 4;
  LOCAL_RH_Y_addr = LOCAL_right_high_corner & 0x0F;

  RH_X_addr = (right_high_corner & 0xF0) >> 4;
  RH_Y_addr = right_high_corner & 0x0F;

  LL_X_addr = (left_low_corner & 0xF0) >> 4;
  LL_Y_addr = left_low_corner & 0x0F;


  // send 
  if((my_X_addr == LOCAL_RH_X_addr) && (my_Y_addr == LOCAL_RH_Y_addr)){
    Seek(CLEAR_SERVICE, master_PE, master_PE, 0);
  }


  // puts("X");puts(itoh(my_X_addr));puts(" ");
  // puts("Y");puts(itoh(my_Y_addr));puts("\n");

  // puts("LL_X");puts(itoh(LL_X_addr));puts(" ");
  // puts("LL_Y");puts(itoh(LL_Y_addr));puts("\n");

  // puts("RH_X");puts(itoh(RH_X_addr));puts(" ");
  // puts("RH_Y");puts(itoh(RH_Y_addr));puts("\n");

  // read actual wrapper value
  isolated_ports = wrapper_value;
  previous_isolated = wrapper_value;


  //seek_puts("previous wrapper: "); seek_puts(itoh(wrapper_value)); seek_puts("\n");
  if(wrapper_value == 0)
  	return;

  if((my_X_addr == LOCAL_RH_X_addr) && (my_Y_addr == LOCAL_RH_Y_addr)){   
      if ((LL_X_addr == master_X_addr) && (LL_Y_addr == master_Y_addr)){
      	  right_high_corner = ((get_net_address() >> 4)& 0XF0) | (get_net_address() &  0X0F);
          Seek(SECURE_ZONE_CLOSED_SERVICE, get_net_address(), master_PE, LOCAL_right_high_corner);
          //puts("ENDSZ RH:");puts(itoh(LOCAL_right_high_corner));puts("\n"); 
          seek_puts("Without CUT - wrapper: ");seek_puts(itoh(isolated_ports));seek_puts("\n");
          seek_puts("RH address: ");seek_puts(itoh(right_high_corner));seek_puts("\n");
          return;
    }
  }

// This is to uncut at RH position
//  // set wrapper port EAST
//  if( (my_X_addr == LL_X_addr - 1) && (my_Y_addr >=  LL_Y_addr) &&  (my_Y_addr <=  RH_Y_addr) )
//      isolated_ports = isolated_ports + 0x3;
//
//  // set wrapper port NORTH
//  if( (my_Y_addr == LL_Y_addr - 1) && (my_X_addr == LL_X_addr ))
//      isolated_ports = isolated_ports + 0x30;
//
//// UNSET wrapper port EAST
//  if( (my_X_addr == LL_X_addr) && (my_Y_addr >=  LL_Y_addr) &&  (my_Y_addr <=  RH_Y_addr) )
//      isolated_ports = isolated_ports - 0x3;
//
//  // UNSET wrapper port NORTH
//  if( (my_Y_addr == RH_Y_addr) && (my_X_addr == RH_X_addr ))
//      isolated_ports = isolated_ports - 0x30;    
//
// This is to uncut at LL position
// set wrapper port WEST
  if( (my_X_addr == LL_X_addr + 1) && (my_Y_addr >=  LL_Y_addr) &&  (my_Y_addr <=  RH_Y_addr) )
      isolated_ports = isolated_ports + 0x0C;

  // set wrapper port SOUTH
  if( (my_Y_addr == RH_Y_addr + 1) && (my_X_addr == RH_X_addr))
      isolated_ports = isolated_ports + 0xC0;

// UNSET wrapper port WEST
  if( (my_X_addr == LL_X_addr) && (my_Y_addr >=  LL_Y_addr) &&  (my_Y_addr <=  RH_Y_addr) )
      isolated_ports = isolated_ports - 0x0C;

  // UNSET wrapper port SOUTH
  if( (my_Y_addr == LL_Y_addr) && (my_X_addr == LL_X_addr ))
      isolated_ports = isolated_ports - 0xC0;    


  //if(isolated_ports != previous_isolated){
  	seek_puts("write wrapper: ");seek_puts(itoh(isolated_ports));seek_puts("\n");
    MemoryWrite(WRAPPER_REGISTER,isolated_ports);
    if((my_X_addr == LOCAL_RH_X_addr) && (my_Y_addr == LOCAL_RH_Y_addr)){
      Seek(SECURE_ZONE_CLOSED_SERVICE, get_net_address(), master_PE, LOCAL_right_high_corner);
      puts("ENDSZ RH:");puts(itoh(LOCAL_right_high_corner));puts("\n"); 
    }
    //seek_puts("wrapper:");seek_puts(itoh(isolated_ports));seek_puts("\n");
  //}
  wrapper_value = isolated_ports;
  seek_puts("RH address: ");seek_puts(itoh(right_high_corner));seek_puts("\n");
  seek_puts("LOCAL RH address: ");seek_puts(itoh(LOCAL_right_high_corner));seek_puts("\n");
}



	/**Processes the information of the path from seek router and stores
	 * the resultant Source Routing path in SR_Table indexed by slot_seek.
	 * \param slot_seek position
	 * \return void function
	 */
int ProcessTurns(unsigned int backtrack, unsigned int backtrack1, unsigned int backtrack2){
	int i,j;
	int slot_seek;
	// unsigned int backtrack, backtrack1, backtrack2;
	unsigned int next_port;
	unsigned int port[MAX_SOURCE_ROUTING_DESTINATIONS];//used for storing hops
	unsigned char algorithm;
	unsigned int shift;
	int addrX, addrY;
	addrX=0;
	addrY=0;
	j=0;//variable j is used to index the final path
	i=0;//variable i is used to index the number of ports (2 bits) read
	//this loop reads the flits and puts each port in each position of table.port[i]
	do {
		port[i] = backtrack & 0x3;
		next_port = (backtrack >> 2) & 0x3;
		backtrack >>= 2;
		i++;
		if(i==16){
			backtrack = backtrack1;
		}
		else if(i==32){
			backtrack = backtrack2;
		}
	}while( !( (port[i-1] == EAST  && next_port == WEST) ||//if the path is EW WE SN NS then we should stop here
			  (port[i-1] == WEST  && next_port == EAST)  ||
			  (port[i-1] == NORTH && next_port == SOUTH) ||
			  (port[i-1] == SOUTH && next_port == NORTH) ) );
	port[i] = next_port;
	//calculates target destination
	for(j=0;j<i;j++){
		switch(port[j]){
			case EAST: addrX++; break;
			case WEST: addrX--; break;
			case NORTH: addrY++; break;
			case SOUTH: addrY--; break;
		}
	}
	//gets target address
	seek_puts("Target: ");seek_puts(itoh((((net_address>>8) + addrX)<<8) | ((net_address + addrY)&0xff)));seek_puts("\n");
	//gets index in SR_Table
	slot_seek = SearchSourceRoutingDestination( (((net_address>>8) + addrX)<<8) | ((net_address + addrY)&0xff) );
	// seek_puts("PATH to ");
	// seek_puts(itoh(SR_Table[slot_seek].target));seek_puts(" = ");
	//SR_Table[slot_seek].path_size = ((i)/7)+1;//path size per hop(32 bits) = 7
	SR_Table[slot_seek].path_size = ((i)/6)+1;//path size per hop(16 bits) = 6
	// seek_puts("SR_Table[slot_seek].path_size:");seek_puts(itoh(SR_Table[slot_seek].path_size));seek_puts("\n");
	// seek_puts("i:");seek_puts(itoh(i));seek_puts("\n");
	//in number of hops
	algorithm = EAST_FIRST;//initialize with west first
	for(j=0;j<i;j++){//calculate the channel to comply with the routing algorithm
		if(algorithm == WEST_FIRST){
			if((port[j] == SOUTH || port[j] == NORTH) && port[j+1] == WEST){//turns prohibited by WEST FIRST :SW and NW
				algorithm = EAST_FIRST;
			}
		}
		else{
			if((port[j] == SOUTH || port[j] == NORTH) && port[j+1] == EAST){//turns prohibited by WEST FIRST :SE and NE
				algorithm = WEST_FIRST;
			}
			port[j] = port[j]+4;
		}
	}
	for (j=0; j<=i; j++){
		print_port(port[j]);
	}
	seek_puts("\n");
	//writes the path as an header of source routing
	shift=24;
	//16 bits flit
	SR_Table[slot_seek].path[0] = 0x70007000;
	for(i=0;i<=j;i++){
		SR_Table[slot_seek].path[i/6] = SR_Table[slot_seek].path[i/6]|((port[i]&0x0f) << shift);
		switch(shift){
			case 16:
				shift = 8;
			break;
			case 0:
				shift = 24;
				SR_Table[slot_seek].path[i/6+1] = 0x70007000;
			break;
			default:
				shift = shift - 4;
			break;
		}
	}
	return slot_seek;
	/*
		//32 bits flit
		// number of hops in the word = 6 for 16 bits, 7 for 32
		// SR_Table[slot_seek].path[0] = 0x70000000;
		// for(i=0;i<=j;i++){
		// 	SR_Table[slot_seek].path[i/7] = SR_Table[slot_seek].path[i/7]|((port[i]&0x0f) << shift);
		// 	switch(shift){
		// 		// case 16:
		// 		//     shift = 8;
		// 		// break;
		// 		case 0:
		// 			shift = 24;
		// 			SR_Table[slot_seek].path[i/7+1] = 0x70000000;
		// 		break;
		// 		default:
		// 			shift = shift - 4;
		// 		break;
		// 	}
		// }
		// SR_Table[slot_seek].tableSlotStatus = SR_USADO;
		// puts("\n:tableSlotStatus:");puts(itoh(SR_Table[slot_seek].tableSlotStatus));
		// for (i = 0; i < SR_Table[slot_seek].path_size; ++i){
		// 	puts(itoh(SR_Table[slot_seek].path[i]));
		// 	puts("\n");
		// }
		// //this read is necessary to point to SEEK_BACKTRACK0 in the next read;
		// MemoryRead(SEEK_BACKTRACK2);
	*/
}

