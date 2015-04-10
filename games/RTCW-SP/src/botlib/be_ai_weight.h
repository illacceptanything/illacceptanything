/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		be_ai_weight.h
 *
 * desc:		fuzzy weights
 *
 *
 *****************************************************************************/

#define WT_BALANCE          1
#define MAX_WEIGHTS         128

//fuzzy seperator
typedef struct fuzzyseperator_s
{
	int index;
	int value;
	int type;
	float weight;
	float minweight;
	float maxweight;
	struct fuzzyseperator_s *child;
	struct fuzzyseperator_s *next;
} fuzzyseperator_t;

//fuzzy weight
typedef struct weight_s
{
	char *name;
	struct fuzzyseperator_s *firstseperator;
} weight_t;

//weight configuration
typedef struct weightconfig_s
{
	int numweights;
	weight_t weights[MAX_WEIGHTS];
	char filename[MAX_QPATH];
} weightconfig_t;

//reads a weight configuration
weightconfig_t *ReadWeightConfig( char *filename );
//free a weight configuration
void FreeWeightConfig( weightconfig_t *config );
//writes a weight configuration, returns true if successfull
qboolean WriteWeightConfig( char *filename, weightconfig_t *config );
//find the fuzzy weight with the given name
int FindFuzzyWeight( weightconfig_t *wc, char *name );
//returns the fuzzy weight for the given inventory and weight
float FuzzyWeight( int *inventory, weightconfig_t *wc, int weightnum );
float FuzzyWeightUndecided( int *inventory, weightconfig_t *wc, int weightnum );
//scales the weight with the given name
void ScaleWeight( weightconfig_t *config, char *name, float scale );
//scale the balance range
void ScaleBalanceRange( weightconfig_t *config, float scale );
//evolves the weight configuration
void EvolveWeightConfig( weightconfig_t *config );
//interbreed the weight configurations and stores the interbreeded one in configout
void InterbreedWeightConfigs( weightconfig_t *config1, weightconfig_t *config2, weightconfig_t *configout );
//frees cached weight configurations
void BotShutdownWeights( void );
