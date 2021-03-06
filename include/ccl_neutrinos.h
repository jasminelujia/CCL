/** @file */
#ifndef __CCL_NEUTRINOS_H_INCLUDED__
#define __CCL_NEUTRINOS_H_INCLUDED__

#include <gsl/gsl_spline.h>
#include <gsl/gsl_const_mksa.h>

// maximum number of species
#define CCL_MAX_NU_SPECIES 3
// limits for the precomputed spline of phase
// space diagram in MNU/T
#define CCL_NU_MNUT_MIN 1e-4
#define CCL_NU_MNUT_MAX 500
// and number of points
#define CCL_NU_MNUT_N 1000

// The combination of constants required in Omeganuh2
#define NU_CONST (8. * pow(M_PI,5) *pow((KBOLTZ/ HPLANCK),3)* KBOLTZ/(15. *pow( CLIGHT,3))* (8. * M_PI * GNEWT) / (3. * 100.*100.*1000.*1000. /MPC_TO_METER /MPC_TO_METER  * CLIGHT * CLIGHT))

CCL_BEGIN_DECLS

typedef enum ccl_neutrino_mass_splits{
  ccl_nu_normal=0,
  ccl_nu_inverted=1,
  ccl_nu_equal=2,
  ccl_nu_sum=3
} ccl_neutrino_mass_splits;

/**
 * Spline for the phasespace integral required for getting the fractional energy density of massive neutrinos.
 * Returns a gsl spline for the phase space integral needed for massive neutrinos.
 * @param status Status flag. 0 if there are no errors, nonzero otherwise.
 * For specific cases see documentation for ccl_error.c
 * @return spl, the gsl spline for the phasespace integral required for massive neutrino calculations.
 */
gsl_spline* calculate_nu_phasespace_spline(int *status);

/** 
 * Returns density of one neutrino species at a scale factor a. 
 * Users are encouraged to access this quantity via the function ccl_omega_x.
 * @param a Scale factor
 * @param Neff The effective number of species with neutrino mass mnu.
 * @param mnu Pointer to array containing neutrino mass (can be 0).
 * @param T_CMB Temperature of the CMB
 * @param accel - Interpolation accelerator to be used with phasespace spline. If not set yet, pass NULL.
 * @param status Status flag. 0 if there are no errors, nonzero otherwise.
 * For specific cases see documentation for ccl_error.c
 * @return OmNuh2 Fractional energy density of neutrions with mass mnu, multiplied by h squared. 
 */

double ccl_Omeganuh2 (double a, int N_nu_mass, double* mnu, double T_CMB, gsl_interp_accel* accel, int * status);

/** 
 * Returns mass of one neutrino species at a scale factor a. 
 * @param a Scale factor
 * @param Neff The effective number of species with neutrino mass mnu.
 * @param OmNuh2 Fractional energy density of neutrions with mass mnu, multiplied by h squared. (can be 0).
 * @param T_CMB Temperature of the CMB
 * @param accel - Interpolation accelerator to be used with phasespace spline. If not set yet, pass NULL.
 * @param status Status flag. 0 if there are no errors, nonzero otherwise.
 * For specific cases see documentation for ccl_error.c
 * @return Mnu Neutrino mass [eV]. 
 */
double* ccl_nu_masses (double OmNuh2, ccl_neutrino_mass_splits mass_split, double T_CMB, int * status);

CCL_END_DECLS
#endif
