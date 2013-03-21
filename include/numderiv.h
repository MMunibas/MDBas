#ifndef NUMDERIVH_INCLUDED
#define NUMDERIVH_INCLUDED

#ifdef	__cplusplus
extern "C" {
#endif

void numforce(CTRL *ctrl,PARAM *param,ENERGY *ener,PBC *box,NEIGH *neigh,
	      BOND bond[],BOND ub[],ANGLE angle[],DIHE dihe[],DIHE impr[],
	      DELTA nForce[],double x[],double y[],double z[],int npoints,double h);

#ifdef	__cplusplus
}
#endif

#endif