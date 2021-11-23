/*
 * Copyright (C) 2016, Matthias Rottmann, Artur Strebel, Simon Heybrock, Simone Bacchio, Bjoern Leder.
 * 
 * This file is part of the DDalphaAMG solver library.
 * 
 * The DDalphaAMG solver library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * The DDalphaAMG solver library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * 
 * You should have received a copy of the GNU General Public License
 * along with the DDalphaAMG solver library. If not, see http://www.gnu.org/licenses/.
 * 
 */

#include "main.h"
#include "vcycle_PRECISION.h"

// ---

// put here all MUMPS-related functions

void mumps_prepare_PRECISION(config_PRECISION clover, int length, level_struct *l, struct Thread *threading ) {
  


//  config_PRECISION clover_pt = l->op_PRECISION.clover; //clover;
  gmres_PRECISION_struct* px = &(l->p_PRECISION);
  operator_PRECISION_struct* op = px->op; //&(l->op_PRECISION);//
  config_PRECISION clover_pt = l->p_PRECISION.op->clover;


// %p to print pointer
   // #########################################################################
   // #1 self-coupling:
   // #########################################################################

  int num_eig_vect = l->num_parent_eig_vect;
  int site_var = l->num_lattice_site_var,
    clover_step_size1 = (num_eig_vect * (num_eig_vect+1))/2,
    clover_step_size2 = SQUARE(num_eig_vect);
//    vector_PRECISION phi_pt=phi, eta_pt=eta, phi_end_pt=phi+length;
  /*
  int ix;
  for (ix = 0; ix < (2*clover_step_size1 + clover_step_size2) * l->num_inner_lattice_sites; ix += 5){ // * l->num_inner_lattice_sites
    printf0("%+-3f%+-3fi, %+-3f%+-3fi, %+-3f%+-3fi, %+-3f%+-3fi, %+-3f%+-3fi\n", CSPLIT(clover_pt[ix]), CSPLIT(clover_pt[ix +1]), CSPLIT(clover_pt[ix +2]), CSPLIT(clover_pt[ix +3]), CSPLIT(clover_pt[ix +4]), CSPLIT(clover_pt[ix +5]));
  }
  exit(0);
  */


  int nr_nodes = l->num_inner_lattice_sites;
  int i, j, k; // k = index in matrix
  int c, r;	// col no., row no.
  int skip = 8 * SQUARE(site_var);	//skip number of elements in Blockrow in large matrix (for self coupl. only skip = 0, else 8 * SQUARE(site_var))
  int j_start = nr_nodes * g.my_rank * site_var, i_start = nr_nodes * g.my_rank * site_var;

  for (j = 0, k = 0; j < nr_nodes; j++){
    for (i = 0; i < SQUARE(site_var); i++, k++){
      *(px->mumps_Is +k) = i_start + j * site_var + (int)(i/site_var);	// col indices
      *(px->mumps_Js +k) = j_start + j * site_var + (i % site_var); 	// row indices
    }
    k += skip;
  }

//      nr_nodes * g.my_rank 			= nodes per process * p_id
//	nr_nodes * (g.my_rank +1)		= first node of next process
  for (j = 0; j < nr_nodes; j++){

	// A store column-wise
    for (k = 0, r = 0; r < num_eig_vect; r++, k++){
      for (c = 0; c < r; c++, k++){
        px->mumps_vals[j*9*SQUARE(site_var) + c * site_var + r] = *(clover_pt + k);
        px->mumps_vals[j*9*SQUARE(site_var) + r * site_var + c] = conj_PRECISION(*(clover_pt + k));
      }
      px->mumps_vals[j*9*SQUARE(site_var) + r * site_var + r] = *(clover_pt + k);
    }


  //remove this line as well, as soon k++ is removed
    clover_pt += clover_step_size1;

	// D store column-wise
    for (k = 0, r = num_eig_vect; r < 2*num_eig_vect; r++, k++){
      for (c = num_eig_vect; c < r; c++, k++){
        px->mumps_vals[j*9*SQUARE(site_var) + c * site_var + r] = *(clover_pt + k);
        px->mumps_vals[j*9*SQUARE(site_var) + r * site_var + c] = conj_PRECISION(*(clover_pt + k));
      }
      px->mumps_vals[j*9*SQUARE(site_var) + r * site_var + r] = *(clover_pt + k);
    }

  //remove this line as well, as soon k++ is removed
    clover_pt += clover_step_size1;
	// C store column-wise
    for (r = num_eig_vect, k = 0; r < 2*num_eig_vect; r++){
      for (c = 0; c < num_eig_vect; c++, k++){
        px->mumps_vals[j*9*SQUARE(site_var) + (r * site_var) + c] = -1.0*(conj_PRECISION(*(clover_pt + k)));
//      px->mumps_vals[j*9*SQUARE(site_var) + (r +  num_eig_vect) * site_var + c] =  -1.0*(conj_PRECISION(*(clover_pt + k)));
      }
    }


  //no clover_pt correction / change this once k++ is removed
	// B store column-wise / transposed from former storage
    for (r = 0, k = 0; r < num_eig_vect; r++){
      for (c = 0; c < num_eig_vect; c++, k++){
        px->mumps_vals[j*9*SQUARE(site_var) + (c * site_var) + r + num_eig_vect] = *(clover_pt + k);
      }
    }
    clover_pt += clover_step_size2;
	// skipping num for hopping terms in mumps_vals
    k += skip;
  }  // end for loop over the blocks








   // #########################################################################
   // #2 hopping-term:
   // #########################################################################
 
    /*	vals = [[self_coupling of site 1][T-_coupling site 1][T+_coupling site 1][Z-_coupling site 1][Z+_coupling site 1] .... [X-_coupling site 1][X+_coupling site 1]
  [self_coup site 2][T-_coup site 2]....[X+_coup site N]]
  each of the inner [] contain num_link_var elements -> to store 1 block row in matrix (entire coupling of one site) we need 9 * num_link_var elements

  vals =   [[self, T-, T+, Z-, Z+, Y-, Y+, X-, X+][self, T-, T+, Z-, Z+, Y-, Y+, X-, X+]....]
  */

START_NO_HYPERTHREADS(threading)

  int mu, index,
      num_4link_var=4*4*l->num_parent_eig_vect*l->num_parent_eig_vect,
      num_link_var=4*l->num_parent_eig_vect*l->num_parent_eig_vect,
      start=0;
  vector_PRECISION in_pt, out_pt;
  config_PRECISION D_pt;

  int core_start;
  int core_end;
  compute_core_start_end_custom(start, start+nr_nodes, &core_start, &core_end, l, threading, 1);
  int p_start = num_link_var * nr_nodes * 9 * g.my_rank; // offset in sparse matrix for each process //FIXME: still needed?
  int p_dt_start = g.my_rank * nr_nodes * num_4link_var; 


  int comm_nr[4] = {0, 0, 0, 0}; 	//number elements to communicate in all directions
  int dir;
  int node;

// FIXME: USE c->num_boundary_sites[2*mu+1] instead
	//calculate the number of elements to communicate:
  for (dir = T; dir <= X; dir++){
    for (node = core_start; node < core_end; node++){
      if (op->neighbor_table[5*node+1+dir] >= nr_nodes){
        comm_nr[dir]++;
      }
    }
  }
	//allocate memory for buffers:
  int *buff_i_send[4], *buff_i_recv[4];				// will contain node nr. in reciever processors domain
  complex_PRECISION *buff_d_send[4], *buff_d_recv[4];		// will contain mu- coupling

  int buffer_i_pt, buffer_d_pt;
  MPI_Request r;
  MPI_Status s;

  int i_start = g.my_rank * l->num_inner_lattice_sites * site_var, 
      j_start = g.my_rank * l->num_inner_lattice_sites * site_var; 	//contains own global row and col. start indices
  int neighbors_j_start; //contains column start index of neighboring process
  
  printf("r: %d, \tT: %d, \tZ: %d, \tY: %d, \tX: %d\n", g.my_rank, comm_nr[T], comm_nr[Z], comm_nr[Y], comm_nr[X]);

  int *boundary_table;// = op->c.boundary_table[...];
  int bt_index;


  int num_site_var=site_var;




  for (dir = T; dir <= X; dir++){
    boundary_table = op->c.boundary_table[2*dir];
    buffer_i_pt = 0;

    buff_i_send[dir] = NULL;
    buff_i_recv[dir] = NULL;
    MALLOC(buff_i_send[dir], int, 2 * comm_nr[dir]);
    MALLOC(buff_i_recv[dir], int, 2 * comm_nr[dir]);

    buff_d_send[dir] = NULL;
    buff_d_recv[dir] = NULL;
    MALLOC(buff_d_send[dir], complex_PRECISION, num_link_var * comm_nr[dir]);
    MALLOC(buff_d_recv[dir], complex_PRECISION, num_link_var * comm_nr[dir]);

    memset(buff_i_send[dir], 0, 2 * comm_nr[dir] * sizeof(int));
    memset(buff_i_recv[dir], 0, 2 * comm_nr[dir] * sizeof(int));
    memset(buff_d_send[dir], 0, num_link_var * comm_nr[dir] * sizeof(complex_PRECISION));
    memset(buff_d_recv[dir], 0, num_link_var * comm_nr[dir] * sizeof(complex_PRECISION));
	


    bt_index = 0;
    neighbors_j_start = l->neighbor_rank[2*dir] * l->num_inner_lattice_sites * site_var;
    for (node = core_start; node < core_end; node ++){
      index = 5 * node;

		// make mu+ couplings as usual (Values + Row indices aka. Is)
// A
/*
      for (k = 0; k < SQUARE(num_site_var/2); k ++){
	printf0("k :%d, neighbor_table: %d, index: %d, n4link: %d, dir: %d, nlink: %d\n", k, op->neighbor_table[index], index, num_4link_var, dir, num_link_var);
//	printf0("k :%4d, D: %+-f%+-fi\n", k, CSPLIT(-1.0 * *(op->D + num_4link_var*op->neighbor_table[index] + dir*num_link_var + k)));
      }
      exit(0);
*/


      for (k = 0; k < SQUARE(num_site_var/2); k ++){
//						find correct block row		skip self coupl.		find pos of mu+ coupl. ("2*mu +1" due to structure of vals[self, T-, T+, Z-, Z+...]
//	l->p_PRECISION.mumps_vals[9 * num_link_var)*op->neighbor_table[index] + num_link_var + 		(2*dir + 1)*num_link_var + k]
        *(l->p_PRECISION.mumps_vals + 	(9 * num_link_var)*op->neighbor_table[index] + num_link_var + 		(2*dir + 1)*num_link_var + k) = 
			-1.0 * *(op->D + 	num_4link_var*op->neighbor_table[index] + 	dir*num_link_var + k);
	//					find correct block row				start of mu- coupling
        *(l->p_PRECISION.mumps_Is +	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 		(2*dir +1)*num_link_var + k) = 
			i_start +	num_site_var * op->neighbor_table[index] + 		k%((int)(num_site_var*0.5));
		//	proc start		block row start						fast changing index
      }
// C
      for (k = 0; k < SQUARE(num_site_var/2); k ++){
        *(l->p_PRECISION.mumps_vals +	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 	(2*dir+1)*num_link_var + 	1 * (int)SQUARE(num_site_var/2) + k) = 
			-1.0 * *(op->D + 	num_4link_var*op->neighbor_table[index] + 	dir*num_link_var + 	1 * (int)SQUARE(num_site_var/2) + k);
        *(l->p_PRECISION.mumps_Is + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 	(2*dir +1)*num_link_var + 	1 * (int)SQUARE(num_site_var/2) + k) = 
			i_start +	num_site_var * op->neighbor_table[index] + 		k%(int)(num_site_var*0.5) + 	(int)(num_site_var*0.5);
      }
// B
      for (k = 0; k < SQUARE(num_site_var/2); k ++){
        *(l->p_PRECISION.mumps_vals + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 		(2*dir+1)*num_link_var + 	2 * (int)SQUARE(num_site_var/2) + k) = 
			-1.0 * *(op->D +	num_4link_var*op->neighbor_table[index] + 	dir*num_link_var + 	2 * (int)SQUARE(num_site_var/2) + k); 	//columwise in D
        *(l->p_PRECISION.mumps_Is + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 	(2*dir+1)*num_link_var + 	2 * (int)SQUARE(num_site_var/2) + k) = 
			i_start +	num_site_var * op->neighbor_table[index] + 		k%(int)(num_site_var*0.5) + 	0;
      }
// D
      for (k = 0; k < SQUARE(num_site_var/2); k ++){
        *(l->p_PRECISION.mumps_vals + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 	(2*dir+1)*num_link_var + 	3 * (int)SQUARE(num_site_var/2) + k) = 
			-1.0 * *(op->D + 	num_4link_var*op->neighbor_table[index] + 	dir*num_link_var + 	3 * (int)SQUARE(num_site_var/2) + k); 	//columwise in D
        *(l->p_PRECISION.mumps_Is + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 	(2*dir +1)*num_link_var + 3 * (int)SQUARE(num_site_var/2) + k) = 
			i_start +	num_site_var * op->neighbor_table[index] + 		k%(int)(num_site_var*0.5) + 	(int)(num_site_var*0.5);
      }


	// DO THE COL INDICES aka. Js
	//FIXME: maybe remove comm_nr > 0 ?
      if (comm_nr[dir] > 0 && op->neighbor_table[index+1+dir] >= l->num_inner_lattice_sites){
        for (k = 0; k < SQUARE(num_site_var/2); k ++){
//A
          *(l->p_PRECISION.mumps_Js +	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 		(2*dir +1)*num_link_var + k) = 
			neighbors_j_start + 	num_site_var * boundary_table[bt_index] +	k/((int)(num_site_var*0.5));
//C
	  *(l->p_PRECISION.mumps_Js + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 		(2*dir +1)*num_link_var + 	1 * (int)SQUARE(num_site_var/2) + k) = 
			neighbors_j_start + 	num_site_var * boundary_table[bt_index] +	k/((int)(num_site_var*0.5));
//B
          *(l->p_PRECISION.mumps_Js + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 		(2*dir+1)*num_link_var + 	2 * (int)SQUARE(num_site_var/2) + k) = 
			neighbors_j_start + 	num_site_var * boundary_table[bt_index] +	k/((int)(num_site_var*0.5)) +	(int)(num_site_var*0.5);
//D
        *(l->p_PRECISION.mumps_Js + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 		(2*dir + 1)*num_link_var + 	3 * (int)SQUARE(num_site_var/2) + k) = 
			neighbors_j_start + 	num_site_var * boundary_table[bt_index] +	k/((int)(num_site_var*0.5)) + 	(int)(num_site_var*0.5);

	}	//						boundary_table[op->neighbor_table[index +1 + dir] % l->num_inner_lattice_sites]

	bt_index++;
      } else {	//my neighbor is on same processor
        for (k = 0; k < SQUARE(num_site_var/2); k ++){
//A
	  *(l->p_PRECISION.mumps_Js +	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 		(2*dir +1)*num_link_var + k) = 
          		j_start +	num_site_var * op->neighbor_table[index +1 + dir] +	k/((int)(num_site_var*0.5));
//C 
	  *(l->p_PRECISION.mumps_Js + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 		(2*dir +1)*num_link_var + 	1 * (int)SQUARE(num_site_var/2) + k) = 
			j_start + 	num_site_var * op->neighbor_table[index +1 + dir] +	k/(int)(num_site_var*0.5) + 	0;
//B
          *(l->p_PRECISION.mumps_Js + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 		(2*dir+1)*num_link_var + 	2 * (int)SQUARE(num_site_var/2) + k) = 
			j_start +	num_site_var * op->neighbor_table[index +1 + dir] +	k/(int)(num_site_var*0.5) + 	(int)(num_site_var*0.5);
//D
          *(l->p_PRECISION.mumps_Js + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 		(2*dir+1)*num_link_var + 	3 * (int)SQUARE(num_site_var/2) + k) = 
			j_start +	num_site_var * op->neighbor_table[index +1 + dir] +	k/(int)(num_site_var*0.5) + 	(int)(num_site_var*0.5);
	//		p_start		find start of T- coupling 		slow changing index

        }
      }

      
	// check whether dir neighbor is in halo

      if (op->neighbor_table[index+1+dir] >= l->num_inner_lattice_sites) {
//	printf("(proc=%d) i am node %d, coupling to node %d\n", g.my_rank, op->neighbor_table[index], op->neighbor_table[index+1+dir]);

        // write mu- coupling to buffer
	// also write global target and source site no. to buffer
	// send both buffers


	buffer_d_pt = buffer_i_pt * num_link_var;


        //write mu- couplings:
	//  A* #################################
        for (k = 0; k < SQUARE(site_var/2); k ++){
          *(buff_d_send[dir] + buffer_d_pt + k) = 
				-1.0 * conj_PRECISION(*(op->D	+ num_4link_var*op->neighbor_table[index] + 	dir*num_link_var + 	k));
        }
	// -C* #################################
        for (k = 0; k < SQUARE(site_var/2); k ++){
          *(buff_d_send[dir] + buffer_d_pt + 1 * (int)SQUARE(site_var/2) +	k) = 
				 1.0 * conj_PRECISION(*(op->D 	+ num_4link_var*op->neighbor_table[index] + 	dir*num_link_var + 	1*(int)SQUARE(site_var/2) + k));
        }
	// -B* #################################
        for (k = 0; k < SQUARE(site_var/2); k ++){
          *(buff_d_send[dir] + buffer_d_pt + 2 * (int)SQUARE(site_var/2) + 	k) = 
				 1.0 * conj_PRECISION(*(op->D 	+ num_4link_var*op->neighbor_table[index] + 	dir*num_link_var + 	2 * (int)SQUARE(site_var/2) + k));
        }
	// D* #################################
        for (k = 0; k < SQUARE(site_var/2); k ++){
          *(buff_d_send[dir] + buffer_d_pt + 3 * (int)SQUARE(site_var/2) +	k) = 
				-1.0 * conj_PRECISION(*(op->D	+ num_4link_var*op->neighbor_table[index] + 	dir*num_link_var + 	3 * (int)SQUARE(site_var/2) + k));
        }

	//write i to buffer:
        *(buff_i_send[dir] + 2 * buffer_i_pt) = boundary_table[op->neighbor_table[index + 1 + dir] % l->num_inner_lattice_sites];
        *(buff_i_send[dir] + 2 * buffer_i_pt + 1) = op->neighbor_table[index];
 	buffer_i_pt++;
      }

    }	//loop over nodes

	//send both buffers:
/*
      MPI_Isend(cont void *buf, int count, MPI_Datatype dt, int dest, int tag, MPI_Comm comm, MPI_Request *req);
*/
    if (comm_nr[dir] > 0){
      MPI_Isend(buff_d_send[dir], comm_nr[dir] * num_link_var, MPI_COMPLEX_PRECISION, l->neighbor_rank[2*dir], dir, g.comm_cart, &r);
      MPI_Isend(buff_i_send[dir], 2 * comm_nr[dir], MPI_INT, l->neighbor_rank[2*dir], dir, g.comm_cart, &r);
      printf("process %d send message in direction %d to process %d\n", g.my_rank, dir, l->neighbor_rank[2*dir]);
      	//HOW TO FIND NEIGHBOR?
	//int l.neighbor_rank[8] contains ranks of neighbors
	//in the order [T+ T- Z+ Z- ...]
      MPI_Barrier(MPI_COMM_WORLD);
	//ensures buffer-sending order: T, Z, Y, X
	//FIXME remove barrier. already replaced by tag = dir
    }
  }	// loop over directions


	// mu- couplings
  for (dir = T; dir <= X; dir++){
    if (comm_nr[dir] > 0){
	//there is stuff to communicate in direction dir

//	MPI_Recv(void *buf, int count, MPI_Datatype dt, int source, int tag, MPI_Comm comm, MPI_Status *status);
//			len					[T+, T-, Z+, Z-...]
      MPI_Recv(buff_d_recv[dir], num_link_var * comm_nr[dir], MPI_COMPLEX_PRECISION, l->neighbor_rank[2*dir+1], dir, g.comm_cart, &s);
      MPI_Recv((buff_i_recv[dir]), 2 * comm_nr[dir], MPI_INT, l->neighbor_rank[2*dir+1], dir, g.comm_cart, &s);
										    // tag = dir
      printf("process %d recieved message from process %d\n", g.my_rank, l->neighbor_rank[2*dir+1]);
	

      neighbors_j_start = l->neighbor_rank[2*dir+1] * l->num_inner_lattice_sites * site_var;	
	// copy buffer content to mumps_vals
      for (buffer_i_pt = 0; buffer_i_pt < comm_nr[dir]; buffer_i_pt++){
        buffer_d_pt = num_link_var * buffer_i_pt;

	// A* #################################
	for (k = 0; k < SQUARE(num_site_var / 2); k++ ){
	  *(l->p_PRECISION.mumps_vals + 9 * num_link_var * buff_i_recv[dir][2 * buffer_i_pt + 1]	+ num_link_var + 	2*dir*num_link_var + k) = 
			*(buff_d_recv[dir] + buffer_d_pt + k);
//		straight copy (orders are set before send)
	  *(l->p_PRECISION.mumps_Is + 9 * num_link_var * buff_i_recv[dir][2 * buffer_i_pt + 1] 	+ num_link_var +	2*dir*num_link_var + k) = 
			i_start +	num_site_var * buff_i_recv[dir][2 * buffer_i_pt] + 		k/(int)(num_site_var*0.5);
	  *(l->p_PRECISION.mumps_Js + 9 * num_link_var * buff_i_recv[dir][2 * buffer_i_pt + 1] 	+ num_link_var +	2*dir*num_link_var + k) = 
			neighbors_j_start +	num_site_var * buff_i_recv[dir][2 * buffer_i_pt + 1] +	k%(int)(num_site_var*0.5);
        }
	// -C* #################################
	for (k = 0; k < SQUARE(num_site_var / 2); k++ ){
	  *(l->p_PRECISION.mumps_vals + 9 * num_link_var * buff_i_recv[dir][2 * buffer_i_pt + 1]+ num_link_var + 	2*dir*num_link_var + 1 * SQUARE((int)(num_site_var*0.5)) + k) =
			*(buff_d_recv[dir] + buffer_d_pt + 1 * SQUARE((int)(num_site_var*0.5)) + k);
//		straight copy (orders are set before send)
	  *(l->p_PRECISION.mumps_Is + 9 * num_link_var * buff_i_recv[dir][2 * buffer_i_pt + 1] 	+ num_link_var +	2*dir*num_link_var + 1 * SQUARE((int)(num_site_var*0.5)) + k) = 
			i_start +	num_site_var * buff_i_recv[dir][2 * buffer_i_pt] + 		k/(int)(num_site_var*0.5);
	  *(l->p_PRECISION.mumps_Js + 9 * num_link_var * buff_i_recv[dir][2 * buffer_i_pt + 1] 	+ num_link_var +	2*dir*num_link_var + 1 * SQUARE((int)(num_site_var*0.5)) + k) = 
			neighbors_j_start +	num_site_var * buff_i_recv[dir][2 * buffer_i_pt + 1] +	k%(int)(num_site_var*0.5) + num_site_var*0.5;
        }
	// -B* #################################
	for (k = 0; k < SQUARE(num_site_var / 2); k++ ){
	  *(l->p_PRECISION.mumps_vals + 9 * num_link_var * buff_i_recv[dir][2 * buffer_i_pt + 1]+ num_link_var + 	2*dir*num_link_var + 2 * SQUARE((int)(num_site_var*0.5)) + k) =
			*(buff_d_recv[dir] + buffer_d_pt + 2 * SQUARE((int)(num_site_var*0.5)) + k);
//		straight copy (orders are set before send)
	  *(l->p_PRECISION.mumps_Is + 9 * num_link_var * buff_i_recv[dir][2 * buffer_i_pt + 1] 	+ num_link_var +	2*dir*num_link_var + 2 * SQUARE((int)(num_site_var*0.5)) + k) = 
			i_start +	num_site_var * buff_i_recv[dir][2 * buffer_i_pt] + 		k/(int)(num_site_var*0.5) + num_site_var*0.5;
	  *(l->p_PRECISION.mumps_Js + 9 * num_link_var * buff_i_recv[dir][2 * buffer_i_pt + 1] 	+ num_link_var +	2*dir*num_link_var + 2 * SQUARE((int)(num_site_var*0.5)) + k) = 
			neighbors_j_start +	num_site_var * buff_i_recv[dir][2 * buffer_i_pt + 1] +	k%(int)(num_site_var*0.5);
        } 
	// D* #################################
	for (k = 0; k < SQUARE(num_site_var / 2); k++ ){
	  *(l->p_PRECISION.mumps_vals + 9 * num_link_var * buff_i_recv[dir][2 * buffer_i_pt + 1]+ num_link_var + 	2*dir*num_link_var + 3 * SQUARE((int)(num_site_var*0.5)) + k) =
			*(buff_d_recv[dir] + buffer_d_pt + 3 * SQUARE((int)(num_site_var*0.5)) + k);
//		straight copy (orders are set before send)
	  *(l->p_PRECISION.mumps_Is + 9 * num_link_var * buff_i_recv[dir][2 * buffer_i_pt + 1] 	+ num_link_var +	2*dir*num_link_var + 3 * SQUARE((int)(num_site_var*0.5)) + k) = 
			i_start +	num_site_var * buff_i_recv[dir][2 * buffer_i_pt] + 		k/(int)(num_site_var*0.5) + num_site_var*0.5;
	  *(l->p_PRECISION.mumps_Js + 9 * num_link_var * buff_i_recv[dir][2 * buffer_i_pt + 1] 	+ num_link_var +	2*dir*num_link_var + 3 * SQUARE((int)(num_site_var*0.5)) + k) = 
			neighbors_j_start +	num_site_var * buff_i_recv[dir][2 * buffer_i_pt + 1] +	k%(int)(num_site_var*0.5) + num_site_var*0.5;
        }

// message comes from process l->neighbor_rank[2*dir+1]
// col nr. in sparse matrix:  	l->neighbor_rank[2*dir+1] * 
//				l->num_inner_lattice_sites * 
//				site_var
      }
    }	//end if (comm_nr[dir] > 0)


		//regular mu- coupling for all nodes except communicated ones
    buffer_i_pt = 0;
    for (i = 0; i < core_end; i++){	//loop over lattice sites
      if (comm_nr[dir] > 0){
        while (i == *(buff_i_recv[dir] + 2 * buffer_i_pt + 1)){
		//skip this node because it was already communicated
          i++;
          buffer_i_pt++;
          if (i >= core_end) break;
        }
	if (i >= core_end) break;
      }
      index = 5 * i;
		//regular mu- coupling
	// A* ##################################
      for (k = 0; k < SQUARE(num_site_var/2); k ++){
//	skip to proc start	find correct block row				skip self coupl.	find pos of T- coupl.
        *(l->p_PRECISION.mumps_vals + (9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 	 	2*dir*num_link_var + k) = 
			-1.0 * conj_PRECISION(*(op->D + 	num_4link_var*op->neighbor_table[index] + 	dir*num_link_var + k));
	//				    proc start		find correct block row				start of T- coupling

        *(l->p_PRECISION.mumps_Is + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 		2*dir*num_link_var + k) = 
			i_start +	num_site_var * op->neighbor_table[index + 1 + dir] + 		k/(int)(num_site_var*0.5);
	//	proc start		block row start						fast changing index

        *(l->p_PRECISION.mumps_Js + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 		2*dir*num_link_var + k) = 
			j_start +	num_site_var * op->neighbor_table[index] +	k%(int)(num_site_var*0.5);
	//		no p_start	find start of T- coupling 				slow changing index	
      }
	// -C* ##################################
      for (k = 0; k < SQUARE(num_site_var/2); k ++){
        *(l->p_PRECISION.mumps_vals + (9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 	 	2*dir*num_link_var + 1 * SQUARE((int)(num_site_var*0.5)) + k ) = 
			1.0 * conj_PRECISION(*(op->D + 	num_4link_var*op->neighbor_table[index] + 	dir*num_link_var + 1 * SQUARE((int)(num_site_var*0.5)) + k));
        *(l->p_PRECISION.mumps_Is + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 		2*dir*num_link_var + 1 * SQUARE((int)(num_site_var*0.5)) + k) = 
			i_start +	num_site_var * op->neighbor_table[index + 1 + dir] + 		k/(int)(num_site_var*0.5);
        *(l->p_PRECISION.mumps_Js + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 		2*dir*num_link_var + 1 * SQUARE((int)(num_site_var*0.5)) + k) = 
			j_start +	num_site_var * op->neighbor_table[index] +	k%(int)(num_site_var*0.5) + num_site_var*0.5;
      }
        // -B* #################################
      for (k = 0; k < SQUARE(num_site_var/2); k ++){
        *(l->p_PRECISION.mumps_vals + (9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 	 	2*dir*num_link_var + 2 * SQUARE((int)(num_site_var*0.5)) + k ) = 
			1.0 * conj_PRECISION(*(op->D + 	num_4link_var*op->neighbor_table[index] + 	dir*num_link_var + 2 * SQUARE((int)(num_site_var*0.5)) + k));
        *(l->p_PRECISION.mumps_Is + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 		2*dir*num_link_var + 2 * SQUARE((int)(num_site_var*0.5)) + k) = 
			i_start +	num_site_var * op->neighbor_table[index + 1 + dir] + 		k/(int)(num_site_var*0.5) + num_site_var*0.5;
        *(l->p_PRECISION.mumps_Js + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 		2*dir*num_link_var + 2 * SQUARE((int)(num_site_var*0.5)) + k) = 
			j_start +	num_site_var * op->neighbor_table[index] +	k%(int)(num_site_var*0.5);
      }
	// D* #################################
      for (k = 0; k < SQUARE(num_site_var/2); k ++){
        *(l->p_PRECISION.mumps_vals + (9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 	 	2*dir*num_link_var + 3 * SQUARE((int)(num_site_var*0.5)) + k ) = 
			-1.0 * conj_PRECISION(*(op->D + 	num_4link_var*op->neighbor_table[index] + 	dir*num_link_var + 3 * SQUARE((int)(num_site_var*0.5)) + k));
        *(l->p_PRECISION.mumps_Is + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 		2*dir*num_link_var + 3 * SQUARE((int)(num_site_var*0.5)) + k) = 
			i_start +	num_site_var * op->neighbor_table[index + 1 + dir] + 		k/(int)(num_site_var*0.5) + num_site_var*0.5;
        *(l->p_PRECISION.mumps_Js + 	(9 * num_link_var)*op->neighbor_table[index] + 	num_link_var + 		2*dir*num_link_var + 3 * SQUARE((int)(num_site_var*0.5)) + k) = 
			j_start +	num_site_var * op->neighbor_table[index] +	k%(int)(num_site_var*0.5) + num_site_var*0.5;
      }

    }	//loop over nodes
  }	//loop over directions




  END_NO_HYPERTHREADS(threading)


/*
// 		TESTING THE INPUT


            vector_PRECISION test_vec=NULL;
            vector_PRECISION test_vec2=NULL;	
            vector_PRECISION test_vec3=NULL;	
            START_MASTER(threading)
              MALLOC(test_vec, complex_PRECISION, (l->p_PRECISION.v_end-l->p_PRECISION.v_start));
              MALLOC(test_vec2, complex_PRECISION, (l->p_PRECISION.v_end-l->p_PRECISION.v_start));
              MALLOC(test_vec3, complex_PRECISION, (l->p_PRECISION.v_end-l->p_PRECISION.v_start));
            END_MASTER(threading)
            memset(test_vec, 0, (l->p_PRECISION.v_end - l->p_PRECISION.v_start) * sizeof(complex_PRECISION));
            memset(test_vec3, 0, (l->p_PRECISION.v_end - l->p_PRECISION.v_start) * sizeof(complex_PRECISION));
            vector_PRECISION_define( test_vec2, 1.0, l->p_PRECISION.v_start, l->p_PRECISION.v_end, l );
	    MPI_Barrier(MPI_COMM_WORLD);
            int nx = (SQUARE(l->num_lattice_site_var)*9) * l->num_inner_lattice_sites;

            spmv_PRECISION(test_vec, px->b, l->p_PRECISION.mumps_vals, l->p_PRECISION.mumps_Is, l->p_PRECISION.mumps_Js, nx, &(l->p_PRECISION), l, threading );

//            apply_operator_PRECISION(test_vec3, px->b, &(l->p_PRECISION), l, threading );
//            apply_operator_PRECISION(test_vec3, px->b, px, l, threading );
	apply_operator_PRECISION(test_vec3, px->b, px, l, threading );
//	apply_coarse_operator_PRECISION(test_vec3, px->b, op, l, threading );
//vector_PRECISION eta, vector_PRECISION phi, operator_PRECISION_struct *op, level_struct *l, struct Thread *threading
            int ix;
		
            for (ix = 0; ix < l->p_PRECISION.v_end - l->p_PRECISION.v_start; ix++){
              printf0("diff: %+-f%+-fi, spmv: %+-f%+-fi, dd: %+-f%+-fi, px->b[%4d]: %+-f%+-fi\n", CSPLIT(test_vec[ix] - test_vec3[ix]), CSPLIT(test_vec[ix]), CSPLIT(test_vec3[ix]), ix,  CSPLIT(px->b[ix]));
            }
            exit(0);
*/
   
}

// ---







void smoother_PRECISION( vector_PRECISION phi, vector_PRECISION Dphi, vector_PRECISION eta,
                         int n, const int res, level_struct *l, struct Thread *threading ) {
  
  ASSERT( phi != eta );

  START_MASTER(threading);
  PROF_PRECISION_START( _SM );
  END_MASTER(threading);
  
  if ( g.method == 1 ) {
    additive_schwarz_PRECISION( phi, Dphi, eta, n, res, &(l->s_PRECISION), l, threading );
  } else if ( g.method == 2 ) {
    red_black_schwarz_PRECISION( phi, Dphi, eta, n, res, &(l->s_PRECISION), l, threading );
  } else if ( g.method == 3 ) {
    sixteen_color_schwarz_PRECISION( phi, Dphi, eta, n, res, &(l->s_PRECISION), l, threading );
  } else {
    int start = threading->start_index[l->depth];
    int end   = threading->end_index[l->depth];
    START_LOCKED_MASTER(threading)
    l->sp_PRECISION.initial_guess_zero = res;
    l->sp_PRECISION.num_restart = n;
    END_LOCKED_MASTER(threading)
    if ( g.method == 4 || g.method == 6 ) {
      if ( g.odd_even ) {
        if ( res == _RES ) {
          apply_operator_PRECISION( l->sp_PRECISION.x, phi, &(l->p_PRECISION), l, threading );
          vector_PRECISION_minus( l->sp_PRECISION.x, eta, l->sp_PRECISION.x, start, end, l );
        }
        block_to_oddeven_PRECISION( l->sp_PRECISION.b, res==_RES?l->sp_PRECISION.x:eta, l, threading );
        START_LOCKED_MASTER(threading)
        l->sp_PRECISION.initial_guess_zero = _NO_RES;
        END_LOCKED_MASTER(threading)
        if ( g.method == 6 ) {
          if ( l->depth == 0 ) g5D_solve_oddeven_PRECISION( &(l->sp_PRECISION), &(l->oe_op_PRECISION), l, threading );
          else g5D_coarse_solve_odd_even_PRECISION( &(l->sp_PRECISION), &(l->oe_op_PRECISION), l, threading );
        } else {
          if ( l->depth == 0 ) solve_oddeven_PRECISION( &(l->sp_PRECISION), &(l->oe_op_PRECISION), l, threading );
          else coarse_solve_odd_even_PRECISION( &(l->sp_PRECISION), &(l->oe_op_PRECISION), l, threading );
        }
        if ( res == _NO_RES ) {
          oddeven_to_block_PRECISION( phi, l->sp_PRECISION.x, l, threading );
        } else {
          oddeven_to_block_PRECISION( l->sp_PRECISION.b, l->sp_PRECISION.x, l, threading );
          vector_PRECISION_plus( phi, phi, l->sp_PRECISION.b, start, end, l );
        }
      } else {
        START_LOCKED_MASTER(threading)
        l->sp_PRECISION.x = phi; l->sp_PRECISION.b = eta;
        END_LOCKED_MASTER(threading)
        fgmres_PRECISION( &(l->sp_PRECISION), l, threading );
      }
    } else if ( g.method == 5 ) {
      vector_PRECISION_copy( l->sp_PRECISION.b, eta, start, end, l );
      bicgstab_PRECISION( &(l->sp_PRECISION), l, threading );
      vector_PRECISION_copy( phi, l->sp_PRECISION.x, start, end, l );
    }
    ASSERT( Dphi == NULL );
  }
  
  START_MASTER(threading);
  PROF_PRECISION_STOP( _SM, n );
  END_MASTER(threading);
}


void vcycle_PRECISION( vector_PRECISION phi, vector_PRECISION Dphi, vector_PRECISION eta,
                       int res, level_struct *l, struct Thread *threading ) {

  if ( g.interpolation && l->level>0 ) {
    for ( int i=0; i<l->n_cy; i++ ) {
      if ( i==0 && res == _NO_RES ) {
        restrict_PRECISION( l->next_level->p_PRECISION.b, eta, l, threading );
      } else {
        int start = threading->start_index[l->depth];
        int end   = threading->end_index[l->depth];
        apply_operator_PRECISION( l->vbuf_PRECISION[2], phi, &(l->p_PRECISION), l, threading );
        vector_PRECISION_minus( l->vbuf_PRECISION[3], eta, l->vbuf_PRECISION[2], start, end, l );
        restrict_PRECISION( l->next_level->p_PRECISION.b, l->vbuf_PRECISION[3], l, threading );
      }
      if ( !l->next_level->idle ) {
        START_MASTER(threading)
        if ( l->depth == 0 )
          g.coarse_time -= MPI_Wtime();
        END_MASTER(threading)
        if ( l->level > 1 ) {
          if ( g.kcycle )
            fgmres_PRECISION( &(l->next_level->p_PRECISION), l->next_level, threading );
          else
            vcycle_PRECISION( l->next_level->p_PRECISION.x, NULL, l->next_level->p_PRECISION.b, _NO_RES, l->next_level, threading );
        } else {
          if ( g.odd_even ) {
            if ( g.method == 6 ) {
              g5D_coarse_solve_odd_even_PRECISION( &(l->next_level->p_PRECISION), &(l->next_level->oe_op_PRECISION), l->next_level, threading );
            } else {

              START_MASTER(threading)
              g.coarsest_time -= MPI_Wtime();
              END_MASTER(threading)

              coarse_solve_odd_even_PRECISION( &(l->next_level->p_PRECISION), &(l->next_level->oe_op_PRECISION), l->next_level, threading );

              START_MASTER(threading)
              g.coarsest_time += MPI_Wtime();
              END_MASTER(threading)

            }
          } else {

            // ---------------------------------------------------------------------------
            gmres_PRECISION_struct* px = &(l->next_level->p_PRECISION);
            level_struct* lx = l->next_level;

            double t0,t1;	//timing
            double mumps_setup_time, mumps_job4_time, mumps_job3_time, mumps_verify_time;




            int num_eig_vect = lx->num_parent_eig_vect, 
            vector_size = lx->num_lattice_site_var,
            clover_size = (2*num_eig_vect*num_eig_vect+num_eig_vect);

            int start, end;
            compute_core_start_end_custom(0, lx->num_inner_lattice_sites, &start, &end, lx, threading, 1);
            // 1. prepare sparse matrix for MUMPS (take into account : matrix-vector mult receives : px->op)




            vector_PRECISION_define_random( px->b, px->v_start, px->v_end, lx );
//            vector_PRECISION_define( px->b, 1.0, px->v_start, px->v_end, lx);
//            memset(px->b, 0, (lx->p_PRECISION.v_end - lx->p_PRECISION.v_start) * sizeof(complex_PRECISION));


            t0 = MPI_Wtime();
//	    mumps_prepare_PRECISION(px->op->clover+start*clover_size, (end-start)*vector_size,lx, threading );
	    mumps_setup_PRECISION(lx, threading);

//		SET UP IS DONE IN setup_generic.c 
//		TODO: 	1. When does this happen? 
//			2. global mumps instance
//			3. change indices Is and Js to match to fortan (aka. +1 everywhere)


            // 2. analyze+factorize
//############################## MUMPS RELATED STUFF ###############################################

#define ICNTL(I) icntl[(I) -1]	//macro according to docu //bridges from fortran indices to c
//#define CNTL(I) cntl[(I) -1]	//same macro for cntl <-- does not work, messes up the line above.

//            CMUMPS_STRUC_C mumps_id;
            int i;
            int chunklen = SQUARE(lx->num_lattice_site_var) *lx->num_inner_lattice_sites *9;

            int mumps_n = lx->num_lattice_site_var * lx->num_inner_lattice_sites * g.num_processes;	//order of Matrix
            int nnz = chunklen * g.num_processes;	//number of nonzero elements
            int nnz_loc = chunklen;
/*
            int* irn_loc = NULL;	//global rowindex local on each process
            int* jcn_loc = NULL;
            START_MASTER(threading)
              MALLOC(irn_loc, int, nnz_loc);
              MALLOC(jcn_loc, int, nnz_loc);
            END_MASTER(threading)

            for (i = 0; i < nnz_loc; i++){	//increase indices by one to match fortran indexing
              *(irn_loc + i) = *(lx->p_PRECISION.mumps_Is + i) + 1;	//save copies, otherwise SPMV won't work anymore!
              *(jcn_loc + i) = *(lx->p_PRECISION.mumps_Js + i) + 1;	//increase indices by one to match fortran indexing
            }
	
	 	//test whether c indices match to fortran
              for (i = 0; i < nnz_loc; i++){	
                if (irn_loc[i] <= 0 || irn_loc[i] > mumps_n) printf("row_index out of range  on index %8d, val: %8d, on p: %d\n", i, irn_loc[i], g.my_rank);
                if (jcn_loc[i] <= 0 || jcn_loc[i] > mumps_n) printf("column_index out of range on index %8d, val: %8d, on p: %d\n", i, jcn_loc[i], g.my_rank);


                if ( lx->p_PRECISION.mumps_Is[i] < 0 || lx->p_PRECISION.mumps_Is[i] >= mumps_n) printf("row_index out of range  on index %8d, val: %8d, on p: %d\n", i, lx->p_PRECISION.mumps_Is[i], g.my_rank);
                if ( lx->p_PRECISION.mumps_Js[i] < 0 || lx->p_PRECISION.mumps_Js[i] >= mumps_n) printf("column_index out of range on index %8d, val: %8d, on p: %d\n", i, lx->p_PRECISION.mumps_Js[i], g.my_rank);
              }


            complex_PRECISION* A_loc = NULL;
            START_MASTER(threading)
              MALLOC(A_loc, complex_PRECISION, nnz_loc);
            END_MASTER(threading)
            for (i = 0; i < nnz_loc; i++){
              A_loc[i] = lx->p_PRECISION.mumps_vals[i];	//save copy, mumps changes A_loc
		//this if statement was used for debugging -> not needed anymore 
              if (A_loc[i] == 0) printf("P%d: A_loc is 0 on index %8d, vals: %8.6f+%8.6fi\n", g.my_rank, i, creal(A_loc[i]), cimag(A_loc[i]));
            }



		//confige MUMPS_struct
            g.mumps_id.job = JOB_INIT;
            g.mumps_id.par = 1;
            g.mumps_id.sym = 0;
            g.mumps_id.comm_fortran = USE_COMM_WORLD;
            cmumps_c(&(g.mumps_id));

            g.mumps_id.ICNTL(5) = 0;	//assembled matrix
            g.mumps_id.ICNTL(18) = 3; 	//distributed local triplets for analysis and factorization
//          mumps_id.ICNTL(18) = 0; 	//centralized triplets for analysis and factorization on host

            g.mumps_id.ICNTL(20) = 10;	//distributed RHS. compare to inctl(20) = 11
//          mumps_id.ICNTL(14) = 50; 	//percentage increase of estimated working space	//default: 20 - 30

            g.mumps_id.ICNTL(35) = 2;	//BLR feature is activated during factorization and solution phase
//          mumps_id.ICNTL(35) = 3;	//BLR feature is activablrted during factorization, not used in solve
            g.mumps_id.cntl[6] = 5e-3;	//dropping parameter ε	(absolute error)	//original 7 but in c 6

            g.mumps_id.n = mumps_n;	//needed at least on P0
            g.mumps_id.nnz_loc = nnz_loc;
            g.mumps_id.irn_loc = irn_loc;
            g.mumps_id.jcn_loc = jcn_loc;
            g.mumps_id.a_loc = A_loc;

//outputs
            g.mumps_id.ICNTL(1) = 6;	//error messages
            g.mumps_id.ICNTL(2) = -1;	//diagnostic printing and statistics local to each MPI process
            g.mumps_id.ICNTL(3) = 0;	//global information, collected on host (default 6)
            g.mumps_id.ICNTL(4) = 0;	//level of printing for error, warning, and diagnostic messages (default 2)


            mumps_setup_time = MPI_Wtime() - t0;


            t0 = MPI_Wtime();
//          mumps_id.job = 6;	//analyze factorize solve
            g.mumps_id.job = 4;	//analyze factorize
//          mumps_id.job = 1;	//analyze
            cmumps_c(&(g.mumps_id));
            mumps_job4_time = MPI_Wtime() - t0;
*/

            t0 = MPI_Wtime();
//######### SET UP RHS #############
            int rhs_len = lx->p_PRECISION.v_end-lx->p_PRECISION.v_start;  //entire vector eta
            complex_PRECISION* rhs_loc = NULL;
//            complex_PRECISION* rhs_loc = px->b;
						//set pointer of rhs to eta
						//FIXME: pointer rhs must be changed, mumps probably change rhs!
            int* irhs_loc;		//row indices for each el in rhs_loc
            START_MASTER(threading)
              MALLOC(irhs_loc, int, rhs_len);
              MALLOC(rhs_loc, complex_PRECISION, rhs_len);
            END_MASTER(threading)
            int Nloc_RHS = rhs_len; 		//no of rows in local rhs
            int LRHS_loc = rhs_len; 		//local leading dimension

            for (i = 0; i < rhs_len; i++){	//set the rhs-indices to global values
              irhs_loc[i] = g.my_rank * rhs_len + i+1;		//+1 due to fortran indexing
              rhs_loc[i] = px->b[i];				//save copy since mumps may change content
            }

            g.mumps_id.nloc_rhs = Nloc_RHS;
            g.mumps_id.rhs_loc = rhs_loc;
            g.mumps_id.irhs_loc = irhs_loc;	
            g.mumps_id.lrhs_loc = LRHS_loc; //leading dimension


 //centralized solution, definitely mention it!
            complex_PRECISION* SOL;
            if (g.my_rank == 0){
              START_MASTER(threading)
                MALLOC(SOL, complex_PRECISION, mumps_n);
              END_MASTER(threading)
              memset(SOL, 0, mumps_n * sizeof(complex_PRECISION));
              g.mumps_id.rhs = SOL;
            }
            mumps_setup_time += MPI_Wtime() - t0;

            // 3. solve!
            t0 = MPI_Wtime();
            g.mumps_id.job = 3;		// solve
            cmumps_c(&(g.mumps_id));
            mumps_job3_time = MPI_Wtime() - t0;

//            MPI_Finalize();
//            exit(0);
           
//####################### CHECK RELATIVE RESIDUAL ##################################
/*
0. Scatter SOL to SOL_dist
1. find A * SOL
2. compute vector_PRECISION_minus A*SOL - eta
3. compute || A*SOL - eta || and || eta ||
4. divide both for relative residual
*/
            t0 = MPI_Wtime();
            int nx = (SQUARE(lx->num_lattice_site_var)*9) * lx->num_inner_lattice_sites;

//	###################### 0. ######################
            vector_PRECISION SOL_dist=NULL;	//will contain distributed Solution
            START_MASTER(threading)
              MALLOC(SOL_dist, complex_PRECISION, (lx->p_PRECISION.v_end-lx->p_PRECISION.v_start));
            END_MASTER(threading)
            memset(SOL_dist, 0, (lx->p_PRECISION.v_end - lx->p_PRECISION.v_start) * sizeof(complex_PRECISION));
//			scatter SOL to SOL_dist on each process
            int send_count = (lx->p_PRECISION.v_end-lx->p_PRECISION.v_start);
//MPI_Scatter(void* send_data, int send_count, MPI_Datatype send_datatype, void* recv_data, int recv_count, MPI_Datatype recv_datatype, int root, MPI_Comm communicator)
            MPI_Scatter(SOL, send_count, MPI_COMPLEX_PRECISION, SOL_dist, send_count, MPI_COMPLEX_PRECISION, 0, MPI_COMM_WORLD);

//	###################### 1. ######################
//			allocate and set new "eta" vector
            vector_PRECISION mumps_eta=NULL;	//will contain the product of A * Sol_dist

            START_MASTER(threading)
              MALLOC(mumps_eta, complex_PRECISION, lx->vector_size);
            END_MASTER(threading)
            memset(mumps_eta, 0, (lx->p_PRECISION.v_end - lx->p_PRECISION.v_start) * sizeof(complex_PRECISION));

            int nxy = (SQUARE(lx->num_lattice_site_var)*9) * lx->num_inner_lattice_sites;
//spmv_PRECISION(mumps_eta, SOL_dist, lx->p_PRECISION.mumps_vals, lx->p_PRECISION.mumps_Is, lx->p_PRECISION.mumps_Js, nxy, &(lx->p_PRECISION), lx, threading );
//apply_coarse_operator_PRECISION(mumps_eta, SOL_dist, px->op, lx, threading );

            apply_operator_PRECISION(mumps_eta, SOL_dist, px, lx, threading );

//	###################### 2. ######################
//void vector_PRECISION_minus( vector_PRECISION z, vector_PRECISION x, vector_PRECISION y, int start, int end, level_struct *l ); // z := x - y
//            vector_PRECISION_minus(mumps_eta, mumps_eta, px->b, 0, (lx->p_PRECISION.v_end-lx->p_PRECISION.v_start), lx);
            vector_PRECISION_minus(mumps_eta, mumps_eta, px->b, 0, (lx->p_PRECISION.v_end-lx->p_PRECISION.v_start), lx);

//	###################### 3. ######################
//PRECISION global_norm_PRECISION( vector_PRECISION phi, int start, int end, level_struct *l, struct Thread *threading );
//  PRECISION mumps_norm;
//            PRECISION mumps_norm = global_norm_PRECISION( mumps_eta, 0, (lx->p_PRECISION.v_end-lx->p_PRECISION.v_start), lx, threading );
            PRECISION mumps_norm = global_norm_PRECISION( mumps_eta, 0, (lx->p_PRECISION.v_end-lx->p_PRECISION.v_start), lx, threading );
//  PRECISION DD_norm;
            PRECISION b_norm = global_norm_PRECISION(px->b, 0, (lx->p_PRECISION.v_end-lx->p_PRECISION.v_start), lx, threading );

//	###################### 4. ######################
  //PRECISION rr;
            mumps_verify_time = MPI_Wtime() - t0;
            printf0("rr: %6f,\tmumps_norm:%6f,\tb_norm:%6f\n", (mumps_norm/b_norm), mumps_norm, b_norm);
            exit(0);


// finish mumps
            t0 = MPI_Wtime();
            g.mumps_id.job = JOB_END;
            cmumps_c(&(g.mumps_id));
            mumps_setup_time += MPI_Wtime() - t0;


            mumps_verify_time += MPI_Wtime() - t0;

            t0 = MPI_Wtime();
            int nr_iters_gmres = fgmres_PRECISION(px, lx, threading );
				//solution will be in px->x
  
            t1 = MPI_Wtime();

            printf0("fgmres time = %f\n", t1-t0);
            printf0("MUMPS times:\nset up: %f,\tanalyze+factorize: %f,\tsolve: %f, verify: %f\n", mumps_setup_time, mumps_job4_time, mumps_job3_time, mumps_verify_time);

            printf0("stopping ...\n");
            MPI_Finalize();
            exit(0);

            // ---------------------------------------------------------------------------

          }
        }
        START_MASTER(threading)
        if ( l->depth == 0 )
          g.coarse_time += MPI_Wtime();
        END_MASTER(threading)
      }
      if( i == 0 && res == _NO_RES )
        interpolate3_PRECISION( phi, l->next_level->p_PRECISION.x, l, threading );
      else
        interpolate_PRECISION( phi, l->next_level->p_PRECISION.x, l, threading );
      smoother_PRECISION( phi, Dphi, eta, l->post_smooth_iter, _RES, l, threading );
      res = _RES;
    }
  } else {
    smoother_PRECISION( phi, Dphi, eta, (l->depth==0)?l->n_cy:l->post_smooth_iter, res, l, threading );
  }
}
