/*!\file osz.h
 * HEMPS VERSION - 8.5 - support for OSZ
 *
 * Distribution:  August 2020
 *
 * Created by: Luciano Caimi - contact: lcaimi@gmail.com
 *             Rafael Faccenda - contact: faccendarafael@gmail.com
 *
 * Research group: GAPH-PUCRS   -  contact:  fernando.moraes@pucrs.br
 *
 * \brief This module implements functions to open and close Opaque Secure Zones
 *
 */

#include "osz_master.h"
#include "utils.h"
#include "processors.h"
#include "applications.h"
#include "define_pairs.h"
#include "seek.h" 
#include "packet.h"

#ifdef BLOQUEIO_IO
#include "../../include/kernel_pkg.h"
#endif


//extern unsigned int app_id_counter;
unsigned int address_go, address_back;
unsigned int port_go, port_back;  // 0-EAST; 1 - WEST ; 2 - NORTH; 3 - SOUTH

////////////////////////////////////////////
void init_Secure_Zone(){
    int i;
    for(i = 0; i < MAX_SHAPES; i++){
        Secure_Zone[i].occuped = 0;
    }
}

////////////////////////////////////////////
int get_static_SZ(int appID){
	//int appID;
	int xi = 255, yi = 255, xf = 0, yf = 0;

	//appID = app_id_counter-1;		// ver se é o atual ou o próximo

#if MAX_STATIC_TASKS
	for(int i=0; i<MAX_STATIC_TASKS; i++){
		if ((static_map[i][0] >> 8) ==  appID){
			if ((static_map[i][1] >> 8) < xi)  // menor X
				xi = (static_map[i][1] >> 8);
			if ((static_map[i][1] & 0XFF) < yi) // menor Y
				yi = (static_map[i][1] & 0XFF);
			if ((static_map[i][1] >> 8) > xf)  // maior X
				xf = (static_map[i][1] >> 8);
			if ((static_map[i][1] & 0XFF) > yf) // maior Y
				yf = (static_map[i][1] & 0XFF);
		}
	}
#endif
	if(xi != 255){
		shape_index = 0;
		shapes[0].excess = 0;
		shapes[0].X_size = xf - xi + 1;
		shapes[0].Y_size = yf - yi + 1;
		shapes[0].position = (shapes[0].X_size << 24)|(shapes[0].Y_size << 16) | (xi << 8) | yi;
		shapes[0].used = 1; //Bug --nome
		shapes[0].cut = -1;
		shapes[0].processors = shapes[0].X_size * shapes[0].Y_size;
//		puts(">>>> Appid "); puts(itoa(appID)); 
//		puts("\n    xi: "); puts(itoh(xi));
//		puts("\n    yi: "); puts(itoh(yi));
//		puts("\n    xf: "); puts(itoh(xf));
//		puts("\n    yf: "); puts(itoh(yf));
//		puts("\n    position: "); puts(itoh(shapes[0].position));
//		puts("\n");
		return shapes[0].position;
	}

	return 0;	
}

int create_shapes(int task_pe, int tasks_app, int app_id, int tasks_with_comm){
    int nb_shapes = 0, t, nb_pe;
#ifdef SOMBREAMENTO
    int PEs_remove = 0;  
#endif
    for(t=1; t <= task_pe; t++ ){ 
        nb_pe = tasks_app/t ;

        if( nb_pe < tasks_app ) nb_pe++;   // ceil
#ifdef SOMBREAMENTO
        if (OSZ_shape[app_id] == 0){
            PEs_remove = (tasks_with_comm-1);
        }
        if (nb_pe < ((tasks_with_comm * tasks_with_comm)-PEs_remove)){
            nb_pe = ((tasks_with_comm * tasks_with_comm)-PEs_remove);
        }
        puts("Necessary PEs:(");puts(itoa(nb_pe));puts(")\n");
#endif
        nb_shapes =  create_valid_shapes(nb_pe, nb_shapes, app_id, tasks_with_comm);
    } 
    return nb_shapes;
}

//////////////////////////////////////////////////////////////////
int create_valid_shapes(int PEs, int cont, int app_id, int tasks_with_comm){
    int x, y, ix1, ix2, delta1, delta2, swap, contador;

#ifdef GRAY_AREA
//   create all possible shapes
  //printf("\ncall:");
    for( y = 1 ; y<= PEs; y++){
        x = PEs/y;
        while (x*y < PEs)   x++;
        shapes[cont+y-1].X_size = x;
        shapes[cont+y-1].Y_size = y;
        shapes[cont+y-1].valid = 1;   // all shapes are valid
        // puts("\nShape:(");puts(itoa(x));puts(" x ");puts(itoa(y));puts(")\n");
    }
#else
  // create all possible shapes
  //printf("\ncall:");
    for( x = 1 ; x<= PEs; x++){
        y = PEs/x;
        while (x*y < PEs)   y++;
        shapes[cont+x-1].X_size = x;
        shapes[cont+x-1].Y_size = y;
        shapes[cont+x-1].valid = 1;   // all shapes are valid
        //puts("\nShape:(");puts(itoa(x));puts(" x ");puts(itoa(y));puts(")\n");
        //printf("\nShape: (%2d x %2d)", x, y);
    }

  // simple bubble sort according to the delta - we want a rectangular shape
    for (ix1 = 0 ; ix1 < PEs-1 ; ix1++)
        for (ix2 = 0 ; ix2 < PEs - ix1 - 1; ix2++){
            if ( shapes[cont+ix2].X_size > shapes[cont+ix2].Y_size) 
                delta1 = shapes[cont+ix2].X_size - shapes[cont+ix2].Y_size;
            else delta1 = shapes[cont+ix2].Y_size - shapes[cont+ix2].X_size;

            if ( shapes[cont+ix2+1].X_size > shapes[cont+ix2+1].Y_size) 
                delta2 = shapes[cont+ix2+1].X_size - shapes[cont+ix2+1].Y_size;
            else delta2 = shapes[cont+ix2+1].Y_size - shapes[cont+ix2+1].X_size;

            if ( delta1 > delta2 ){
                swap            = shapes[cont+ix2].X_size;
                shapes[cont+ix2].X_size   = shapes[cont+ix2+1].X_size;
                shapes[cont+ix2+1].X_size = swap;

                swap            = shapes[cont+ix2].Y_size;
                shapes[cont+ix2].Y_size   = shapes[cont+ix2+1].Y_size;
                shapes[cont+ix2+1].Y_size = swap;
            }
        } 
#endif

    // suppress invalid shapes -  those that are enclosed in other shapes 
    for( ix1= 0; ix1<PEs-1; ix1++){
        if  (shapes[cont+ix1].Y_size == shapes[cont+ix1+1].Y_size){
            shapes[cont+ix1+1].valid=0;
        }
    }

#ifndef GRAY_AREA
#ifdef SOMBREAMENTO
    // suppress invalid shapes -  that are smaller than the tasks_with_comm size
    for( ix1= 0; ix1<PEs; ix1++){
        if ((shapes[cont+ix1].Y_size < tasks_with_comm) || (shapes[cont+ix1].X_size < tasks_with_comm)){
            shapes[cont+ix1].valid=0;
        }
    }
#endif
#endif

  // suppress invalid shapes -  that are larger than the cluster size //ADD - LARGER THAN SecureArea
#ifdef GRAY_AREA
    if ((ga.cols[MAX_GRAY_COLS-1] == XCLUSTER -1) || (ga.cols[0] == 0)){ // If GrayArea at the sides 
        for( ix1=0; ix1<PEs; ix1++){
            if( (shapes[cont+ix1].X_size > (XCLUSTER - MAX_GRAY_COLS)) || (shapes[cont+ix1].Y_size > (YCLUSTER - MAX_GRAY_ROWS))){
                shapes[cont+ix1].valid=0;
            }
        }
    }//else{ //TODO: Caso GrayArea for no meio 
    //}
#else
    for( ix1=0; ix1<PEs; ix1++)
        if( shapes[cont+ix1].X_size>XCLUSTER || shapes[cont+ix1].Y_size>YCLUSTER)
            shapes[cont+ix1].valid=0; 
#endif
  
    // recreate the list and the correct number of shapes
    for( contador=x=0; x<PEs; x++)
        if (shapes[cont+x].valid){
            shapes[cont+contador].X_size = shapes[cont+x].X_size;
            shapes[cont+contador].Y_size = shapes[cont+x].Y_size;
            shapes[cont+contador].valid = shapes[cont+x].valid;
            shapes[cont+contador].processors = shapes[cont+contador].X_size * shapes[cont+contador].Y_size;
            if (OSZ_shape[app_id] == 0){
                shapes[cont+contador].excess = ((shapes[cont+x].X_size) *  (shapes[cont+x].Y_size))-PEs;
            } else{
                shapes[cont+contador].excess = 0;
            }
            contador++;
        }
    return( cont+contador );
}

//////////////////////////////////////////////////////////////////////////////////////
void print_shapes_found(int nb_shapes){
  int x;
       
        puts("Number of shapes: "); puts(itoa(nb_shapes)); puts("\n");
        for(x=0; x<nb_shapes; x++) {
           //printf("      (%2d  %2d) - excess: %d  processors: %d\n",  shapes[x].X_size, shapes[x].Y_size, 
           //                   shapes[x].excess, shapes[x].processors) ;
            puts("      ("); puts(itoa(shapes[x].X_size)); puts(" - "); puts(itoa(shapes[x].Y_size)); puts(") excess: ");  
            puts(itoa(shapes[x].excess)); puts("  processors: "); puts(itoa(shapes[x].processors));  puts("\n");
        }
}

//////////////////////////////////////////////////////////////////////////////////////
int search_shape(int valid_shapes){
    int x, used_looking, indice;

    for(used_looking = 0; used_looking <= MAX_MIGRATIONS; used_looking++){
        if( used_looking == 1){
            puts("Inverting... \n"); 
            shapes_invert_order(shapes, valid_shapes);
        }

        for(x = 0; x < valid_shapes; x++){
            indice = search_shape_in_cluster(shapes[x].X_size, shapes[x].Y_size, shapes, x, used_looking);
            if(indice != -1){
                //return x;
                shape_index = x;
                return shapes[x].position;
            }          
        }
    }
    return -1;
}
//////////////////////////////////////////////////////////////////////////////////////
void shapes_invert_order(Shapes shapes[], int nb_valid_shapes){
  int i;
  Shapes shape_aux;

  for(i = 0; i < nb_valid_shapes/2; i++){
    shape_aux.processors = shapes[i].processors;
    shapes[i].processors = shapes[nb_valid_shapes-i-1].processors;
    shapes[nb_valid_shapes-i-1].processors = shape_aux.processors;

    shape_aux.X_size = shapes[i].X_size;
    shapes[i].X_size = shapes[nb_valid_shapes-i-1].X_size;
    shapes[nb_valid_shapes-i-1].X_size = shape_aux.X_size;

    shape_aux.Y_size = shapes[i].Y_size;
    shapes[i].Y_size = shapes[nb_valid_shapes-i-1].Y_size;
    shapes[nb_valid_shapes-i-1].Y_size = shape_aux.Y_size;

    shape_aux.excess = shapes[i].excess;
    shapes[i].excess = shapes[nb_valid_shapes-i-1].excess;
    shapes[nb_valid_shapes-i-1].excess = shape_aux.excess;
  }
  //print_shapes_found(nb_valid_shapes);
}

//////////////////////////////////////////////////////////////////////////////////////
int search_shape_in_cluster(int X_size, int Y_size, Shapes shape[], int cont, int used_looking ){
    int x, y, used_in_SWS, result;

#ifdef GRAY_AREA // Se for GA, mapear da Esquerda pra direita
    for(y = YCLUSTER-Y_size; y >= 0; y--){
        for(x = 0; x<=XCLUSTER-X_size; x++){
#else // SWS TOP-RIGHT left
    for(y = YCLUSTER-Y_size; y >= 0; y--){
        for(x = XCLUSTER-X_size; x >= 0; x--){
#endif

        used_in_SWS = shape_recog(X_size, Y_size, x, y);
  
        //discart strip shapes using all column or all line at the middle of cluster
        if(used_in_SWS == used_looking){
            if(shape[cont].X_size == XCLUSTER && (y != YCLUSTER - shape[cont].Y_size))
                continue;
        }
        result = (x<<8) + y;
  
        shape[cont].position = (X_size << 24 | Y_size << 16) | result;
        shape[cont].used = used_in_SWS;
        shape[cont].cut = -1;
  
        if(used_in_SWS == used_looking){
            if(shape[cont].excess > 0){
                shape[cont].cut = (x <<24) | ((y + shape[cont].excess -1) << 16) | (x  << 8) | y ;
            }
            return cont;
        }
      }
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////////////////
int shape_recog(int X_size, int Y_size, int X_init, int Y_init){
    int  x, y, desl_Y, used;
    int XMASTER, YMASTER;

    XMASTER = (get_net_address() >> 8) & 0xFF;
    YMASTER = get_net_address() & 0xFF;

    used = 0;
    for(y = Y_init; y < Y_size + Y_init; y++){
        desl_Y = y * XCLUSTER;
        for(x = X_init; x < X_size + X_init; x++){
            used += MAX_LOCAL_TASKS - processors[x+desl_Y].free_pages;
            if((x == XMASTER) && (y == YMASTER))
                return  100;
            if(PE_belong_SZ(x,y) == 1)
                return 100;
            #ifdef GRAY_AREA
            if(PE_belong_GA(x,y) == 1)
                return 100;
            #elif BLOQUEIO_IO
            if(PE_belong_SZ_shadow(x,y) == 1)
                return 100;
            if(PE_belong_IO_shadow(x,y) == 1)
                return 100;
            #endif
        }
    }
    return used;
}

//////////////////////////////////////////////////////////////////////////////////////
//void set_Secure_Zone(int shape_index){
void alloc_Secure_Zone(int shape_index){
    int i;

    for(i = 0; i < MAX_SHAPES; i++){
        if(Secure_Zone[i].occuped == 0){
            //puts("set_SZ index: "); puts(itoh(i)); puts("\n");
            Secure_Zone[i].processors = shapes[shape_index].processors;
            Secure_Zone[i].X_size = shapes[shape_index].X_size;
            Secure_Zone[i].Y_size = shapes[shape_index].Y_size;
            Secure_Zone[i].excess = shapes[shape_index].excess;
            Secure_Zone[i].used = shapes[shape_index].used;
            Secure_Zone[i].cut = shapes[shape_index].cut;
            Secure_Zone[i].position = shapes[shape_index].position;
            Secure_Zone[i].occuped = 1;

            //updateOSZstatus(MemoryRead(TICK_COUNTER), i, )
            set_SZ_migrations(i);
            freeze_application();
            return;
        }
    }    
}

//////////////////////////////////////////////////////////////////////////////////////
int set_SZ_migrations(int SZ_index) {
    int desl_Y, y, x, proc_address;
    int i, flag, init_cut_index, end_cut_index;
    int xi, yi, xf, yf;
    int taskID0, taskID1, index;
    xi =  (Secure_Zone[SZ_index].position >> 8) & 0XFF;
    yi =   Secure_Zone[SZ_index].position  & 0XFF;
    xf =  xi + Secure_Zone[SZ_index].X_size;
    yf =  yi + Secure_Zone[SZ_index].Y_size;
    if(Secure_Zone[SZ_index].cut != -1){
        init_cut_index = (Secure_Zone[SZ_index].cut & 0XFFFF);
        init_cut_index = ((init_cut_index & 0XFF) * XCLUSTER) + (init_cut_index >> 8);

        end_cut_index = (Secure_Zone[SZ_index].cut >> 16);
        end_cut_index = ((end_cut_index & 0XFF) * XCLUSTER) + (end_cut_index >> 8);
    }
    for(y = yi;  y < yf; y++){
        desl_Y = (y)*XCLUSTER ;
        for(x = xi; x < xf; x++){
            //----------- avoid the SZ excess ----------------
            if(Secure_Zone[SZ_index].cut != -1){
                flag = 0;
                for(i = init_cut_index; i <= end_cut_index; i += XCLUSTER){
                    if((x+desl_Y) == i)
                        flag = 1;
                }
                if(flag == 1){
                    continue;
                }
            }
            //-----------------------------------------------
            proc_address = get_proc_address(x+desl_Y);
            //puts("address: "); puts(itoh(proc_address)); puts("\n");
            if (get_proc_free_pages(proc_address) != MAX_LOCAL_TASKS){
                taskID0 = get_task_id_processor(proc_address);
                taskID1 = (taskID0 >> 16) &  0XFFFF;
                taskID0 = taskID0 &  0XFFFF;

                for(index = 0; index < MAX_MIGRATIONS; index++){
                    if(migration_list[index].status == OFF ){
                        migration_list[index].status = ACTIVE;
                        migration_list[index].actual_address = proc_address;
                        migration_list[index].actual_taskID = taskID0;
                        break;
                    }
                }
                if(taskID1 != 0XFFFF){
                    for(; index < MAX_MIGRATIONS; index++){
                        if(migration_list[index].status == OFF ){
                            migration_list[index].status = ACTIVE;
                            migration_list[index].actual_address = proc_address;
                            migration_list[index].actual_taskID = taskID1;
                            break;
                        }
                    }                    
                }
            }
        }
    }
    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////
void set_RH_Address(int App_ID, int Address){
	for(int i=0; i<MAX_CLUSTER_APP; i++){
		if(applications[i].app_ID == App_ID){
			applications[i].RH_Address = Address;
			//puts("\nFound App: "); puts(itoh(App_ID)); puts(" to set RH: "); puts(itoh(Address)); puts("\n");
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////
int get_AppID_with_RH_Address(int Address){
	for(int i=0; i<MAX_CLUSTER_APP; i++){
		//puts("\nApp: "); puts(itoh(applications[i].app_ID)); puts(" index: "); puts(itoh(i)); puts(" Address: "); puts(itoh(applications[i].RH_Address)); puts("\n");
		if(applications[i].RH_Address == Address)
			return applications[i].app_ID;
	}
	return -1;
}


//////////////////////////////////////////////////////////////////////////////////////
int get_Secure_Zone_index(int RH_address){
    int i, aux;

    //puts("get_SZ RH: "); puts(itoh(RH_address)); puts("\n");

    RH_address = ((RH_address & 0XF0) << 4) + (RH_address & 0X0F);

    //puts("RH: "); puts(itoh(RH_address)); puts("\n");

    for(i = 0; i < MAX_SHAPES; i++){
        aux = (Secure_Zone[i].position & 0XFFFF) + (Secure_Zone[i].position >> 16) - 0x0101;

        //puts("aux RH: "); puts(itoh(aux)); puts("\n");
        if((Secure_Zone[i].occuped == 1) && (aux == RH_address)){
            return i;
        }
    }
    return -1;
}



//////////////////////////////////////////////////////////////////////////////////////
void free_Secure_Zone(int RH_address){ // falta implementar
    int i, RH_SZ;

    RH_address = ((RH_address << 4) & 0XF00) | (RH_address & 0x0F);
    puts("unset_SZ RH_address: "); puts(itoh(RH_address)); puts("\n");
    for(i = 0; i < MAX_SHAPES; i++){
        RH_SZ = (Secure_Zone[i].position & 0XFFFF) + (Secure_Zone[i].position >> 16) - 0x0101;
        puts("RH_SZ: "); puts(itoh(RH_SZ)); puts("\n");
        if((Secure_Zone[i].occuped == 1) && (RH_SZ == RH_address)){
             Secure_Zone[i].occuped = 0;
             return;
        }
    }    
}

#ifdef GRAY_AREA 
//////////////////////////////////////////////////////////////////////////////////////
int PE_belong_GA(int PE_x, int PE_y){

    for (int i = 0; i < MAX_GRAY_COLS; i++)
        if (PE_x == ga.cols[i])
            return 1;
        
    for (int i = 0; i < MAX_GRAY_ROWS; i++)
        if (PE_y == ga.rows[i])
            return 1;

    return 0;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////
int PE_belong_SZ(int PE_x, int PE_y){

        int i,  xi_cut, xf_cut, yi_cut, yf_cut;
        int xi, yi, xf, yf;

        for(i = 0; i < MAX_SHAPES; i++){
            if(Secure_Zone[i].occuped == 1){

                xi =  (Secure_Zone[i].position >> 8) & 0XFF;
                yi =   Secure_Zone[i].position  & 0XFF;
                xf =  xi + Secure_Zone[i].X_size;
                yf =  yi + Secure_Zone[i].Y_size;
                if(Secure_Zone[i].cut != -1){
                    xi_cut =  (Secure_Zone[i].cut >> 8) & 0XFF;
                    yi_cut =   Secure_Zone[i].cut  & 0XFF;
        
                    xf_cut = (Secure_Zone[i].cut >> 24) & 0XFF;
                    yf_cut = (Secure_Zone[i].cut >> 16) & 0XFF;

                    if(((PE_x >= xi_cut) && (PE_x <= xf_cut)) && ((PE_y >= yi_cut) && (PE_y <= yf_cut))  )
                        continue;
                }
                //puts("\nBelong? X: ");  puts(itoa(PE_x));  puts(" Y: ");  puts(itoa(PE_y));
                if(((PE_x >= xi) && (PE_x < xf)) && ((PE_y >= yi) && (PE_y < yf))  )
                    return 1;
            }
        }
    return 0;
}

#ifdef BLOQUEIO_IO
//////////////////////////////////////////////////////////////////////////////////////
int PE_belong_SZ_shadow(int PE_x, int PE_y){
    int i;
    int xi, yi, xf, yf;

    for(i = 0; i < MAX_SHAPES; i++){
        if(Secure_Zone[i].occuped == 1){
            xi =  ((Secure_Zone[i].position >> 8) & 0XFF)-1;
            yi =   (Secure_Zone[i].position  & 0XFF)-1;
            xf =  xi + Secure_Zone[i].X_size+1;
            yf =  yi + Secure_Zone[i].Y_size+1;

            if(((PE_x >= xi) && (PE_x < xf)) && ((PE_y >= yi) && (PE_y < yf))  )
                return 1;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////
int PE_belong_IO_shadow(int PE_x, int PE_y){
    int i;
    int xi, yi, xf, yf;

    if(((PE_x == 0) && (PEs_With_IO[1] == 1))||((PE_x == (XDIMENSION-1))&&(PEs_With_IO[2] == 1))||
       ((PE_y == 0) && (PEs_With_IO[0] == 1))||((PE_y == (YDIMENSION-1))&&(PEs_With_IO[3] == 1))){
        return 1;
    }else{
        return 0;
    }
}
#endif

//////////////////////////////////////////////////////////////////////////////////////
void open_wrapper_IO_SZ(int peripheral_id, int io_service){ // io_service: 0 - request; 1 - delivery
	int aux;

	ServiceHeader *p = get_service_header_slot();

	// aux = find_SZ_position_and_direction_to_IO(peripheral_id);

	// if(aux == -1)
	// 	return;

    //puts("forward OPEN WRAPPER: "); puts(itoh(address_go)); puts("\n");

    //-----------------------------------------------------------------------------
    //OUT_WRAPPER
	p->header[MAX_SOURCE_ROUTING_PATH_SIZE-2] = (0x1 << 28) | ((0X3F00 & address_go) << 14) | ((0X003F & address_go) << 16)| address_go;
	p->header[MAX_SOURCE_ROUTING_PATH_SIZE-1] = (0x1 << 28) | ((0X3F00 & address_go) << 14) | ((0X003F & address_go) << 16)| address_go;

	p->payload_size = (CONSTANT_PKT_SIZE - 2);

	p->service = IO_OPEN_WRAPPER;

	p->source_PE = get_net_address();

	p->io_port = port_go;

	p->io_direction = OUTPUT_DIRECTION;

	if(io_service == 0)  //io_service: 0 - REQUEST   1 - DELIVERY
		p->io_service = IO_REQUEST;
	else
		p->io_service = IO_DELIVERY;

	send_packet(p, 0, 0);

    //------------------------------------------------------------------------------
    //IN_WRAPPER
	p->header[MAX_SOURCE_ROUTING_PATH_SIZE-2] = (0x1 << 28) | ((0X3F00 & address_back) << 14)| ((0X003F & address_back) << 16)| address_back;
	p->header[MAX_SOURCE_ROUTING_PATH_SIZE-1] = (0x1 << 28) | ((0X3F00 & address_back) << 14)| ((0X003F & address_back) << 16)| address_back;

	p->io_port = port_back;
	p->io_direction = INPUT_DIRECTION;

	if(io_service == 0)  //io_service: 0 - REQUEST   1 - DELIVERY
		p->io_service = IO_DELIVERY;
	else
		p->io_service = IO_ACK;

	send_packet(p, 0, 0);

}

// ////////////////////////////
// void updateOSZstatus(int timeStamp, ){

//     FILE *logOSZ = fopen("logOSZ.txt", "w");

// }
