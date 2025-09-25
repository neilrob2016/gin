#include "globals.h"

void playersInit(void)
{
	knock_player = NO_PLY;
	gin_player = NO_PLY;
	bzero(meldsets,sizeof(meldsets));
	bzero(meldruns,sizeof(meldruns));
	bzero(seencards,sizeof(seencards));
	for(int i=0;i < NUM_PLAYERS;++i) 
	{
		meldsetscnt[i] = meldrunscnt[i] = 0;
		seencnt[i] = 0;
		for(int j=0;j < HAND_SIZE;++j) playerPushSeen(i,hands[i][j]);
	}
}



bool playerPushSeen(int player, t_card card)
{
	assert(seencnt[player] < DECK_SIZE);

	// Make sure we don't already have it
	if (playerHasSeenCard(player,card)) return false;

	dbgprintf(player,"Pushing seen card %s\n",cardString(card));
	seencards[player][seencnt[player]++] = card;
	return true;
}



/*** Remove last card in seen list ***/
void playerPopSeen(int player)
{
	if (seencnt[player]) 
	{
		int pos = --seencnt[player];
		dbgprintf(player,"Popping seen card %s\n",
			cardString(seencards[player][pos]));
		seencards[player][pos] = invalid_card;
	}
}



int playerHasSeenCard(int player, t_card card)
{
	assert(VALID_CARD(card));
	for(int i=0;i < DECK_SIZE && i < seencnt[player];++i)
		if (SAME_CARD(seencards[player][i],card)) return 1;
	return 0;
}



/*** As above but only checks the value, not the suit and returns the count ***/
int playerHasSeenCardType(int player, int type)
{
	assert(type >= ACE && type <= KING);
	int cnt = 0;
	for(int i=0;i < DECK_SIZE && i < seencnt[player];++i)
		cnt += (seencards[player][i].type == type);
	assert(cnt < 5);
	return cnt;
}


void playerKnock(int player)
{
	colprintf("\n~F%c~BM~LI*** %s KNOCKS! ***\n",
		player_col[player],player_name[flags.self_play][player]);
	knock_player = player;
}



void playerGin(int player)
{
	colprintf("\n~BR~FW~LI*** %s Goes Gin! ***\n",
		player_name[flags.self_play][player]);
	gin_player = player;
}



/*** Layoff players hand against knockers melded cards ***/
void playerLayOff(int player)
{
	pcolprintf(player,"'s hand before layoff:\n");
	handPrint(hands[player],false);
	pcolprintf(player," attempts to lay off cards onto %s's melds...\n",
		player_name[flags.self_play][knock_player]);

	int laycnt = 0;
	int cnt;
	int i;
	int j;
	for(i=0;i < HAND_SIZE;++i)
	{
		t_card card = hands[player][i];
		if (!VALID_CARD(card)) continue;

		// See if we fit the card into one of the knockers melded sets
		// eg if we have 8c check for more than 1 other 8 in their sets.
		for(j=cnt=0;j < meldsetscnt[knock_player];++j)
		{
			cnt += (meldsets[knock_player][j].type == card.type);
			if (cnt > 1)
			{
				colprintf("   ~FMLays:~RS %s against set.\n",
					cardString(card));
				hands[player][i] = invalid_card;
				++laycnt;
				break;
			}
		}
		if (j < meldsetscnt[knock_player]) continue;

		/* See if we can fit it into a run. To do this we see if it will
		   fit on either end of a run so if we have 4c check if the 
		   knock player has either 5c or 3c in their melded run cards */
		t_card c1 = card;
		t_card c2 = card;
		--c1.type;
		++c2.type;
		for(j=cnt=0;j < meldrunscnt[knock_player];++j)
		{
			t_card c3 = meldruns[knock_player][j];
			if (SAME_CARD(c1,c3) || SAME_CARD(c2,c3))
			{
				colprintf("   ~FMLays:~RS %s against run ",
					cardString(card));
				hands[player][i] = invalid_card;
				++laycnt;
				if (SAME_CARD(c1,c3)) 
					colprintf("ending with %s\n",cardString(c1));
				else
					colprintf("starting with %s\n",cardString(c2));
				break;
			}
		}
	}
	if (laycnt)
	{
		pcolprintf(player," laid off ~FY%d ~F%ccard(s) to give hand:\n",
			laycnt,player_col[player]);
		handShiftLeft(hands[player]);
		handPrint(hands[player],false);
	}
	else pcolprintf(player," couldn't lay off any cards.\n\n");
}
