#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include <time.h>

#include "global.h"
#include "init.h"
#include "io.h"
#include "list.h"
#include "energy.h"
#include "ewald.h"
#include "spme.h"
#include "rand.h"
#include "memory.h"
#include "utils.h"
#include "errors.h"
#include "integrate.h"
#include "shake.h"

#if (defined TIMING && defined __unix__ && !defined __STRICT_ANSI__)
#define TIMER
#include "timing.h"
#endif

/** Pointer to the output file. **/
extern FILE *outFile;

void init_system(int argc, char* argv[],IO *inout,CTRL *ctrl,PARAM *param,ENERGY *ener,BATH *bath,
		 NEIGH *neigh,EWALD *ewald,PBC *box,ATOM **atom,CONSTRAINT **constList,
		 BOND **bond,ANGLE **angle,DIHE **dihe,DIHE **impr,BOND **ub,double **x,
		 double **y, double **z,double **vx,double **vy,double **vz,double **fx,
		 double **fy, double **fz,double **mass,double **rmass,double **q,
		 double **eps,double **sig,double **eps14,double **sig14,int **frozen,
		 int **nAtConst,int **neighList,int **neighPair,int **neighOrder,
		 int **neighList14,int ***exclList,int **exclPair)
{
  
  char outName[FINAMELEN];
  int i;
  
  /** Initialization of the simulation starts here. */
  
  init_rand(time(NULL));
  
  outFile=NULL;
  
  strcpy(outName,"OUTPUT");
  
  strcpy(inout->simuName,"SIMU");
  
  i=1;
  while(i<argc)
  {
    
    if(!strcmp(argv[i],"-i"))
    {
      strcpy(inout->simuName,argv[++i]);
    }
    else if (!strcmp(argv[i],"-o"))
    {
      strcpy(outName,argv[++i]);
    }
    else if (!strcmp(argv[i],"--help"))
    {
      printf("%s [-i input_file] [-o output_file] [--help]\n",argv[0]);
      exit(0);
    }
    else
      my_error(0,__FILE__,__LINE__,0);
    
    i++;
  }
    
  outFile=fopen(outName,"w");
  if(outFile==NULL)
  {
    outFile=stdout;
    my_error(1,__FILE__,__LINE__,0);
  }
  
  read_SIMU(inout,ctrl,param,bath,neigh,ewald,box);
  fprintf(outFile,"%s file read\n",inout->simuName);
  
  #ifdef TIMER
  /** create timers **/
  create_new_timer(TIMER_ENERGY_TOT);
  create_new_timer(TIMER_ENERGY_NB);
  create_new_timer(TIMER_ENERGY_NB14);
  create_new_timer(TIMER_ENERGY_BOND);
  create_new_timer(TIMER_ENERGY_ANGL);
  create_new_timer(TIMER_ENERGY_UB);
  create_new_timer(TIMER_ENERGY_DIHE);
  create_new_timer(TIMER_SHAKE);
  create_new_timer(TIMER_INTEGRATE);
#endif
  
  if(ctrl->keyRand)
    init_rand(ctrl->seed);
  
  if(ctrl->mdType==1)
    param->chargeConst=chgcharmm*kcaltoiu;
  else if(ctrl->mdType==2)
    param->chargeConst=chgnamd*kcaltoiu;
  else if(ctrl->mdType==3)
    param->chargeConst=chgdlpolyiu;
  else
    param->chargeConst=mu0*X2(clight)*X2(elemchg)*NA*0.1/(angstr);
  
  if(ctrl->keyRest)
  {
    read_rest(inout,param,ener,bath,atom,x,y,z,vx,vy,vz,fx,fy,fz);
    fprintf(outFile,"%s file read\n",inout->restName);
    
    *q=(double*)my_malloc(param->nAtom*sizeof(double));
    
    *mass=(double*)my_malloc(param->nAtom*sizeof(double));
    *rmass=(double*)my_malloc(param->nAtom*sizeof(double));
    
    *frozen=(int*)my_malloc(param->nAtom*sizeof(int));
    
    *nAtConst=(int*)my_malloc(param->nAtom*sizeof(int));
    
    *eps=(double*)my_malloc(param->nAtom*sizeof(double));
    *sig=(double*)my_malloc(param->nAtom*sizeof(double));
    *eps14=(double*)my_malloc(param->nAtom*sizeof(double));
    *sig14=(double*)my_malloc(param->nAtom*sizeof(double));
    
  }
  else
  {
    read_CONF(inout,param,atom,x,y,z);
    fprintf(outFile,"%s file read\n",inout->confName);
    
    *vx=(double*)my_malloc(param->nAtom*sizeof(double));
    *vy=(double*)my_malloc(param->nAtom*sizeof(double));
    *vz=(double*)my_malloc(param->nAtom*sizeof(double));
    
    *fx=(double*)my_malloc(param->nAtom*sizeof(double));
    *fy=(double*)my_malloc(param->nAtom*sizeof(double));
    *fz=(double*)my_malloc(param->nAtom*sizeof(double));
    
    *q=(double*)my_malloc(param->nAtom*sizeof(double));
    
    *mass=(double*)my_malloc(param->nAtom*sizeof(double));
    *rmass=(double*)my_malloc(param->nAtom*sizeof(double));
    
    *frozen=(int*)my_malloc(param->nAtom*sizeof(int));
    
    *nAtConst=(int*)my_malloc(param->nAtom*sizeof(int));
    
    *eps=(double*)my_malloc(param->nAtom*sizeof(double));
    *sig=(double*)my_malloc(param->nAtom*sizeof(double));
    *eps14=(double*)my_malloc(param->nAtom*sizeof(double));
    *sig14=(double*)my_malloc(param->nAtom*sizeof(double));
    
  }
  
  read_FORF(inout,param,*atom,constList,bond,angle,dihe,impr,ub,*eps,*sig,
	    *eps14,*sig14,*mass,*q,*frozen,*nAtConst);
  
  fprintf(outFile,"%s file read\n",inout->forfName);
  
  setup(ctrl,param,*atom,constList,bond,angle,dihe,impr,ub,*mass,*rmass,*frozen,*nAtConst);
  
  fprintf(outFile,"Setup done\n");
  
  get_kinfromtemp(param,box);
  
  init_box(box);
  
  image_update(param,box,*x,*y,*z);

  makelist(ctrl,param,box,neigh,*constList,*bond,*angle,*dihe,*impr,*x,*y,*z,*frozen,
	   neighList,neighPair,neighOrder,neighList14,exclList,exclPair);
  
  init_energy_ptrs(ctrl);
  
  if(ctrl->keyEwald==1)
  {
    init_ewald(ctrl,param,ewald,box);
    fprintf(outFile,"\n");
    fprintf(outFile,"Ewald Sum requested for coulombic interaction.\n\n");
    fprintf(outFile,"Ewald percision: %e\n",ewald->prec);
    fprintf(outFile,"Ewald gaussian width: %lf\n",ewald->alpha);
    fprintf(outFile,"Ewald wavevectors: %d %d %d\n",ewald->m1max,ewald->m2max,ewald->m3max);
    fprintf(outFile,"Ewald maximum wavevectors: %d\n\n",ewald->mmax);
  }
  else if(ctrl->keyEwald==2)
  {
    init_spme(ctrl,param,ewald,box);
    fprintf(outFile,"\n");
    fprintf(outFile,"Ewald Sum requested for coulombic interaction.\n\n");
    fprintf(outFile,"The Smooth Particle-Mesh Ewald is used to calculate\n");
    fprintf(outFile,"the reciprocal space contibution.\n\n");
    fprintf(outFile,"B-splines order: %d\n",ewald->nbsp);
    fprintf(outFile,"Ewald percision: %e\n",ewald->prec);
    fprintf(outFile,"Ewald gaussian width: %lf\n",ewald->alpha);
    fprintf(outFile,"Ewald wavevectors: %d %d %d\n",ewald->m1max,ewald->m2max,ewald->m3max);
    fprintf(outFile,"Ewald maximum wavevectors: %d\n\n",ewald->mmax);
  }
  
  /** Initialization of the simulation ends here. */
  
  /** Initialization of velocities starts here. */
  
  if(!ctrl->keyRest)
  {
    
    remove(inout->propName);
    
    init_vel(param,box,*constList,*x,*y,*z,*vx,*vy,*vz,*mass,*rmass,*frozen,*nAtConst);
    
    bath->chiT=0.;
    bath->chiP=0.;
    ener->conint=0.;
    ener->consv=0.;
    
    ener->virshake=0.;
  }
  
  /** Initialization of velocities ends here. */
  
  box->stress1=0.;
  box->stress2=0.;
  box->stress3=0.;
  box->stress4=0.;
  box->stress5=0.;
  box->stress6=0.;
  box->stress7=0.;
  box->stress8=0.;
  box->stress9=0.;
  
  /** Prepares DCD header if needed */
  
  if(ctrl->keyTraj)
  {
    write_DCD_header(inout,ctrl,param,box,*frozen);
  }
  
  /** allocate arrays for integrators and for shake **/
  integrators_allocate_arrays(ctrl,param);
  if(param->nConst>0) shake_allocate_arrays(param);
    
}

void setup(CTRL *ctrl,PARAM *param,ATOM atom[],CONSTRAINT **constList,
	   BOND **bond,ANGLE **angle,DIHE **dihe,DIHE **impr,BOND **ub,
	   double mass[],double rmass[],int frozen[],int nAtConst[])
{
  int i,j,k,ia,ib,ic,id,iend;
  int constBond1,constBond2,constBond3;
  
  param->nFrozen=0;
  for(i=0;i<param->nAtom;i++)
  {
    param->nFrozen+=frozen[i];
    
    if( (frozen[i]) || (mass[i]<DBL_EPSILON) )
    {
      mass[i]=0.;
      rmass[i]=0.;
    }
    else
    {
      rmass[i]=1.0/mass[i];
    }
  }
  
  CONSTRAINT *buffer1;
 
  if(param->nConst>0)
  {
    buffer1=(CONSTRAINT*)my_malloc(param->nConst*sizeof(CONSTRAINT));
    
    memcpy(buffer1,*constList,param->nConst*sizeof(CONSTRAINT));

    k=0;
    iend=param->nConst;
    for(i=0;i<iend;i++)
    {
      ia=buffer1[i].a;
      ib=buffer1[i].b;
      
      if( !(frozen[ia]*frozen[ib]) )
      {
	memcpy(&(*constList)[k],&buffer1[i],sizeof(CONSTRAINT));
	nAtConst[ia]--;
	nAtConst[ib]--;
	k++;
      }
      else
	param->nConst--;
      
    }

    if(k!=param->nConst)
      my_error(1000,__FILE__,__LINE__,0);

    free(buffer1);
  }
  
  if(ctrl->keyConstH)
    *constList=(CONSTRAINT*)my_realloc(*constList,(param->nConst+param->nBond)*sizeof(CONSTRAINT));
  
  BOND *buffer2;
  
  if(param->nBond>0)
  {
    buffer2=(BOND*)my_malloc(param->nBond*sizeof(BOND));
    
    memcpy(buffer2,*bond,param->nBond*sizeof(BOND));
    
    k=0;
    iend=param->nBond;
    for(i=0;i<iend;i++)
    {
      ia=buffer2[i].a;
      ib=buffer2[i].b;
	    
      if(ctrl->keyConstH) /** Constraint on bonds involving hydrogens. */
      {
	if( atom[ia].label[0]=='H' || atom[ib].label[0]=='H' ) /** Test on the nature of the atoms */
	{
	  if( (frozen[ia]*frozen[ib]) ) /** Test if both atoms are Frozen. */
	  {
	    param->nBond--;
	  }
	  else /** Test if both atoms are Frozen. */
	  {
	    (*constList)[param->nConst].a=ia;
	    (*constList)[param->nConst].b=ib;
	    (*constList)[param->nConst].rc2=X2(buffer2[i].r0);
	    
	    nAtConst[ia]++;
	    nAtConst[ib]++;
	    
	    param->nConst++;
	    param->nBond--;
	    
	  } /** Test if both atoms are Frozen. */
	  
	}
	else /** Test on the nature of the atoms */
	{
	  
	  if( (frozen[ia]*frozen[ib]) ) /** Test if both atoms are Frozen. */
	  {
	    param->nBond--;
	  }
	  else /** Test if both atoms are Frozen. */
	  {
	    
	    memcpy(&(*bond)[k],&buffer2[i],sizeof(BOND));
	    k++;
	    
	  } /** Test if both atoms are Frozen. */
	  
	} /** Test on the nature of the atoms */
	  
      }
      else /** Constraint on bonds involving hydrogens. */
      {
	
	if( (frozen[ia]*frozen[ib]) ) /** Test if both atoms are Frozen. */
	{	
	    param->nBond--;
	}
	else /** Test if both atoms are Frozen. */
	{
	  
	  memcpy(&(*bond)[k],&buffer2[i],sizeof(BOND));
	  k++;
	  
	} /** Test if both atoms are Frozen. */
	
      }/** Constraint on bonds involving hydrogens. */
      
    }
    
    if(k!=param->nBond)
      my_error(1000,__FILE__,__LINE__,0);
    
    free(buffer2);
  }
  
  if(param->nUb>0)
  {
    buffer2=(BOND*)my_malloc(param->nUb*sizeof(BOND));
    
    memcpy(buffer2,*ub,param->nUb*sizeof(BOND));
    
    k=0;
    iend=param->nUb;
    for(i=0;i<iend;i++)
    {
      
      ia=buffer2[i].a;
      ib=buffer2[i].b;
      
      if( (frozen[ia]*frozen[ib]) ) /** Test if both atoms are Frozen. */
      { 
	param->nUb--;
      }
      else /** Test if both atoms are Frozen. */
      {
	
	if(ctrl->keyConstH) /** Constraint on bonds involving hydrogens. */
	{
	  
	  constBond1=0;
	  
	  for(j=0;j<param->nConst;j++)
	  {
	    constBond1= ( ( (*constList)[j].a==ia && (*constList)[j].b==ib ) || ( (*constList)[j].a==ib && (*constList)[j].b==ia ) );
	    if(constBond1)
	      break;
	  }
	  
	  if(constBond1)
	  {
	    param->nUb--;
	  }
	  else
	  {
	    memcpy(&(*ub)[k],&buffer2[i],sizeof(BOND));
	    k++;
	  }
	  
	}
	else
	{
	  memcpy(&(*ub)[k],&buffer2[i],sizeof(BOND));
	  k++;
	}
	
      } /** Test if both atoms are Frozen. */
      
    }
    
    if(k!=param->nUb)
      my_error(1000,__FILE__,__LINE__,0);
    
    free(buffer2);
  }
  
  ANGLE *buffer3;
  
  if(param->nAngle>0)
  {
    buffer3=(ANGLE*)my_malloc(param->nAngle*sizeof(ANGLE));
    
    memcpy(buffer3,*angle,param->nAngle*sizeof(ANGLE));
    
    k=0;
    iend=param->nAngle;
    for(i=0;i<iend;i++)
    {
      
      ia=buffer3[i].a;
      ib=buffer3[i].b;
      ic=buffer3[i].c;
	    
      if( (frozen[ia]*frozen[ib]*frozen[ic]) ) /** Test if the three atoms are Frozen. */
      { 
	param->nAngle--;
      }
      else /** Test if the three atoms are Frozen. */
      {
	
	if(ctrl->keyConstH)
	{
	  
	  constBond1=0;
	  constBond2=0;
	  constBond3=0;
	  
	  for(j=0;j<param->nConst;j++)
	  {
	    if(!constBond1)
	      constBond1= ( ( (*constList)[j].a==ia && (*constList)[j].b==ib ) || ( (*constList)[j].a==ib && (*constList)[j].b==ia ) );
	    
	    if(!constBond2)
	      constBond2= ( ( (*constList)[j].a==ia && (*constList)[j].b==ic ) || ( (*constList)[j].a==ic && (*constList)[j].b==ia ) );
	    
	    if(!constBond3)
	      constBond3= ( ( (*constList)[j].a==ic && (*constList)[j].b==ib ) || ( (*constList)[j].a==ib && (*constList)[j].b==ic ) );
	  }
	  
	  if(constBond1*constBond2*constBond3)
	  {
	    param->nAngle--;
	  }
	  else
	  {
	    memcpy(&(*angle)[k],&buffer3[i],sizeof(ANGLE));
	    k++;
	  }
	  
	}
	else
	{
	  memcpy(&(*angle)[k],&buffer3[i],sizeof(ANGLE));
	  k++;
	  
        }
      
      } /** Test if the three atoms are Frozen. */
      
    }
    
    if(k!=param->nAngle)
      my_error(1000,__FILE__,__LINE__,0);
    
    free(buffer3);
  }
  
  DIHE *buffer4;
  
  if(param->nDihedral>0)
  {
    buffer4=(DIHE*)my_malloc(param->nDihedral*sizeof(DIHE));
    
    memcpy(buffer4,*dihe,param->nDihedral*sizeof(DIHE));
    
    k=0;
    iend=param->nDihedral;
    for(i=0;i<iend;i++)
    {
      
      ia=buffer4[i].a;
      ib=buffer4[i].b;
      ic=buffer4[i].c;
      id=buffer4[i].d;
      
      if( (frozen[ia]*frozen[ib]*frozen[ic]*frozen[id]) ) /** Test if the four atoms are Frozen. */
      {
	param->nDihedral--;
      }
      else /** Test if the four atoms are Frozen. */
      {
	memcpy(&(*dihe)[k],&buffer4[i],sizeof(DIHE));
	k++;
      }
    }
    
    if(k!=param->nDihedral)
      my_error(1000,__FILE__,__LINE__,0);
    
    free(buffer4);
  }
  
  if(param->nImproper>0)
  {
    buffer4=(DIHE*)my_malloc(param->nImproper*sizeof(DIHE));
    
    memcpy(buffer4,*impr,param->nImproper*sizeof(DIHE));
    
    k=0;
    iend=param->nImproper;
    for(i=0;i<iend;i++)
    {
      
      ia=buffer4[i].a;
      ib=buffer4[i].b;
      ic=buffer4[i].c;
      id=buffer4[i].d;
      
      if( (frozen[ia]*frozen[ib]*frozen[ic]*frozen[id]) ) /** Test if the four atoms are Frozen. */
      {
	param->nImproper--;
      }
      else /** Test if the four atoms are Frozen. */
      {
	memcpy(&(*impr)[k],&buffer4[i],sizeof(DIHE));
	k++;
      }
    }
    
    if(k!=param->nImproper)
      my_error(1000,__FILE__,__LINE__,0);
    
    free(buffer4);
  }
  
  if(param->nConst>0)
    *constList=(CONSTRAINT*)my_realloc(*constList,param->nConst*sizeof(CONSTRAINT));
  else
    free(*constList);
  
  if(param->nBond>0)
    *bond=(BOND*)my_realloc(*bond,param->nBond*sizeof(BOND));
  else
    free(*bond);
  
  if(param->nUb>0)
    *ub=(BOND*)my_realloc(*ub,param->nUb*sizeof(BOND));
  else
    free(*ub);
  
  if(param->nAngle>0)
    *angle=(ANGLE*)my_realloc(*angle,param->nAngle*sizeof(ANGLE));
  else
    free(*angle);
  
  if(param->nDihedral>0)
    *dihe=(DIHE*)my_realloc(*dihe,param->nDihedral*sizeof(DIHE));
  else
    free(*dihe);
  
  if(param->nImproper>0)
    *impr=(DIHE*)my_realloc(*impr,param->nImproper*sizeof(DIHE));
  else
    free(*impr);
}

/**
 * \param atom Array of structure ATOM (coordinates, forces, etc...).
 * \param simulCond Pointer to structure SIMULPARAMS containing parameters of the current simulation.
 * \param constList Pointer to structure CONSTRAINT containing parameters of constraints.
 * \param box Pointer to structure PBC containing Periodic Boundaries Conditions parameters.
 *
 * \brief Initialise velocity of atoms according to a random normal distribution.
 * \remarks If constraints are used, init_constvel is internally called.
 */
void init_vel(PARAM *param,PBC *box,CONSTRAINT constList[],double x[],
	      double y[],double z[],double vx[],double vy[],double vz[],
	      double mass[],double rmass[],int frozen[],int nAtConst[])
{
  int i,natoms;
  double cmvx=0.,cmvy=0.,cmvz=0.,cmm=0.,initKin=0.;
  double factor;  
  
  natoms=2*(param->nAtom/2);
  
//   Obtain gaussian distribution of velocities via Box-Muller method.
  
  for(i=0;i<natoms;i+=2)
  {
    get_BoxMuller(&(vx[i]),&(vx[i+1]));
    get_BoxMuller(&(vy[i]),&(vy[i+1]));
    get_BoxMuller(&(vz[i]),&(vz[i+1]));
  }
  
  if(natoms!=param->nAtom)
  {
    double v;
    get_BoxMuller(&(vx[natoms]),&v);
    get_BoxMuller(&(vy[natoms]),&v);
    get_BoxMuller(&(vz[natoms]),&v);
  }
  
//   Random numers provided by the generator have to be weighted
//   by the square root of the atoms mass to obtain velocities:
//   P(vx_i) ~ exp(-m_i*vx_i**2/(2*kb*T))
  
  for(i=0;i<param->nAtom;i++)
  {
    vx[i]*=sqrt(rmass[i]);
    vy[i]*=sqrt(rmass[i]);
    vz[i]*=sqrt(rmass[i]);
  }
  
  if(param->nConst>0)
    init_constvel(param,box,constList,x,y,z,vx,vy,vz,mass,nAtConst);
  
//   Set the system total momentum to zero to acheive momentum conservation.
  
  for(i=0;i<param->nAtom;i++)
  {
    if( (!frozen[i]) && (mass[i]>=DBL_EPSILON) )
    {
      cmvx+=vx[i]*mass[i];
      cmvy+=vy[i]*mass[i];
      cmvz+=vz[i]*mass[i];
      
      cmm+=mass[i];
    }
  }
  
  cmvx/=cmm;
  cmvy/=cmm;
  cmvz/=cmm;
  
  for(i=0;i<param->nAtom;i++)
  {
    if( (!frozen[i]) && (mass[i]>=DBL_EPSILON) )
    {
      vx[i]-=cmvx;
      vy[i]-=cmvy;
      vz[i]-=cmvz;
    }
    else
    {
      vx[i]=0.;
      vy[i]=0.;
      vz[i]=0.;
    }
  }
  
  initKin=kinetic(param,vx,vy,vz,mass);
  
  factor=sqrt(param->kinTemp0/initKin);
  
  for(i=0;i<param->nAtom;i++)
  {
    vx[i]*=factor;
    vy[i]*=factor;
    vz[i]*=factor;
  }
  
}

/**
 * \param atom Array of structure ATOM (coordinates, forces, etc...).
 * \param simulCond Pointer to structure SIMULPARAMS containing parameters of the current simulation.
 * \param constList Pointer to structure CONSTRAINT containing parameters of constraints.
 * \param box Pointer to structure PBC containing Periodic Boundaries Conditions parameters.
 *
 * \brief Initialise velocity of atoms according to a random normal distribution.
 * \remarks Called by init_vel if constraints are used.
 */
void init_constvel(PARAM *param,PBC *box,CONSTRAINT constList[],double x[],
		   double y[],double z[],double vx[],double vy[],double vz[],
		   double mass[],int nAtConst[])
{
  int i,ia,ib,icycle,converged;
  double *vxu=NULL,*vyu=NULL,*vzu=NULL;
  double rt,maxdv,dv,w1,w2,nia,nib;
  double *dtx=NULL,*dty=NULL,*dtz=NULL;
  
  vxu=(double*)my_malloc(param->nAtom*sizeof(*vxu));
  vyu=(double*)my_malloc(param->nAtom*sizeof(*vyu));
  vzu=(double*)my_malloc(param->nAtom*sizeof(*vzu));
  
  dtx=(double*)my_malloc(param->nConst*sizeof(*dtx));
  dty=(double*)my_malloc(param->nConst*sizeof(*dty));
  dtz=(double*)my_malloc(param->nConst*sizeof(*dtz));
  
  for(i=0;i<param->nConst;i++)
  {
    ia=constList[i].a;
    ib=constList[i].b;
    
    dtx[i]=x[ib]-x[ia];
    dty[i]=y[ib]-y[ia];
    dtz[i]=z[ib]-z[ia];
    
    rt=sqrt( dist2(box,&(dtx[i]),&(dty[i]),&(dtz[i])) );
    
    dtx[i]/=rt;
    dty[i]/=rt;
    dtz[i]/=rt;
    
  }
  
  icycle=0;
  converged=0;
  
  do
  {
    for(i=0;i<param->nAtom;i++)
    {
      vxu[i]=0.;
      vyu[i]=0.;
      vzu[i]=0.;
    }
    
    maxdv=0.;
    
    for(i=0;i<param->nConst;i++)
    {
      
      ia=constList[i].a;
      ib=constList[i].b;
      
      dv=dtx[i]*(vx[ib]-vx[ia])+dty[i]*(vy[ib]-vy[ia])+
	 dtz[i]*(vz[ib]-vz[ia]);
      
      maxdv=MAX(maxdv,fabs(dv));
      
      w1=mass[ib]*dv/(mass[ia]+mass[ib]);
      w2=mass[ia]*dv/(mass[ia]+mass[ib]);
      
      vxu[ia]+=w1*dtx[i];
      vyu[ia]+=w1*dty[i];
      vzu[ia]+=w1*dtz[i];
      
      vxu[ib]-=w2*dtx[i];
      vyu[ib]-=w2*dty[i];
      vzu[ib]-=w2*dtz[i];
      
    }
    
    if(maxdv<param->tolShake)
      converged=1;
    
    if(!converged)
    {
      for(i=0;i<param->nConst;i++)
      {
	ia=constList[i].a;
	ib=constList[i].b;
	
	nia=(double)nAtConst[ia];
	nib=(double)nAtConst[ib];
	
	vx[ia]+=vxu[ia]/nia;
	vy[ia]+=vyu[ia]/nia;
	vz[ia]+=vzu[ia]/nia;
	
	vx[ib]+=vxu[ib]/nib;
	vy[ib]+=vyu[ib]/nib;
	vz[ib]+=vzu[ib]/nib;
      }
    }
    
    icycle++;
    
  }while( (!converged) && (icycle<param->maxCycle) );
  
  if(!converged)
    my_error(CONVERG_VEL_ERROR,__FILE__,__LINE__,0);
  
  free(vxu);
  free(vyu);
  free(vzu);
  
  free(dtx);
  free(dty);
  free(dtz);
}

/**
 * \param box Pointer to structure PBC containing Periodic Boundaries Conditions parameters.
 *
 * \brief Computes norms, Wigner-Seitz cell, determinant, volume for a given box.
 */
void init_box(PBC *box)
{
  // get norm
  box->a=sqrt(X2(box->a1)+X2(box->a2)+X2(box->a3));
  box->b=sqrt(X2(box->b1)+X2(box->b2)+X2(box->b3));
  box->c=sqrt(X2(box->c1)+X2(box->c2)+X2(box->c3));
  
  // computes Wigner-Seitz cell
  box->u1=box->b2*box->c3-box->c2*box->b3;
  box->u2=box->c1*box->b3-box->b1*box->c3;
  box->u3=box->b1*box->c2-box->c1*box->b2;
  
  box->v1=box->c2*box->a3-box->a2*box->c3;
  box->v2=box->a1*box->c3-box->c1*box->a3;
  box->v3=box->c1*box->a2-box->a1*box->c2;
  
  box->w1=box->a2*box->b3-box->b2*box->a3;
  box->w2=box->b1*box->a3-box->a1*box->b3;
  box->w3=box->a1*box->b2-box->b1*box->a2;
  
  // determinant
  box->det=box->a1*box->u1+box->a2*box->u2+box->a3*box->u3;
  // volume
  box->vol=fabs(box->det);
  box->vol0=box->vol; //for NPT and NST purpose
  
  /* obtain parameters of a virutal orthorombic cell containing
   * the triclinic cell
   */
  box->pa=box->vol/sqrt(X2(box->u1)+X2(box->u2)+X2(box->u3));
  box->pb=box->vol/sqrt(X2(box->v1)+X2(box->v2)+X2(box->v3));
  box->pc=box->vol/sqrt(X2(box->w1)+X2(box->w2)+X2(box->w3));
  
  box->u=1.0/box->pa;
  box->v=1.0/box->pb;
  box->w=1.0/box->pc;
  
  // normalise Wigner-Seitz vectors
  if(fabs(box->det)>0.)
  {
    box->u1/=box->det;
    box->u2/=box->det;
    box->u3/=box->det;
    
    box->v1/=box->det;
    box->v2/=box->det;
    box->v3/=box->det;
    
    box->w1/=box->det;
    box->w2/=box->det;
    box->w3/=box->det;
  }
  else
  {
    // avoid div by 0
    box->u1=0.0;
    box->v1=0.0;
    box->w1=0.0;
    
    box->u2=0.0;
    box->v2=0.0;
    box->w2=0.0;
    
    box->u3=0.0;
    box->v3=0.0;
    box->w3=0.0;
  }
  
}

void free_all(CTRL *ctrl,PARAM *param,EWALD *ewald,ATOM **atom,CONSTRAINT **constList,
	      BOND **bond,ANGLE **angle,DIHE **dihe,DIHE **impr,BOND **ub,double **x,
	      double **y, double **z,double **vx,double **vy,double **vz,double **fx,
	      double **fy, double **fz,double **mass,double **rmass,double **q,
	      double **eps,double **sig,double **eps14,double **sig14,int **frozen,
	      int **nAtConst,int **neighList,int **neighPair,int **neighOrder,
	      int **neighList14,int ***exclList,int **exclPair)
{
  
  integrators_free_arrays(ctrl,param);
  
  if(param->nConst>0) shake_free_arrays();
  
  if(param->nConst>0) free(*constList);
  
  if(param->nBond>0) free(*bond);
  
  if(param->nAngle>0) free(*angle);
  
  if(param->nDihedral>0) free(*dihe);
  
  if(param->nImproper>0) free(*impr);
  
  if(param->nUb>0) free(*ub);
  
  free(*atom); free(*x); free(*y); free(*z); free(*vx); free(*vy); free(*vz);
  
  free(*fx); free(*fy); free(*fz); free(*mass); free(*rmass); free(*q); free(*eps);
  
  free(*sig); free(*eps14); free(*sig14); free(*frozen); free(*nAtConst);
  
  free(*neighList); free(*neighPair); free(*neighOrder); free(*neighList14);
  
  free(*exclPair);
  
  int i,nDealloc;
  
  if(ctrl->keyLink)
    nDealloc=param->nAtom;
  else
    nDealloc=param->nAtom-1;
  
  for(i=0;i<nDealloc;i++)
  {
    free((*exclList)[i]);
  }
  free(*exclList);
  
  if(ctrl->keyEwald==1)
  {
    ewald_free(ewald);
  }
  else if(ctrl->keyEwald==2)
  {
    spme_free(param);
  }
  
#ifdef TIMER
  free_timers();
#endif
  
}
