#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <gsl/gsl_integration.h>
#include <gsl/gsl_interp.h>
#include <gsl/gsl_spline.h>
#include <gsl/gsl_errno.h>

#include <class.h> /* from extern/ */

#include "ccl.h"
#include "ccl_params.h"
#include "ccl_emu17.h"
#include "ccl_emu17_params.h"


/*------ ROUTINE: ccl_cosmology_compute_power_class -----
INPUT: ccl_cosmology * cosmo
*/
static void ccl_free_class_structs(ccl_cosmology *cosmo,
				   struct background *ba,
				   struct thermo *th,
				   struct perturbs *pt,
				   struct transfers *tr,
				   struct primordial *pm,
				   struct spectra *sp,
				   struct nonlinear *nl,
				   struct lensing *le,
				   int *init_arr,
				   int * status)
{
  int i_init=6;
  if(init_arr[i_init--]) {
    if (spectra_free(sp) == _FAILURE_) {
      *status = CCL_ERROR_CLASS;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_free_class_structs(): Error freeing CLASS spectra:%s\n", sp->error_message);
      return;
    }
  }

  if(init_arr[i_init--]) {
    if (transfer_free(tr) == _FAILURE_) {
      *status = CCL_ERROR_CLASS;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_free_class_structs(): Error freeing CLASS transfer:%s\n", tr->error_message);
      return;
    }
  }

  if(init_arr[i_init--]) {
    if (nonlinear_free(nl) == _FAILURE_) {
      *status = CCL_ERROR_CLASS;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_free_class_structs(): Error freeing CLASS nonlinear:%s\n", nl->error_message);
      return;
    }
  }

  if(init_arr[i_init--]) {
    if (primordial_free(pm) == _FAILURE_) {
      *status = CCL_ERROR_CLASS;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_free_class_structs(): Error freeing CLASS pm:%s\n", pm->error_message);
      return;
    }
  }

  if(init_arr[i_init--]) {
    if (perturb_free(pt) == _FAILURE_) {
      *status = CCL_ERROR_CLASS;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_free_class_structs(): Error freeing CLASS pt:%s\n", pt->error_message);
      return;
    }
  }

  if(init_arr[i_init--]) {
    if (thermodynamics_free(th) == _FAILURE_) {
      *status = CCL_ERROR_CLASS;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_free_class_structs(): Error freeing CLASS thermo:%s\n", th->error_message);
      return;
    }
  }

  if(init_arr[i_init--]) {
    if (background_free(ba) == _FAILURE_) {
      *status = CCL_ERROR_CLASS;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_free_class_structs(): Error freeing CLASS bg:%s\n", ba->error_message);
      return;
    }
  }

  return;
}

static void ccl_class_preinit(struct background *ba,
			      struct thermo *th,
			      struct perturbs *pt,
			      struct transfers *tr,
			      struct primordial *pm,
			      struct spectra *sp,
			      struct nonlinear *nl,
			      struct lensing *le)
{
  //pre-initialize all fields that are freed by *_free() routine
  //prevents crashes if *_init()failed and did not initialize all tables freed by *_free()

  //init for background_free
  ba->tau_table = NULL;
  ba->z_table = NULL;
  ba->d2tau_dz2_table = NULL;
  ba->background_table = NULL;
  ba->d2background_dtau2_table = NULL;

  //init for thermodynamics_free
  th->z_table = NULL;
  th->thermodynamics_table = NULL;
  th->d2thermodynamics_dz2_table = NULL;

  //init for perturb_free
  pt->tau_sampling = NULL;
  pt->tp_size = NULL;
  pt->ic_size = NULL;
  pt->k = NULL;
  pt->k_size_cmb = NULL;
  pt->k_size_cl = NULL;
  pt->k_size = NULL;
  pt->sources = NULL;

  //init for primordial_free
  pm->amplitude = NULL;
  pm->tilt = NULL;
  pm->running = NULL;
  pm->lnpk = NULL;
  pm->ddlnpk = NULL;
  pm->is_non_zero = NULL;
  pm->ic_size = NULL;
  pm->ic_ic_size = NULL;
  pm->lnk = NULL;

  //init for nonlinear_free
  nl->k = NULL;
  nl->tau = NULL;
  nl->nl_corr_density = NULL;
  nl->k_nl = NULL;

  //init for transfer_free
  tr->tt_size = NULL;
  tr->l_size_tt = NULL;
  tr->l_size = NULL;
  tr->l = NULL;
  tr->q = NULL;
  tr->k = NULL;
  tr->transfer = NULL;

  //init for spectra_free
  //spectra_free checks all other data fields before freeing
  sp->is_non_zero = NULL;
  sp->ic_size = NULL;
  sp->ic_ic_size = NULL;
}

static void ccl_run_class(ccl_cosmology *cosmo,
			  struct file_content *fc,
			  struct precision* pr,
			  struct background* ba,
			  struct thermo* th,
			  struct perturbs* pt,
			  struct transfers* tr,
			  struct primordial* pm,
			  struct spectra* sp,
			  struct nonlinear* nl,
			  struct lensing* le,
			  struct output* op,
			  int *init_arr,
			  int * status)
{
  ErrorMsg errmsg;            // for error messages
  int i_init=0;
  ccl_class_preinit(ba,th,pt,tr,pm,sp,nl,le);

  if(input_init(fc,pr,ba,th,pt,tr,pm,sp,nl,le,op,errmsg) == _FAILURE_) {
    *status = CCL_ERROR_CLASS;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): Error running CLASS input:%s\n", errmsg);
    return;
  }
  if (background_init(pr,ba) == _FAILURE_) {
    *status = CCL_ERROR_CLASS;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): Error running CLASS background:%s\n", ba->error_message);
    return;
  }
  init_arr[i_init++]=1;
  if (thermodynamics_init(pr,ba,th) == _FAILURE_) {
    *status = CCL_ERROR_CLASS;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): Error running CLASS thermodynamics:%s\n", th->error_message);
    return;
  }
  init_arr[i_init++]=1;
  if (perturb_init(pr,ba,th,pt) == _FAILURE_) {
    *status = CCL_ERROR_CLASS;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): Error running CLASS pertubations:%s\n", pt->error_message);
    return;
  }
  init_arr[i_init++]=1;
  if (primordial_init(pr,pt,pm) == _FAILURE_) {
    *status = CCL_ERROR_CLASS;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): Error running CLASS primordial:%s\n", pm->error_message);
    return;
  }
  init_arr[i_init++]=1;
  if (nonlinear_init(pr,ba,th,pt,pm,nl) == _FAILURE_) {
    *status = CCL_ERROR_CLASS;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): Error running CLASS nonlinear:%s\n", nl->error_message);
    return;
  }
  init_arr[i_init++]=1;
  if (transfer_init(pr,ba,th,pt,nl,tr) == _FAILURE_) {
    *status = CCL_ERROR_CLASS;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): Error running CLASS transfer:%s\n", tr->error_message);
    return;
  }
  init_arr[i_init++]=1;
  if (spectra_init(pr,ba,pt,pm,nl,tr,sp) == _FAILURE_) {
    *status = CCL_ERROR_CLASS;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): Error running CLASS spectra:%s\n", sp->error_message);
    return;
  }
  init_arr[i_init++]=1;
}

static double ccl_get_class_As(ccl_cosmology *cosmo, struct file_content *fc, int position_As,
			       double sigma8, int * status)
{
  //structures for class test run
  struct precision pr;        // for precision parameters
  struct background ba;       // for cosmological background
  struct thermo th;           // for thermodynamics
  struct perturbs pt;         // for source functions
  struct transfers tr;        // for transfer functions
  struct primordial pm;       // for primordial spectra
  struct spectra sp;          // for output spectra
  struct nonlinear nl;        // for non-linear spectra
  struct lensing le;
  struct output op;

  //temporarily overwrite P_k_max_1/Mpc to speed up sigma8 calculation
  double k_max_old = 0.;
  int position_kmax =2;
  double A_s_guess;
  int init_arr[7]={0,0,0,0,0,0,0};

  if (strcmp(fc->name[position_kmax],"P_k_max_1/Mpc")) {
    k_max_old = strtof(fc->value[position_kmax],NULL);
    sprintf(fc->value[position_kmax],"%.15e",10.);
  }
  A_s_guess = 2.43e-9/0.87659*sigma8;
  sprintf(fc->value[position_As],"%.15e",A_s_guess);

  ccl_run_class(cosmo, fc,&pr,&ba,&th,&pt,&tr,&pm,&sp,&nl,&le,&op,init_arr,status);
  if (cosmo->status != CCL_ERROR_CLASS) A_s_guess*=pow(sigma8/sp.sigma8,2.);
  ccl_free_class_structs(cosmo, &ba,&th,&pt,&tr,&pm,&sp,&nl,&le,init_arr,status);

  if (k_max_old >0) {
    sprintf(fc->value[position_kmax],"%.15e",k_max_old);
  }
  return A_s_guess;
}

static void ccl_fill_class_parameters(ccl_cosmology * cosmo, struct file_content * fc,
				      int parser_length, int * status)

{

  // initialize fc fields
  //silences Valgrind's "Conditional jump or move depends on uninitialised value" warning
  for (int i = 0; i< parser_length; i++){
    strcpy(fc->name[i]," ");
    strcpy(fc->value[i]," ");
  }

  strcpy(fc->name[0],"output");
  strcpy(fc->value[0],"mPk");

  strcpy(fc->name[1],"non linear");
  if (cosmo->config.matter_power_spectrum_method == ccl_halofit)
    strcpy(fc->value[1],"Halofit");
  else
    strcpy(fc->value[1],"none");

  strcpy(fc->name[2],"P_k_max_1/Mpc");
  sprintf(fc->value[2],"%.15e",ccl_splines->K_MAX_SPLINE); //in units of 1/Mpc, corroborated with ccl_constants.h

  strcpy(fc->name[3],"z_max_pk");
  sprintf(fc->value[3],"%.15e",1./ccl_splines->A_SPLINE_MINLOG_PK-1.);

  strcpy(fc->name[4],"modes");
  strcpy(fc->value[4],"s");

  strcpy(fc->name[5],"lensing");
  strcpy(fc->value[5],"no");

  // now, copy over cosmology parameters
  strcpy(fc->name[6],"h");
  sprintf(fc->value[6],"%.15e",cosmo->params.h);

  strcpy(fc->name[7],"Omega_cdm");
  sprintf(fc->value[7],"%.15e",cosmo->params.Omega_c);

  strcpy(fc->name[8],"Omega_b");
  sprintf(fc->value[8],"%.15e",cosmo->params.Omega_b);

  strcpy(fc->name[9],"Omega_k");
  sprintf(fc->value[9],"%.15e",cosmo->params.Omega_k);

  strcpy(fc->name[10],"n_s");
  sprintf(fc->value[10],"%.15e",cosmo->params.n_s);


  //cosmological constant?
  // set Omega_Lambda = 0.0 if w !=-1
  if ((cosmo->params.w0 !=-1.0) || (cosmo->params.wa !=0)) {
    strcpy(fc->name[11],"Omega_Lambda");
    sprintf(fc->value[11],"%.15e",0.0);

    strcpy(fc->name[12],"w0_fld");
    sprintf(fc->value[12],"%.15e",cosmo->params.w0);

    strcpy(fc->name[13],"wa_fld");
    sprintf(fc->value[13],"%.15e",cosmo->params.wa);
  }
  //neutrino parameters
  //massless neutrinos
  if (cosmo->params.N_nu_rel > 1.e-4) {
    strcpy(fc->name[14],"N_ur");
    sprintf(fc->value[14],"%.15e",cosmo->params.N_nu_rel);
  }
  else {
    strcpy(fc->name[14],"N_ur");
    sprintf(fc->value[14],"%.15e", 0.);
  }
  if (cosmo->params.N_nu_mass > 0) {
    strcpy(fc->name[15],"N_ncdm");
    sprintf(fc->value[15],"%d",cosmo->params.N_nu_mass);
    strcpy(fc->name[16],"m_ncdm");
    sprintf(fc->value[16],"%f", (cosmo->params.mnu)[0]);
    if (cosmo->params.N_nu_mass >=1){
		for (int i = 1; i < cosmo->params.N_nu_mass; i++) {
			char tmp[20];
			sprintf(tmp,", %f",(cosmo->params.mnu)[i]);
			strcat(fc->value[16],tmp);
		}
	}

  }

  strcpy(fc->name[17],"T_cmb");
  sprintf(fc->value[17],"%.15e",cosmo->params.T_CMB);

  //normalization comes last, so that all other parameters are filled in for determining A_s if sigma8 is specified
  if (isfinite(cosmo->params.sigma8) && isfinite(cosmo->params.A_s)){
      *status = CCL_ERROR_INCONSISTENT;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: class_parameters(): Error initializing CLASS parameters: both sigma8 and A_s defined\n");
    return;
  }
  if (isfinite(cosmo->params.sigma8)) {
    strcpy(fc->name[parser_length-1],"A_s");
    sprintf(fc->value[parser_length-1],"%.15e",ccl_get_class_As(cosmo,fc,parser_length-1,cosmo->params.sigma8, status));
  }
  else if (isfinite(cosmo->params.A_s)) {
    strcpy(fc->name[parser_length-1],"A_s");
    sprintf(fc->value[parser_length-1],"%.15e",cosmo->params.A_s);
  }
  else {
    *status = CCL_ERROR_INCONSISTENT;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: class_parameters(): Error initializing CLASS pararmeters: neither sigma8 nor A_s defined\n");
    return;
  }

}

static void ccl_cosmology_compute_power_class(ccl_cosmology * cosmo, int * status)
{
  struct precision pr;        // for precision parameters
  struct background ba;       // for cosmological background
  struct thermo th;           // for thermodynamics
  struct perturbs pt;         // for source functions
  struct transfers tr;        // for transfer functions
  struct primordial pm;       // for primordial spectra
  struct spectra sp;          // for output spectra
  struct nonlinear nl;        // for non-linear spectra
  struct lensing le;
  struct output op;
  struct file_content fc;

  ErrorMsg errmsg; // for error messages
  // generate file_content structure
  // CLASS configuration parameters will be passed through this structure,
  // to avoid writing and reading .ini files for every call
  int parser_length = 20;
  int init_arr[7]={0,0,0,0,0,0,0};
  if (parser_init(&fc,parser_length,"none",errmsg) == _FAILURE_) {
    *status = CCL_ERROR_CLASS;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): parser init error:%s\n", errmsg);
    return;
  }

  ccl_fill_class_parameters(cosmo,&fc,parser_length, status);

  if (*status != CCL_ERROR_CLASS)
    ccl_run_class(cosmo, &fc,&pr,&ba,&th,&pt,&tr,&pm,&sp,&nl,&le,&op,init_arr,status);

  if (*status == CCL_ERROR_CLASS) {
    //printed error message while running CLASS
    ccl_free_class_structs(cosmo, &ba,&th,&pt,&tr,&pm,&sp,&nl,&le,init_arr,status);
    return;
  }

  if (parser_free(&fc)== _FAILURE_) {
    *status = CCL_ERROR_CLASS;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): Error freeing CLASS parser\n");
    ccl_free_class_structs(cosmo, &ba,&th,&pt,&tr,&pm,&sp,&nl,&le,init_arr,status);
    return;
  }

  //These are the limits of the splining range
  cosmo->data.k_min_lin=2*exp(sp.ln_k[0]);
  cosmo->data.k_max_lin=ccl_splines->K_MAX_SPLINE;

  //CLASS calculations done - now allocate CCL splines
  double kmin = cosmo->data.k_min_lin;
  double kmax = ccl_splines->K_MAX_SPLINE;
  //Compute nk from number of decades and N_K = # k per decade
  double ndecades = log10(kmax) - log10(kmin);
  int nk = (int)ceil(ndecades*ccl_splines->N_K);
  double amin = ccl_splines->A_SPLINE_MINLOG_PK;
  double amax = ccl_splines->A_SPLINE_MAX;
  int na = ccl_splines->A_SPLINE_NA_PK+ccl_splines->A_SPLINE_NLOG_PK-1;

  // The x array is initially k, but will later
  // be overwritten with log(k)
  double * x = ccl_log_spacing(kmin, kmax, nk);
  double * a = ccl_linlog_spacing(amin, ccl_splines->A_SPLINE_MIN_PK, amax, ccl_splines->A_SPLINE_NLOG_PK, ccl_splines->A_SPLINE_NA_PK);
  double * y2d_lin = malloc(nk * na * sizeof(double));
  double * y2d_nl = malloc(nk * na * sizeof(double));

  
  //If error, store status, we will free later
  if (a==NULL|| x==NULL || y2d_lin==NULL || y2d_nl==NULL) {
    *status = CCL_ERROR_SPLINE;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): memory allocation error\n");
  }

  //Status flags
  int newstatus=0;
  int pwstatus=0;
  
  //If not, proceed
  if(!*status){
    
    // After this loop x will contain log(k)
    // all in Mpc, not Mpc/h units!
    double psout_l,ic;
    for (int i=0; i<nk; i++) {
      for (int j = 0; j < na; j++) {
	//The 2D interpolation routines access the function values y_{k_ia_j} with the following ordering:
	//y_ij = y2d[j*N_k + i]
	//with i = 0,...,N_k-1 and j = 0,...,N_a-1.
	newstatus |= spectra_pk_at_k_and_z(&ba, &pm, &sp,x[i],1./a[j]-1., &psout_l,&ic);
	y2d_lin[j*nk+i] = log(psout_l);
      }
      x[i] = log(x[i]);
    }

    
    //If error, store status, we will free later
    if(newstatus) {
      *status = CCL_ERROR_CLASS;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): Error computing CLASS power spectrum\n");
    }

      
  }

  
  //If no error, proceed
  if(!*status) {
    
    gsl_spline2d * log_power = gsl_spline2d_alloc(PLIN_SPLINE_TYPE, nk,na);
    pwstatus = gsl_spline2d_init(log_power, x, a, y2d_lin,nk,na);
    
    //If not, proceed
    if(!pwstatus){
      cosmo->data.p_lin = log_power;
    } else {
      gsl_spline2d_free(log_power);
      *status = CCL_ERROR_SPLINE;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): Error creating log_power spline\n");
    }

  }
  
  if(*status){ //Linear power spec failed, so we return without proceeding to nonlinear.
    free(x);
    free(a);
    free(y2d_nl);
    free(y2d_lin);
    ccl_free_class_structs(cosmo, &ba,&th,&pt,&tr,&pm,&sp,&nl,&le,init_arr,status);
    return;
  }

  //Non-linear power
  //At the moment KMIN can't be less than CLASS's kmin in the nonlinear case.
  
  //If error, store status, we will free later
  if (kmin<(exp(sp.ln_k[0]))) {
    *status = CCL_ERROR_CLASS;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): K_MIN is less than CLASS's kmin. Not yet supported for nonlinear P(k).\n");
  }

  //If not, proceed
  if(!*status){

    //These are the limits of the splining range
    cosmo->data.k_min_nl=2*exp(sp.ln_k[0]);
    cosmo->data.k_max_nl=ccl_splines->K_MAX_SPLINE;
    
    if(cosmo->config.matter_power_spectrum_method==ccl_halofit) {
	
      double psout_nl;
      for (int i=0; i<nk; i++) {
	for (int j = 0; j < na; j++) {
	  newstatus |= spectra_pk_nl_at_k_and_z(&ba, &pm, &sp,exp(x[i]),1./a[j]-1.,&psout_nl);
	  y2d_nl[j*nk+i] = log(psout_nl);
	}
      }
    }

    if(newstatus){
      *status = CCL_ERROR_CLASS;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): Error computing CLASS power spectrum\n");
    }
    
  }

  if(!*status){
	
    gsl_spline2d * log_power_nl = gsl_spline2d_alloc(PNL_SPLINE_TYPE, nk,na);
    pwstatus = gsl_spline2d_init(log_power_nl, x, a, y2d_nl,nk,na);
    
    if(!pwstatus){
      cosmo->data.p_nl = log_power_nl;
    } else {
      gsl_spline2d_free(log_power_nl);
      *status = CCL_ERROR_SPLINE;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): Error creating log_power_nl spline\n");
    }

  }
      
  ccl_free_class_structs(cosmo, &ba,&th,&pt,&tr,&pm,&sp,&nl,&le,init_arr,status);
  free(x);
  free(a);
  free(y2d_nl);
  free(y2d_lin);

  return;
  
}

/* BCM correction 
   See Schneider & Teyssier (2015) for details of the model.
   The equations inside this function correspond to those in that
   work for:
   gf -> G(k)
   scomp -> S(k)
*/
double ccl_bcm_model_fka(ccl_cosmology * cosmo, double k, double a, int *status){

  double fka;
  double b0;
  double bfunc, bfunc4;
  double kg;
  double gf,scomp;
  double kh;
  double z;

  z=1./a-1.;
  kh = k/cosmo->params.h; //convert to h/Mpc
  b0 = 0.105*cosmo->params.bcm_log10Mc-1.27; //Eq. 4.4
  bfunc = b0/(1.+pow(z/2.3,2.5)); //Eq. 4.3
  bfunc4 = (1-bfunc)*(1-bfunc)*(1-bfunc)*(1-bfunc); //B^4(z)
  kg = 0.7*bfunc4*pow(cosmo->params.bcm_etab,-1.6); //Eq. 4.3
  gf = bfunc/(1+pow(kh/kg,3.))+1.-bfunc; //Eq. 4.2, k in h/Mpc
  scomp = 1+(kh/cosmo->params.bcm_ks)*(kh/cosmo->params.bcm_ks); //Eq 4.5, k in h/Mpc
  fka = gf*scomp;
  return fka;
}

void ccl_cosmology_write_power_class_z(char *filename, ccl_cosmology * cosmo, double z, int * status)
{
  struct precision pr;        // for precision parameters
  struct background ba;       // for cosmological background
  struct thermo th;           // for thermodynamics
  struct perturbs pt;         // for source functions
  struct transfers tr;        // for transfer functions
  struct primordial pm;       // for primordial spectra
  struct spectra sp;          // for output spectra
  struct nonlinear nl;        // for non-linear spectra
  struct lensing le;
  struct output op;
  struct file_content fc;

  ErrorMsg errmsg; // for error messages
  // generate file_content structure
  // CLASS configuration parameters will be passed through this structure,
  // to avoid writing and reading .ini files for every call
  int parser_length = 20;
  int init_arr[7]={0,0,0,0,0,0,0};
  if (parser_init(&fc,parser_length,"none",errmsg) == _FAILURE_) {
    *status = CCL_ERROR_CLASS;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: write_power_class_z(): parser init error:%s\n", errmsg);
    return;
  }

  ccl_fill_class_parameters(cosmo,&fc,parser_length, status);

  if (*status != CCL_ERROR_CLASS)
    ccl_run_class(cosmo, &fc,&pr,&ba,&th,&pt,&tr,&pm,&sp,&nl,&le,&op,init_arr,status);

  if (*status == CCL_ERROR_CLASS) {
    //printed error message while running CLASS
    ccl_free_class_structs(cosmo, &ba,&th,&pt,&tr,&pm,&sp,&nl,&le,init_arr,status);
    return;
  }
  if (parser_free(&fc)== _FAILURE_) {
    *status = CCL_ERROR_CLASS;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: write_power_class_z(): Error freeing CLASS parser\n");
    ccl_free_class_structs(cosmo, &ba,&th,&pt,&tr,&pm,&sp,&nl,&le,init_arr,status);
    return;
  }
  FILE *f;
  f = fopen(filename,"w");
  if (!f){
    *status = CCL_ERROR_CLASS;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: write_power_class_z(): Error opening output file\n");
    ccl_free_class_structs(cosmo, &ba,&th,&pt,&tr,&pm,&sp,&nl,&le,init_arr,status);
    fclose(f);
    return;
  }
  double psout_l,ic;
  int s=0;
  for (int i=0; i<sp.ln_k_size; i++) {
    s |= spectra_pk_at_k_and_z(&ba, &pm, &sp,exp(sp.ln_k[i]),z, &psout_l,&ic);
    fprintf(f,"%e %e\n",exp(sp.ln_k[i]),psout_l);
  }
  fclose(f);
  if(s) {
    *status = CCL_ERROR_CLASS;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: write_power_class_z(): Error writing CLASS power spectrum\n");
  }

  ccl_free_class_structs(cosmo, &ba,&th,&pt,&tr,&pm,&sp,&nl,&le,init_arr,status);
}


typedef struct {
  double rsound;
  double zeq;
  double keq;
  double zdrag;
  double kSilk;
  double rsound_approx;
  double th2p7;
  double alphac;
  double alphab;
  double betac;
  double betab;
  double bnode;
} eh_struct;

static eh_struct *eh_struct_new(ccl_parameters *params)
{
  //////
  // Computes Eisenstein & Hu parameters for
  // P_k and r_sound
  // see astro-ph/9709112 for the relevant equations
  double OMh2,OBh2;
  double th2p7;
  eh_struct *eh=malloc(sizeof(eh_struct));
  if(eh==NULL)
    return NULL;

  OMh2=params->Omega_m*params->h*params->h; //Cosmo params scaled by h^2
  OBh2=params->Omega_b*params->h*params->h;
  th2p7=params->T_CMB/2.7; //This is Theta_{2.7} in E&Hu notation
  eh->th2p7=th2p7; //This is Theta_{2.7} in E&Hu notation
  eh->zeq=2.5E4*OMh2/pow(th2p7,4);// Eq 2
  eh->keq=0.0746*OMh2/(params->h*th2p7*th2p7);//Eq. 3

  //These group corresponds to Eq. 4
  double b1,b2;
  b1=0.313*pow(OMh2,-0.419)*(1+0.607*pow(OMh2,0.674));
  b2=0.238*pow(OMh2,0.223);
  eh->zdrag=1291*pow(OMh2,0.251)*(1+b1*pow(OBh2,b2))/(1+0.659*pow(OMh2,0.828)); 

  //These are the baryon-to-photon ratios
  //at equality (Req) and drag (Rd) epochs
  //Eq. 5
  double Req,Rd;
  Req=31.5*OBh2*1000./(eh->zeq*pow(th2p7,4));
  Rd=31.5*OBh2*1000./((1+eh->zdrag)*pow(th2p7,4));
  eh->rsound=2/(3*eh->keq)*sqrt(6/Req)*
    log((sqrt(1+Rd)+sqrt(Rd+Req))/(1+sqrt(Req)));

  //This is Eq. 7 (but in h/Mpc)
  eh->kSilk=1.6*pow(OBh2,0.52)*pow(OMh2,0.73)*(1+pow(10.4*OMh2,-0.95))/params->h; 

  //These are Eqs. 11
  double a1,a2,b_frac;
  a1=pow(46.9*OMh2,0.670)*(1+pow(32.1*OMh2,-0.532));
  a2=pow(12.0*OMh2,0.424)*(1+pow(45.0*OMh2,-0.582));
  b_frac=OBh2/OMh2;
  eh->alphac=pow(a1,-b_frac)*pow(a2,-b_frac*b_frac*b_frac);

  //These are Eqs. 12
  double bb1,bb2;
  bb1=0.944/(1+pow(458*OMh2,-0.708));
  bb2=pow(0.395*OMh2,-0.0266);
  eh->betac=1/(1+bb1*(pow(1-b_frac,bb2)-1));

  double y=eh->zeq/(1+eh->zdrag);
  double sqy=sqrt(1+y);
  double gy=y*(-6*sqy+(2+3*y)*log((sqy+1)/(sqy-1))); //Eq 15
  //Baryon suppression Eq. 14
  eh->alphab=2.07*eh->keq*eh->rsound*pow(1+Rd,-0.75)*gy; 

  //Baryon envelope shift Eq. 24
  eh->betab=0.5+b_frac+(3-2*b_frac)*sqrt(pow(17.2*OMh2,2)+1);

  //Node shift parameter Eq. 23
  eh->bnode=8.41*pow(OMh2,0.435);

  //Approx for the sound horizon, Eq. 26
  eh->rsound_approx=params->h*44.5*log(9.83/OMh2)/
    sqrt(1+10*pow(OBh2,0.75));
  
  return eh;
}

static double tkEH_0(double keq,double k,double a,double b)
{
  //////
  // Eisentstein & Hu's Tk_0
  // see astro-ph/9709112 for the relevant equations
  double q=k/(13.41*keq); //Eq 10
  double c=14.2/a+386./(1+69.9*pow(q,1.08)); //Eq 20
  double l=log(M_E+1.8*b*q); //Change of var for Eq 19
  return l/(l+c*q*q); //Returns Eq 19
}

static double tkEH_c(eh_struct *eh,double k)
{
  //////
  // Eisenstein & Hu's Tk_c
  // see astro-ph/9709112 for the relevant equations
  double f=1/(1+pow(k*eh->rsound/5.4,4)); //Eq 18
  return f*tkEH_0(eh->keq,k,1,eh->betac)+ 
    (1-f)*tkEH_0(eh->keq,k,eh->alphac,eh->betac); //Returns Eq 17
}

static double jbes0(double x)
{
  double jl;
  double ax2=x*x;

  if(ax2<1e-4) jl=1-ax2*(1-ax2/20.)/6.;
  else jl=sin(x)/x;

  return jl;
}

static double tkEH_b(eh_struct *eh,double k)
{
  //////
  // Eisenstein & Hu's Tk_b (Eq 21)
  // see astro-ph/9709112 for the relevant equations
  double x_bessel,part1,part2,jbes;
  double x=k*eh->rsound;

  //First term of Eq. 21
  if(k==0) x_bessel=0;
  else {
    x_bessel=x*pow(1+eh->bnode*eh->bnode*eh->bnode/(x*x*x),-1./3.);
  }

  part1=tkEH_0(eh->keq,k,1,1)/(1+pow(x/5.2,2)); 

  //Second term of Eq. 21
  if(k==0)
    part2=0;
  else
    part2=eh->alphab/(1+pow(eh->betab/x,3))*exp(-pow(k/eh->kSilk,1.4));

  return jbes0(x_bessel)*(part1+part2);
}

static double tsqr_EH(ccl_parameters *params,eh_struct * eh,double k,int wiggled)
{
  //////
  // Eisenstein & Hu's Tk_c
  // see astro-ph/9709112 for the relevant equations
  // Notice the last parameter in eh_power controls
  // whether to introduce wiggles (BAO) in the power spectrum.
  // We do this by default when obtaining the power spectrum.
  double tk;
  double b_frac=params->Omega_b/params->Omega_m;
  if(wiggled)
    //Case with baryons (Eq 8)
    tk=b_frac*tkEH_b(eh,k)+(1-b_frac)*tkEH_c(eh,k);
  else {
    //Zero baryon case (sec 4.2)
    double OMh2=params->Omega_m*params->h*params->h;
    // Compute Eq. 31
    double alpha_gamma=1-0.328*log(431*OMh2)*b_frac+0.38*log(22.3*OMh2)*b_frac*b_frac;
    // Compute Eq. 30
    double gamma_eff=params->Omega_m*params->h*(alpha_gamma+(1-alpha_gamma)/
						(1+pow(0.43*k*eh->rsound_approx,4)));
    // Compute Eq. 28 (assume k in h/Mpc)
    double q=k*eh->th2p7*eh->th2p7/gamma_eff;
    // Compute Eqs. 29
    double l0=log(2*M_E+1.8*q);
    double c0=14.2+731/(1+62.5*q);
    tk=l0/(l0+c0*q*q); //T_0 of Eq. 29
  }

  return tk*tk; //Return T_0^2
}

static double eh_power(ccl_parameters *params,eh_struct *eh,double k,int wiggled)
{
  //Wavenumber in units of Mpc^-1
  double kinvh=k/params->h; //Changed to h/Mpc
  return pow(k,params->n_s)*tsqr_EH(params,eh,kinvh,wiggled);
}
static void ccl_cosmology_compute_power_eh(ccl_cosmology * cosmo, int * status)
{
  //These are the limits of the splining range
  cosmo->data.k_min_lin = ccl_splines->K_MIN;
  cosmo->data.k_min_nl = ccl_splines->K_MIN;
  cosmo->data.k_max_lin = ccl_splines->K_MAX;
  cosmo->data.k_max_nl = ccl_splines->K_MAX;
  double kmin = cosmo->data.k_min_lin;
  double kmax = ccl_splines->K_MAX;

  // Compute nk from number of decades and N_K = # k per decade
  double ndecades = log10(kmax) - log10(kmin);
  int nk = (int)ceil(ndecades*ccl_splines->N_K);

  // Compute na using predefined spline spacing
  double amin = ccl_splines->A_SPLINE_MINLOG_PK;
  double amax = ccl_splines->A_SPLINE_MAX;
  int na = ccl_splines->A_SPLINE_NA_PK + ccl_splines->A_SPLINE_NLOG_PK - 1;

  // Exit if sigma8 wasn't specified
  if (isnan(cosmo->params.sigma8)) {
    *status = CCL_ERROR_INCONSISTENT;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_eh(): sigma8 was not set, but is required for E&H method\n");
    return;
  }

  // New struct for EH parameters
  eh_struct *eh = eh_struct_new(&(cosmo->params));
  if (eh == NULL) {
    *status = CCL_ERROR_MEMORY;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_eh(): memory allocation error\n");
    return;
  }

  // Build grids in k and a that P(k, a) will be evaluated on
  // NB: The x array is initially k, but will later be overwritten with log(k)
  double * x = ccl_log_spacing(kmin, kmax, nk);
  double * y = malloc(sizeof(double)*nk);
  double * a = ccl_linlog_spacing(amin, ccl_splines->A_SPLINE_MIN_PK,
                                  amax, ccl_splines->A_SPLINE_NLOG_PK,
                                  ccl_splines->A_SPLINE_NA_PK);
  double * y2d = malloc(nk * na * sizeof(double));
  if (a==NULL || y==NULL || x==NULL || y2d==NULL) {
    free(eh);free(x);free(y);
    free(a);free(y2d);
    *status = CCL_ERROR_MEMORY;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_eh(): memory allocation error\n");
    return;
  }

  // Calculate P(k) on k grid. After this loop, x will contain log(k) and y
  // will contain log(pk) [which has not yet been normalized]
  // Notice the last parameter in eh_power controls
  // whether to introduce wiggles (BAO) in the power spectrum.
  // We do this by default.
  for (int i=0; i<nk; i++) {
    y[i] = log(eh_power(&cosmo->params, eh, x[i], 1));
    x[i] = log(x[i]);
  }

  // Apply growth factor, D(a), to P(k) and store in 2D (k, a) array
  double gfac, g2;
  gsl_spline2d *log_power_lin = gsl_spline2d_alloc(PLIN_SPLINE_TYPE, nk,na);
  for (int j = 0; j < na; j++) {
    gfac = ccl_growth_factor(cosmo, a[j], status); 
    g2 = 2.*log(gfac);
    for (int i=0; i<nk; i++) {
      y2d[j*nk+i] = y[i]+g2;
    } // end loop over k
  } // end loop over a

  // Check that ccl_growth_factor didn't fail
  if (*status) {
    free(eh); free(x); free(y); free(a); free(y2d);
    gsl_spline2d_free(log_power_lin);
    return;
  }

  // Initialize a 2D spline over P(k, a) [which is still unnormalized by sigma8]
  int splinstatus = gsl_spline2d_init(log_power_lin, x, a, y2d,nk,na);
  if (splinstatus) {
    free(eh); free(x); free(y); free(a); free(y2d);
    gsl_spline2d_free(log_power_lin);
    *status = CCL_ERROR_SPLINE;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_eh(): Error creating log_power_lin spline\n");
    return;
  }

  // Calculate sigma8 for the unnormalized P(k), using the standard
  // ccl_sigma8() function
  cosmo->data.p_lin = log_power_lin;
  cosmo->computed_power = true; // Temporarily set this to true
  double sigma8 = ccl_sigma8(cosmo, status);
  cosmo->computed_power = false;

  // Check that ccl_sigma8 didn't fail
  if (*status) {
    free(eh); free(x); free(y); free(a); free(y2d);
    gsl_spline2d_free(log_power_lin);
    return;
  }

  // Calculate normalization factor using computed value of sigma8, then
  // recompute P(k, a) using this normalization
  double log_normalization_factor = 2*(log(cosmo->params.sigma8) - log(sigma8));
  for (int i=0; i < nk; i++) {
    y[i] += log_normalization_factor;
  }
  for (int j = 0; j < na; j++) {
    gfac = ccl_growth_factor(cosmo, a[j], status);
    g2 = 2.*log(gfac);
    for (int i=0; i<nk; i++) {
      y2d[j*nk+i] = y[i]+g2; // Replace previous values
    } // end k loop
  } // end a loop

  splinstatus = gsl_spline2d_init(log_power_lin, x, a, y2d, nk, na);
  if (splinstatus) {
    free(eh); free(x); free(y); free(a); free(y2d);
    gsl_spline2d_free(log_power_lin);
    *status = CCL_ERROR_SPLINE;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_eh(): Error creating log_power_lin spline\n");
    return;
  }
  else
    cosmo->data.p_lin = log_power_lin;

  // Allocate a 2D spline for the nonlinear P(k) [which is just a copy of the
  // linear one for E&H]
  gsl_spline2d * log_power_nl = gsl_spline2d_alloc(PNL_SPLINE_TYPE, nk, na);
  splinstatus = gsl_spline2d_init(log_power_nl, x, a, y2d, nk, na);

  if (splinstatus) {
    free(eh); free(x); free(y); free(a); free(y2d);
    gsl_spline2d_free(log_power_lin);
    gsl_spline2d_free(log_power_nl);
    *status = CCL_ERROR_SPLINE;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_eh(): Error creating log_power_nl spline\n");
    return;
  }
  else
    cosmo->data.p_nl = log_power_nl;

  // Free temporary arrays
  free(eh); free(x); free(y); free(a); free(y2d);
}

/*------ ROUTINE: tsqr_BBKS -----
INPUT: ccl_parameters and k wavenumber in 1/Mpc
TASK: provide the square of the BBKS transfer function with baryonic correction
NOTE: Bardeen et al. (1986) as implemented in Sugiyama (1995) 
*/

static double tsqr_BBKS(ccl_parameters * params, double k)
{
  double q = k/(params->Omega_m*params->h*params->h*exp(-params->Omega_b*(1.0+pow(2.*params->h,.5)/params->Omega_m)));
  return pow(log(1.+2.34*q)/(2.34*q),2.0)/pow(1.+3.89*q+pow(16.1*q,2.0)+pow(5.46*q,3.0)+pow(6.71*q,4.0),0.5);
}

/*------ ROUTINE: bbks_power -----
INPUT: ccl_parameters and k wavenumber in 1/Mpc
TASK: provide the BBKS power spectrum with baryonic correction at single k
*/

//Calculate Normalization see Cosmology Notes 8.105 (TODO: whose comment is this?)
static double bbks_power(ccl_parameters * params, double k)
{
  return pow(k,params->n_s)*tsqr_BBKS(params, k);
}

/*------ ROUTINE: ccl_cosmology_compute_bbks_power -----
INPUT: cosmology
TASK: provide spline for the BBKS power spectrum with baryonic correction
*/

static void ccl_cosmology_compute_power_bbks(ccl_cosmology * cosmo, int * status)
{
  //These are the limits of the splining range
  cosmo->data.k_min_lin=ccl_splines->K_MIN;
  cosmo->data.k_min_nl=ccl_splines->K_MIN;
  cosmo->data.k_max_lin=ccl_splines->K_MAX;
  cosmo->data.k_max_nl=ccl_splines->K_MAX;
  double kmin = cosmo->data.k_min_lin;
  double kmax = ccl_splines->K_MAX;
  //Compute nk from number of decades and N_K = # k per decade
  double ndecades = log10(kmax) - log10(kmin);
  int nk = (int)ceil(ndecades*ccl_splines->N_K);
  double amin = ccl_splines->A_SPLINE_MINLOG_PK;
  double amax = ccl_splines->A_SPLINE_MAX;
  int na = ccl_splines->A_SPLINE_NA_PK+ccl_splines->A_SPLINE_NLOG_PK-1;

  // Exit if sigma8 wasn't specified
  if (isnan(cosmo->params.sigma8)) {
    *status = CCL_ERROR_INCONSISTENT;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_bbks(): sigma8 not set, required for BBKS\n");
    return; //Return without anything to free
  }

  // The x array is initially k, but will later
  // be overwritten with log(k)
  double * x = ccl_log_spacing(kmin, kmax, nk);
  double * y = malloc(sizeof(double)*nk);
  double * a = ccl_linlog_spacing(amin, ccl_splines->A_SPLINE_MIN_PK, amax, ccl_splines->A_SPLINE_NLOG_PK, ccl_splines->A_SPLINE_NA_PK);
  double * y2d = malloc(nk * na * sizeof(double));  
  
  //If error, store status, we will free later
  if (a==NULL||y==NULL|| x==NULL || y2d==0) {
    *status=CCL_ERROR_MEMORY;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_bbks(): memory allocation error\n");
  } 

  //Status flags
  int splinstatus=0;
  
  if(!*status){
    
    // After this loop x will contain log(k)
    for (int i=0; i<nk; i++) {
      y[i] = log(bbks_power(&cosmo->params, x[i]));
      x[i] = log(x[i]);
    }
    
    for (int j = 0; j < na; j++) {
      double gfac = ccl_growth_factor(cosmo,a[j], status);
      double g2 = 2.*log(gfac);
      for (int i=0; i<nk; i++) {
	y2d[j*nk+i] = y[i]+g2;
      }
    }
  }
  
  if(!*status){
    
    // Initialize a 2D spline over P(k, a) [which is still unnormalized by sigma8]
    gsl_spline2d * log_power_lin_unnorm = gsl_spline2d_alloc(PLIN_SPLINE_TYPE, nk,na);
    splinstatus = gsl_spline2d_init(log_power_lin_unnorm, x, a, y2d,nk,na);

    //If not, proceed
    if (!splinstatus) {     
      cosmo->data.p_lin=log_power_lin_unnorm;
      cosmo->computed_power=true;
    } else {
      gsl_spline2d_free(log_power_lin_unnorm);
      *status = CCL_ERROR_SPLINE;
      ccl_cosmology_set_status_message(cosmo,"ccl_power.c: ccl_cosmology_compute_power_bbks(): Error creating log_power_lin spline\n");
    }
  }

  if(*status){
    free(x); free(y); free(a); free(y2d);
    return;
  }

  // Calculate sigma8 for the unnormalized P(k), using the standard
  // ccl_sigma8() function
  double sigma8 = ccl_sigma8(cosmo,status);
  cosmo->computed_power=false;

  // Check that ccl_sigma8 didn't fail
  if (!*status) {
    
    // Calculate normalization factor using computed value of sigma8, then
    // recompute P(k, a) using this normalization
    double log_normalization_factor = 2*(log(cosmo->params.sigma8) - log(sigma8));
    for (int i=0; i<nk; i++) {
      y[i] += log_normalization_factor;
    }
    for (int j = 0; j < na; j++) {
      double gfac = ccl_growth_factor(cosmo,a[j], status);
      double g2 = 2.*log(gfac);
      for (int i=0; i<nk; i++) {
	y2d[j*nk+i] = y[i]+g2;
      }
    }
  }

  //Check growth didn't fail
  if (!*status) {
    
    gsl_spline2d * log_power_lin = gsl_spline2d_alloc(PLIN_SPLINE_TYPE, nk,na);
    splinstatus = gsl_spline2d_init(log_power_lin, x, a, y2d,nk,na);

    if (!splinstatus) {
      cosmo->data.p_lin=log_power_lin;
    } else {
      gsl_spline2d_free(log_power_lin);
      *status = CCL_ERROR_SPLINE;
      ccl_cosmology_set_status_message(cosmo,"ccl_power.c: ccl_cosmology_compute_power_bbks(): Error creating log_power_lin spline\n");
    }
  }

  if(!*status){
    // Allocate a 2D spline for the nonlinear P(k)
    //[which is just a copy of the linear one for BBKS]
    gsl_spline2d * log_power_nl = gsl_spline2d_alloc(PNL_SPLINE_TYPE, nk,na);
    splinstatus = gsl_spline2d_init(log_power_nl, x, a, y2d,nk,na);
    if (!splinstatus) {
      cosmo->data.p_nl = log_power_nl;
    } else {
      gsl_spline2d_free(log_power_nl);
      *status = CCL_ERROR_SPLINE;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_bbks(): Error creating log_power_nl spline\n");
    }
  }

  free(x); free(y); free(a); free(y2d);
  return;
}



/*------ ROUTINE: ccl_cosmology_compute_power_emu -----
INPUT: cosmology
TASK: provide spline for the emulated power spectrum from Cosmic EMU
*/

static void ccl_cosmology_compute_power_emu(ccl_cosmology * cosmo, int * status)
{

  struct precision pr;        // for precision parameters
  struct background ba;       // for cosmological background
  struct thermo th;           // for thermodynamics
  struct perturbs pt;         // for source functions
  struct transfers tr;        // for transfer functions
  struct primordial pm;       // for primordial spectra
  struct spectra sp;          // for output spectra
  struct nonlinear nl;        // for non-linear spectra
  struct lensing le;
  struct output op;
  struct file_content fc;

  double Omeganuh2_eq;
  double w0wacomb = -cosmo->params.w0 - cosmo->params.wa;
  
  ErrorMsg errmsg; // for error messages
  // generate file_content structure
  // Configuration parameters will be passed through this structure,
  // to avoid writing and reading .ini files for every call
  int parser_length = 20;
  int init_arr[7]={0,0,0,0,0,0,0};

  //Check initialization
  if (parser_init(&fc,parser_length,"none",errmsg) == _FAILURE_) {
    *status = CCL_ERROR_CLASS;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): parser init error:%s\n", errmsg);
  }

  // Check ranges to see if the cosmology is valid
  if((cosmo->params.h<0.55) || (cosmo->params.h>0.85)){
    *status=CCL_ERROR_INCONSISTENT;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_emu(): h is outside allowed range\n");
  }
  if(w0wacomb<8.1e-3){ //0.3^4
    *status=CCL_ERROR_INCONSISTENT;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_emu(): w0 and wa do not satisfy the emulator bound\n");
  }
  if(cosmo->params.Omega_n_mass*cosmo->params.h*cosmo->params.h>0.01){
    *status=CCL_ERROR_INCONSISTENT;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_emu(): Omega_nu does not satisfy the emulator bound\n");
  }
  
  // Check to see if sigma8 was defined
  if(isnan(cosmo->params.sigma8)){
    *status=CCL_ERROR_INCONSISTENT;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_emu(): sigma8 is not defined; specify sigma8 instead of A_s\n");
  }

  //If one of the previous test was unsuccessful, quit:
  if(*status) return;

  // Check if the cosmology has been set up with equal neutrino masses for the emulator
  // If not, check if the user has forced redistribution of masses and if so do this.
  if(cosmo->params.N_nu_mass>0) {
    if (cosmo->config.emulator_neutrinos_method == ccl_emu_strict){
      if (cosmo->params.N_nu_mass==3){
	//double diff1 = pow((cosmo->params.mnu[0] - cosmo->params.mnu[1]) * (cosmo->params.mnu[0] - cosmo->params.mnu[1]), 0.5);
	//double diff2 = pow((cosmo->params.mnu[1] - cosmo->params.mnu[2]) * (cosmo->params.mnu[1] - cosmo->params.mnu[2]), 0.5);
	//double diff3 = pow((cosmo->params.mnu[2] - cosmo->params.mnu[0]) * (cosmo->params.mnu[2] - cosmo->params.mnu[0]), 0.5);
	//if (diff1>1e-12 || diff2>1e-12 || diff3>1e-12){
	if (cosmo->params.mnu[0] != cosmo->params.mnu[1] || cosmo->params.mnu[0] != cosmo->params.mnu[2] || cosmo->params.mnu[1] != cosmo->params.mnu[2]){
	  *status = CCL_ERROR_INCONSISTENT;
	  ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_emu(): In the default configuration, you must pass a list of 3 equal neutrino masses or pass a sum and set mnu_type = ccl_mnu_sum_equal. If you wish to over-ride this, set config->emulator_neutrinos_method = 'ccl_emu_equalize'. This will force the neutrinos to be of equal mass but will result in internal inconsistencies.\n");
	}
      } else if (cosmo->params.N_nu_mass!=3){
	*status = CCL_ERROR_INCONSISTENT;
	ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_emu(): In the default configuration, you must pass a list of 3 equal neutrino masses or pass a sum and set mnu_type = ccl_mnu_sum_equal. If you wish to over-ride this, set config->emulator_neutrinos_method = 'ccl_emu_equalize'. This will force the neutrinos to be of equal mass but will result in internal inconsistencies.\n");
      }
    } else if (cosmo->config.emulator_neutrinos_method == ccl_emu_equalize){
      // Reset the masses to equal
      double mnu_eq[3] = {cosmo->params.sum_nu_masses / 3., cosmo->params.sum_nu_masses / 3., cosmo->params.sum_nu_masses / 3.};
      Omeganuh2_eq = ccl_Omeganuh2(1.0, 3, mnu_eq, cosmo->params.T_CMB, cosmo->data.accelerator, status);
    }
  } else {
    if(fabs(cosmo->params.N_nu_rel - 3.04)>1.e-6){
      *status=CCL_ERROR_INCONSISTENT;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_emu(): Set Neff = 3.04 for cosmic emulator predictions in absence of massive neutrinos.\n");
    }
  }

  if(!*status){
    // Prepare to run CLASS for linear scales
    ccl_fill_class_parameters(cosmo,&fc,parser_length, status);
  }
  
  if (!*status){
    ccl_run_class(cosmo, &fc,&pr,&ba,&th,&pt,&tr,&pm,&sp,&nl,&le,&op,init_arr,status);
  }

  if (*status){
    ccl_free_class_structs(cosmo, &ba,&th,&pt,&tr,&pm,&sp,&nl,&le,init_arr,status);
    if (parser_free(&fc)== _FAILURE_) {
      *status = CCL_ERROR_CLASS;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): Error freeing CLASS parser\n");
    }
    return;
  }
  
  //These are the limits of the splining range
  cosmo->data.k_min_lin=2*exp(sp.ln_k[0]);
  cosmo->data.k_max_lin=ccl_splines->K_MAX_SPLINE;
  //CLASS calculations done - now allocate CCL splines
  double kmin = cosmo->data.k_min_lin;
  double kmax = ccl_splines->K_MAX_SPLINE;
  //Compute nk from number of decades and N_K = # k per decade
  double ndecades = log10(kmax) - log10(kmin);
  int nk = (int)ceil(ndecades*ccl_splines->N_K);
  double amin = ccl_splines->A_SPLINE_MINLOG_PK;
  double amax = ccl_splines->A_SPLINE_MAX;
  int na = ccl_splines->A_SPLINE_NA_PK+ccl_splines->A_SPLINE_NLOG_PK-1;

  // The x array is initially k, but will later
  // be overwritten with log(k)
  double * x = ccl_log_spacing(kmin, kmax, nk);
  double * a = ccl_linlog_spacing(amin, ccl_splines->A_SPLINE_MIN_PK, amax, ccl_splines->A_SPLINE_NLOG_PK, ccl_splines->A_SPLINE_NA_PK);
  double * y2d_lin = malloc(nk * na * sizeof(double));
  if (a==NULL|| x==NULL || y2d_lin==NULL) {
    *status = CCL_ERROR_SPLINE;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_class(): memory allocation error\n");
  }
  
  if(!*status){
    // After this loop x will contain log(k), y will contain log(P_nl), z will contain log(P_lin)
    // all in Mpc, not Mpc/h units!
    double psout_l,ic;
    int s=0;
    for (int i=0; i<nk; i++) {
      for (int j = 0; j < na; j++) {
	//The 2D interpolation routines access the function values y_{k_ia_j} with the following ordering:
	//y_ij = y2d[j*N_k + i]
	//with i = 0,...,N_k-1 and j = 0,...,N_a-1.
	s |= spectra_pk_at_k_and_z(&ba, &pm, &sp,x[i],1./a[j]-1., &psout_l,&ic);
	y2d_lin[j*nk+i] = log(psout_l);
      }
      x[i] = log(x[i]);
    }
    if(s) {
      *status = CCL_ERROR_CLASS;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_emu(): Error computing CLASS power spectrum\n");
    }
  }

  if(!*status){
    
    gsl_spline2d * log_power = gsl_spline2d_alloc(PLIN_SPLINE_TYPE, nk,na);
    int pwstatus = gsl_spline2d_init(log_power, x, a, y2d_lin,nk,na);
    if (!pwstatus) {
      cosmo->data.p_lin = log_power;
    } else {
      gsl_spline2d_free(log_power);
      *status = CCL_ERROR_SPLINE;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_emu(): Error creating log_power spline\n");
    }
  }

  if(*status){ //Linear power spectrum failed, so quit.
      free(x);
      free(a);
      free(y2d_lin);
      ccl_free_class_structs(cosmo, &ba,&th,&pt,&tr,&pm,&sp,&nl,&le,init_arr,status);
      return;
  }
  
  //Now start the NL computation with the emulator
  //These are the limits of the splining range
  cosmo->data.k_min_nl=K_MIN_EMU;
  cosmo->data.k_max_nl=K_MAX_EMU;
  amin = A_MIN_EMU; //limit of the emulator
  amax = ccl_splines->A_SPLINE_MAX;
  na = ccl_splines->A_SPLINE_NA_PK;
  // The x array is initially k, but will later
  // be overwritten with log(k)
  double * logx= malloc(NK_EMU*sizeof(double));
  double * y;
  double * xstar = malloc(9 * sizeof(double));
  double * aemu = ccl_linear_spacing(amin,amax, na);
  double * y2d = malloc(NK_EMU * na * sizeof(double));
  if (aemu==NULL || y2d==NULL || logx==NULL || xstar==NULL){
    *status=CCL_ERROR_MEMORY;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_emu(): memory allocation error\n");
  }

  if(!*status){
    
    //For each redshift:
    for (int j = 0; j < na; j++){

      //Turn cosmology into xstar:
      xstar[0] = (cosmo->params.Omega_c+cosmo->params.Omega_b)*cosmo->params.h*cosmo->params.h;
      xstar[1] = cosmo->params.Omega_b*cosmo->params.h*cosmo->params.h;
      xstar[2] = cosmo->params.sigma8;
      xstar[3] = cosmo->params.h;
      xstar[4] = cosmo->params.n_s;
      xstar[5] = cosmo->params.w0;
      xstar[6] = cosmo->params.wa;
      if ((cosmo->params.N_nu_mass>0) && (cosmo->config.emulator_neutrinos_method == ccl_emu_equalize)){
	xstar[7] = Omeganuh2_eq;
      } else { 
        xstar[7] = cosmo->params.Omega_n_mass*cosmo->params.h*cosmo->params.h;
      }
      xstar[8] = 1./aemu[j]-1;
      //Need to have this here because otherwise overwritten by emu in each loop

      //Call emulator at this redshift
      ccl_pkemu(xstar,&y, status, cosmo);
      if (y == NULL){
	*status = CCL_ERROR_MEMORY;
	ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_emu(): Error obtaining emulator predictions\n");
      }
      for (int i=0; i<NK_EMU; i++){
	logx[i] = log(mode[i]);
	y2d[j*NK_EMU+i] = log(y[i]);
      }
    }
  }

  if(!*status){

    gsl_spline2d * log_power_nl = gsl_spline2d_alloc(PLIN_SPLINE_TYPE, NK_EMU,na);
    int splinstatus = gsl_spline2d_init(log_power_nl, logx, aemu, y2d,NK_EMU,na);
    //Note the minimum k of the spline is different from the linear one.

    if (!splinstatus){
      cosmo->data.p_nl = log_power_nl;
    } else {
      gsl_spline2d_free(log_power_nl);
      *status = CCL_ERROR_SPLINE;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power_emu(): Error creating log_power spline\n");
    }
  }

  free(x); free(a); free(y);
  free(xstar); free(logx); free(aemu);
  free(y2d_lin); free(y2d);
}



/*------ ROUTINE: ccl_cosmology_compute_power -----
INPUT: ccl_cosmology * cosmo
TASK: compute power spectrum
*/
void ccl_cosmology_compute_power(ccl_cosmology * cosmo, int * status)
{

  if (cosmo->computed_power) return;
    switch(cosmo->config.transfer_function_method){
        case ccl_bbks:
	  ccl_cosmology_compute_power_bbks(cosmo,status);
	  break;
        case ccl_eisenstein_hu:
	  ccl_cosmology_compute_power_eh(cosmo,status);
	  break;
        case ccl_boltzmann_class:
	  ccl_cosmology_compute_power_class(cosmo,status);
	  break;
        case ccl_emulator:
	  ccl_cosmology_compute_power_emu(cosmo,status);
	  break;
        default:
	  *status = CCL_ERROR_INCONSISTENT;
	  ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_cosmology_compute_power(): Unknown or non-implemented transfer function method: %d \n", cosmo->config.transfer_function_method);
    }

    ccl_check_status(cosmo,status);
    if (*status==0){
      cosmo->computed_power = true;
    }
  return;
}


/*------ ROUTINE: ccl_power_extrapol_highk -----
INPUT: ccl_cosmology * cosmo, a, k [1/Mpc]
TASK: extrapolate power spectrum at high k
*/
static double ccl_power_extrapol_highk(ccl_cosmology * cosmo, double k, double a,
				       gsl_spline2d * powerspl, double kmax_spline, int * status)
{
  double log_p_1;
  double deltak=1e-2; //step for numerical derivative;
  double deriv_pk_kmid,deriv2_pk_kmid;
  double lkmid;
  double lpk_kmid;

  lkmid = log(kmax_spline)-2*deltak;

  int gslstatus =  gsl_spline2d_eval_e(powerspl, lkmid,a,NULL ,NULL ,&lpk_kmid);
  if(gslstatus != GSL_SUCCESS) {
    ccl_raise_gsl_warning(gslstatus, "ccl_power.c: ccl_power_extrapol_highk():");
    *status = CCL_ERROR_SPLINE_EV;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_power_extrapol_highk(): Spline evaluation error\n");
    return NAN;
  }
  //GSL derivatives
  gslstatus = gsl_spline2d_eval_deriv_x_e (powerspl, lkmid, a, NULL,NULL,&deriv_pk_kmid);
  if(gslstatus != GSL_SUCCESS) {
    ccl_raise_gsl_warning(gslstatus, "ccl_power.c: ccl_power_extrapol_highk():");
    *status = CCL_ERROR_SPLINE_EV;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_power_extrapol_highk(): Spline evaluation error\n");
    return NAN;
  }
  gslstatus = gsl_spline2d_eval_deriv_xx_e (powerspl, lkmid, a, NULL,NULL,&deriv2_pk_kmid);
  if(gslstatus != GSL_SUCCESS) {
    ccl_raise_gsl_warning(gslstatus, "ccl_power.c: ccl_power_extrapol_highk():");
    *status = CCL_ERROR_SPLINE_EV;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_power_extrapol_highk(): Spline evaluation error\n");
    return NAN;
  }
  log_p_1=lpk_kmid+deriv_pk_kmid*(log(k)-lkmid)+deriv2_pk_kmid/2.*(log(k)-lkmid)*(log(k)-lkmid);

  return log_p_1;

}

/*------ ROUTINE: ccl_power_extrapol_lowk -----
INPUT: ccl_cosmology * cosmo, a, k [1/Mpc]
TASK: extrapolate power spectrum at low k
*/
static double ccl_power_extrapol_lowk(ccl_cosmology * cosmo, double k, double a,
				      gsl_spline2d * powerspl, double kmin_spline, int * status)
{
  double log_p_1;
  double deltak=1e-2; //safety step
  double lkmin=log(kmin_spline)+deltak;
  double lpk_kmin;
  int gslstatus = gsl_spline2d_eval_e(powerspl,lkmin,a,NULL,NULL,&lpk_kmin);

  if(gslstatus != GSL_SUCCESS) {
    ccl_raise_gsl_warning(gslstatus, "ccl_power.c: ccl_power_extrapol_lowk():");
    *status=CCL_ERROR_SPLINE_EV;
    ccl_cosmology_set_status_message(cosmo,"ccl_power.c: ccl_power_extrapol_lowk(): Spline evaluation error\n");
    return NAN;
  }

  return lpk_kmin+cosmo->params.n_s*(log(k)-lkmin);
}


/*------ ROUTINE: ccl_linear_matter_power -----
INPUT: ccl_cosmology * cosmo, k [1/Mpc],a
TASK: compute the linear power spectrum at a given redshift
      by rescaling using the growth function
*/

double ccl_linear_matter_power(ccl_cosmology * cosmo, double k, double a, int * status)

{
  if ((cosmo->config.transfer_function_method == ccl_emulator) && (a<A_MIN_EMU)){
    *status = CCL_ERROR_INCONSISTENT;
    ccl_cosmology_set_status_message(cosmo, "ccl_power.c: the cosmic emulator cannot be used above a=%f\n",A_MIN_EMU);
    return NAN;
  }

  if (!cosmo->computed_power) ccl_cosmology_compute_power(cosmo, status);
  // Return if compilation failed
  //if (cosmo->data.p_lin == NULL) return NAN;
  if (!cosmo->computed_power) return NAN;

  double log_p_1;
  int gslstatus;

  if(a<ccl_splines->A_SPLINE_MINLOG_PK) {  //Extrapolate linearly at high redshift
    double pk0=ccl_linear_matter_power(cosmo,k,ccl_splines->A_SPLINE_MINLOG_PK,status);
    double gf=ccl_growth_factor(cosmo,a,status)/ccl_growth_factor(cosmo,ccl_splines->A_SPLINE_MINLOG_PK,status);

    return pk0*gf*gf;
  }
  if (*status!=CCL_ERROR_INCONSISTENT){
    if(k<=cosmo->data.k_min_lin) {
      log_p_1=ccl_power_extrapol_lowk(cosmo,k,a,cosmo->data.p_lin,cosmo->data.k_min_lin,status);

      return exp(log_p_1);
    }
    else if(k<cosmo->data.k_max_lin){
      gslstatus = gsl_spline2d_eval_e(cosmo->data.p_lin, log(k), a,NULL,NULL,&log_p_1);
      if(gslstatus != GSL_SUCCESS) {
        ccl_raise_gsl_warning(gslstatus, "ccl_power.c: ccl_linear_matter_power():");
        *status = CCL_ERROR_SPLINE_EV;
        ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_linear_matter_power(): Spline evaluation error\n");
        return NAN;
      }
      else {
        return exp(log_p_1);
      }
    }
    else { //Extrapolate using log derivative
      log_p_1 = ccl_power_extrapol_highk(cosmo,k,a,cosmo->data.p_lin,cosmo->data.k_max_lin,status);
      return exp(log_p_1);
    }
  }

  return exp(log_p_1);
}


/*------ ROUTINE: ccl_nonlin_matter_power -----
INPUT: ccl_cosmology * cosmo, a, k [1/Mpc]
TASK: compute the nonlinear power spectrum at a given redshift
*/

double ccl_nonlin_matter_power(ccl_cosmology * cosmo, double k, double a, int *status)
{
  double log_p_1, pk;

  switch(cosmo->config.matter_power_spectrum_method) {

  //If the matter PS specified was linear, then do the linear compuation
  case ccl_linear:
    return ccl_linear_matter_power(cosmo,k,a,status);

  case ccl_halofit:
    if (!cosmo->computed_power)
      ccl_cosmology_compute_power(cosmo, status);
    if (cosmo->data.p_nl == NULL) return NAN; // Return if computation failed

    if(a<ccl_splines->A_SPLINE_MINLOG_PK) { //Extrapolate linearly at high redshift
      double pk0=ccl_nonlin_matter_power(cosmo,k,ccl_splines->A_SPLINE_MINLOG_PK,status);
      double gf=ccl_growth_factor(cosmo,a,status)/ccl_growth_factor(cosmo,ccl_splines->A_SPLINE_MINLOG_PK,status);
      return pk0*gf*gf;
    }
		break;

  case ccl_emu:
    if ((cosmo->config.transfer_function_method == ccl_emulator) && (a<A_MIN_EMU)){
      *status = CCL_ERROR_EMULATOR_BOUND;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: the cosmic emulator cannot be used above z=2\n");
      return NAN;
    }

    // Compute power spectrum if needed; return if computation failed
    if (!cosmo->computed_power){
      ccl_cosmology_compute_power(cosmo,status);
    }
    if (cosmo->data.p_nl == NULL) return NAN;
		break;

  default:
    ccl_raise_warning(
			CCL_ERROR_NOT_IMPLEMENTED,
      "config.matter_power_spectrum_method = %d not yet supported "
			"continuing with linear power spectrum\n", cosmo->config.matter_power_spectrum_method);
    cosmo->config.matter_power_spectrum_method=ccl_linear;
    return ccl_linear_matter_power(cosmo,k,a,status);
  } // end switch

  // if we get here, try to evaluate the power spectrum
  // we need to account for bounds below and above
  if (k <= cosmo->data.k_min_nl) {
    // we assume no baryonic effects below k_min_nl
    log_p_1 = ccl_power_extrapol_lowk(cosmo, k, a, cosmo->data.p_nl, cosmo->data.k_min_nl, status);
    return exp(log_p_1);
  }

  if (k < cosmo->data.k_max_nl) {
    int gslstatus = gsl_spline2d_eval_e(cosmo->data.p_nl, log(k), a, NULL ,NULL, &log_p_1);
    if (gslstatus != GSL_SUCCESS) {
      ccl_raise_gsl_warning(gslstatus, "ccl_power.c: ccl_nonlin_matter_power():");
      *status = CCL_ERROR_SPLINE_EV;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_nonlin_matter_power(): Spline evaluation error\n");
      return NAN;
    } else {
      pk = exp(log_p_1);
    }
  } else {
    // Extrapolate NL regime using log derivative
    log_p_1 = ccl_power_extrapol_highk(cosmo, k, a, cosmo->data.p_nl, cosmo->data.k_max_nl, status);
    pk = exp(log_p_1);
  }

	// Add baryonic correction
  if (cosmo->config.baryons_power_spectrum_method == ccl_bcm) {
    int pwstatus = 0;
    double fbcm = ccl_bcm_model_fka(cosmo, k, a, &pwstatus);
    pk *= fbcm;
    if (pwstatus) {
      *status = CCL_ERROR_SPLINE_EV;
      ccl_cosmology_set_status_message(cosmo, "ccl_power.c: ccl_nonlin_matter_power(): Error in BCM correction\n");
      return NAN;
    }
  }

  return pk;
}

// Params for sigma(R) integrand
typedef struct {
  ccl_cosmology *cosmo;
  double R;
  int* status;
} SigmaR_pars;

typedef struct {
  ccl_cosmology *cosmo;
  double R;
  int* status;
} SigmaV_pars;

/* --------- ROUTINE: w_tophat ---------
INPUT: kR, ususally a wavenumber multiplied by a smoothing radius
TASK: Output W(x)=[sin(x)-x*cos(x)]*(3/x)^3
*/
static double w_tophat(double kR)
{
  double w;
  double kR2 = kR*kR;

  // This is the Maclaurin expansion of W(x)=[sin(x)-xcos(x)]*(3/x)**3 to O(x^7), with x=kR.
  // Necessary numerically because at low x W(x) relies on the fine cancellation of two terms
  if(kR<0.1) {
     w = 1. + kR2*(-0.1 +
		   kR2*(0.003561429 +
			kR2*(-6.61376e-5 +
			     kR2*(7.51563e-7))));
     /*w =1.-0.1*kR*kR+0.003571429*kR*kR*kR*kR
      -6.61376E-5*kR*kR*kR*kR*kR*kR
      +7.51563E-7*kR*kR*kR*kR*kR*kR*kR*kR;*/
  }
  else
    w = 3.*(sin(kR) - kR*cos(kR))/(kR2*kR);
  return w;
}

// Integrand for sigmaR integral
static double sigmaR_integrand(double lk,void *params)
{
  SigmaR_pars *par=(SigmaR_pars *)params;

  double k=pow(10.,lk);
  double pk=ccl_linear_matter_power(par->cosmo,k, 1.,par->status);
  double kR=k*par->R;
  double w = w_tophat(kR);

  return pk*k*k*k*w*w;
}

// Integrand for sigmaV integral
static double sigmaV_integrand(double lk,void *params)
{
  SigmaV_pars *par=(SigmaV_pars *)params;

  double k=pow(10.,lk);
  double pk=ccl_linear_matter_power(par->cosmo,k, 1.,par->status);
  double kR=k*par->R;
  double w = w_tophat(kR);

  return pk*k*w*w/3.0;
}

/* --------- ROUTINE: ccl_sigmaR ---------
INPUT: cosmology, comoving smoothing radius, scale factor
TASK: compute sigmaR, the variance in the *linear* density field
smoothed with a tophat filter of comoving size R
*/
double ccl_sigmaR(ccl_cosmology *cosmo,double R,double a,int *status)
{
  SigmaR_pars par;
  par.status = status;

  par.cosmo=cosmo;
  par.R=R;
  gsl_integration_cquad_workspace *workspace=gsl_integration_cquad_workspace_alloc(ccl_gsl->N_ITERATION);
  gsl_function F;
  F.function=&sigmaR_integrand;
  F.params=&par;
  double sigma_R;
  int gslstatus = gsl_integration_cquad(&F, log10(ccl_splines->K_MIN), log10(ccl_splines->K_MAX),
				                                0.0, ccl_gsl->INTEGRATION_SIGMAR_EPSREL,
                                        workspace,&sigma_R,NULL,NULL);
  if(gslstatus != GSL_SUCCESS) {
    ccl_raise_gsl_warning(gslstatus, "ccl_power.c: ccl_sigmaR():");
    *status |= gslstatus;
  }

  gsl_integration_cquad_workspace_free(workspace);

  return sqrt(sigma_R*M_LN10/(2*M_PI*M_PI))*ccl_growth_factor(cosmo, a, status);
}

/* --------- ROUTINE: ccl_sigmaV ---------
INPUT: cosmology, comoving smoothing radius, scale factor
TASK: compute sigmaV, the variance in the *linear* displacement field
smoothed with a tophat filter of comoving size R
The linear displacement field is the gradient of the linear density field
*/
double ccl_sigmaV(ccl_cosmology *cosmo,double R,double a,int *status)
{
  SigmaV_pars par;
  par.status = status;

  par.cosmo=cosmo;
  par.R=R;
  gsl_integration_cquad_workspace *workspace=gsl_integration_cquad_workspace_alloc(ccl_gsl->N_ITERATION);
  gsl_function F;
  F.function=&sigmaV_integrand;
  F.params=&par;
  double sigma_V;
	int gslstatus = gsl_integration_cquad(&F, log10(ccl_splines->K_MIN), log10(ccl_splines->K_MAX),
																				0.0, ccl_gsl->INTEGRATION_SIGMAR_EPSREL,
																				workspace,&sigma_V,NULL,NULL);

  if(gslstatus != GSL_SUCCESS) {
    ccl_raise_gsl_warning(gslstatus, "ccl_power.c: ccl_sigmaV():");
    *status |= gslstatus;
  }

  gsl_integration_cquad_workspace_free(workspace);

  return sqrt(sigma_V*M_LN10/(2*M_PI*M_PI))*ccl_growth_factor(cosmo, a, status);
}

/* --------- ROUTINE: ccl_sigma8 ---------
INPUT: cosmology
TASK: compute sigma8, the variance in the *linear* density field at a=1
smoothed with a tophat filter of comoving size 8 Mpc/h
*/
double ccl_sigma8(ccl_cosmology *cosmo, int *status)
{
  return ccl_sigmaR(cosmo,8/cosmo->params.h, 1.,status);
}
