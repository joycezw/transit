/*
 * makesample.c   - create array sampling after initial, final and spacing
 *                  parameters. Component of the Transit program.
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

#include <transit.h>

/* \fcnfh
   Create a sampling array. Take values from hint or else from a 
   reference sampling.

   Get units factor.  Get inital and final values.
   Check that there is a non-zero interval.
   Check that only the spacing or the number of points have been defined.
   If numpoints was provided, use the given array of values and ommit
   oversampling.  If spacing was provided, calculate numpoints, oversample,
   and fill in values.

   Return: 1 for modified initial value
           2 for modified final   value
           0 if nothing was changed but there is a sampled array
          -1 if hinted initial is bigger than maximum allowed
          -2 if hinted final is smaller than minimum allowed
          -3 if accepted initial value is greater or equal to final one
          -4 if both spacing and number of elements were hinted
          -5 if none or both of spacing or number of elements were in
             the referenced
          -6 Not valid oversampling was given by ref when requested      */
int
makesample(prop_samp *samp,       /* transit sampling             */
           prop_samp *hint,       /* hint    sampling             */
           prop_samp *ref,        /* transit's reference sampling */
           const long fl,         /* Sampling flag                */
           const float margini,   /* Sampling initial margin      */
           const float marginf){  /* Sampling final   margin      */

  int res=0;              /* Return output             */
  PREC_RES *v;            /* The sampling values       */
  double osd,             /* Oversampled delta         */
         si;              /* Sample initial value      */
  _Bool nhint=hint->n> 0, /* True if hint.n is defined */
        dhint=hint->d!=0; /* True if hint.d is defined */
  /* Acceptable ratio exceeding final value without truncating the last bin: */
  double okfinalexcess = 1e-8;

  /* Get units factor: */
  if(hint->fct<=0)
    samp->fct = ref->fct;
  else
    samp->fct = hint->fct;

  /* Check initial value: */
  /* If unset hint->i or if hint is less than reference+margin: */
  if(hint->i<=0 || (margini!=0 && hint->i < ref->i+margini)){
    samp->i = ref->i + margini;
    transitprint(4, verblevel,
                 "Using ref sampling %g [cgs] (w/margin %g [cgs]) for initial "
                 "value of %s.\n", samp->i*samp->fct, margini*samp->fct,
                 TRH_NAME(fl));
    res |= 0x1; /* Turn on modification flag for return output */
  }
  /* Initial value is larger than final value: */
  /* FINDME: This should be done after assessing samp.f */
  else if(hint->i > ref->f-marginf){
    transiterror(TERR_SERIOUS|TERR_ALLOWCONT,
                 "Hinted initial value for %s sampling (%g) is larger than "
                 "maximum allowed final value %.8g. Consider final margin "
                 "%.8g\n", TRH_NAME(fl), hint->i, ref->f-marginf, marginf);
    return -1;
  }
  else
    samp->i = hint->i;
  si = samp->i;  /* FINDME: delete si and keep using samp->i */

  /* Check final value: */
  /* If default value or if hint is larger than ref-margin: */
  if(hint->f<=0 || (marginf!=0 && hint->f > ref->f-marginf)){
    samp->f = ref->f-marginf;
    transitprint(4, verblevel,
                 "Using ref sampling %g [cgs] (w/margin %g [cgs]) for final "
                 "value of %s.\n", samp->f*samp->fct, marginf*samp->fct,
                 TRH_NAME(fl));
    res |= 0x2; /* Turn on modification flag for return output */
  }
  else if(hint->f < ref->i+margini){
    transiterror(TERR_SERIOUS|TERR_ALLOWCONT,
                 "Hinted final value for %s sampling (%g) is smaller than "
                 "minimum allowed initial value %.8g. Consider initial margin "
                 "%.8g\n", TRH_NAME(fl), hint->f, ref->i+margini, margini);
    return -2;
  }
  else
    samp->f = hint->f;

  /* Check non-zero interval and FINDME: */
  if(samp->f<=si && ref->d>=0 && hint->d>=0){
    transiterror(TERR_SERIOUS|TERR_ALLOWCONT,
                 "Initial accepted sampling value (%g) is greater or equal "
                 "than final accepted sample value (%g). %s was being "
                 "hinted.\n", si, samp->f, TRH_NAME(fl));
    return -3;
  }

  /* Progress report: print flag, spacing, and number of points from hint: */
  transitprint(21, verblevel,
               "Flags: 0x%lx    hint.d: %g   hint.n: %li\n",
               fl, hint->d, hint->n);
  /* Check that only one of spacing or number of elements field have been
     hinted: */
  if(nhint && dhint){
    transiterror(TERR_SERIOUS|TERR_ALLOWCONT,
                 "Both spacing (%g) and number of elements (%i) have been "
                 "hinted. That doesn't makes sense!. %s was being sampled.\n",
                 hint->d, hint->n, TRH_NAME(fl));
    return -4;
  }

  /* If none has been hinted then use ref's:       */
  if(!nhint && !dhint){
    /* If none of the ref exists then throw error: */
    if((ref->d==0 && ref->n<=0)){
      transiterror(TERR_SERIOUS|TERR_ALLOWCONT,
                   "Spacing (%g) and number of elements (%i) were either both "
                   "or none in the reference for %s sampling. And yes, none "
                   "were hinted.\n", ref->d, ref->n, TRH_NAME(fl));
      return -5;
    }
    /* If spacing exists then trust it: */
    if(ref->d!=0){
      samp->d = ref->d;
      samp->n = 0;
    }
    /* Else, use set array: */
    else{
      /* If initial or final value were modified, then print out a warning: */
      if(res){
        transiterror(TERR_WARNING,
                     "Array of length %i was given as reference for %s "
                     "sampling, but the initial (%g -> %g) or final "
                     "(%g -> %g) values MIGHT have been modified.\n",
                     ref->n, TRH_NAME(fl), ref->i, si, ref->f, samp->f);
      }
      samp->n = ref->n;
      samp->d = 0;
      samp->v = (PREC_RES *)calloc(samp->n, sizeof(PREC_RES));
      memcpy(samp->v, ref->v, samp->n*sizeof(PREC_RES));
      if(ref->o != 0)
        transiterror(TERR_WARNING,
                     "Fixed sampling array of length %i was referenced. " 
                     "But also oversampling was given (%li), ignoring it "
                     "in %s sampling.\n", samp->n, ref->o, TRH_NAME(fl));

      /* Return any possible modification: */
      return res;
    }
  }
  /* Else if spacing was hinted, then it has to be positive at this point: */
  else if(dhint){
    transitASSERT(hint->d <= 0,
                  "Error: Logic test 1 failed in %s's makesample()\n",
                  TRH_NAME(fl));
    samp->d = hint->d;
  }
  /* Else, we have a hinted n: */
  else{
    transitASSERT(hint->n==0,
                  "Error: Logic test 2 failed in %s's makesample()\n",
                  TRH_NAME(fl));
    samp->n = hint->n;
    samp->d = -1; /* FINDME: will be used later? Differs from ref default */
    samp->v = (PREC_RES *)calloc(samp->n, sizeof(PREC_RES));
    memcpy(samp->v, hint->v, samp->n*sizeof(PREC_RES));
    if(hint->o != 0)
      transiterror(TERR_WARNING,
                   "Fixed sampling array of length %li was hinted. "
                   "But also oversampling was given (%li). Ignoring in "
                   "%s sampling.\n", samp->n, hint->o, TRH_NAME(fl));
    return res;
  }

  /* Make sampling based on spacing:       */
  if(samp->d<0) okfinalexcess = -okfinalexcess; /* FINDME: can d be < 0 ?*/
  /* Define number of points:              */
  samp->n = ((1.0+okfinalexcess)*samp->f - si)/samp->d+1;
  if(samp->n<0)  /* FINDME: Explain how can this happen */
    samp->n = -samp->n;

  /* Check for hinted oversampling:        */
  if(hint->o<=0){
    /* If not, check for ref oversampling: */
    if(ref->o<=0){  /* If not, throw error */
      transiterror(TERR_SERIOUS|TERR_ALLOWCONT,
                   "Not valid oversampling in the reference for "
                   "%s sampling.\n", TRH_NAME(fl));
      return -6;
    }
    samp->o = ref->o;
  }
  else
    samp->o = hint->o;

  /* Oversampled number of points:      */
  samp->n = (samp->n-1)*samp->o+1;
  /* Oversampled delta:                 */
  osd = samp->d/(double)samp->o;

  /* Allocate and fill sampling values: */
  v = samp->v = (PREC_RES *)calloc(samp->n, sizeof(PREC_RES));
  /* Fill-in values:                    */
  PREC_NREC n = samp->n;
  *v = si;
  v += --n;
  while(n)
    *v-- = si + n--*osd;
  /* FINDME: Why so complicated? This is far easier:
  for (i=0; i<samp->n; i++)
    *v+i = samp->i + i*osd               */

  /* Check the final point: */
  if(si!=0 && samp->v[samp->n-1]!=samp->f && verblevel>2)
    /* FINDME: Consider removig the verblevel condition */
    transiterror(TERR_WARNING,
                 "Final sampled value (%g) of the %li points doesn't coincide "
                 "exactly with required value (%g). %s sampling with "
                 "pre-oversampling spacing of %g.\n", samp->v[samp->n-1],
                 samp->n, samp->f, TRH_NAME(fl), samp->d);

  /* Return the flags of accepted values: */
  return res;
}


/* \fcnfh
   Call makesample to create transit's wavelet sampling using
   lineinfo.wavs as reference.

 Return: makesample's output or,
         -10 if neither the spacing nor number was hinted.            */
int
makewavsample(struct transit *tr){
  int res;  /* Return output */
  prop_samp *samp = &tr->ds.th->wavs;   /* transithint's wavelength sampling */
  prop_samp *lin = &(tr->ds.li->wavs);  /* lineinfo's    wavelength sampling */

  /* Check that checkrange has been already called: */
  transitcheckcalled(tr->pi, "makewavsample", 1, "checkrange", TRPI_CHKRNG);

  /* Check that wavelength's spacing and number are defined in hint: */
  if(samp->d<=0 && samp->n<=0){
    transiterror(TERR_SERIOUS|TERR_ALLOWCONT,
                 "Spacing or number must be hinted for wavelength, "
                 "cannot just guess them.\n");
    return -10;
  }

  /* TD: Fix the mess if user chooses a factor different from the TLI's
     (then margin given to makesample won't make sense) */

  /* Make the sampling: */
  transitDEBUG(22, verblevel,
               "Making wavelength sampling:\n"
               " Margin: %g (factor: %g)   \n"
               " Number of points: ref: %li (user: %li)\n"
               " Delta: ref: %g (user: %g).\n",
               tr->margin, tr->wavs.fct, lin->n, samp->n, lin->d, samp->d);

  /* Create the sampling, use lineinfo's wavs as reference: */
  res = makesample(&tr->wavs, samp, lin, TRH_WAV, tr->margin/lin->fct,
                   tr->margin/lin->fct);
  transitDEBUG(22, verblevel,
               "Made following wavelength sampling:\n"
               " Initial/Final/FCT : %g/%g/%g\n"
               " Nsample/Delta     : %li/%g\n",
               tr->wavs.i, tr->wavs.f, tr->wavs.fct, tr->wavs.n, tr->wavs.d);

  /* Set progress indicator if sampling was successful and return status: */
  if(res>=0)
    tr->pi |= TRPI_MAKEWAV;
  return res;
}


/* \fcnfh
   Calls makesample with the appropiate parameters and set the flags
   for a wavenumber sampling

   Return: makesample() output                                       */
int
makewnsample(struct transit *tr){
  int res;            /* Result status                 */
  prop_samp fromwav;  /* Reference wavenumber sampling */
  memset(&fromwav, 0, sizeof(prop_samp));
  struct transithint *trh = tr->ds.th;
  prop_samp *wsamp = &tr->wavs;

  /* Check that makewavsample has been already called: */
  transitcheckcalled(tr->pi, "makewnsample", 1, "makewavsample", TRPI_MAKEWAV);

  /* Set up reference wavenumber sampling:  */
  /* Copy wavelength's oversampling and multiplicative factor: */
  fromwav.o = wsamp->o;
  /* Reference's wavenumber units are cm-1: */
  fromwav.fct = 1;

  /* Wavenumber to wavelength units ratio: */
  double wnu_o_wlu = fromwav.fct/wsamp->fct;

  /* Get initial wavenumber from wavelength maximum: */
  fromwav.i = wnu_o_wlu/wsamp->f;
  /* Get final   wavenumber from wavelength minimum: */
  fromwav.f = wnu_o_wlu/wsamp->i;

  /* Set margin. If not given, take it from wavelength's: */
  if(trh->wnm>0)
    tr->wnmf = tr->wnmi = trh->wnm;
  else{
    /* FINDME: why like this? */
    tr->wnmf = tr->margin * fromwav.f*fromwav.f * fromwav.fct*fromwav.fct;
    tr->wnmi = tr->margin * fromwav.i*fromwav.i * fromwav.fct*fromwav.fct;
  }

  /* Don't set the number of elements: */
  fromwav.n = 0;

  /* Set spacing such that the wavenumber grid has the same number of
     points as the wavelength grid. Then change reference initial and
     final point to include margin: */
  if(wsamp->n<2 && trh->wns.d<=0)
    transiterror(TERR_SERIOUS,
                 "Wavelength spacing (%g) is too big, "
                 "unusable as reference for wavenumber spacing.\n", wsamp->d);
  fromwav.d = (fromwav.f-fromwav.i)/((wsamp->n-1)/wsamp->o);
  fromwav.f -= tr->wnmf;
  fromwav.i += tr->wnmi;

  /* Make the wavenumber sampling: */ 
  res = makesample(&tr->wns, &trh->wns, &fromwav, TRH_WN, 0, 0);

  /* Check that wavenumber's range lies within the wavelength's range: */
  if ((1.0/(tr->wns.i*tr->wns.fct) > tr->wavs.f*tr->wavs.fct) || 
      (1.0/(tr->wns.f*tr->wns.fct) < tr->wavs.i*tr->wavs.fct))
    transiterror(TERR_SERIOUS,
                 "Wavenumber range (%g-%g cm-1), where extinction is "
                 "going to be computed, is beyond wavelength range "
                 "(%g-%g cm), where line info was read. "
                 "Conversion factor: %g. Wavenumber margin: %g, %g "
                 "Given wavn: (%g-%g cm-1). "
                 "Wavelength check (low: %i, high: %i).\n",
                 tr->wns.i*tr->wns.fct,   tr->wns.f*tr->wns.fct,
                 tr->wavs.i*tr->wavs.fct, tr->wavs.f*tr->wavs.fct,
                 wnu_o_wlu, tr->wnmi, tr->wnmf, fromwav.i, fromwav.f,
                 (1.0/(tr->wns.i*tr->wns.fct) > tr->wavs.f*tr->wavs.fct),
                 (1.0/(tr->wns.f*tr->wns.fct) < tr->wavs.i*tr->wavs.fct));

  /* Set progress indicator if sampling was successful and return status: */
  if(res>=0)
    tr->pi |= TRPI_MAKEWN;
  return res;
}


/* \fcnfh
 Calls makesample with the appropiate parameters and set the flags

 @returns makesample() output
          1 Only one point value was requested                       */
int
makeradsample(struct transit *tr){
  // 'limit' are limiting values that the sampling can take
  // 'rad', 'isovs', 'atms' and 'in' are auxiliary pointers to the radius
  //    sampling, variable isotope info, atmosphere content, and variable
  //    isotope structure pre-info, respectively.

  int res,    /* Return output                       */
      i, j,   /* Auxiliary for indices               */
      iso1db; /* First isotope's index in a database */
  struct isotopes *iso=tr->ds.iso;
  int nrad,            /* Number of radii to sample  */
      neiso=iso->n_e,  /* Number of extra isotopes   */
      niso =iso->n_i,  /* Number of regular isotopes */
      ndb  =iso->n_db; /* Number of data bases       */

  int flag;         /* Flag for interpolation function */
  prop_isov *isovs; /* lineinfo's isotope variable information  */

  struct lineinfo *li=tr->ds.li;   /* transit's lineinfo struct */
  struct atm_data *atms=tr->ds.at; /* transit's atmosphere data struct     */
  prop_atm        *atmt=&tr->atm;  /* Array to store p, t, and mm sampling */

  prop_isov *isovt=iso->isov;   /* iso's isotope variable information */
  prop_samp *rsamp=&atms->rads; /* Atmosphere's radii sampling        */
  prop_samp *rad  =&tr->rads;   /* Output radius sampling             */

  /* Check that getatm() and readinfo_tli() have been already called: */
  transitcheckcalled(tr->pi, "makeradsample", 2, "getatm",       TRPI_GETATM,
                                                 "readinfo_tli", TRPI_READINFO);
  /* Check that variables are not NULL: */
  transitASSERT(atms->rads.n<1 || !ndb || !niso || !neiso,
                "makeradsample():: called but essential variables are "
                "missing!.\n");

  /* Set interpolation function flag:  */
  switch(tr->fl & TRU_SAMPBITS){
  case TRU_SAMPLIN:
    flag = SAMP_LINEAR;
    break;
  case TRU_SAMPSPL:
    flag = SAMP_SPLINE;
    break;
  default:
    transiterror(TERR_SERIOUS, "Invalid sampling function specified");
  }

  /* We need to set-up limit so that the hinted values are compatible
     with the atmosphere */
 
  /* If there is only one atmospheric point, don't do makesample: */
  if(rsamp->n==1){
    /* rad->f = rad->i = rsamp->v[0]; */ /* FINDME: commented out */
    rad->n   = 1;
    rad->i   = rsamp->i;
    rad->f   = rsamp->f;
    rad->fct = rsamp->fct;
    rad->d = 0;
    rad->v = (PREC_RES *)calloc(1, sizeof(PREC_RES));
    rad->v[0] = rsamp->v[0]; 
    res = 0; /* makesample()-like output */
    /* TD: warn that hinted values are going to be useless */
  }
  /* If there is more than one atmospheric point: */
  else{
    /* If no initial value is hinted, take atmosphere's min and max: */
    res = makesample(rad, &tr->ds.th->rads, rsamp, TRH_RAD, 0, 0);
  }
  nrad = rad->n;

  /* Allocate arrays that will receive the interpolated data: */
  isovt->d = (PREC_ATM  *)calloc(nrad*neiso, sizeof(PREC_ATM ));
  isovt->q = (PREC_ATM  *)calloc(nrad*neiso, sizeof(PREC_ATM ));
  isovt->c = (PREC_CS   *)calloc(nrad*niso,  sizeof(PREC_CS  ));
  isovt->z = (PREC_ZREC *)calloc(nrad*niso,  sizeof(PREC_ZREC));
  for(i=1; i<neiso; i++){
    isovt[i].d = isovt->d+i*nrad;
    isovt[i].q = isovt->q+i*nrad;
    if(i < niso){
      isovt[i].c = isovt->c+i*nrad;
      isovt[i].z = isovt->z+i*nrad;
    }
  }
  atmt->tfct = atms->atm.tfct;
  atmt->pfct = atms->atm.pfct;
  atmt->t  = (PREC_ATM *)calloc(nrad, sizeof(PREC_ATM));
  atmt->p  = (PREC_ATM *)calloc(nrad, sizeof(PREC_ATM));
  atmt->mm = (double   *)calloc(nrad, sizeof(double));

  resamplex(flag, rsamp->n, rsamp->v, nrad, rad->v);
  /* Interpolate tr.atm values according to rad.v sampling: */
  resampley(flag, 3, atms->atm.t, atmt->t, atms->atm.p, atmt->p,
                     atms->mm,    atmt->mm);

  /* Interpolate density and abundance arrays for all isotopes,
     according to tr.rads.v:                                    */
  for(i=0; i<neiso; i++){
  /* TD-BUG: Find out why it fails if I take the brackets away. */
    resampley(flag, 2, atms->isov[i].d, isovt[i].d, atms->isov[i].q,
              isovt[i].q);
  }
  resample_free();
     
  /* Interpolate partition-function and cross-section values of
     line-database isotopes according to temperature sampling:        */
  for(i=0; i<ndb; i++){       /* For each database separately:        */
    iso1db = iso->db[i].s;    /* Index of first isotope in current DB */
    isovs  = li->isov+iso1db;

    resamplex(flag, li->db[i].t, li->db[i].T, nrad, atmt->t);
    for(j=0; j<iso->db[i].i; j++){
      transitASSERT(iso1db+j>niso-1,
                    "Trying to reference an isotope (%i) outside the extended "
                    "limit (%i).\n", iso1db+j, niso-1);
      resampley(flag, 2, isovs[j].z, isovt[iso1db+j].z, isovs[j].c, 
                isovt[iso1db+j].c);
    }
  }
  resample_free();

  /* Free atmosphere info that won't be used anymore: */
  freemem_atmosphere(tr->ds.at, &tr->pi);

  /* Set progress indicator and return: */
  if(res>=0)
    tr->pi |= TRPI_MAKERAD;
  return res;
}


/* \fcnfh
   Calls makesample with the appropiate parameters and set the flags for
   an impact parameter sampling

   @returns makesample() output                                     */
int
makeipsample(struct transit *tr){
  int res; /* Return output */

  /* An array of ip cannot be specified it has to be equispaced: */
  struct transithint *trh = tr->ds.th;

  /* Hinted ip sample:    */
  prop_samp usamp = {0, -trh->ips.d,   trh->ips.f,               trh->ips.i, 
                     trh->ips.o,       NULL,                     trh->ips.fct};
  /* Reference ip sample: */
  prop_samp rsamp = {0, -tr->rads.d,   tr->rads.v[tr->rads.n-1], tr->rads.v[0],
                     tr->rads.o,       NULL,                     tr->rads.fct};

  if(usamp.f < usamp.i)
    transiterror(TERR_SERIOUS,
                 "Wrong specification of impact parameter, final value (%g) "
                 "has to be bigger than initial (%g).\n", usamp.f, usamp.i);

  transitcheckcalled(tr->pi, "makeipsample", 1, "makeradsample", TRPI_MAKERAD);

  /* Make the sampling taking as reference the radius sampling: */
  res = makesample(&tr->ips, &usamp, &rsamp, TRH_IPRM, 0, 0);

  /* Set progress indicator: */
  if(res>=0)
    tr->pi |= TRPI_MAKEIP;
  return res;
}


/* \fcnfh
   Print sample info for a structure */
static void
printsample(FILE *out,        /* File pointer to write out  */
            prop_samp *samp,  /* Sample strucuture to print */
            char *desc,       /* Description                */
            long fl){         /* Various flag               */
  int i;

  /* Print header:                            */
  fprintf(out,
          "############################\n"
          "   %-12s Sampling\n"
          "----------------------------\n", desc);

  /* Print units, sample limits, and spacing: */
  fprintf(out, "Factor to cgs units: %g\n",            samp->fct);
  fprintf(out, "Initial value: %g\nFinal value: %g\n", samp->i, samp->f);
  fprintf(out, "Spacing: %g\n",                        samp->d);

  /* Print oversampling if necessary:         */
  if(!(fl&TRF_NOOVERSAMP))
    fprintf(out, "Oversample: %i\n", samp->o);

  /* Print sample size:                       */
  fprintf(out, "Number of elements: %li\n", samp->n);

  /* Print sample array:                      */
  if(!(fl&TRF_NOVALUE)){
    fprintf(out, "Values: ");
    for(i=0; i<samp->n; i++)
      fprintf(out, " %12.8g", samp->v[i]);
    fprintf(out, "\n");
  }
}


/* \fcnfh
   Saves in binary the sample structure               */
void
savesample(FILE *out,        /* File pointer to write */
           prop_samp *samp){ /* Sampling strucuture   */
  fwrite(samp, sizeof(prop_samp), 1, out);
  savesample_arr(out, samp);
}


/* \fcnfh
   Saves in binary the sample structure's arrays */
void
savesample_arr(FILE *out,        /* File pointer to write */
               prop_samp *samp){ /* Sampling strucuture   */
  if(samp->n>0)
    fwrite(samp->v, sizeof(PREC_RES), samp->n, out);
}


/* \fcnfh
   Restore a binary sample structure

   Return: 0 on success, else
          -1 if not all the expected information is read
          -2 if info read is wrong
          -3 if cannot allocate memory
           1 if information read was suspicious      */
int
restsample(FILE *in,         /* File pointer to read */
           prop_samp *samp){ /* Sampling strucuture  */
  if(fread(samp, sizeof(prop_samp), 1, in) != 1)
    return -1;
  return restsample_arr(in, samp);
}


/* \fcnfh
   Restore a binary sample structure

   Return: 0 on success, else
          -1 if not all the expected information is read
          -2 if info read is wrong
          -3 if cannot allocate memory
           1 if information read was suspicious             */
int
restsample_arr(FILE *in,         /* File pointer to read */
               prop_samp *samp){ /* Sampling strucuture  */
  if(samp->n<0)
    return -2;
  if(samp->n>1000000)
    return 1;
  if((samp->v = (PREC_RES *)calloc(samp->n, sizeof(PREC_RES)))==NULL)
    return -3;
  if(samp->n==0)
    return 0;
  if(fread(samp->v, sizeof(PREC_RES), samp->n, in) != samp->n)
    return -1;

  return 0;
}


/* \fcnfh
   Print the sample data to file.

   Return: 0 on success, else
           1 if cannot write to file  */
int
outsample(struct transit *tr){
  FILE *out = stdout;
  char *filename = tr->f_outsample; /* Sample filename */

  /* Check output filename exists: */
  if(!filename)
    return 0;

  /* If default f_outsample, print to screen: */
  if(strcmp(filename, "-") != 0)
    if((out=fopen(filename, "w"))==NULL){
      transiterror(TERR_WARNING, "Cannot open file '%s' for writing sampling "
                                 "data.\n", filename);
      return 1;
    }

  transitprint(1, verblevel, "\nPrinting sampling information in '%s'.\n\n",
               filename);

  /* Print each sample: */
  printsample(out, &tr->wns,  "Wavenumber",       TRF_NOVALUE);
  printsample(out, &tr->wavs, "Wavelength",       TRF_NOVALUE);
  printsample(out, &tr->rads, "Radius",           TRF_NOOVERSAMP);
  printsample(out, &tr->ips,  "Impact parameter", 0);

  fclose(out);

  return 0;
}


/* \fcnfh
 Frees the sampling structure */
void
freemem_samp(prop_samp *samp){
  if(samp->n)
    free(samp->v);
}


#ifdef DBGSAMPLE
int
main(int argc, char *argv[]){

  prop_samp lim, hint={0,0,0,0,0}, res;
  float mar=0;
  int i;

  if(argc<5){
    fprintf(stderr, "Syntax:\n"
            "    dbgsample <ini> <fin> <delt> <oversampling> [<margin>]\n");
    exit(0);
  }

  lim.i = atof(argv[1]);
  lim.f = atof(argv[2]);
  lim.d = atof(argv[3]);
  lim.o = atof(argv[4]);
  lim.n = 0;
  if(argc==6)
    mar = atof(argv[5]);

  i = makesample(&res, &hint, &lim, 0, 0, mar);

  fprintf(stderr, "Makesample returned %i\n"
          "Initial %g, final %g, delta %g, oversamp %i, number %li, "
          "margin %g\n", i, res.i, res.f, res.d, res.o, res.n, mar);

  if(i<0)
    exit(1);

  for(i=0; i<res.n; i++)
    fprintf(stderr," rad(%i): %g\n", i, res.v[i]);
}

#endif /* debug */