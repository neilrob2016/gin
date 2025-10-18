#include "globals.h"


void stateInit(void)
{
	state_start = 0;
	state_next = 0;
}



/*** Store the current game state. This is called at the start of the users 
     move ***/
void stateStore(void)
{
	memcpy(state[state_next].deck,deck,sizeof(deck));
	state[state_next].flags         = flags;
	state[state_next].ply[USER]     = ply[USER];
	state[state_next].ply[COMPUTER] = ply[COMPUTER];
	state[state_next].move          = move;
	state[state_next].decktop       = decktop;
	state[state_next].max_decktop   = max_decktop;
	state[state_next].knock_player  = knock_player;
	state[state_next].gin_player    = gin_player;

	state_next = (state_next + 1) % MAX_ROLLBACK;
	if (state_next == state_start)
		state_start = (state_start + 1) % MAX_ROLLBACK;

	dbgprintf(NO_PLY,"Store state: start = %d, next = %d, move = %d, decktop = %d, max_decktop = %d\n",
		state_start,state_next,move,decktop,max_decktop);
}



/*** Roll back the user and computer states back to the beginning of the users
     previous move ***/
bool stateRestorePrevious(void)
{
	dbgprintf(NO_PLY,"state_start = %d, state_next = %d\n",
		state_start,state_next);

	// Need min 2 states available as the top state is the one saved at
	// the start of the users current move - we want the previous one.
	if (state_next > state_start)
	{
		if (state_next - state_start < 2) return false;
	}
	else if (MAX_ROLLBACK - state_start + state_next < 2) return false;

	// state_next is the next empty slot so need 2 below
	int pos = state_next - 2;
	if (pos < 0) pos += MAX_ROLLBACK;

	ply[USER]     = state[pos].ply[USER];
	ply[COMPUTER] = state[pos].ply[COMPUTER];

	// Copy everything
	memcpy(deck,state[pos].deck,sizeof(deck));
	flags        = state[pos].flags;
	move         = state[pos].move;
	decktop      = state[pos].decktop;
	max_decktop  = state[pos].max_decktop;
	knock_player = state[pos].knock_player;

	state_next = (pos + 1) % MAX_ROLLBACK;

	dbgprintf(NO_PLY,"Rollback state: start = %d, next = %d, move = %d, decktop = %d, max_decktop = %d\n",
		state_start,state_next,move,decktop,max_decktop);
	return true;
}

