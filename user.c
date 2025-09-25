#include "globals.h"

#define BUFFSIZE 100
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



void userMove(void)
{
	bool next = true;
	int cnt;

	if (knock_player == COMPUTER)
		colprintf("\n~FY~LIThe computer has knocked. You must meld as many cards as you can.\n");
	SPEECH_OFF();
	colprintf("\n~FYYour hand:\n");
	handPrint(hands[USER],true);

	while(true)
	{
		// Store them in case we switch to self play mode with 'p' 
		playerPushSeen(USER,deck[decktop]);

		SPEECH_OFF();
		colprintf("~FTYour move (h for help):~RS ");
		fflush(stdout);
		if (!userInput()) continue;
		SPEECH_ON();

		char c = toupper(buff[0]);
		if (bufflen > 1 && strchr("CDHKNQRS",c))
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
			SPEECH_ON();
			handPrint(hands[USER],true);
			continue;
		case 'C':
			flags.colour_save = !flags.colour;
			flags.colour = 1;
			colprintf("Colour: %s\n",ONOFF(flags.colour_save));
			flags.colour = flags.colour_save;
			continue;
		case 'D':
			return;
		case 'E':
			if (userExchange())
			{
				if (knock_player == COMPUTER)
					colprintf("~FY~LIYou must meld.\n\n");
				else
					return;
			}
			continue;
		case 'F':
			colprintf("Laying off     : %s\n",ONOFF(flags.layoff));
			colprintf("Comp melds ASAP: %s\n",ONOFF(flags.meld_asap));
			colprintf("Colour         : %s\n",ONOFF(flags.colour));
			continue;
		case 'H':
			userHelp();
			continue;
		case 'I':
			flags.meld_asap = !flags.meld_asap;
			colprintf("Computer melds ASAP: %s\n",
				ONOFF(flags.meld_asap));
			continue;
		case 'K':
			if (userKnock()) return;
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
			if (next)
			{
				userNextCard();
				next = false;
			}
			else errprintf("You only have one next card and only if you haven't melded this move.\n");
			continue;
		case 'P':
			colprintf("\n~FMSwitching to self play mode...\n");
			flags.self_play = 1;
			return;
		case 'Q':
			colprintf("\n~BM~FW*** ~LIGoodbye~RS~BM~FW ***\n\n");
			exit(0);
		case 'R':
			SPEECH_OFF();
			colprintf("\n~FYHand sorted into runs:\n");
			handSortRun(hands[USER]);
			handPrint(hands[USER],true);
			SPEECH_ON();
			continue;
		case 'S':
			SPEECH_OFF();
			colprintf("\n~FYHand sorted into sets:\n");
			handSortSet(hands[USER]);
			handPrint(hands[USER],true);
			SPEECH_ON();
			continue;
		case 'T':
			if ((cnt = handShiftLeft(hands[USER])))
			{
				printf("%d cards moved.\n",cnt);
				SPEECH_OFF();
				handPrint(hands[USER],true);
				SPEECH_ON();
			}
			else puts("Cannot shift any cards left.");
			continue;
		case 'V':
			version();
			continue;
		}
		errprintf("Unknown command. Type 'H' for help.\n");
	}
}



void userHelp(void)
{
	SPEECH_OFF();
	colprintf("\n~FGArgument commands:\n");
	colprintf("   ~FTE<pos>~RS                         : Exchange the card at this position for the\n");
	puts("                                    next card and then its the computers turn.");
	colprintf("   ~FTE<pos1><pos2>~RS                  : Exchange cards at these positions to arrange\n");
	puts("                                    your hand.");
	colprintf("   ~FTM<pos1><pos2><pos3>[<pos> * N]~RS : Meld the cards at the given positions.\n");
	colprintf("\n~FMSimple commands:\n");
	colprintf("   ~FTA~RS : Display hand.\n");
	colprintf("   ~FTC~RS : Toggle colour.\n");
	colprintf("   ~FTD~RS : You don't want to exchange any cards, hand back to the computer.\n");
	colprintf("   ~FTF~RS : Show togglable flags.\n");
	colprintf("   ~FTH~RS : This help.\n");
	colprintf("   ~FTI~RS : Toggle computer melding ASAP instead of waiting until it can knock.\n");
	colprintf("   ~FTK~RS : Knock.\n");
	colprintf("   ~FTL~RS : Toggle laying off.\n");
	colprintf("   ~FTN~RS : Next card.\n");
	colprintf("   ~FTP~RS : Switch to self play mode.\n");
	colprintf("   ~FTQ~RS : Quit.\n");
	colprintf("   ~FTR~RS : Sort hand into runs.\n");
	colprintf("   ~FTS~RS : Sort hand into sets.\n");
	colprintf("   ~FTT~RS : Tidy up hand by shifting left to remove any gaps after melding.\n");
	colprintf("   ~FTV~RS : Print version info.\n");
	colprintf("\n~FYNote: Card positions go from 0 to 9, left to right.\n\n");
	SPEECH_ON();
}



int userInput(void)
{
	if ((bufflen = read(STDIN_FILENO,buff,BUFFSIZE)) < 1)
	{
		perror("ERROR: read()");
		exit(1);
	}
	if (buff[bufflen-1] == '\n') --bufflen;
	buff[bufflen] = 0;	
	return bufflen;
}



/*** Exchange the card at the given position for the next card or simply
     swap 2 cards in the hand ***/
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

	t_card tmp = hands[USER][pos1];
	if (bufflen == 3)
	{
		int pos2 = userGetPosition(buff[2]);
		if (pos2 == -1) goto INVALID;
		if (pos1 == pos2)
		{
			errprintf("Hand positions cannot be the same.\n");
			return false;
		}
		hands[USER][pos1] = hands[USER][pos2];
		hands[USER][pos2] = tmp;
		handPrint(hands[USER],true);
	}
	else
	{
		if (!VALID_CARD(hands[USER][pos1]))
		{
			errprintf("You cannot exchange into a vacant position.\n");
			return false;
		}
		hands[USER][pos1] = deck[decktop];
		deck[decktop] = tmp;
		handPrint(hands[USER],false);
	}
	return (bufflen == 2);

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
	int val = handGetValue(hands[USER]);
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
	++decktop;
	handPrint(hands[USER],true);
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

	t_card melded[HAND_SIZE];
	t_card *uhand = hands[USER];
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
		if (!VALID_CARD(uhand[pos]))
		{
			errprintf("No card at position %d.\n",pos);
			return;
		}
		melded[hpos++] = uhand[pos];
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
			meldsets[USER][meldsetscnt[USER]++] = uhand[meldpos[i]];
		else
			meldruns[USER][meldrunscnt[USER]++] = uhand[meldpos[i]];
		uhand[meldpos[i]] = invalid_card;
	}
	colprintf("~FYMelded.\n");
	dbgprintf(USER,"Has %d stored melded cards.\n",
		meldsetscnt[USER] + meldrunscnt[USER]);
	handPrint(uhand,true);
}



int userGetPosition(char c)
{
	c -= '0';
	return (c < 0 || c >= HAND_SIZE) ? -1 : (int)c;
}
