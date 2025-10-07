#include "globals.h"

// Buffer only needs to be big enough to hold M command with all cards 0 to 9
#define BUFFSIZE 15
#define ONOFF(F) (F ? "~FGON" : "~FROFF")

static char buff[BUFFSIZE+1];
static int bufflen;

int  userInput(void);
void userHelp(void);
bool userExchange(void);
bool userKnock(void);
void userNextCard(void);
void userMeld(void);
int  userGetPosition(char c);
void userPrintSeen(void);

bool userFirst(void)
{	
	while(true)
	{
		colprintf("\n~FTWould you like to go first (y/n)?:~RS ");
		fflush(stdout);
		if (userInput() == 1)
		{
			if (toupper(buff[0]) == 'Y') return true;
			if (toupper(buff[0]) == 'N') return false;
		}
	}
	return false;
}



/*** Returns true is a restart is requested ***/
bool userMove(void)
{
	bool check_knock = true;
	int cnt;

	flags.user_next = 1;

	if (knock_player == COMPUTER)
		colprintf("\n~FR~LIThe computer has knocked. You must meld as many cards as you can.\n");
	SPEECH_OFF();
	colprintf("\n~FYYour hand:\n");
	handPrint(ply[USER].hand,true);

	// Store them in case we switch to self play mode with 'p' 
	playerAddSeen(USER,deck[decktop]);

	while(true)
	{
		SPEECH_OFF();
		colprintf("~FTYour move (h for help):~RS ");
		fflush(stdout);
		if (!userInput()) continue;
		SPEECH_ON();

		char c = toupper(buff[0]);

		// Only the E and M commands take arguments
		if (bufflen > 1 && strchr("ADHIKNPQRSTVXYCFGLOW",c))
		{
			errprintf("The '%c' command takes no arguments.\n",c);
			continue;
		}

		switch(c)
		{
		case 'A':
			if (bufflen != 1) break;
			SPEECH_OFF();
			colprintf("\n~FYYour hand:\n");
			handPrint(ply[USER].hand,true);
			continue;
		case 'C':
			flags.colour_save = !flags.colour;
			flags.colour = 1;
			colprintf("Colour: %s\n",ONOFF(flags.colour_save));
			flags.colour = flags.colour_save;
			continue;
		case 'D':
			// Check to see if user can knock
			if (check_knock && 
			    (cnt = handGetValue(ply[USER].hand)) <= min_knock_val)
			{
				colprintf("~BM~FWNote:~RS Your current hand value is ~FY~LI%d~RS, you can knock! Press 'D' again to ignore.\n",cnt);
				check_knock = false;
				continue;
			}
			return false;
		case 'E':
			if (userExchange())
			{
				if (knock_player == COMPUTER)
					colprintf("~FY~LIYou must meld.\n\n");
				else
					return false;
			}
			continue;
		case 'F':
			colprintf("Laying off      : %s\n",ONOFF(flags.layoff));
			colprintf("Gin after layoff: %s\n",ONOFF(flags.layoff_gin));
			colprintf("Comp melds ASAP : %s\n",ONOFF(flags.meld_asap));
			colprintf("Colour          : %s\n",ONOFF(flags.colour));
			colprintf("Debug mode      : %s\n",ONOFF(flags.debug));
			continue;
		case 'G':
			flags.debug = !flags.debug;
			colprintf("Debug mode: %s\n",ONOFF(flags.debug));
			continue;
		case 'H':
			userHelp();
			continue;
		case 'K':
			if (userKnock()) return false;
			continue;
		case 'L':
			flags.layoff = !flags.layoff;
			colprintf("Laying off: %s\n",ONOFF(flags.layoff));
			continue;
		case 'M':
			userMeld();
			continue;
		case 'N':
			// Can only do it once per round
			if (flags.user_next)
			{
				userNextCard();
				flags.user_next = 0;
			}
			else errprintf("You only have one next card and only if you haven't melded this move.\n");
			continue;
		case 'O':
			flags.layoff_gin = !flags.layoff_gin;
			colprintf("Going Gin after layoff: %s\n",
				ONOFF(flags.layoff_gin));
			continue;
		case 'P':
			colprintf("\n~FMSwitching to self play mode...\n");
			flags.self_play = 1;
			return false;
		case 'Q':
			colprintf("\n~BM~FW*** ~LIGoodbye~RS~BM~FW ***\n\n");
			exit(0);
		case 'R':
			colprintf("\n~FYHand sorted into runs:\n");
			handSortRun(ply[USER].hand);
			handPrint(ply[USER].hand,true);
			continue;
		case 'S':
			colprintf("\n~FYHand sorted into sets:\n");
			handSortSet(ply[USER].hand);
			handPrint(ply[USER].hand,true);
			continue;
		case 'T':
			if ((cnt = handShiftLeft(ply[USER].hand)))
			{
				colprintf("~FY%d~RS cards moved.\n",cnt);
				handPrint(ply[USER].hand,true);
			}
			else errprintf("Cannot shift any cards left.\n");
			continue;
		case 'U':
			flags.suggest = 1;
			computerMove(USER);
			flags.suggest = 0;
			continue;
		case 'V':
			version();
			continue;
		case 'W':
			flags.meld_asap = !flags.meld_asap;
			colprintf("Computer melds ASAP: %s\n",
				ONOFF(flags.meld_asap));
			continue;
		case 'X':
			colprintf("\n\n~BM~FW***~LI RESTART~RS~BM~FW ***\n\n");
			return true;
		case 'Y':
			// Only allow in debug mode otherwise its cheating
			if (flags.debug)
			{
				userPrintSeen();
				continue;
			}
			break;
		case 'Z':
			if (flags.debug)
			{
				deckPrint();
				continue;
			}
			break;
		}
		errprintf("Unknown command. Type 'H' for help.\n");
	}
	return false;
}



void userHelp(void)
{
	SPEECH_OFF();
	colprintf("\n~FGArgument commands:\n");
	colprintf("   ~FTE<pos>~RS                         : Exchange the card at this position for the\n");
	puts("                                    deck card and then its the computers turn.");
	colprintf("   ~FTE<pos1><pos2>~RS                  : Exchange cards at these positions to arrange\n");
	puts("                                    your hand.");
	colprintf("   ~FTM<pos1><pos2><pos3>[<pos> * N]~RS : Meld the cards at the given positions.\n");
	colprintf("\n~FMSimple commands:\n");
	colprintf("   ~FTA~RS : Display hand.\n");
	colprintf("   ~FTD~RS : You don't want to exchange any cards, hand back to the computer.\n");
	colprintf("   ~FTH~RS : This help.\n");
	colprintf("   ~FTK~RS : Knock.\n");
	colprintf("   ~FTN~RS : Next card.\n");
	colprintf("   ~FTP~RS : Switch to self play mode.\n");
	colprintf("   ~FTQ~RS : Quit.\n");
	colprintf("   ~FTR~RS : Sort hand into runs.\n");
	colprintf("   ~FTS~RS : Sort hand into sets.\n");
	colprintf("   ~FTT~RS : Tidy up hand by shifting left to remove any gaps after melding.\n");
	colprintf("   ~FTU~RS : The computer will suggest a move for you.\n");
	colprintf("   ~FTV~RS : Print version info.\n");
	colprintf("   ~FTX~RS : Clear scores and restart match.\n");
	colprintf("\n~FYFlag commands:\n");
	colprintf("   ~FTC~RS : Toggle colour.\n");
	colprintf("   ~FTF~RS : Show togglable flags.\n");
	colprintf("   ~FTG~RS : Toggle debug mode.\n");
	colprintf("   ~FTL~RS : Toggle laying off.\n");
	colprintf("   ~FTO~RS : Toggle Going Gin after laying off.\n");
	colprintf("   ~FTW~RS : Toggle computer melding ASAP instead of waiting until it can knock.\n");
	if (flags.debug)
	{
		colprintf("\n~FRDebug commands:\n");
		colprintf("   ~FTY~RS : Print seen cards.\n");
		colprintf("   ~FTZ~RS : Print deck.\n");
	}
	putchar('\n');
	SPEECH_ON();
}



int userInput(void)
{
	char c;

	for(bufflen = 0;;)
	{
		if ((c = getchar()) == EOF)
		{
			errprintf("EOF on stdin.\n");
			exit(1);
		}
		if (c == '\n') break;
		if (bufflen < BUFFSIZE && !isspace(c)) buff[bufflen++] = c;
	}
	buff[bufflen] = 0;
	return bufflen;
}



/*** Exchange the card at the given position for the next card or simply
     swap 2 cards in the hand. Returns true if user exchanged a card. ***/
bool userExchange(void)
{
	switch(bufflen)
	{
	case 2:
		if (decktop == max_decktop)
		{
			errprintf("There are no further cards.\n");
			return false;
		}
		break;
	case 3:
		break;
	default:
		puts("Usage: E<pos1>[<pos2]>");
		return false;
	}
	int pos1 = userGetPosition(buff[1]);
	if (pos1 == -1) goto INVALID;

	t_card tmp = ply[USER].hand[pos1];
	if (bufflen == 3)
	{
		// Swap 2 cards in the hand
		int pos2 = userGetPosition(buff[2]);
		if (pos2 == -1) goto INVALID;
		if (pos1 == pos2)
		{
			errprintf("Hand positions cannot be the same.\n");
			return false;
		}
		ply[USER].hand[pos1] = ply[USER].hand[pos2];
		ply[USER].hand[pos2] = tmp;
		colprintf("\n~FYSwapped cards %s ~FYand %s:\n",
			cardString(tmp),cardString(ply[USER].hand[pos1]));
		handPrint(ply[USER].hand,true);
		return false;
	}

	// Exchange with the given card location in the hand
	if (!VALID_CARD(ply[USER].hand[pos1]))
	{
		errprintf("You cannot exchange into a vacant position.\n");
		return false;
	}
	ply[USER].hand[pos1] = deck[decktop];
	deck[decktop] = tmp;
	colprintf("\n~FYExchanged card %s ~FYfor %s:\n",
		cardString(tmp),cardString(ply[USER].hand[pos1]));
	handPrint(ply[USER].hand,false);
	return true;

	INVALID:
	errprintf("Invalid position '%c'.\n",buff[1]);
	return false;
}



bool userKnock(void)
{
	if (knock_player == COMPUTER)
	{
		errprintf("The computer has already knocked!\n");
		return false;
	}
	int val = handGetValue(ply[USER].hand);
	if (val > min_knock_val)
	{
		errprintf("The value of your hand must be <= %d to knock. It is currently %d.\n",
			min_knock_val,val);
		return false;
	}
	playerKnock(USER);
	return true;
}



void userNextCard(void)
{
	// Don't increment just yet as we want the final card to remain
	// displayed with the hand.
	if (decktop + 1 >= max_decktop)
	{
		errprintf("There are no further cards.\n");
		return;
	}
	sayprintf("next card");
	++decktop;
	playerAddSeen(USER,deck[decktop]);
	handPrint(ply[USER].hand,true);
}



/*** Returns true if user has won the game ***/
void userMeld(void)
{
	if (bufflen < 4)
	{
		puts("Usage: M<pos1><pos2><pos3>[<pos> * N]");
		return;
	}
	if (bufflen > HAND_SIZE + 1)
	{
		errprintf("Too many positions.\n");
		return;
	}

	struct st_player *pu = &ply[USER]; // Makes code easier to read
	t_card melded[HAND_SIZE];
	bool set;
	int meldpos[HAND_SIZE];
	int mpos = 0;
	int hpos = 0;
	int pos;
	int i;
	int j;

	// Initialise list of melded cards to check
	bzero(melded,sizeof(melded));

	for(i=1;i < bufflen;++i)
	{
		if ((pos = userGetPosition(buff[i])) == -1)
		{
			errprintf("Invalid position '%c'.\n",buff[i]);
			return;
		}
		if (!VALID_CARD(pu->hand[pos]))
		{
			errprintf("No card at position %d.\n",pos);
			return;
		}
		melded[hpos++] = pu->hand[pos];
		meldpos[mpos++] = pos;
	}

	// Sanity check positions for duplicates
	for(i=0;i < mpos-1;++i)
	{
		for(j=i+1;j < mpos;++j)
		{
			if (meldpos[i] == meldpos[j])
			{
				errprintf("Duplicate position '%d'\n",meldpos[i]);
				return;
			}
		}
	}

	// Check for set, eg: 666
	for(i=1;i < hpos && melded[i].type == melded[i-1].type;++i);

	if (i < hpos)
	{
		set = false;
		// Not a set. Check if run, eg 3h4h5h
		handSortRun(melded);
		for(i=1;i < hpos && 
			melded[i].suit == melded[i-1].suit &&
			melded[i].type == melded[i-1].type + 1;++i);
		if (i < hpos)
		{
			errprintf("Not a valid meld.\n");
			return;
		}
	}
	else set = true;

	// Remove melded cards from users hand
	for(i=0;i < mpos;++i) 
	{
		if (set)
			pu->meldsets[pu->meldsetscnt++] = pu->hand[meldpos[i]];
		else
			pu->meldruns[pu->meldrunscnt++] = pu->hand[meldpos[i]];
		pu->hand[meldpos[i]] = invalid_card;
	}
	colprintf("\n~FYMelded.\n");
	dbgprintf(USER,"Has %d stored melded cards.\n",
		pu->meldsetscnt + pu->meldrunscnt);
	handPrint(pu->hand,true);
}



int userGetPosition(char c)
{
	c -= '0';
	return (c < 0 || c >= HAND_SIZE) ? -1 : (int)c;
}



void userPrintSeen(void)
{
	colprintf("\n~BB~FWCards you have seen:\n\n");
	SPEECH_OFF();
	for(int i=0;i < ply[USER].seencnt;++i)
	{
		if (i && !(i % 10)) putchar('\n');
		colprintf("%s ",cardString(ply[USER].seencards[i]));
	}
	puts("\n");
	SPEECH_ON();
}
