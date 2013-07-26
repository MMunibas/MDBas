/*
 * Copyright (c) 2013 Pierre-Andre Cazade
 * Copyright (c) 2013 Florent hedin
 *
 * This file is part of MDBas.
 *
 * MDBas is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MDBas is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MDBas.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <math.h>

#include "global.h"
#include "utils.h"
#include "memory.h"

#ifdef USING_MPI
#include "parallel.h"
#else
#include "serial.h"
#endif

static double **cm1,**sm1,**cm2,**sm2,**cm3,**sm3;
static double *cm,*sm,*cms,*sms;

void init_ewald(CTRL *ctrl,PARAM *param,PARALLEL *parallel,EWALD *ewald,PBC *box)
{

    int i,m1,m2,m3,m2min,m3min;
    double rm,rmx,rmy,rmz;
    double rm1x,rm1y,rm1z,rm2x,rm2y,rm2z;
    double recCutOff,recCutOff2;

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

    recCutOff=fmin( ( (double)ewald->m1max*box->u ) , ( (double)ewald->m2max*box->v ) );
    recCutOff=fmin( recCutOff , (double)ewald->m3max*box->w );
    recCutOff=recCutOff*1.05*TWOPI;
    recCutOff2=X2(recCutOff);

    ewald->mmax=0;

    m2min=0;
    m3min=1;
    for(m1=0; m1<=ewald->m1max; m1++)
    {

        rm1x=TWOPI*box->u1*(double)m1;
        rm1y=TWOPI*box->u2*(double)m1;
        rm1z=TWOPI*box->u3*(double)m1;

        for(m2=m2min; m2<=ewald->m2max; m2++)
        {

            rm2x=rm1x+(TWOPI*box->v1*(double)m2);
            rm2y=rm1y+(TWOPI*box->v2*(double)m2);
            rm2z=rm1z+(TWOPI*box->v3*(double)m2);


            for(m3=m3min; m3<=ewald->m3max; m3++)
            {

                rmx=rm2x+(TWOPI*box->w1*(double)m3);
                rmy=rm2y+(TWOPI*box->w2*(double)m3);
                rmz=rm2z+(TWOPI*box->w3*(double)m3);

                rm=X2(rmx)+X2(rmy)+X2(rmz);

                if(rm<=recCutOff2)
                    ewald->mmax++;
            }
            m3min=-ewald->m3max;
        }
        m2min=-ewald->m2max;
    }

    cm=(double*)my_malloc(parallel->maxAtProc*sizeof(*cm));
    sm=(double*)my_malloc(parallel->maxAtProc*sizeof(*sm));

    cms=(double*)my_malloc(parallel->maxAtProc*sizeof(*cms));
    sms=(double*)my_malloc(parallel->maxAtProc*sizeof(*sms));

    cm1=(double**)my_malloc(ewald->mmax*sizeof(*cm1));
    cm2=(double**)my_malloc(ewald->mmax*sizeof(*cm2));
    cm3=(double**)my_malloc(ewald->mmax*sizeof(*cm3));

    sm1=(double**)my_malloc(ewald->mmax*sizeof(*sm1));
    sm2=(double**)my_malloc(ewald->mmax*sizeof(*sm2));
    sm3=(double**)my_malloc(ewald->mmax*sizeof(*sm3));

    for(i=0; i<ewald->mmax; i++)
    {
        cm1[i]=(double*)my_malloc(parallel->maxAtProc*sizeof(**cm1));
        cm2[i]=(double*)my_malloc(parallel->maxAtProc*sizeof(**cm2));
        cm3[i]=(double*)my_malloc(parallel->maxAtProc*sizeof(**cm3));

        sm1[i]=(double*)my_malloc(parallel->maxAtProc*sizeof(**sm1));
        sm2[i]=(double*)my_malloc(parallel->maxAtProc*sizeof(**sm2));
        sm3[i]=(double*)my_malloc(parallel->maxAtProc*sizeof(**sm3));
    }

}

void ewald_free(EWALD *ewald)
{
    int i;

    free(cm);
    free(sm);

    free(cms);
    free(sms);

    for(i=0; i<ewald->mmax; i++)
    {
        free(cm1[i]);
        free(cm2[i]);
        free(cm3[i]);

        free(sm1[i]);
        free(sm2[i]);
        free(sm3[i]);
    }

    free(cm1);
    free(cm2);
    free(cm3);

    free(sm1);
    free(sm2);
    free(sm3);
}

double ewald_rec(PARAM *param,PARALLEL *parallel,EWALD *ewald,PBC *box,const double x[],
                 const double y[],const double z[],double fx[],double fy[],double fz[],
                 const double q[],double stress[6],double *virEwaldRec,double dBuffer[])
{

    int i,l,m1,m2,m3,am2,am3,m2min,m3min;
    double sx,sy,sz,rm,rrm,rmx,rmy,rmz;
    double rm1x,rm1y,rm1z,rm2x,rm2y,rm2z;
    double recCutOff,recCutOff2,rAlpha2,rVol;
    double eEwaldRec,dEwaldRec,virtmp;
    double eEwaldself,systq,eNonNeutral;
    double cmss,smss,am,vam;
    double fact0,fact1;

    rVol=TWOPI/box->vol;
    rAlpha2=-0.25/(X2(ewald->alpha));

    recCutOff=fmin( ( (double)ewald->m1max*box->u ) , ( (double)ewald->m2max*box->v ) );
    recCutOff=fmin( recCutOff , (double)ewald->m3max*box->w );
    recCutOff=recCutOff*1.05*TWOPI;
    recCutOff2=X2(recCutOff);

    eEwaldRec=0.;
    *virEwaldRec=0.;
    eEwaldself=0.;
    eNonNeutral=0.;
    systq=0.;

    stress[0]=0.;
    stress[1]=0.;
    stress[2]=0.;
    stress[3]=0.;
    stress[4]=0.;
    stress[5]=0.;

    for(i=parallel->fAtProc; i<parallel->lAtProc; i++)
    {
        systq+=q[i];
        eEwaldself+=X2(q[i]);
    }

    if(parallel->nProc>1)
        sum_double_para(&systq,dBuffer,1);

    eEwaldself=-param->chargeConst*ewald->alpha*eEwaldself/SQRTPI;

    i=0;
    for(l=parallel->fAtProc; l<parallel->lAtProc; l++)
    {
        cm1[0][i]=1.0;
        cm2[0][i]=1.0;
        cm3[0][i]=1.0;

        sm1[0][i]=0.0;
        sm2[0][i]=0.0;
        sm3[0][i]=0.0;

        sx=TWOPI*(x[l]*box->u1+y[l]*box->u2+z[l]*box->u3);
        sy=TWOPI*(x[l]*box->v1+y[l]*box->v2+z[l]*box->v3);
        sz=TWOPI*(x[l]*box->w1+y[l]*box->w2+z[l]*box->w3);

        cm1[1][i]=cos(sx);
        cm2[1][i]=cos(sy);
        cm3[1][i]=cos(sz);

        sm1[1][i]=sin(sx);
        sm2[1][i]=sin(sy);
        sm3[1][i]=sin(sz);

        i++;
    }

    for(m1=2; m1<=ewald->m1max; m1++)
    {
        for(i=0; i<parallel->nAtProc; i++)
        {
            cm1[m1][i]=cm1[m1-1][i]*cm1[1][i]-sm1[m1-1][i]*sm1[1][i];
            sm1[m1][i]=sm1[m1-1][i]*cm1[1][i]+cm1[m1-1][i]*sm1[1][i];
        }
    }

    for(m2=2; m2<=ewald->m2max; m2++)
    {
        for(i=0; i<parallel->nAtProc; i++)
        {
            cm2[m2][i]=cm2[m2-1][i]*cm2[1][i]-sm2[m2-1][i]*sm2[1][i];
            sm2[m2][i]=sm2[m2-1][i]*cm2[1][i]+cm2[m2-1][i]*sm2[1][i];
        }
    }

    for(m3=2; m3<=ewald->m3max; m3++)
    {
        for(i=0; i<parallel->nAtProc; i++)
        {
            cm3[m3][i]=cm3[m3-1][i]*cm3[1][i]-sm3[m3-1][i]*sm3[1][i];
            sm3[m3][i]=sm3[m3-1][i]*cm3[1][i]+cm3[m3-1][i]*sm3[1][i];
        }
    }

    m2min=0;
    m3min=1;
    for(m1=0; m1<=ewald->m1max; m1++)
    {

        rm1x=TWOPI*box->u1*(double)m1;
        rm1y=TWOPI*box->u2*(double)m1;
        rm1z=TWOPI*box->u3*(double)m1;

        for(m2=m2min; m2<=ewald->m2max; m2++)
        {
            am2=abs(m2);

            rm2x=rm1x+(TWOPI*box->v1*(double)m2);
            rm2y=rm1y+(TWOPI*box->v2*(double)m2);
            rm2z=rm1z+(TWOPI*box->v3*(double)m2);

            if(m2>=0)
            {
                for(i=0; i<parallel->nAtProc; i++)
                {
                    cm[i]=cm1[m1][i]*cm2[am2][i]-sm1[m1][i]*sm2[am2][i];
                    sm[i]=sm1[m1][i]*cm2[am2][i]+cm1[m1][i]*sm2[am2][i];
                }
            }
            else
            {
                for(i=0; i<parallel->nAtProc; i++)
                {
                    cm[i]=cm1[m1][i]*cm2[am2][i]+sm1[m1][i]*sm2[am2][i];
                    sm[i]=sm1[m1][i]*cm2[am2][i]-cm1[m1][i]*sm2[am2][i];
                }
            }

            for(m3=m3min; m3<=ewald->m3max; m3++)
            {
                am3=abs(m3);

                rmx=rm2x+(TWOPI*box->w1*(double)m3);
                rmy=rm2y+(TWOPI*box->w2*(double)m3);
                rmz=rm2z+(TWOPI*box->w3*(double)m3);

                rm=X2(rmx)+X2(rmy)+X2(rmz);

                if(rm<=recCutOff2)
                {
                    i=0;

                    if(m3>=0)
                    {
                        for(l=parallel->fAtProc; l<parallel->lAtProc; l++)
                        {
                            cms[i]=q[l]*(cm[i]*cm3[am3][i]-sm[i]*sm3[am3][i]);
                            sms[i]=q[l]*(sm[i]*cm3[am3][i]+cm[i]*sm3[am3][i]);
                            i++;
                        }
                    }
                    else
                    {
                        for(l=parallel->fAtProc; l<parallel->lAtProc; l++)
                        {
                            cms[i]=q[l]*(cm[i]*cm3[am3][i]+sm[i]*sm3[am3][i]);
                            sms[i]=q[l]*(sm[i]*cm3[am3][i]-cm[i]*sm3[am3][i]);
                            i++;
                        }
                    }

                    cmss=0.0;
                    smss=0.0;

                    for(i=0; i<parallel->nAtProc; i++)
                    {
                        cmss+=cms[i];
                        smss+=sms[i];
                    }

                    if(parallel->nProc>1)
                    {
                        dBuffer[0]=cmss;
                        dBuffer[1]=smss;
                        sum_double_para(dBuffer,&(dBuffer[2]),2);
                        cmss=dBuffer[0];
                        smss=dBuffer[1];
                    }

                    rrm=1.0/rm;
                    am=exp(rAlpha2*rm)*rrm;
                    vam=2.0*am*(rrm-rAlpha2);

                    virtmp=X2(cmss)+X2(smss);
                    eEwaldRec+=am*virtmp;
                    virtmp*=vam;

                    stress[0]-=virtmp*rmx*rmx;
                    stress[1]-=virtmp*rmx*rmy;
                    stress[2]-=virtmp*rmx*rmz;
                    stress[3]-=virtmp*rmy*rmy;
                    stress[4]-=virtmp*rmy*rmz;
                    stress[5]-=virtmp*rmz*rmz;

                    i=0;
                    for(l=parallel->fAtProc; l<parallel->lAtProc; l++)
                    {
                        dEwaldRec=am*(sms[i]*cmss-cms[i]*smss);
                        fx[l]+=rmx*dEwaldRec;
                        fy[l]+=rmy*dEwaldRec;
                        fz[l]+=rmz*dEwaldRec;

                        i++;
                    }
                }
            }
            m3min=-ewald->m3max;
        }
        m2min=-ewald->m2max;
    }

    eEwaldRec/=(double)parallel->nProc;

    for(i=0; i<6; i++)
    {
        stress[i]/=(double)parallel->nProc;
    }

    eNonNeutral=-(0.5*PI*param->chargeConst)*(X2(systq/ewald->alpha)/box->vol)/(double)parallel->nProc;;

    double etmp;

    etmp=eEwaldRec;

    fact1=2.0*rVol*param->chargeConst;
    fact0=2.0*fact1;

    eEwaldRec=fact1*eEwaldRec+eEwaldself+eNonNeutral;

    for(i=parallel->fAtProc; i<parallel->lAtProc; i++)
    {
        fx[i]*=fact0;
        fy[i]*=fact0;
        fz[i]*=fact0;
    }

    stress[0]=fact1*(stress[0]+etmp)+eNonNeutral;
    stress[1]=fact1*stress[1];
    stress[2]=fact1*stress[2];
    stress[3]=fact1*(stress[3]+etmp)+eNonNeutral;
    stress[4]=fact1*stress[4];
    stress[5]=fact1*(stress[5]+etmp)+eNonNeutral;

    *virEwaldRec=-(stress[0]+stress[3]+stress[5]);

    return eEwaldRec;

}

double ewald_dir(EWALD *ewald,double *dEwaldDir,const double qel,
                 const double r,const double rt)
{

    double eEwaldDir,alphar;

    alphar=ewald->alpha*r;

    eEwaldDir=qel*erfc(alphar)*rt;
    *dEwaldDir=2.0*qel*ewald->alpha*exp(-X2(alphar))/SQRTPI;
    *dEwaldDir=-(eEwaldDir+*dEwaldDir)*rt;

    return eEwaldDir;
}

double ewald_corr(EWALD *ewald,double *dEwaldCorr,const double qel,
                  const double r,const double rt)
{

    double eEwaldCorr,alphar;

    alphar=ewald->alpha*r;

    eEwaldCorr=-qel*erf(alphar)*rt;
    *dEwaldCorr=2.0*qel*ewald->alpha*exp(-X2(alphar))/SQRTPI;
    *dEwaldCorr=-(eEwaldCorr+*dEwaldCorr)*rt;

    return eEwaldCorr;
}

double ewald_dir14(PARAM *param,EWALD *ewald,double *dEwaldDir,const double qel,
                   const double r,const double rt)
{

    double eEwaldDir,alphar;

    alphar=ewald->alpha*r;

    eEwaldDir=param->scal14*qel*erfc(alphar)*rt;
    *dEwaldDir=2.0*param->scal14*qel*ewald->alpha*exp(-X2(alphar))/SQRTPI;
    *dEwaldDir=-(eEwaldDir+*dEwaldDir)*rt;

    return eEwaldDir;
}

double ewald_corr14(PARAM *param,EWALD *ewald,double *dEwaldCorr,const double qel,
                    const double r,const double rt)
{

    double eEwaldCorr,alphar;

    alphar=ewald->alpha*r;

    eEwaldCorr=(param->scal14-1.0)*qel*erf(alphar)*rt;
    *dEwaldCorr=2.0*(param->scal14-1.0)*qel*ewald->alpha*exp(-X2(alphar))/SQRTPI;
    *dEwaldCorr=(*dEwaldCorr-eEwaldCorr)*rt;

    return eEwaldCorr;
}
