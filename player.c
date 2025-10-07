#include "globals.h"

void playersInit(void)
{
	knock_player = NO_PLY;
	gin_player = NO_PLY;
	// Can't just zero whole player structure because of points and games 
	// won stats.
	for(int i=0;i < NUM_PLAYERS;++i) 
	{
		bzero(ply[i].seencards,sizeof(ply[i].seencards));
		bzero(ply[i].meldsets,sizeof(ply[i].meldsets));
		bzero(ply[i].meldruns,sizeof(ply[i].meldruns));
		ply[i].seencnt = 0;
		ply[i].meldsetscnt = 0;
		ply[i].meldrunscnt= 0; 
		for(int j=0;j < HAND_SIZE;++j) playerAddSeen(i,ply[i].hand[j]);
	}
}



void playerAddSeen(int player, t_card card)
{
	assert(ply[player].seencnt < DECK_SIZE);

	// Might have already seen card if we did a swap but other player didn't
	// take any cards.
	if (playerHasSeenCard(player,card)) 
		dbgprintf(player,"Already seen card: %s\n",cardString(card));
	else
	{
		dbgprintf(player,"Adding seen card: %s\n",cardString(card));
		ply[player].seencards[ply[player].seencnt++] = card;
	}
}



int playerHasSeenCard(int player, t_card card)
{
	assert(VALID_CARD(card));
	for(int i=0;i < DECK_SIZE && i < ply[player].seencnt;++i)
		if (SAME_CARD(ply[player].seencards[i],card)) return 1;
	return 0;
}



/*** As above but only checks the value, not the suit and returns the count ***/
int playerHasSeenCardType(int player, int type)
{
	assert(type >= ACE && type <= KING);
	int cnt = 0;
	for(int i=0;i < DECK_SIZE && i < ply[player].seencnt;++i)
		cnt += (ply[player].seencards[i].type == type);
	assert(cnt < 5);
	return cnt;
}



void playerKnock(int player)
{
	colprintf("\n~F%c~BM*** ~LI%s KNOCKS!~RS~F%c~BM ***\n",
		player_col[player],
		player_name[flags.self_play][player],
		player_col[player]);
	knock_player = player;
}



void playerGin(int player)
{
	colprintf("\n~BR~FW*** ~LI%s Goes Gin!~RS~BR~FW ***\n",
		player_name[flags.self_play][player]);
	gin_player = player;
}



/*** Layoff players hand against knockers melded cards ***/
void playerLayOff(int player)
{
	pcolprintf(player,"'s hand before layoff:\n");
	handPrint(ply[player].hand,false);
	pcolprintf(player," attempts to lay off cards onto %s's melds...\n",
		player_name[flags.self_play][knock_player]);

	int laycnt = 0;
	int cnt;
	int i;
	int j;
	for(i=0;i < HAND_SIZE;++i)
	{
		t_card card = ply[player].hand[i];
		if (!VALID_CARD(card)) continue;

		// See if we fit the card into one of the knockers melded sets
		// eg if we have 8c check for more than 1 other 8 in their sets.
		for(j=cnt=0;j < ply[knock_player].meldsetscnt;++j)
		{
			cnt += (ply[knock_player].meldsets[j].type == card.type);
			if (cnt > 1)
			{
				colprintf("   ~FMLays:~RS %s against set.\n",
					cardString(card));
				ply[player].hand[i] = invalid_card;
				++laycnt;
				break;
			}
		}
		if (j < ply[knock_player].meldsetscnt) continue;

		/* See if we can fit it into a run. To do this we see if it will
		   fit on either end of a run so if we have 4c check if the 
		   knock player has either 5c or 3c in their melded run cards */
		t_card c1 = card;
		t_card c2 = card;
		--c1.type;
		++c2.type;
		for(j=cnt=0;j < ply[knock_player].meldrunscnt;++j)
		{
			t_card c3 = ply[knock_player].meldruns[j];
			if (SAME_CARD(c1,c3) || SAME_CARD(c2,c3))
			{
				colprintf("   ~FMLays:~RS %s against run ",
					cardString(card));
				ply[player].hand[i] = invalid_card;
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
		handShiftLeft(ply[player].hand);
		handPrint(ply[player].hand,false);
	}
	else pcolprintf(player," couldn't lay off any cards.\n\n");
}
