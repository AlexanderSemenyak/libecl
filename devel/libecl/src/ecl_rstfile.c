/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'ecl_rstfile.c' is part of ERT - Ensemble based Reservoir Tool. 
    
   ERT is free software: you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, either version 3 of the License, or 
   (at your option) any later version. 
    
   ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE.   
    
   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
   for more details. 
*/

/*****************************************************************/
/*                   R E S T A R T   F I L E S                   */
/*****************************************************************/


/*
  This file is included from the ecl_file.c file with a #include
  statement, i.e. it is the same compilation unit as ecl_file. The
  seperation is only to increase readability.  
*/

/* 
   There is no special datastructure for working with restart files,
   they are 100% stock ecl_file instances with the following limited
   structure:

   * They are organized in blocks; where each block starts with a
     SEQNUM keyword, which contains the report step.

   * Each block contains an INTEHEAD keyword, immediately after the
     SEQNUM keyword, which contains the true simulation date of of the
     block, and also some other data. Observe that also INIT files and
     GRID files contain an INTEHEAD keyword.

   Here comes a couple of function which utilize this knowledge about
   the content and structure of restart files.
*/


/*

About the time-direction
========================

For the following discussion we will focus on the following simplified
unified restart file. The leading number is the global index of the
keyword, the value in [] corresponds to the relevant part of the
content of the keyword on the line, the labels A,B,C,D,E are used for
references in the text below.

 0 | SEQNUM   [0]           \  A     
 1 | INTEHEAD [01.01.2005]  |
 2 | PRESSURE [... ]        |        
 3 | SWAT     [...]         |  
   | -----------------------+
 4 | SEQNUM   [5]           |  B 
 5 | INTEHEAD [01.06.2005]  |
 6 | PRESSURE [... ]        |
 7 | SWAT     [...]         |
   |------------------------+
 8 | SEQNUM   [10]          |  C
 9 | INTEHEAD [01.12.2006]  |
10 | PRESSURE [...]         |
11 | SWAT     [...]         |
   |------------------------+
12 | SEQNUM   [20]          |  D
13 | INTEHEAD [01.12.2007]  |
14 | PRESSURE [...]         |
15 | SWAT     [...]         |
16 | OIL_DEN  [...]         |
   |------------------------+
12 | SEQNUM   [40]          |  E
13 | INTEHEAD [01.12.2009]  |
14 | PRESSURE [...]         |
15 | SWAT     [...]         /
                          

This restart file has the following features:

 o It contains in total 16 keywords.

 o It contains 5 blocks of collected keywords corresponding to one
   time instant, each of these blocks is called a report_step,
   typcially coming from one DATES keyword in the ECLIPSE
   datafile. Observe that the file does not have the block structure
   visualized on this figure, the only thing separating the blocks in
   the file is the occurence of a SEQNUM keyword.
 
 o Only a few of the report steps are present, namely 0, 5, 10, 20 and
   40.

 o The different keywords are not equally long, the fourth block has
   an extra keyword OIL_DEN.

To adress these keywords and blocks using different time coordinates
we have introduced the following concepts:

 report_step: This corresponds to the value of the SEQNUM keword,
    i.e. to do queries based on the report_step we must load the
    seqnum kewyord and read the value.

        ecl_file_get_unrstmap_report_step( ecl_file , 0 ) => A
        ecl_file_get_unrstmap_report_step( ecl_file , 1 ) => NULL

        ecl_file_has_report_step( ecl_file , 5 ) => True
        ecl_file_has_report_step( ecl_file , 2 ) => False

 sim_time: This correpsonds to the true simulation time of the report
    step, the simulation time is stored as integers DAY, MONTH, YEAR
    in the INTEHEAD keyword; the function INTEHEAD_date() will extract
    the DAY, MONTH and YEAR values from an INTEHEAD keyword instance
    and convert to a time_t instance. The functions:

     ecl_file_get_unrstmap_sim_time() and ecl_file_has_has_sim_time() 

    can be used to query for simulation times and get the
    corresponding block maps.

 index/global_index : This is typically the global running index of
    the keyword in the file; this is the unique address of the keyword
    which is used for the final lookup.

 occurence: The nth' time a particular keyword has occured in the
    file, i.e. the SEQNUM keyword in block C is the third occurence of
    SEQNUM. Instead of occurence xxxx_index is also used to indicate
    the occurence of keyword xxxx. The occurence number is the integer
    argument to the xxx_iget_named_kw() function, and also the final
    call to create blockmaps.

*/

static time_t INTEHEAD_date( const ecl_kw_type * intehead_kw ) {
  return util_make_date( ecl_kw_iget_int( intehead_kw , INTEHEAD_DAY_INDEX)   , 
                         ecl_kw_iget_int( intehead_kw , INTEHEAD_MONTH_INDEX) , 
                         ecl_kw_iget_int( intehead_kw , INTEHEAD_YEAR_INDEX)  );
}

bool ecl_file_map_has_report_step( const ecl_file_map_type * file_map , int report_step) {
  int global_index = ecl_file_map_find_kw_value( file_map , SEQNUM_KW , &report_step );
  if (global_index >= 0)
    return true;
  else
    return false;
}


static int ecl_file_map_find_sim_time(const ecl_file_map_type * file_map , time_t sim_time) {
  int global_index = -1;
  if ( ecl_file_map_has_kw( file_map , INTEHEAD_KW)) {
    const int_vector_type * intehead_index_list = hash_get( file_map->kw_index , INTEHEAD_KW );
    int index = 0;
    while (index < int_vector_size( intehead_index_list )) {
      const ecl_kw_type * intehead_kw = ecl_file_map_iget_kw( file_map , int_vector_iget( intehead_index_list , index ));
      if (INTEHEAD_date( intehead_kw ) == sim_time) {
        global_index = int_vector_iget( intehead_index_list , index );
        break;
      }
      index++;
    }
  }
  return global_index;
}

bool ecl_file_map_has_sim_time( const ecl_file_map_type * file_map , time_t sim_time) {
  int global_index = ecl_file_map_find_sim_time( file_map , sim_time );
  if (global_index >= 0)
    return true;
  else
    return false;
}

time_t ecl_file_map_iget_restart_sim_date(const ecl_file_map_type * file_map , int index) {
  if (ecl_file_map_get_num_named_kw( file_map , INTEHEAD_KW) > index) {
    ecl_kw_type * intehead_kw = ecl_file_map_iget_named_kw( file_map , INTEHEAD_KW , index);
    return INTEHEAD_date( intehead_kw );
  } else
    return -1;
}


int ecl_file_map_get_restart_index( const ecl_file_map_type * file_map , time_t sim_time) {
  int num_INTEHEAD = ecl_file_map_get_num_named_kw( file_map , INTEHEAD_KW );
  if (num_INTEHEAD == 0)
    return -1;       /* We have no INTEHEAD headers - probably not a restart file at all. */
  else {
    /*
      Should probably do something smarter than a linear search; but I dare not
      take the chance that all INTEHEAD headers are properly set. This is from
      Schlumberger after all.
    */
    int intehead_index = 0;
    while (true) {
      time_t itime = ecl_file_map_iget_restart_sim_date( file_map , intehead_index );
      
      if (itime == sim_time) /* Perfect hit. */
        return intehead_index;

      if (itime > sim_time)  /* We have gone past the target_time - i.e. we do not have it. */
        return -1;
      
      intehead_index++;
      if (intehead_index == num_INTEHEAD)  /* We have iterated through the whole thing without finding sim_time. */
        return -1;
    }
  }
}

/**
   This function will look up the INTEHEAD keyword in a ecl_file_type
   instance, and calculate simulation date from this instance.

   Will return -1 if the requested INTEHEAD keyword can not be found.
*/

time_t ecl_file_iget_restart_sim_date( const ecl_file_type * restart_file , int index ) {
  return ecl_file_map_iget_restart_sim_date( restart_file->active_map , index );
}



/**
   This function will scan through the ecl_file looking for INTEHEAD
   headers corresponding to sim_time. If sim_time is found the
   function will return the INTEHEAD occurence number, i.e. for a
   unified restart file like:

   INTEHEAD  /  01.01.2000
   ...
   PRESSURE
   SWAT
   ...
   INTEHEAD  /  01.03.2000
   ...
   PRESSURE
   SWAT
   ...
   INTEHEAD  /  01.05.2000
   ...
   PRESSURE
   SWAT
   ....

   The function call:

   ecl_file_get_restart_index( restart_file , (time_t) "01.03.2000")

   will return 1. Observe that this will in general NOT agree with the
   DATES step number.

   If the sim_time can not be found the function will return -1, that
   includes the case when the file in question is not a restart file
   at all, and no INTEHEAD headers can be found.
   
   Observe that the function requires on-the-second-equality; which is
   of course quite strict.
*/


int ecl_file_get_restart_index( const ecl_file_type * restart_file , time_t sim_time) {
  return ecl_file_map_get_restart_index( restart_file->active_map , sim_time );
}


/**
   Will look through all the INTEHEAD kw instances of the current
   ecl_file and look for @sim_time. If the value is found true is
   returned, otherwise false.
*/



bool ecl_file_has_sim_time( const ecl_file_type * ecl_file , time_t sim_time) {
  return ecl_file_map_has_sim_time( ecl_file->active_map , sim_time );
}


/**
   Will look through all the SEQNUM kw instances of the current
   ecl_file and look for @report_step. If the value is found true is
   returned, otherwise false.
*/

bool ecl_file_has_report_step( const ecl_file_type * ecl_file , int report_step) {
  return ecl_file_map_has_report_step( ecl_file->active_map , report_step );
}



ecl_file_map_type * ecl_file_iget_unrstmap( ecl_file_type * ecl_file , int index) {
  return ecl_file_get_blockmap( ecl_file , SEQNUM_KW , index );
}


ecl_file_map_type * ecl_file_get_unrstmap_time_t( ecl_file_type * ecl_file , time_t sim_time) {
  int seqnum_index = ecl_file_map_get_restart_index( ecl_file->active_map , sim_time );
  if (seqnum_index >= 0)
    return ecl_file_get_blockmap( ecl_file , SEQNUM_KW , seqnum_index);
  else
    return NULL;
}


ecl_file_map_type * ecl_file_get_unrstmap_report_step( ecl_file_type * ecl_file , int report_step) {
  int global_index = ecl_file_map_find_kw_value( ecl_file->active_map , SEQNUM_KW , &report_step);
  if ( global_index >= 0) {
    int seqnum_index = ecl_file_map_iget_occurence( ecl_file->active_map , global_index );
    return ecl_file_get_blockmap( ecl_file , SEQNUM_KW , seqnum_index );
  } else 
    return NULL;
}


/**
   The SEQNUM number found in unified restart files corresponds to the 
   REPORT_STEP.
*/




