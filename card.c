#include "globals.h"

#define SUITSTR " csdh"
#define TYPESTR " A23456789TJQK"
// No suit = white, clubs = blue, spades = black, diamonds = magenta, 
// hearts = red
#define SUITCOL "WBKMR"

static char *suitname[NUM_SUITS] =
{
	"","clubs","spades","diamonds","hearts"
};


static char *typename[NUM_TYPES] =
{
	"","ace","2","3","4","5","6","7","8","9","10","jack","queen","king"
};



char *cardString(t_card card)
{
	// Have array of 2 so we can call this twice inside a single printf
	static char cstr[2][13];
	static int index = 0;
	snprintf(cstr[index],12,"~BW~F%c%c%c~RS",
		SUITCOL[card.suit],TYPESTR[card.type],SUITSTR[card.suit]);
	index = !index;
	return cstr[!index];
}



char *cardGetName(t_card card)
{
	static char name[20];
	snprintf(name,sizeof(name),"%s of %s",
		typename[card.type],suitname[card.suit]);
	return name;
}



char *cardGetNameFromChars(char suit, char type, int *len)
{
	static char name[20];
	int p1 = (int)(strchr(TYPESTR,type) - TYPESTR);
	int p2 = (int)(strchr(SUITSTR,suit) - SUITSTR);
	if (p1 > 0 && p2 > 0)
	{
		*len = snprintf(name,sizeof(name)," %s of %s ",
			typename[p1],suitname[p2]);
	}
	else *len = 0;
	return name;	
}
