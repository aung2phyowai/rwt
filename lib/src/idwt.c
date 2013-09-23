/*! \file idwt.c
    \brief Implementation of the inverse discrete wavelet transform

*/

#include "rwt_platform.h"

/*!
 * Perform convolution for idwt
 *
 * @param x_out
 * @param lx
 * @param g0
 * @param g1
 * @param lh_minus_one
 * @param lh_halved_minus_one
 * @param x_in_low
 * @param x_in_high
 * 
 */
void idwt_convolution(double *x_out, int lx, double *g0, double *g1, int lh_minus_one, int lh_halved_minus_one, double *x_in_low, double *x_in_high) {
  int i, j, ind, tj;
  double x0, x1;

  for (i=lh_halved_minus_one-1; i > -1; i--){
    x_in_low[i] = x_in_low[lx+i];
    x_in_high[i] = x_in_high[lx+i];
  }
  ind = 0;
  for (i=0; i<(lx); i++){
    x0 = 0;
    x1 = 0;
    tj = -2;
    for (j=0; j<=lh_halved_minus_one; j++){
      tj+=2;
      x0 = x0 + x_in_low[i+j]*g0[lh_minus_one-1-tj] + x_in_high[i+j]*g1[lh_minus_one-1-tj] ;
      x1 = x1 + x_in_low[i+j]*g0[lh_minus_one-tj] + x_in_high[i+j]*g1[lh_minus_one-tj] ;
    }
    x_out[ind++] = x0;
    x_out[ind++] = x1;
  }
}


/*!
 * Allocate memory for idwt
 *
 * @param m the number of rows of the input matrix
 * @param n the number of columns of the input matrix
 * @param lh the number of scaling coefficients
 * @param xdummy
 * @param y_dummy_low
 * @param y_dummy_high
 * @param g0
 * @param g1
 *
 */
void idwt_allocate(int m, int n, int lh, double **xdummy, double **y_dummy_low, double **y_dummy_high, double **g0, double **g1) {
  *xdummy       = (double *) rwt_calloc(max(m,n),        sizeof(double));
  *y_dummy_low  = (double *) rwt_calloc(max(m,n)+lh/2-1, sizeof(double));
  *y_dummy_high = (double *) rwt_calloc(max(m,n)+lh/2-1, sizeof(double));
  *g0           = (double *) rwt_calloc(lh,              sizeof(double));
  *g1           = (double *) rwt_calloc(lh,              sizeof(double));
}


/*!
 * Free memory we allocated for idwt
 *
 * @param xdummy
 * @param y_dummy_low
 * @param y_dummy_high
 * @param g0
 * @param g1
 *
 */
void idwt_free(double **xdummy, double **y_dummy_low, double **y_dummy_high, double **g0, double **g1) {
  rwt_free(*xdummy);
  rwt_free(*y_dummy_low);
  rwt_free(*y_dummy_high);
  rwt_free(*g0);
  rwt_free(*g1);
}


/*!
 * Put the scaling coeffients into a form ready for use in the convolution function
 *
 * @param lh length of h / the number of scaling coefficients
 * @param h  the wavelet scaling coefficients
 * @param g0 same as h
 * @param g1 reversed h, even values are sign flipped
 *
 */
void idwt_coefficients(int lh, double *h, double **g0, double **g1) {
  int i;
  for (i=0; i<lh; i++){
    (*g0)[i] = h[i];
    (*g1)[i] = h[lh-i-1];
  }
  for (i=1; i<=lh; i+=2)
    (*g1)[i] = -((*g1)[i]);
}


/*!
 * Perform the inverse discrete wavelet transform
 *
 * @param x  the output signal with the inverse wavelet transform applied
 * @param m  number of rows in the input
 * @param n  number of columns in the input
 * @param h  wavelet scaling coefficients
 * @param lh length of h / the number of scaling coefficients
 * @param L  the number of levels
 * @param y  the input signal
 *
 */
void idwt(double *x, int m, int n, double *h, int lh, int L, double *y) {
  double  *g0, *g1, *y_dummy_low, *y_dummy_high, *xdummy;
  long i;
  int current_level, current_rows, current_cols, row_of_a, column_of_a, ir, ic, lh_minus_one, lh_halved_minus_one, sample_f;

  idwt_allocate(m, n, lh, &xdummy, &y_dummy_low, &y_dummy_high, &g0, &g1);
  idwt_coefficients(lh, h, &g0, &g1);

  if (n==1){
    n = m;
    m = 1;
  }
  
  lh_minus_one = lh - 1;
  lh_halved_minus_one = lh/2 - 1;
  /* 2^L */
  sample_f = 1;
  for (i=1; i<L; i++)
    sample_f = sample_f*2;
  
  if (m>1)
    current_rows = m/sample_f;
  else 
    current_rows = 1;
  current_cols = n/sample_f;

  for (i=0; i<(m*n); i++)
    x[i] = y[i];
  
  /* main loop */
  for (current_level=L; current_level >= 1; current_level--){
    row_of_a = current_rows/2;
    column_of_a = current_cols/2;
    
    /* go by columns in case of a 2D signal*/
    if (m>1){
      for (ic=0; ic<current_cols; ic++){            /* loop over column */
	/* store in dummy variables */
	ir = row_of_a;
	for (i=0; i<row_of_a; i++){    
	  y_dummy_low[i+lh_halved_minus_one]  = mat(x, i,    ic, m, n);  
	  y_dummy_high[i+lh_halved_minus_one] = mat(x, ir++, ic, m, n);  
	}
	/* perform filtering lowpass and highpass*/
	idwt_convolution(xdummy, row_of_a, g0, g1, lh_minus_one, lh_halved_minus_one, y_dummy_low, y_dummy_high); 
	/* restore dummy variables in matrix */
	for (i=0; i<current_rows; i++)
	  mat(x, i, ic, m, n) = xdummy[i];  
      }
    }
    /* go by rows */
    for (ir=0; ir<current_rows; ir++){            /* loop over rows */
      /* store in dummy variable */
      ic = column_of_a;
      for  (i=0; i<column_of_a; i++){    
	y_dummy_low[i+lh_halved_minus_one]  = mat(x, ir, i,    m, n);  
	y_dummy_high[i+lh_halved_minus_one] = mat(x, ir, ic++, m, n);  
      } 
      /* perform filtering lowpass and highpass*/
      idwt_convolution(xdummy, column_of_a, g0, g1, lh_minus_one, lh_halved_minus_one, y_dummy_low, y_dummy_high); 
      /* restore dummy variables in matrices */
      for (i=0; i<current_cols; i++)
        mat(x, ir, i, m, n) = xdummy[i];  
    }  
    if (m==1)
      current_rows = 1;
    else
      current_rows = current_rows*2;
    current_cols = current_cols*2;
  }
  idwt_free(&xdummy, &y_dummy_low, &y_dummy_high, &g0, &g1);
}

