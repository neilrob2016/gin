#include "globals.h"


void deckInit(void)
{
	int i;
	int j;
	int pos = 0;

	colprintf("\n~FY~LIShuffling deck...\n");
	for(i=1;i < NUM_SUITS;++i)
	{
		for(int j=1;j < NUM_TYPES;++j,++pos)
		{
			deck[pos].suit = i;
			deck[pos].type = j;
		}
	}

	t_card tmp;

	// Shuffle deck using Fisher-Yates algo.
	for(i=1;i <= DECK_SIZE;++i) 	
	{
		tmp = deck[i-1];
		j = random() % i;
		deck[i-1] = deck[j];
		deck[j] = tmp;
	}
}



void deckPrint(void)
{
	SPEECH_OFF();
	colprintf("\n~BB~FGDeck:\n\n");
	for(int i=0;i < DECK_SIZE;++i)
	{
		if (i && !(i % 10)) putchar('\n');
		colprintf("%s ",cardString(deck[i]));
	}
	puts("\n");
	SPEECH_ON();
}
