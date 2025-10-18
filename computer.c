#include "globals.h"

void  computerKnockAndMeld(int player, t_handval hv);
int   computerGetMeldedValue(
	int player, t_card *hand, int algo, bool adjust_for_doubles);
void  computerEndOfMoveMeld(int player, int algo);
void  computerMeld(int player, t_card *hand, int algo);
void  computerMeldSetsThenRuns(int player, t_card *hand);
void  computerMeldRunsThenSets(int player, t_card *hand);
void  computerMeldRuns(int player, t_card *hand);
void  computerMeldSets(int player, t_card *hand);
void  computerInvalidateCards(
	int player, bool set, t_card *hand, int from, int to);

t_handval computerGetMinValue(
	int player, t_card *hand, bool adjust_for_doubles);
int   computerGetValueIncDoubles(int player, t_card *hand);
float computerGetRunDoubleValue(int player, t_card prev, t_card card, float val);
float computerGetSetDoubleValue(int player, t_card prev, t_card card, float val);

void computerSuggestMeld(int algo);

/////////////////////////////// EXTERNAL FUNCS ///////////////////////////////


/*** Get the nextcard card and try it in every hand position then check each of 
     these hands for their value. Pick the lowest if there is one. Have 
     player parameter for self play mode. ***/
void computerMove(int player)
{
	t_card *phand = ply[player].hand;

	// Get current hand value if player melded not adjusting for doubles
	t_handval currhv = computerGetMinValue(player,phand,false); 
	int currval = currhv.value;

	if (flags.suggest)
	{
		assert(player == USER);
		if (knock_player != NO_PLY)
		{
			computerSuggestMeld(currhv.algo);
			return;
		}
	}
	else
	{
		// mainloop shouldn't call us again if we've already knocked
		assert(knock_player != player);
		putchar('\n');
		if (PRINT_INFO())
		{
			pcolprintf(player,"'s move. Current hand:\n");
			handPrint(phand,true);
		}
		else pcolprintf(player,"'s move...\n");
	}

	if (flags.debug)
	{
		dbgprintf(player,"Current min hand value with algo %d: %d\n",
			currhv.algo,currhv.value);
	}

	// See if we have a potential winning hand and can knock
	if (knock_player == NO_PLY && currval <= min_knock_val)
	{
		if (flags.suggest)
		{
			assert(player == USER);
			computerSuggestMeld(currhv.algo);
			return;
		}
		computerKnockAndMeld(player,currhv);
		return;
	}

	// If no more cards do final meld
	if (decktop == max_decktop)
	{
		if (flags.suggest)
		{
			assert(player == USER);
			computerSuggestMeld(currhv.algo);
		}
		else
		{
			pcolprintf(player," can't draw another card.\n");
			computerEndOfMoveMeld(player,currhv.algo);
		}
		return;
	}

	t_card nextcard;
	t_card tmp;
	t_handval hv;
	int algo;
	int minval;
	int minpos;
	bool exchanged = false;

	// Get value adjusted for doubles
	currhv = computerGetMinValue(player,phand,true); 
	dbgprintf(player,"Current min hand value with algo %d adjusted for doubles: %d\n",
		currhv.algo,currhv.value);

	/* Try the next cards in our hand and see if they improve it. Loop 
	   twice because we can take a 2nd card from the deck if we don't like 
	   the first unless the other player has knocked so just do it once */
	for(int i=0;;++i)
	{
		// Sanity check
		assert(i < 2);

		nextcard = deck[decktop];
		if (PRINT_INFO()) 
			pcolprintf(player,"'s deck card: %s\n",cardString(nextcard));
		minval = currhv.value;
		minpos = -1;

		playerAddSeen(player,nextcard);
		for(int j=0;j < HAND_SIZE;++j)
		{
			if (!VALID_CARD(phand[j])) continue; 

			tmp = phand[j];
			phand[j] = nextcard;
			hv = computerGetMinValue(player,phand,true); 
			dbgprintf(player,"Value after exchange at pos %d with algo %d: %d\n",j,hv.algo,hv.value);

			/* Could use an array to store all the possible
			   equivalent lowest value positions and pick one at 
			   the end but too much hassle. Just use random() here 
			   instead unless its a user suggestion (we want a 
			   consistent response then) */
			if (hv.value < minval || 
			    (!flags.suggest && 
			     hv.value == minval && 
			     minval < currhv.value && 
			     !(random() % 3)))
			{
				algo = hv.algo;
				minval = hv.value;
				minpos = j;
				dbgprintf(player,"Setting minpos: %d\n",minpos);
			}
			phand[j] = tmp;
		}

		// Minpos == -1 means there's no improvement in our deck
		// value inserting the card anywhere so get the next one.
		if (minpos == -1)
		{
			if (flags.suggest)
			{
				if (flags.user_next)
					sugprintf("Get the next card.\n");
					
				else if (knock_player == NO_PLY)
					sugprintf("Do nothing and end the move.\n");
				else computerSuggestMeld(currhv.algo);
				return;
			}
			if (i == 1) 
			{
				// Had 2 goes, end of move.
				pcolprintf(player," rejects the card.\n");
				break;
			}
			if (decktop == max_decktop - 1)
			{
				pcolprintf(player," rejects the card but can't draw any more.\n");
				break;
			}
			pcolprintf(player," rejects the card and gets the next.\n");
			++decktop;
			continue;
		}

		// Exchange cards, do final melds if possible and finish move.
		tmp = phand[minpos];
		if (flags.suggest)
		{
			sugprintf(
				"Exchange %s at position ~FY%d~RS.\n",
				cardString(tmp),minpos);
			return;
		}
		if (PRINT_INFO())
		{
			pcolprintf(player," exchanges at position %d:~RS %s replaces %s\n",
				minpos,
				cardString(nextcard),
				cardString(tmp));
		}
		else pcolprintf(player," exchanges cards.\n");

		phand[minpos] = nextcard;
		deck[decktop] = tmp;
		exchanged = true;
		if (PRINT_INFO())
		{
			pcolprintf(player,"'s hand after exchange:\n");
			// !i = don't print deck card if we've just taken it
			handPrint(phand,!i);
		}
		break;
	}
	if (knock_player != NO_PLY) computerEndOfMoveMeld(player,algo);
	else if (exchanged)
	{
		// Get non double adjusted value if we melded and see if we 
		// can knock.
		currhv = computerGetMinValue(player,phand,false); 
		if (currhv.value <= min_knock_val)
			computerKnockAndMeld(player,currhv);
		else if (flags.meld_asap)
			computerEndOfMoveMeld(player,algo);
	}
	pcolprintf(player,"'s move ends.\n");
}




//////////////////////////////// MELDING //////////////////////////////////

/*** Meld everything after we've knocked. We leave melding until now (unless
     other player has knocked) because we don't want to meld eg: KKK if 
     another king might come along to give KKKK ***/
void computerKnockAndMeld(int player, t_handval hv)
{
	assert(knock_player == NO_PLY);

	dbgprintf(player,"Melding down to value %d with algo %d below min knock value of %d\n",
		hv.value,hv.algo,min_knock_val);

	playerKnock(player);
	handPrint(ply[player].hand,false);
	computerEndOfMoveMeld(player,hv.algo);

	assert(handGetValue(ply[player].hand) == hv.value);
}



/*** Try to meld at end of a move ***/
void computerEndOfMoveMeld(int player, int algo)
{
	dbgprintf(player,"Melding with algo %d\n",algo);
	t_card *hand = ply[player].hand;

	// Meld if we can
	flags.show_meld = (PRINT_INFO() || knock_player != NO_PLY);
	if (flags.show_meld)
	{
		if (!flags.self_play && knock_player == USER)
		{
			pcolprintf(player,"'s hand before meld:\n");
			handPrint(hand,true);
		}
		pcolprintf(player," attempts to meld...\n");
	}

	flags.store_melds = 1;
	computerMeld(player,hand,algo);
	flags.store_melds = 0;
	handShiftLeft(ply[player].hand);

	if (flags.show_meld)
	{
		if (flags.melded)
		{
			pcolprintf(player," has melded.\n");
			if (PRINT_INFO()) handPrint(hand,true);
		}
		else pcolprintf(player," cannot meld.\n");
	}
	flags.show_meld = 0;
}



void computerMeld(int player, t_card *hand, int algo)
{
	flags.melded = 0;
	if (algo == ALGO_MELD_SET_THEN_RUN) 
		computerMeldSetsThenRuns(player,hand);
	else
		computerMeldRunsThenSets(player,hand);
}



void computerMeldSetsThenRuns(int player, t_card *hand)
{
	handSortSet(hand);
	computerMeldSets(player,hand);
	handSortRun(hand);
	computerMeldRuns(player,hand);
}



void computerMeldRunsThenSets(int player, t_card *hand)
{
	handSortRun(hand);
	computerMeldRuns(player,hand);
	handSortSet(hand);
	computerMeldSets(player,hand);
}



/*** Meldruns eg: 2c3c4c ***/
void computerMeldRuns(int player, t_card *hand)
{
	t_card prev;
	int from = 0;
	int i;

	prev = hand[0];
	// Hand is sorted so quit as soon as we hit invalid card
	for(i=1;i < HAND_SIZE && VALID_CARD(hand[i]);++i)
	{
		if (hand[i].suit != prev.suit || 
		    hand[i].type != prev.type + 1)
		{
			if (i - from > 2) 
				computerInvalidateCards(player,false,hand,from,i);
			from = i;
		}
		prev = hand[i];
	}
	if (i - from > 2) computerInvalidateCards(player,false,hand,from,i);
}



/*** Meld sets eg: 2c2h2d ***/
void computerMeldSets(int player, t_card *hand)
{
	t_card prev;
	int from = 0;
	int i;

	prev = hand[0];
	for(i=1;i < HAND_SIZE && VALID_CARD(hand[i]);++i)
	{
		if (hand[i].type != prev.type)
		{
			if (i - from > 2) 
				computerInvalidateCards(player,true,hand,from,i);
			from = i;
		}
		prev = hand[i];
	}
	if (i - from > 2) computerInvalidateCards(player,true,hand,from,i);
}



void computerInvalidateCards(
	int player, bool set, t_card *hand, int from, int to)
{
	assert(from < to);
	t_player *pp = &ply[player]; // Makes code easier to read

	if (flags.show_meld) colprintf("   ~FYMelding:~RS ");
	flags.melded = 1;
	for(int i=from;i < to;++i)
	{
		if (flags.show_meld) colprintf("%s ",cardString(hand[i]));

		// Store the melded cards for potential layoff later
		if (flags.layoff && flags.store_melds)
		{
			if (set)
				pp->meldsets[pp->meldsetscnt++] = hand[i];
			else
				pp->meldruns[pp->meldrunscnt++] = hand[i];
		}
		hand[i] = invalid_card;
	}
	if (flags.show_meld)
	{
		putchar('\n');
		if (flags.layoff)
		{
			dbgprintf(player,"Has %d stored melded cards.\n",
				pp->meldsetscnt + pp->meldrunscnt);
		}
	}
}


//////////////////////////////// VALUATION //////////////////////////////////

/*** Get the minium hand value after we've removed any runs or sets.
     Algo:
     1) Sort for sets, remove them, sort for runs, remove, get value
     2) Sort for runs, remove them, sort for sets, remove, get value
     Then get min value of 1 & 2.
***/
t_handval computerGetMinValue(int player, t_card *hand, bool adjust_for_doubles)
{
	int val1 = computerGetMeldedValue(
		player,hand,ALGO_MELD_SET_THEN_RUN,adjust_for_doubles);
	int val2 = computerGetMeldedValue(
		player,hand,ALGO_MELD_RUN_THEN_SET,adjust_for_doubles);

	// Want lowest value
	if (val1 <= val2) return (t_handval){ val1, ALGO_MELD_SET_THEN_RUN }; 
	return (t_handval){ val2, ALGO_MELD_RUN_THEN_SET };
}



int computerGetMeldedValue(
	int player, t_card *hand, int algo, bool adjust_for_doubles)
{
	t_card hand1[HAND_SIZE];

	handCopy(hand,hand1);
	computerMeld(player,hand1,algo);

	return (adjust_for_doubles ? 
	        computerGetValueIncDoubles(player,hand1) : handGetValue(hand1));
}



/*** This function is for when the computer shouldn't ignore doubles, eg: 6H6S,
     3C4C when valuing its hand before a possible exchange. Higher runs and 
     sets will have already been removed from the hand. This checks for whether
     cards that could append to the double have already been seen. Eg no point
     counting 3C4C if both 2C and 5C have already been dealt and gone. ***/
int computerGetValueIncDoubles(int player, t_card *hand)
{
	t_card hand1[HAND_SIZE];
	float val[2] = { 0, 0 };
	t_card prev = invalid_card;
	t_card card;
	int nval;

	handCopy(hand,hand1);
	handSortSet(hand1);

	// Check for sets, then runs
	for(int i=0;i < 2;++i)
	{
		for(int j=0;j < HAND_SIZE;++j)
		{
			card = hand1[j];
			if (!VALID_CARD(card)) 
			{
				prev = card;
				continue;
			}
			nval = typeval[card.type];
			// 1st (i == 0) loop check for sets, 2nd check for runs.
			if (i) 
				val[i] += computerGetRunDoubleValue(player,prev,card,nval);
			else 
				val[i] += computerGetSetDoubleValue(player,prev,card,nval);
			prev = card;
		}
		handSortRun(hand1);
	}
	// Return the lowest
	return (int)(val[0] < val[1] ? ceilf(val[0]) : ceilf(val[1]));
}



/*** A run double can be eg: 2c3c or seperated by a single value, eg: 2c4c.
     This requires the hand to be sorted into run order first. Remember the
     LOWER the return value the better ***/
float computerGetRunDoubleValue(int player, t_card prev, t_card card, float val)
{
	if (card.suit != prev.suit) return val;

	if (card.type == prev.type + 1)
	{
		int cnt = 0;

		/* Check if we've seen cards either side. Eg if double is 2c3c
		   then check for Ac and 4c. If we have then the double is 
		   useless. Also if its an ACE or KING we can't have cards below
		   or above respectively */
		if (--prev.type < ACE) 
			cnt = 1;
		else
			cnt = playerHasSeenCard(player,prev);
		if (++card.type > KING)
			++cnt;
		else
			cnt += playerHasSeenCard(player,card);

		/* If we haven't seen other cards (cnt = 0) then best result, 
		   seen 1 = ok, seen 2 = useless so invert for pow() exponent
		   because double_adjust_mult is 0 -> 1 so the higher the 
		   exponent the lower the return value */
		return val * pow(double_adjust_mult,2 - cnt);
	}
	if (card.type == prev.type + 2)
	{
		// Only check if we've seen the one in the middle. eg for 2c4c
		// check if we've seen 3c.
		++prev.type;
		return playerHasSeenCard(player,prev) ? 
		       val : val * double_adjust_mult;
	}
	return val;
}



/*** Check for double sets, eg if card.type is six have we already seen 2 sixes
     in our hand so see if we've seen the other 2 ***/
float computerGetSetDoubleValue(int player, t_card prev, t_card card, float val)
{
	if (card.type != prev.type) return val;

	// Should return 2 to 4, with 2 of them already being in our hand.
	int cnt = playerHasSeenCardType(player,card.type);
	assert(cnt > 1);

	// Having seen 0 other cards is best, 1 is ok, 2 is useless.
	return val * pow(double_adjust_mult,2 - (cnt - 2));
}


//////////////////////////////// MISCELLANIOUS /////////////////////////////////

void computerSuggestMeld(int algo)
{
	switch(algo)
	{
	case ALGO_MELD_SET_THEN_RUN:
		sugprintf("Meld sets then runs where possible then knock.\n");
		break;
	case ALGO_MELD_RUN_THEN_SET:
		sugprintf("Meld runs then sets where possible then knock.\n");
		break;
	default:
		assert(0);
	}
}
