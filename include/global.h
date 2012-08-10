#ifndef GLOBALH_INCLUDED
#define GLOBALH_INCLUDED

#define clight  (299792458.)
#define elemchg (1.602176565e-19)
#define angstr  (1.e-10)
#define calory  (4.184)
#define kcaltoiu (418.4)
#define NA	(6.02214129e+23)
#define kboltz  (1.3806488e-23)
#define rboltz  (8.3144621)
#define rboltzui  (0.83144621)
#define mu0     (1.e-7)
#define chgcharmm (332.0716)
#define chgnamd   (332.0636)
#define sq6rt2  (1.12246204830937)
#define PI      (3.14159265358979)

#define X2(x) ((x)*(x))
#define X3(x) (X2(x)*(x))
#define X4(x) (X2(x)*X2(x))
#define X6(x) (X3(x)*X3(x))
#define X12(x) (X6(x)*X6(x))
#define MAX(x,y) ((x)>=(y)?(x):(y))
#define MIN(x,y) ((x)<=(y)?(x):(y))

#define NOELEC 0
#define FULL   1
#define SHIFT1 2
#define SHIFT2 3
#define SWITCH 4

#define NOVDW   0
#define VFULL   1
#define VSWITCH 2

#define COSDIH  1
#define HARMDIH 2

typedef struct
{
  char label[5],segi[5],resi[5];
  int type,resn,inconst;
  double x,y,z;
  double vx,vy,vz;
  double fx,fy,fz;
  double m,q;
}ATOM;

typedef struct
{
  int type;
  double a,a1,a2,a3,b,b1,b2,b3,c,c1,c2,c3;
  double u,u1,u2,u3,v,v1,v2,v3,w,w1,w2,w3;
  double pa,pb,pc,det,vol;
}PBC;

typedef struct
{
//   FILE *cntrFile,*topFile,*psfFile,*parmFile,*corFile;
  char **types;
  int *typesNum;
  int nTypes,nBondTypes,nAngTypes,nUbTypes,nDiheTypes,nImprTypes,nNonBonded;
  int **bondTypes,**angTypes,**ubTypes,*nDiheTypesParm,**diheTypes,**imprTypes;
  double **bondTypesParm,**angTypesParm,**ubTypesParm;
  double **diheTypesParm,**imprTypesParm,**nonBondedTypesParm;
}INPUTS;

typedef struct
{
  int nBond,nAngle,nDihedral,nImproper,nUb,ncells;
  int **verList,*verPair,*verCumSum,npr,**ver14,npr14,*nParmDihe;
  double **parmVdw,scal14;
  double **parmBond,**parmUb,**parmAngle,**parmDihe;
  double **parmImpr;
}FORCEFIELD;

typedef struct
{
  int natom,nb14,step,nsteps,degfree,firstener,listupdate;
  int printo,printtr,integrator,ens,nconst,maxcycle;
  int keyrand,seed,keytraj,keyener,keyforf,keymd,keyconsth;
  int keyminim,maxminst,elecType,vdwType,mdNature,numDeriv;
  int linkRatio,keylink,nolink;
  int *excludeNum,**excludeAtom;
  int *bondType,*ubType,*angleType,*diheType,*imprType;
  int **iBond,**iUb,**iAngle,**iDihedral,**iImproper;
  double chargeConst,cutoff,cuton,delr,tolshake,kintemp0,taut;
  double tolminim,maxminsiz,temp,timeStep;
}SIMULPARAMS;

typedef struct
{
  double tot,pot,kin;
  double elec,vdw;
  double bond,ang,ub,dihe,impr;
}ENERGY;

typedef struct
{
  int a,b;
  double rc2;
}CONSTRAINT;

typedef struct
{
  double x,y,z;
}DELTA;

#endif
