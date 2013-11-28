/*
 * transit.c   - Models the modulation produced by a Planet crossing in
 *               front of the star. Main component of the Transit program.
 *
 * Copyright (C) 2003-2006 Patricio Rojo (pato@astro.cornell.edu)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General 
 * Public License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

/* TBD: calloc checks */

#include <transit.h>

/* \fcnfh  */
int main(int argc,      /* Number of variables */
         char **argv){  /* Variables           */

  /* FINDME: What is s_lt? */
  /* Initialization of data structure's pointers. Note that s_lt is not
     assigned because the array that is going to point has not been
     initialized yet.  */
  struct transit transit;
  struct timeval tv1, tv2;
  long itr=0;

  memset(&transit, 0, sizeof(struct transit));
  verblevel=2;
  gettimeofday(&tv1, NULL);

  /* Command line parameters' processing:        */
  fw(processparameters, !=0, argc, argv, &transit);
  timecheck(verblevel, 0, 0, "processparameters", tv1, tv2);

  /* Accept all general hints:                   */
  fw(acceptgenhints, !=0, &transit);

  /* Presentation:                               */
  printintro();

  /* No program warnings if verblevel is 0 or 1: */
  if(verblevel<2)
    transit_nowarn = 1;

  /* Hack: add an index to f_out filename to get different files
     and compare them:                                               */
  int fout_len = (int)strlen(transit.f_out);  /* Length of tr.f_out  */
  char *dot    = strchr(transit.f_out, '.');  /* Search for '.' char */
  int dot_pos  = fout_len - (int)strlen(dot); /* Position of dot     */
  char fout[fout_len+1];         /* Copy of tr.f_out           */     
  char str_iter[1];              /* String of iteration number */
  strcpy(fout, transit.f_out);
  strcpy(fout+dot_pos+1, dot);
  strncpy(fout+dot_pos, "0", 1);

  /* EDIT: The loop should enclose getatm (getatm will change in the future,
     but for the moment we will leave it as it is).  */
  for (itr=0; itr<2; itr++){
    //timecheck(verblevel, itr, 0, "Start loop", tv1, tv2);
    /* Read line info:                             */
    fw(readlineinfo, !=0, &transit);
    //timecheck(verblevel, itr, 1, "readlineinfo", tv1, tv2);

    /* Read Atmosphere information:                */
    fw(getatm, !=0, &transit);
    //timecheck(verblevel, itr, 2, "getatm", tv1, tv2);

    /* Make wavelength binning:                    */
    fw(makewavsample, <0, &transit);
    //timecheck(verblevel, itr, 3, "makewavsample", tv1, tv2);
    if(fw_status>0)
      transitprint(7, verblevel,
                   "makewavsample() modified some of the hinted "
                   "parameters according to returned flag: 0x%lx.\n",
                   fw_status);
 
    /* Make wavenumber binning:                    */
    fw(makewnsample, <0, &transit);
    //timecheck(verblevel, itr, 4, "makewnsample", tv1, tv2);
    if(fw_status>0)
      transitprint(7, verblevel,
                   "makewnsample() modified some of the hinted "
                   "parameters according to returned flag: 0x%lx.\n",
                   fw_status);
 
    /* Make radius binning and interpolate data to new value: */
    fw(makeradsample, <0, &transit);
    //timecheck(verblevel, itr, 5, "makeradsample", tv1, tv2);
    if(fw_status>0)
      transitprint(7, verblevel,
                   "makeradsample() modified some of the hinted "
                   "parameters according to returned flag: 0x%lx.\n",
                   fw_status);
 
    /* Initializes CIA:                       */
    fw(interpolatecia, !=0, &transit);
    //timecheck(verblevel, itr, 6, "interpolatecia", tv1, tv2);
 
    /* Computes index of refraction:          */
    fw(idxrefrac, !=0, &transit);
    //timecheck(verblevel, itr, 7, "idxrefrac", tv1, tv2);
 
    /* Calculates extinction coefficient:     */
    fw(extwn, !=0, &transit);
    //timecheck(verblevel, itr, 8, "extwn", tv1, tv2);
 
    /* Computes sampling of impact parameter: */
    fw(makeipsample, <0, &transit);
    //timecheck(verblevel, itr, 9, "makeipsample", tv1, tv2);
    if(fw_status>0)
      transitprint(7, verblevel,
                   "makeipsample() modified some of the hinted "
                   "parameters according to returned flag: 0x%lx.\n",
                   fw_status);
 
    /* Prints sampling info:                  */
    fw(outsample, !=0, &transit);
    //timecheck(verblevel, itr, 10, "outsample", tv1, tv2);
 
    /* Calculates optical depth:              */
    fw(tau, !=0, &transit);
    //timecheck(verblevel, itr, 11, "tau", tv1, tv2);

    transitprint(1, verblevel, "len: %i, word: %s.\n",
                               (int)strlen(transit.f_out), transit.f_out); 
   

    sprintf(str_iter, "%li", itr);
    strncpy(fout+dot_pos, str_iter, 1);
    strcpy(transit.f_out, fout);

    /* Calculates eclipse modulation:         */
    fw(modulation, !=0, &transit);
    //timecheck(verblevel, itr, 12, "modulation", tv1, tv2);
 
    free(transit.save.ext);
    freemem_isotopes (transit.ds.iso, &transit.pi);
    freemem_cia      (transit.ds.cia, &transit.pi);
    freemem_outputray(transit.ds.out, &transit.pi);
    transitprint(1, verblevel, "----------------------------\n");
    timecheck(verblevel, itr, 13, "THE END", tv1, tv2);
  }
  freemem_transit(&transit);

  return EXIT_SUCCESS;
}


/* \fcnfh
   Frees transit structure.                 */
void
freemem_transit(struct transit *tr){
  freemem_hints(tr->ds.th);

  freemem_samp(&tr->rads);
  freemem_samp(&tr->wavs);
  freemem_samp(&tr->wns);
  freemem_samp(&tr->ips);
  free_atm(&tr->atm);

  free(tr->outpret);
  /* TBD: Free saves once it is enabled
  freemem_saves();                          */
}
