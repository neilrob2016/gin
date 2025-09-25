#include "globals.h"

void handsDeal(void)
{
	int i;

	colprintf("~FYDealing hands...\n");
	bzero(hands,sizeof(hands));
	decktop = HAND_SIZE * 2;
	for(i=0;i < HAND_SIZE;++i)
	{
		hands[USER][i] = deck[i];
		hands[COMPUTER][i] = deck[i+HAND_SIZE];
	}
}



void handPrint(t_card *hand, bool print_next)
{
	SPEECH_OFF();
	colprintf("\n~BB~FY 0  1  2  3  4  5  6  7  8  9  \n");
	colprintf("~BW~FT -- -- -- -- -- -- -- -- -- -- \n");
	colprintf("~BW~FT|");
	for(int i=0;i < HAND_SIZE;++i)
	{
		if (i && !(i % NUM_TYPES)) putchar('\n');
		colprintf("%s~BW~FT|",cardString(hand[i]));
	}
	colprintf("~RS value = ~FY%d~RS ",handGetValue(hand));
	if (print_next)
	{
		if (decktop < max_decktop) 
			colprintf(", next card = %s",cardString(deck[decktop]));
		else 
			printf(", no further cards.");
	}
	colprintf("\n~BW~FT -- -- -- -- -- -- -- -- -- -- \n\n");
	SPEECH_ON();
}



int handGetValue(t_card *hand)
{
	int val = 0;
	for(int i=0;i < HAND_SIZE;++i)
		if (VALID_CARD(hand[i])) val += typeval[hand[i].type];
	return val;
}



/*** Sort hand into sets, eg: 666. Selection sort is efficient for an array
     of size 10 (HAND_SIZE) ***/
void handSortSet(t_card *hand)
{
	t_card *card1;
	t_card *card2;
	t_card tmp;
	int minpos;
	int i;
	int j;

	for(i=0;i < HAND_SIZE-1;++i)
	{
		card1 = &hand[i];
		minpos = i;
		for(j=i+1;j < HAND_SIZE;++j)
		{
			card2 = &hand[j];
			if (card1->type > card2->type)
			{
				minpos = j;
				card1 = &hand[j];
			}
		}
		if (minpos != i)
		{
			tmp = hand[i];
			hand[i] = hand[minpos];
			hand[minpos] = tmp;
		}
	}
	// Want all the valid cards shifted to the left so they start at 
	// position 0 for later analysis.
	handShiftLeft(hand);
}



/*** Sort hand into runs, eg: Ah2h3h ***/
void handSortRun(t_card *hand)
{
	t_card *card1;
	t_card *card2;
	t_card tmp;
	int minpos;
	int i;
	int j;

	for(i=0;i < HAND_SIZE-1;++i)
	{
		card1 = &hand[i];
		minpos = i;
		for(j=i+1;j < HAND_SIZE;++j)
		{
			card2 = &hand[j];
			if (card1->suit > card2->suit || 
			    (card1->suit == card2->suit && card1->type > card2->type))
			{
				minpos = j;
				card1 = &hand[j];
			}
		}
		if (minpos != i)
		{
			tmp = hand[i];
			hand[i] = hand[minpos];
			hand[minpos] = tmp;
		}
	}
	handShiftLeft(hand);
}



/*** Move all valid cards to the left if there's room ***/
int handShiftLeft(t_card *hand)
{
	int to = 0;
	int moved = 0;
	for(int from=0;from < HAND_SIZE;++from)
	{
		if (VALID_CARD(hand[to])) ++to;
		else if (VALID_CARD(hand[from]))
		{
			hand[to] = hand[from];
			hand[from] = invalid_card;
			++moved;
			++to;
		}
	}
	return moved;
}



int handGetCardCount(t_card *hand)
{
	int cnt = 0;
	for(int i=0;i < HAND_SIZE;++i)
		if (VALID_CARD(hand[i])) ++cnt;
	return cnt;
}



void handCopy(t_card *from, t_card *to)
{
	memcpy(to,from,sizeof(t_card) * HAND_SIZE);
}
