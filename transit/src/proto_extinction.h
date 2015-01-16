#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* src/extinction.c */
extern inline int newprofile P_((float **pr, int vf, double dwn, float dop,
                                 float lor, float ta, int nwave));
extern inline int getprofile P_((float  *pr,         double dwn, float dop,
                                 float lor, float ta, int nwave));
extern void savefile_extinct P_((char *filename, double **e, short *c,
                                 long nrad, long nwav));
extern void restfile_extinct P_((char *filename, double **e, short *c,
                                 long nrad, long nwav));
extern void outputinfo P_((char *outfile, long w, long dw, long ln, long dln,
                           double **kiso, double timesalpha, int fbinvoigt,
                           double temp, double rad));
extern int extwn P_((struct transit *tr));
extern void printone P_((struct transit *tr));
extern int freemem_extinction P_((struct extinction *ex, long *pi));
extern int restextinct P_((FILE *in, long nrad, short niso, long nwn,
                           struct extinction *ex));
extern int computemolext P_((struct transit *tr, PREC_NREC r, PREC_RES **kiso));
extern int interpolmolext P_((struct transit *tr, PREC_NREC r, PREC_RES **kiso));
#undef P_
