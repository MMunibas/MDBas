#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "global.h"
#include "utils.h"
#include "shake.h"
#include "integrate.h"

void lf_integrate(ATOM *atom, ENERGY *ener, SIMULPARAMS *simulCond,CONSTRAINT *constList,PBC *box)
{
  switch (simulCond->ens)
  {
    case 0:
      lf_nve(atom,ener,simulCond,constList,box);
      break;
    case 1:
      lf_nvt_b(atom,ener,simulCond,constList,box);
      break;
    default:
      lf_nve(atom,ener,simulCond,constList,box);
      break;
  }
}

void lf_nve(ATOM *atom, ENERGY *ener, SIMULPARAMS *simulCond,CONSTRAINT *constList,PBC *box)
{
  int i,ia,ib;
  double *xo=NULL,*yo=NULL,*zo=NULL,*vxu=NULL,*vyu=NULL,*vzu=NULL;
  DELTA *dd=NULL;
  
  vxu=(double*)malloc(simulCond->natom*sizeof(*vxu));
  vyu=(double*)malloc(simulCond->natom*sizeof(*vyu));
  vzu=(double*)malloc(simulCond->natom*sizeof(*vzu));
  
  if(simulCond->nconst>0)
  {
    dd=(DELTA*)malloc(simulCond->nconst*sizeof(*dd));
    
    for(i=0;i<simulCond->nconst;i++)
    {
      ia=constList[i].a;
      ib=constList[i].b;
      
      dd[i].x=atom[ib].x-atom[ia].x;
      dd[i].y=atom[ib].y-atom[ia].y;
      dd[i].z=atom[ib].z-atom[ia].z;
    }
    
    image_array(simulCond->nconst,dd,simulCond,box);
    
    xo=(double*)malloc(simulCond->natom*sizeof(*xo));
    yo=(double*)malloc(simulCond->natom*sizeof(*yo));
    zo=(double*)malloc(simulCond->natom*sizeof(*zo));
    
    for(i=0;i<simulCond->natom;i++)
    {
      
// Store old coordinates.

      xo[i]=atom[i].x;
      yo[i]=atom[i].y;
      zo[i]=atom[i].z;
      
    }
    
  }

// move atoms by leapfrog algorithm
  
  for(i=0;i<simulCond->natom;i++)
  {
    
// update velocities
    
    vxu[i]=atom[i].vx+simulCond->timeStep*atom[i].fx/atom[i].m;
    vyu[i]=atom[i].vy+simulCond->timeStep*atom[i].fy/atom[i].m;
    vzu[i]=atom[i].vz+simulCond->timeStep*atom[i].fz/atom[i].m;
    
// update positions
    
    atom[i].x+=simulCond->timeStep*vxu[i];
    atom[i].y+=simulCond->timeStep*vyu[i];
    atom[i].z+=simulCond->timeStep*vzu[i];
    
  }
  
  if(simulCond->nconst>0)
  {
// Apply constraint with Shake algorithm.

    lf_shake(atom,simulCond,constList,dd,box);
    for(i=0;i<simulCond->natom;i++)
    {
        
// Corrected velocities
    
      vxu[i]=(atom[i].x-xo[i])/simulCond->timeStep;
      vyu[i]=(atom[i].y-yo[i])/simulCond->timeStep;
      vzu[i]=(atom[i].z-zo[i])/simulCond->timeStep;
    
// Corrected Forces
    
      atom[i].fx=(vxu[i]-atom[i].vx)*atom[i].m/simulCond->timeStep;
      atom[i].fy=(vyu[i]-atom[i].vy)*atom[i].m/simulCond->timeStep;
      atom[i].fz=(vzu[i]-atom[i].vz)*atom[i].m/simulCond->timeStep;
    
    }
  }
  
// calculate full timestep velocity

  for(i=0;i<simulCond->natom;i++)
  {
    
    atom[i].vx=0.5*(atom[i].vx+vxu[i]);
    atom[i].vy=0.5*(atom[i].vy+vyu[i]);
    atom[i].vz=0.5*(atom[i].vz+vzu[i]);
    
  }
  
// calculate kinetic energy
  
  ener->kin=kinetic(atom,simulCond);
  
// periodic boundary condition
  
  image_update(atom,simulCond,box);
  
// updated velocity
  
  for(i=0;i<simulCond->natom;i++)
  {
    
    atom[i].vx=vxu[i];
    atom[i].vy=vyu[i];
    atom[i].vz=vzu[i];
    
  }
  
  free(vxu);
  free(vyu);
  free(vzu);
  
  if(simulCond->nconst>0)
  {
    free(xo);
    free(yo);
    free(zo);
    free(dd);
  }
      
}

void lf_nvt_b(ATOM *atom, ENERGY *ener, SIMULPARAMS *simulCond,CONSTRAINT *constList,PBC *box)
{
  int i,k,ia,ib,bercycle;
  double lambda,ts2;
  double *xo=NULL,*yo=NULL,*zo=NULL;
  double *vxo=NULL,*vyo=NULL,*vzo=NULL;
  double *xt=NULL,*yt=NULL,*zt=NULL;
  double *vxu=NULL,*vyu=NULL,*vzu=NULL;
  DELTA *dd=NULL;
  
  vxu=(double*)malloc(simulCond->natom*sizeof(*vxu));
  vyu=(double*)malloc(simulCond->natom*sizeof(*vyu));
  vzu=(double*)malloc(simulCond->natom*sizeof(*vzu));
  
  xo=(double*)malloc(simulCond->natom*sizeof(*xo));
  yo=(double*)malloc(simulCond->natom*sizeof(*yo));
  zo=(double*)malloc(simulCond->natom*sizeof(*zo));
  
  vxo=(double*)malloc(simulCond->natom*sizeof(*vxo));
  vyo=(double*)malloc(simulCond->natom*sizeof(*vyo));
  vzo=(double*)malloc(simulCond->natom*sizeof(*vzo));
    
  for(i=0;i<simulCond->natom;i++)
  {
    
// Store old coordinates and old velocities.

    xo[i]=atom[i].x;
    yo[i]=atom[i].y;
    zo[i]=atom[i].z;
    
    vxo[i]=atom[i].vx;
    vyo[i]=atom[i].vy;
    vzo[i]=atom[i].vz;
    
  }
  
  if(simulCond->nconst>0)
  {
    dd=(DELTA*)malloc(simulCond->nconst*sizeof(*dd));
    
    for(i=0;i<simulCond->nconst;i++)
    {
      ia=constList[i].a;
      ib=constList[i].b;
      
      dd[i].x=atom[ib].x-atom[ia].x;
      dd[i].y=atom[ib].y-atom[ia].y;
      dd[i].z=atom[ib].z-atom[ia].z;
    }
    
    image_array(simulCond->nconst,dd,simulCond,box);
    
    xt=(double*)malloc(simulCond->natom*sizeof(*xt));
    yt=(double*)malloc(simulCond->natom*sizeof(*yt));
    zt=(double*)malloc(simulCond->natom*sizeof(*zt));
    
  }
  
  ts2=X2(simulCond->timeStep);
  
  for(i=0;i<simulCond->natom;i++)
  { 
    atom[i].vx+=0.5*simulCond->timeStep*atom[i].fx/atom[i].m;
    atom[i].vy+=0.5*simulCond->timeStep*atom[i].fy/atom[i].m;
    atom[i].vz+=0.5*simulCond->timeStep*atom[i].fz/atom[i].m;
  }
  
  ener->kin=kinetic(atom,simulCond);
  
  if(simulCond->nconst>0)
    bercycle=2;
  else
    bercycle=3;
  
  for(k=0;k<bercycle;k++)
  {
   
    lambda=sqrt(1.0+simulCond->timeStep/simulCond->taut*(simulCond->kintemp0/ener->kin-1.0));
    
// move atoms by leapfrog algorithm
    
    for(i=0;i<simulCond->natom;i++)
    {
      
// update velocities
      
      vxu[i]=(vxo[i]+simulCond->timeStep*atom[i].fx/atom[i].m)*lambda;
      vyu[i]=(vyo[i]+simulCond->timeStep*atom[i].fy/atom[i].m)*lambda;
      vzu[i]=(vzo[i]+simulCond->timeStep*atom[i].fz/atom[i].m)*lambda;
      
// update positions
      
      atom[i].x=xo[i]+simulCond->timeStep*vxu[i];
      atom[i].y=yo[i]+simulCond->timeStep*vyu[i];
      atom[i].z=zo[i]+simulCond->timeStep*vzu[i];
      
// Temporary storage of the uncorrected positions
      
      if(simulCond->nconst>0)
      {
	xt[i]=atom[i].x;
	yt[i]=atom[i].y;
	zt[i]=atom[i].z;
      }
      
    }
    
    if(simulCond->nconst>0)
    {
// Apply constraint with Shake algorithm.
      
      lf_shake(atom,simulCond,constList,dd,box);
      for(i=0;i<simulCond->natom;i++)
      {
        
// Corrected velocities
      
	vxu[i]+=(atom[i].x-xt[i])/simulCond->timeStep;
	vyu[i]+=(atom[i].y-yt[i])/simulCond->timeStep;
	vzu[i]+=(atom[i].z-zt[i])/simulCond->timeStep;
      
// Corrected Forces
      
	atom[i].fx+=(atom[i].x-xt[i])*atom[i].m/ts2;
	atom[i].fy+=(atom[i].y-yt[i])*atom[i].m/ts2;
	atom[i].fz+=(atom[i].z-zt[i])*atom[i].m/ts2;
      
      }
    }
    
// calculate full timestep velocity

    for(i=0;i<simulCond->natom;i++)
    {
      
      atom[i].vx=0.5*(vxo[i]+vxu[i]);
      atom[i].vy=0.5*(vyo[i]+vyu[i]);
      atom[i].vz=0.5*(vzo[i]+vzu[i]);
      
    }
    
// calculate kinetic energy
    
    ener->kin=kinetic(atom,simulCond);
    
  }
  
// periodic boundary condition
  
  image_update(atom,simulCond,box);
  
// updated velocity
  
  for(i=0;i<simulCond->natom;i++)
  {
    
    atom[i].vx=vxu[i];
    atom[i].vy=vyu[i];
    atom[i].vz=vzu[i];
    
  }
  
// Free temporary arrays
  
  free(vxu);
  free(vyu);
  free(vzu);
  
  free(xo);
  free(yo);
  free(zo);
  
  free(vxo);
  free(vyo);
  free(vzo);
  
  if(simulCond->nconst>0)
  {
    free(dd);
    
    free(xt);
    free(yt);
    free(zt);
  }
      
}


void vv_integrate(ATOM *atom, ENERGY *ener, SIMULPARAMS *simulCond,CONSTRAINT *constList,PBC *box,int stage)
{
  switch (simulCond->ens)
  {
    case 0:
      vv_nve(atom,ener,simulCond,constList,box,stage);
      break;
    case 1:
      vv_nvt_b(atom,ener,simulCond,constList,box,stage);
      break;
    default:
      vv_nve(atom,ener,simulCond,constList,box,stage);
      break;
  }
}

void vv_nve(ATOM *atom, ENERGY *ener, SIMULPARAMS *simulCond,CONSTRAINT *constList,PBC *box,int stage)
{
  int i,ia,ib;
  DELTA *dd=NULL;
  
  if(simulCond->nconst>0)
  {
    dd=(DELTA*)malloc(simulCond->nconst*sizeof(*dd));
    
    for(i=0;i<simulCond->nconst;i++)
    {
      ia=constList[i].a;
      ib=constList[i].b;
      
      dd[i].x=atom[ib].x-atom[ia].x;
      dd[i].y=atom[ib].y-atom[ia].y;
      dd[i].z=atom[ib].z-atom[ia].z;
    }
    
    image_array(simulCond->nconst,dd,simulCond,box);
    
  }

// move atoms by leapfrog algorithm
  
  for(i=0;i<simulCond->natom;i++)
  {
// update velocities
    
    atom[i].vx+=0.5*simulCond->timeStep*atom[i].fx/atom[i].m;
    atom[i].vy+=0.5*simulCond->timeStep*atom[i].fy/atom[i].m;
    atom[i].vz+=0.5*simulCond->timeStep*atom[i].fz/atom[i].m;
  }
  
  if(stage==1)
  {
    for(i=0;i<simulCond->natom;i++)
    {
// update positions
      
      atom[i].x+=simulCond->timeStep*atom[i].vx;
      atom[i].y+=simulCond->timeStep*atom[i].vy;
      atom[i].z+=simulCond->timeStep*atom[i].vz;
      
    }
    
    if(simulCond->nconst>0)
    {
      
// Apply constraint with Shake algorithm.

      vv_shake_r(atom,simulCond,constList,dd,box);
      
    }
    
  }
  else
  {
// calculate kinetic energy

    if(simulCond->nconst>0)
    {
      
// Apply constraint with Shake algorithm.

      vv_shake_v(atom,simulCond,constList,dd);
      
    }
  
    ener->kin=kinetic(atom,simulCond);
  }
  
  if(stage==2)
  {
    
// periodic boundary condition
    
    image_update(atom,simulCond,box);
  }
  
  if(simulCond->nconst>0)
  {
    free(dd);
  }
   
}

void vv_nvt_b(ATOM *atom, ENERGY *ener, SIMULPARAMS *simulCond,CONSTRAINT *constList,PBC *box,int stage)
{
  int i,ia,ib;
  double lambda;
  DELTA *dd=NULL;
  
  if(simulCond->nconst>0)
  {
    dd=(DELTA*)malloc(simulCond->nconst*sizeof(*dd));
    
    for(i=0;i<simulCond->nconst;i++)
    {
      ia=constList[i].a;
      ib=constList[i].b;
      
      dd[i].x=atom[ib].x-atom[ia].x;
      dd[i].y=atom[ib].y-atom[ia].y;
      dd[i].z=atom[ib].z-atom[ia].z;
    }
    
    image_array(simulCond->nconst,dd,simulCond,box);
    
  }

// move atoms by leapfrog algorithm
  
  for(i=0;i<simulCond->natom;i++)
  {
// update velocities
    
    atom[i].vx+=0.5*simulCond->timeStep*atom[i].fx/atom[i].m;
    atom[i].vy+=0.5*simulCond->timeStep*atom[i].fy/atom[i].m;
    atom[i].vz+=0.5*simulCond->timeStep*atom[i].fz/atom[i].m;
  }
  
  if(stage==1)
  {
    for(i=0;i<simulCond->natom;i++)
    {
// update positions
      
      atom[i].x+=simulCond->timeStep*atom[i].vx;
      atom[i].y+=simulCond->timeStep*atom[i].vy;
      atom[i].z+=simulCond->timeStep*atom[i].vz;
      
    }
    
    if(simulCond->nconst>0)
    {
      
// Apply constraint with Shake algorithm.

      vv_shake_r(atom,simulCond,constList,dd,box);
      
    }
    
  }
  else
  {
// calculate kinetic energy

    if(simulCond->nconst>0)
    {
      
// Apply constraint with Shake algorithm.

      vv_shake_v(atom,simulCond,constList,dd);
      
    }
  
    ener->kin=kinetic(atom,simulCond);
    
    lambda=sqrt(1.0+simulCond->timeStep/simulCond->taut*(simulCond->kintemp0/ener->kin-1.0));
    
    for(i=0;i<simulCond->natom;i++)
    {
      atom[i].vx*=lambda;
      atom[i].vy*=lambda;
      atom[i].vz*=lambda;
    }
    
    ener->kin*=X2(lambda);
    
  }
  
  if(stage==2)
  {
    
// periodic boundary condition
    
    image_update(atom,simulCond,box);
  }
  
  if(simulCond->nconst>0)
  {
    free(dd);
  }
   
}
