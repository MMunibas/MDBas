#ifndef LISTH_INCLUDED
#define LISTH_INCLUDED

#ifdef	__cplusplus
extern "C" {
#endif

void makelist(CTRL *ctrl,PARAM *param,PBC *box,NEIGH *neigh,CONSTRAINT constList[],
	      BOND bond[],ANGLE angle[],DIHE dihe[],DIHE impr[],double x[], double y[],
	      double z[],int frozen[],int **neighList,int **neighPair,int **neighOrder,
	      int **neighList14,int ***exclList,int **exclPair);

void exclude_list(CTRL *ctrl,PARAM *param,NEIGH *neigh,CONSTRAINT constList[],
		  BOND bond[],ANGLE angle[],DIHE dihe[],DIHE impr[],
		  int **neighList14,int ***exclList,int **exclPair);

void verlet_list(PARAM *param,PBC *box,NEIGH *neigh,double x[],double y[],double z[],
		 int frozen[],int **neighList,int **neighPair,int **neighOrder,
		 int **exclList,int exclPair[]);

void verlet_list_update(PARAM *param,PBC *box,NEIGH *neigh,double x[],double y[],double z[],
			int frozen[],int **neighList,int neighPair[],int **exclList,
			int exclPair[]);

void link_cell_exclude_list(CTRL *ctrl,PARAM *param,NEIGH *neigh,CONSTRAINT constList[],
			    BOND bond[],ANGLE angle[],DIHE dihe[],DIHE impr[],
			    int **neighList14,int ***exclList,int **exclPair);

void link_cell_verlet_list(PARAM *param,PBC *box,NEIGH *neigh,double x[],double y[],double z[],
			   int frozen[],int **neighList,int **neighPair,int **neighOrder,
			   int **exclList,int exclPair[]);

void link_cell_verlet_list_update(PARAM *param,PBC *box,NEIGH *neigh,double x[],double y[],double z[],
				  int frozen[],int **neighList,int neighPair[],int neighOrder[],
				  int **exclList,int exclPair[]);

void fast_verlet_list(PARAM *param,PBC *box,NEIGH *neigh,double x[],double y[],double z[],
		      int frozen[],int **neighList,int **neighPair,int **neighOrder,
		      int **exclList,int exclPair[]);

#ifdef	__cplusplus
}
#endif

#endif