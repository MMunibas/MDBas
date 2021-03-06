#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>

#include "global.h"
#include "utils.h"
#include "spme.h"

typedef double complex cplx;

static int newJob;

static double *sx,*sy,*sz;

static double *bsp,*qsp;
static double **bsp1,**bsp2,**bsp3;
static double **bsd1,**bsd2,**bsd3;

static cplx *bspc1,*bspc2,*bspc3;
static cplx *epl1,*epl2,*epl3;

#ifdef FFTW
static fftw_complex *ftqsp;
static fftw_plan fft3d1;
static fftw_plan fft3d2;
#else
static cplx *ftqsp;
#endif

void init_spme(CTRL *ctrl,PARAM *param,EWALD *ewald,PBC *box)
{
  
  int i,m1maxp2,m2maxp2,m3maxp2;
  
  newJob=1;
  
  ewald->prec=fmin( fabs( ewald->prec ) , 0.5 );
  ewald->tol=sqrt( fabs( log( ewald->prec * param->cutOff ) ) );
    
  if(!ctrl->keyAlpha)
    ewald->alpha=sqrt( fabs( log( ewald->prec * param->cutOff * ewald->tol ) ) ) / param->cutOff;
    
  ewald->tol1=sqrt( -log( ewald->prec * param->cutOff * X2( 2.0 * ewald->tol * ewald->alpha ) ) );
  
  if(!ctrl->keyMmax)
  {
    ewald->m1max=nint(0.25+box->pa*ewald->alpha*ewald->tol1/PI);
    ewald->m2max=nint(0.25+box->pb*ewald->alpha*ewald->tol1/PI);
    ewald->m3max=nint(0.25+box->pc*ewald->alpha*ewald->tol1/PI);
  }
  
  m1maxp2=1;
  while( (ewald->m1max>m1maxp2) && (m1maxp2<256) )
  {
    m1maxp2*=2;
  }
  ewald->m1max=2*m1maxp2;
  
  m2maxp2=1;
  while( (ewald->m2max>m2maxp2) && (m2maxp2<256) )
  {
    m2maxp2*=2;
  }
  ewald->m2max=2*m2maxp2;
  
  m3maxp2=1;
  while( (ewald->m3max>m3maxp2) && (m3maxp2<256) )
  {
    m3maxp2*=2;
  }
  ewald->m3max=2*m3maxp2;
  
  epl1=(cplx*)malloc(ewald->m1max*sizeof(*epl1));
  epl2=(cplx*)malloc(ewald->m2max*sizeof(*epl2));
  epl3=(cplx*)malloc(ewald->m3max*sizeof(*epl3));
  
  bspc1=(cplx*)malloc(ewald->m1max*sizeof(*bspc1));
  bspc2=(cplx*)malloc(ewald->m2max*sizeof(*bspc2));
  bspc3=(cplx*)malloc(ewald->m3max*sizeof(*bspc3));
  
  sx=(double*)malloc(param->nAtom*sizeof(*sx));
  sy=(double*)malloc(param->nAtom*sizeof(*sy));
  sz=(double*)malloc(param->nAtom*sizeof(*sz));
  
  bsp=(double*)malloc(ewald->nbsp*sizeof(*bsp));
  
  bsp1=(double**)malloc(param->nAtom*sizeof(*bsp1));
  bsp2=(double**)malloc(param->nAtom*sizeof(*bsp2));
  bsp3=(double**)malloc(param->nAtom*sizeof(*bsp3));
  
  bsd1=(double**)malloc(param->nAtom*sizeof(*bsd1));
  bsd2=(double**)malloc(param->nAtom*sizeof(*bsd2));
  bsd3=(double**)malloc(param->nAtom*sizeof(*bsd3));
  
  for(i=0;i<param->nAtom;i++)
  {
    bsp1[i]=(double*)malloc(ewald->nbsp*sizeof(**bsp1));
    bsp2[i]=(double*)malloc(ewald->nbsp*sizeof(**bsp2));
    bsp3[i]=(double*)malloc(ewald->nbsp*sizeof(**bsp3));
    
    bsd1[i]=(double*)malloc(ewald->nbsp*sizeof(**bsd1));
    bsd2[i]=(double*)malloc(ewald->nbsp*sizeof(**bsd2));
    bsd3[i]=(double*)malloc(ewald->nbsp*sizeof(**bsd3));
  }
  
  ewald->mmax=ewald->m1max*ewald->m2max*ewald->m3max;
  
  qsp=(double*)malloc(ewald->mmax*sizeof(*qsp));
  
#ifdef FFTW
  ftqsp=(fftw_complex*)fftw_malloc(ewald->mmax*sizeof(*ftqsp));
#else
  ftqsp=(cplx*)malloc(ewald->mmax*sizeof(*ftqsp));
#endif
  
}

void spme_free(PARAM *param)
{
  int i;
  
  free(epl1);
  free(epl2);
  free(epl3);
  
  free(bspc1);
  free(bspc2);
  free(bspc3);
  
  free(sx);
  free(sy);
  free(sz);
  
  free(bsp);
    
  for(i=0;i<param->nAtom;i++)
  {
    free(bsp1[i]);
    free(bsp2[i]);
    free(bsp3[i]);
    
    free(bsd1[i]);
    free(bsd2[i]);
    free(bsd3[i]);
  }
  
  free(bsp1);
  free(bsp2);
  free(bsp3);
  
  free(bsd1);
  free(bsd2);
  free(bsd3);
  
  free(qsp);
  
#ifdef FFTW
  fftw_free(ftqsp);
  
  fftw_destroy_plan(fft3d1);
  fftw_destroy_plan(fft3d2);
#else
  free(ftqsp);
#endif
  
}

void epl_cplx(EWALD *ewald)
{
  
  int i,hm1max,hm2max,hm3max;
  
  hm1max=ewald->m1max/2;
  hm2max=ewald->m2max/2;
  hm3max=ewald->m3max/2;
  
  epl1[0]=1.0+I*0.0;
  
  for(i=1;i<=hm1max;i++)
  {
    epl1[i]=cexp(I*TWOPI*(double)i/(double)ewald->m1max);
    epl1[ewald->m1max-i]=conj(epl1[i]);
  }
  
  epl2[0]=1.0+I*0.0;
  
  for(i=1;i<=hm2max;i++)
  {
    epl2[i]=cexp(I*TWOPI*(double)i/(double)ewald->m2max);
    epl2[ewald->m2max-i]=conj(epl2[i]);
  }
  
  epl3[0]=1.0+I*0.0;
  
  for(i=1;i<=hm3max;i++)
  {
    epl3[i]=cexp(I*TWOPI*(double)i/(double)ewald->m3max);
    epl3[ewald->m3max-i]=conj(epl3[i]);
  }
  
}

void bspcoef(EWALD *ewald)
{
  
  int i,j,k;
  cplx coeff;
  
  bsp[0]=0.0;
  bsp[1]=1.0;
    
  for(k=2;k<ewald->nbsp;k++)
  {
    
    bsp[k]=0.0;
    
    for(j=k;j>0;j--)
    {
      bsp[j] = ( (double)j*bsp[j]+(double)(k+1-j)*bsp[j-1] ) / ( (double)k );
    }
  }
  
  for(i=0;i<ewald->m1max;i++)
  {
    
    coeff=0.+I*0.0;
    
    for(k=0;k<ewald->nbsp-1;k++)
    {
      coeff+=bsp[k+1]*epl1[( (i*k) % ewald->m1max )];
    }
    
    bspc1[i]=epl1[( ( i* (ewald->nbsp-1) ) % ewald->m1max )]/coeff;
    
  }
  
  for(i=0;i<ewald->m2max;i++)
  {
    
    coeff=0.+I*0.0;
    
    for(k=0;k<ewald->nbsp-1;k++)
    {
      coeff+=bsp[k+1]*epl2[( (i*k) % ewald->m2max )];
    }
    
    bspc2[i]=epl2[( ( i* (ewald->nbsp-1) ) % ewald->m2max )]/coeff;
    
  }
  
  for(i=0;i<ewald->m3max;i++)
  {
    
    coeff=0.+I*0.0;
    
    for(k=0;k<ewald->nbsp-1;k++)
    {
      coeff+=bsp[k+1]*epl3[( (i*k) % ewald->m3max )];
    }
    
    bspc3[i]=epl3[( ( i* (ewald->nbsp-1) ) % ewald->m3max )]/coeff;
    
  }
  
}

void bspgen(PARAM *param,EWALD *ewald)
{
  
  int i,j,k;
  double tsx,tsy,tsz;
  
  for(i=0;i<param->nAtom;i++)
  {
    
    bsd1[i][0]=1.0;
    bsd2[i][0]=1.0;
    bsd3[i][0]=1.0;
    
    bsd1[i][1]=-1.0;
    bsd2[i][1]=-1.0;
    bsd3[i][1]=-1.0;
    
    bsp1[i][0]=sx[i]-(int)sx[i];
    bsp2[i][0]=sy[i]-(int)sy[i];
    bsp3[i][0]=sz[i]-(int)sz[i];
    
    bsp1[i][1]=1.0-bsp1[i][0];
    bsp2[i][1]=1.0-bsp2[i][0];
    bsp3[i][1]=1.0-bsp3[i][0];
    
  }
    
  for(k=2;k<ewald->nbsp;k++)
  {
    
    for(i=0;i<param->nAtom;i++)
    {
      bsp1[i][k]=0.0;
      bsp2[i][k]=0.0;
      bsp3[i][k]=0.0;
    }
    
    for(j=k;j>0;j--)
    {
    
      if( k == (ewald->nbsp-1) )
      {
	for(i=0;i<param->nAtom;i++)
	{
	  bsd1[i][j]=bsp1[i][j]-bsp1[i][j-1];
	  bsd2[i][j]=bsp2[i][j]-bsp2[i][j-1];
	  bsd3[i][j]=bsp3[i][j]-bsp3[i][j-1];
	}
      }
      
      for(i=0;i<param->nAtom;i++)
      {
	tsx=sx[i]+(double)j-(int)sx[i];
	tsy=sy[i]+(double)j-(int)sy[i];
	tsz=sz[i]+(double)j-(int)sz[i];
	
	bsp1[i][j]=(tsx*bsp1[i][j]+((double)(k+1)-tsx)*bsp1[i][j-1])/((double)k);
	bsp2[i][j]=(tsy*bsp2[i][j]+((double)(k+1)-tsy)*bsp2[i][j-1])/((double)k);
	bsp3[i][j]=(tsz*bsp3[i][j]+((double)(k+1)-tsz)*bsp3[i][j-1])/((double)k);
      }
      
    }
    
    if( k == (ewald->nbsp-1) )
    {
      for(i=0;i<param->nAtom;i++)
      {
	bsd1[i][0]=bsp1[i][0];
	bsd2[i][0]=bsp2[i][0];
	bsd3[i][0]=bsp3[i][0];
      }
    }
    
    for(i=0;i<param->nAtom;i++)
    {
      tsx=sx[i]-(int)sx[i];
      tsy=sy[i]-(int)sy[i];
      tsz=sz[i]-(int)sz[i];
      
      bsp1[i][0]=tsx*bsp1[i][0]/((double)k);
      bsp2[i][0]=tsy*bsp2[i][0]/((double)k);
      bsp3[i][0]=tsz*bsp3[i][0]/((double)k);
    }
    
  }
  
}

double spme_energy(PARAM *param,EWALD *ewald,PBC *box,const double x[],const double y[],const double z[],
	           double fx[],double fy[],double fz[],const double q[],double stress[6],double *virEwaldRec)
{
  
  int i,ii,j,jj,k,kk,l;
  int m1,m2,m3;
  int hm1max,hm2max,hm3max;
  
  cplx cam,etmp;
  
  double vam,qtmp,fact1;
  double tt,bm1,bm2,bm3;
  double rm,rrm,rmx,rmy,rmz;
  double rm1x,rm1y,rm1z,rm2x,rm2y,rm2z;
  double recCutOff,recCutOff2,rAlpha2,rVol;
  double eEwaldRec,eEwaldself,systq,eNonNeutral;
  double fxm,fym,fzm;
  double fbx,fby,fbz;
  
  if(newJob)
  {
    newJob=0;
    
    systq=0.;
    eEwaldself=0.;
    
    for(i=0;i<param->nAtom;i++)
    { 
      systq+=q[i];
      eEwaldself+=X2(q[i]);
    }
    
    eEwaldself=-param->chargeConst*ewald->alpha*eEwaldself/SQRTPI;
    
//     Calculate complex exponentials
    epl_cplx(ewald);
    
//     Calculate the coefficient of the B-spline
    bspcoef(ewald);
    
#ifdef FFTW
    
    fft3d1=fftw_plan_dft_3d(ewald->m1max,ewald->m2max,ewald->m3max,
			    ftqsp,ftqsp,FFTW_BACKWARD,FFTW_MEASURE);
    
    fft3d2=fftw_plan_dft_3d(ewald->m1max,ewald->m2max,ewald->m3max,
			    ftqsp,ftqsp,FFTW_FORWARD,FFTW_MEASURE);
#else
    
#endif
    
  } //   End if(newJob)
  
  hm1max=ewald->m1max/2;
  hm2max=ewald->m2max/2;
  hm3max=ewald->m3max/2;
  
  eEwaldRec=0.;
  *virEwaldRec=0.;
  
  stress[0]=0.;
  stress[1]=0.;
  stress[2]=0.;
  stress[3]=0.;
  stress[4]=0.;
  stress[5]=0.;
  
  rVol=TWOPI/box->vol;
  rAlpha2=-0.25/(X2(ewald->alpha));
  
  //   Set the cutoff in reciprocal space
  recCutOff=fmin( ( (double)ewald->m1max*box->u ) , ( (double)ewald->m2max*box->v ) );
  recCutOff=fmin( recCutOff , (double)ewald->m3max*box->w );
  recCutOff=recCutOff*1.05*TWOPI;
  recCutOff2=X2(recCutOff);
  
//   Address atoms to the cells of the mesh [0...mimax]
  for(i=0;i<param->nAtom;i++)
  {
    sx[i]=(double)ewald->m1max*(x[i]*box->u1+y[i]*box->u2+z[i]*box->u3+0.5);
    sy[i]=(double)ewald->m2max*(x[i]*box->v1+y[i]*box->v2+z[i]*box->v3+0.5);
    sz[i]=(double)ewald->m3max*(x[i]*box->w1+y[i]*box->w2+z[i]*box->w3+0.5);
  }
  
//   Construct the B-splines
  bspgen(param,ewald);
  
//   Initialise charge array Q(k1,k2,k3)
  for(i=0;i<ewald->mmax;i++)
    qsp[i]=0.0;
  
//   Fill charge array Q(k1,k2,k3)
  for(l=0;l<param->nAtom;l++)
  {
    for(i=0;i<ewald->nbsp;i++)
    {
      ii=(int)sx[l]-i;
      
      if(ii>=ewald->m1max)
	ii=0;
      
      if(ii<0)
	ii+=ewald->m1max;
      
      m1=ewald->m2max*ii;
      
      for(j=0;j<ewald->nbsp;j++)
      {
	jj=(int)sy[l]-j;
	
	if(jj>=ewald->m2max)
	  jj=0;
	
	if(jj<0)
	  jj+=ewald->m2max;
	
	m2=ewald->m3max*(jj+m1);
	
	for(k=0;k<ewald->nbsp;k++)
	{
	  kk=(int)sz[l]-k;
	  
	  if(kk>=ewald->m3max)
	    kk=0;
	  
	  if(kk<0)
	    kk+=ewald->m3max;
	  
	  m3=kk+m2;
	  qsp[m3]+=q[l]*bsp1[l][i]*bsp2[l][j]*bsp3[l][k];
	  
	}
      }
    }
  }
  
  for(m3=0;m3<ewald->mmax;m3++)
  {
    ftqsp[m3]=qsp[m3]+I*0.0;
  }
  
//   Perform the Fourier Transform of Q(k1,k2,k3)
#ifdef FFTW
  fftw_execute(fft3d1);
#else
  
#endif
    
  for(i=0;i<ewald->m1max;i++)
  {
    ii=i;
    
    if(i>hm1max)
      ii=i-ewald->m1max;
    
    m1=ewald->m2max*i;
    
    tt=TWOPI*(double)ii;
    
    rm1x=tt*box->u1;
    rm1y=tt*box->u2;
    rm1z=tt*box->u3;
    
    bm1=creal(bspc1[i]*conj(bspc1[i]));
    
    for(j=0;j<ewald->m2max;j++)
    {
      jj=j;
      
      if(j>hm2max)
	jj=j-ewald->m2max;
      
      m2=ewald->m3max*(j+m1);
      
      tt=TWOPI*(double)jj;
      
      rm2x=rm1x+(tt*box->v1);
      rm2y=rm1y+(tt*box->v2);
      rm2z=rm1z+(tt*box->v3);
      
      bm2=bm1*creal(bspc2[j]*conj(bspc2[j]));
      
      for(k=0;k<ewald->m3max;k++)
      {
	
	kk=k;
	
	if(k>hm3max)
	  kk=k-ewald->m3max;
	
	m3=k+m2;
	
	tt=TWOPI*(double)kk;
	
	rmx=rm2x+(tt*box->w1);
	rmy=rm2y+(tt*box->w2);
	rmz=rm2z+(tt*box->w3);
	
	bm3=bm2*creal(bspc3[k]*conj(bspc3[k]));
	
	rm=X2(rmx)+X2(rmy)+X2(rmz);
	
	if( (rm>DBL_EPSILON) && (rm<=recCutOff2 ) )
	{
	  rrm=1.0/rm;
	  
	  cam=bm3*exp(rAlpha2*rm)*rrm*ftqsp[m3];
	  
	  vam=2.0*(rrm-rAlpha2)*creal(cam*conj(ftqsp[m3]));
	  
	  stress[0]-=vam*rmx*rmx;
	  stress[1]-=vam*rmx*rmy;
	  stress[2]-=vam*rmx*rmz;
	  stress[3]-=vam*rmy*rmy;
	  stress[4]-=vam*rmy*rmz;
	  stress[5]-=vam*rmz*rmz;
	  
	  ftqsp[m3]=cam;
	  
	}
	else
	{
	  ftqsp[m3]=0.0+I*0.0;
	}
      }
    }
  }
  
/**   Beginning of the forces calculation section   */
  
#ifdef FFTW
  fftw_execute(fft3d2);
#else
  
#endif
  
  fact1=-2.0*rVol*param->chargeConst;
  
  for(l=0;l<param->nAtom;l++)
  { 
    for(i=0;i<ewald->nbsp;i++)
    {
      ii=(int)sx[l]-i;
      
      if(ii>=ewald->m1max)
	ii=0;
      
      if(ii<0)
	ii+=ewald->m1max;
      
      m1=ewald->m2max*ii;
      
      for(j=0;j<ewald->nbsp;j++)
      {
	jj=(int)sy[l]-j;
	
	if(jj>=ewald->m2max)
	  jj=0;
	
	if(jj<0)
	  jj+=ewald->m2max;
	
	m2=ewald->m3max*(jj+m1);
	
	for(k=0;k<ewald->nbsp;k++)
	{
	  kk=(int)sz[l]-k;
	  
	  if(kk>=ewald->m3max)
	    kk=0;
	  
	  if(kk<0)
	    kk+=ewald->m3max;
	  
	  m3=kk+m2;
	  
	  qtmp=creal(ftqsp[m3]);
	  
	  fbx=qtmp*bsd1[l][i]*bsp2[l][j]*bsp3[l][k]*(double)ewald->m1max;
	  fby=qtmp*bsp1[l][i]*bsd2[l][j]*bsp3[l][k]*(double)ewald->m2max;
	  fbz=qtmp*bsp1[l][i]*bsp2[l][j]*bsd3[l][k]*(double)ewald->m3max;
	  
	  fx[l]+=fact1*q[l]*(fbx*box->u1+fby*box->v1+fbz*box->w1);
	  fy[l]+=fact1*q[l]*(fbx*box->u2+fby*box->v2+fbz*box->w2);
	  fz[l]+=fact1*q[l]*(fbx*box->u3+fby*box->v3+fbz*box->w3);
	  
	}
      }
    }
  }
  
//   Set the sum of the forces to 0
  
  fxm=0.0;
  fym=0.0;
  fzm=0.0;
  
  for(l=0;l<param->nAtom;l++)
  {
    fxm+=fx[l];
    fym+=fy[l];
    fzm+=fz[l];
  }
  
  fxm/=(double)param->nAtom;
  fym/=(double)param->nAtom;
  fzm/=(double)param->nAtom;
  
  for(l=0;l<param->nAtom;l++)
  {
    fx[l]-=fxm;
    fy[l]-=fym;
    fz[l]-=fzm;
  }
  
/**   End of the forces calculation section   */
  
  etmp=0.0+I*0.0;
  for(m3=0;m3<ewald->mmax;m3++)
  {
    etmp+=ftqsp[m3]*qsp[m3];
  }
  
  eNonNeutral=-(0.5*PI*param->chargeConst)*(X2(systq/ewald->alpha)/box->vol);
  
  fact1=rVol*param->chargeConst;
  
  eEwaldRec=creal(etmp);
  
  stress[0]=fact1*(stress[0]+eEwaldRec)+eNonNeutral;
  stress[1]=fact1*stress[1];
  stress[2]=fact1*stress[2];
  stress[3]=fact1*(stress[3]+eEwaldRec)+eNonNeutral;
  stress[4]=fact1*stress[4];
  stress[5]=fact1*(stress[5]+eEwaldRec)+eNonNeutral;
  
  eEwaldRec=fact1*eEwaldRec+eEwaldself+eNonNeutral;
    
  *virEwaldRec=-(stress[0]+stress[3]+stress[5]);
  
  return eEwaldRec;
  
}

