/**
 * \file io.h
 * \brief Prototypes for file io.c
 * \author Pierre-Andre Cazade and Florent Hedin
 * \version alpha-branch
 * \date 2012
 */

#ifndef IOH_INCLUDED
#define IOH_INCLUDED

void read_SIMU(SIMULPARAMS *simulCond,FORCEFIELD *ff,PBC *box);
void read_TOP(INPUTS *inp);
void read_PSF(INPUTS *inp,ATOM **atom,FORCEFIELD *ff,SIMULPARAMS *simulCond,CONSTRAINT **constList);
void read_PAR(INPUTS *inp);
void read_CONF(ATOM atom[],SIMULPARAMS *simulCond);
void setup(INPUTS *inp,ATOM atom[],FORCEFIELD *ff,SIMULPARAMS *simulCond,CONSTRAINT *constList);
void write_FORF(INPUTS *inp,ATOM atom[],FORCEFIELD *ff,SIMULPARAMS *simulCond);
void free_temp_array(INPUTS *inp);
void error(int errorNumber);

#endif
