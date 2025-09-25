#define MAINFILE
#include "globals.h"

#define MATCH_WIN_VAL 100
#define MIN_KNOCK_VAL 10
#define DOUBLE_ADJUST_MULT 0.75

void parseCmdLine(int argc, char **argv);
void version(void);
void init(void);
void mainloop(void);
void endOfGame(void);
void sighandler(int sig);

int main(int argc, char **argv)
{
	parseCmdLine(argc,argv);
	version();
	init();
	mainloop();
	return 0;
}



void parseCmdLine(int argc, char **argv)
{
	bzero(&flags,sizeof(flags));
	flags.colour = 1;
	flags.prompt = 1;
	flags.layoff = 1;
	flags.layoff_gin = 1;
#ifdef __APPLE__
	flags.wait_for_speech = 1;
	voice = NULL;
	speech_rate = SPEECH_RATE;
#endif
	float play_delay = PLAY_DELAY;
	double_adjust_mult = DOUBLE_ADJUST_MULT;
	min_knock_val = MIN_KNOCK_VAL;

	for(int i=1;i < argc;++i)
	{
		if (argv[i][0] != '-' || strlen(argv[i]) != 2) goto USAGE;

		char c = argv[i][1];
		switch(c)
		{
#ifdef __APPLE__
		case 's':
			flags.speech = 1;
			continue;
		case 'w':
			flags.wait_for_speech = 0;
			flags.speech = 1;
			continue;
#endif
		case 'c':
			flags.colour = 0;
			continue;
		case 'd':
			flags.debug = 1;
			continue;
		case 'i':
			flags.meld_asap = 1;
			continue;
		case 'g':
			flags.layoff_gin = 0;
			continue;
		case 'l':
			flags.layoff = 0;
			continue;
		case 'p':
			flags.self_play = 1;
			continue;
		case 't':
			flags.prompt = 0;
			continue;
		case 'v':
			// Delay printing version to parse rest of cmd line so
			// user can switch colour off first.
			flags.version = 1;
			continue;
		}
		if (++i == argc) goto USAGE;
#ifdef __APPLE__
		char tmp[10];
		int val;
#endif
		switch(c)
		{
#ifdef __APPLE__
		case 'r':
			if ((val = atoi(argv[i])) < 1)
			{
				errprintf("Invalid speech rate. Must be > 0.\n");
				exit(1);
			}
			snprintf(tmp,sizeof(tmp),"%d",val);
			speech_rate = strdup(tmp);
			flags.speech = 1;
			break;
		case 'o':
			voice = argv[i];
			flags.speech = 1;
			break;
#endif
		case 'a':
			double_adjust_mult = atof(argv[i]);
			if (double_adjust_mult < 0 || double_adjust_mult > 1)
			{
				errprintf("Double adjust value must be from 0 to 1.\n");
				exit(1);
			}
			break;
		case 'k':
			min_knock_val = atoi(argv[i]);
			if (min_knock_val < 0 || min_knock_val >= MATCH_WIN_VAL)
			{
				errprintf("Invalid min knock value. Must be 0 to %d.\n",
					MATCH_WIN_VAL-1);
				exit(1);
			}
			break;
		case 'y':
			if ((play_delay = atof(argv[i])) < 0)
			{
				errprintf("Invalid play delay. Must be > 0.\n");
				exit(1);
			}
			break;
		default:
			goto USAGE;
		}
	}
	if (flags.version)
	{
		version();
		exit(0);
	}
	flags.speech_orig = flags.speech;
	flags.self_play_orig = flags.self_play;
	play_delay_usec = (int)ceil(play_delay * 1000000);
	return;

	USAGE:
	printf("Usage: %s\n"
#ifdef __APPLE__
	       "       -r <rate>  : Speech rate. Default = %s.\n"
	       "       -o <voice> : Speech voice. Default = system voice.\n"
	       "       -s         : Do speech.\n"
	       "       -w         : Don't wait for speech to finish before continuing.\n"
#endif
	       "       -a <0-1>   : Double adjust multiplication value for when the computer\n"
	       "                    calculates potential hand values. Default = %.2f\n"
	       "       -k <0-100> : Mininum knock value. Default = %d\n"
	       "       -y <secs>  : Self play delay between moves. Default = %.1f\n"
	       "       -c         : No colour.\n"
	       "       -d         : Debug.\n"
	       "       -i         : Make the computer meld as soon as possible rather than\n"
	       "                    waiting until it can knock. This will often prevent it\n"
	       "                    having runs and sets longer than 3 cards.\n"
	       "       -g         : Don't allow Going Gin after layoff.\n"
	       "       -l         : Don't layoff cards against knockers melds at end.\n"
	       "       -p         : Self play.\n"
	       "       -t         : Don't prompt to continue after each self play game.\n"
	       "       -v         : Show version then exit.\n"
#ifdef __APPLE__
	       "Note: -a, -o and -r imply -s.\n",
		argv[0],
		SPEECH_RATE,DOUBLE_ADJUST_MULT,MIN_KNOCK_VAL,PLAY_DELAY);
#else
		,argv[0],
		DOUBLE_ADJUST_MULT,MIN_KNOCK_VAL,PLAY_DELAY);
#endif
	exit(1);
}



void version(void)
{
	SPEECH_OFF();
        colprintf("\n~BM*** Gin Rummy ***\n\n");
	colprintf("~BB~FWCopyright (C) Neil Robertson 2025\n\n");
        colprintf("~FYVersion~RS   : %s\n",VERSION);
#ifdef __APPLE__
	colprintf("~FTBuild~RS     : MacOS\n");
#else
	colprintf("~FTBuild~RS     : Generic\n");
#endif
        colprintf("~FGBuild date~RS: %s, %s\n\n",__DATE__,__TIME__);
	SPEECH_ON();
}



void init(void)
{
	srandom(time(0));
	bzero(score,sizeof(score));
	signal(SIGINT,sighandler);
	signal(SIGQUIT,sighandler);
	signal(SIGTERM,sighandler);
#ifdef __APPLE__
	// Stop speech processes becoming zombies
	if (flags.speech && flags.wait_for_speech) signal(SIGCHLD, SIG_IGN);
#endif
}



void mainloop(void)
{
	int moves;
	int player;
	int prev_decktop;
	int first_player;
	int self_play_start_player = USER;

	for(int game=1;;++game)
	{
		deckInit();
		if (PRINT_INFO()) deckPrint();

		colprintf("\n~BM~FW---{ Game ~FG%d~FW }---\n\n",game);
		handsDeal();
		playersInit();

		if (game == 1)
		{
			if (flags.self_play)
				player = self_play_start_player;
			else if (userFirst()) player = USER;
			else
			{
				pcolprintf(COMPUTER," goes first...\n");
				player = COMPUTER;
			}
		}
		else
		{
			// Different players start at each game
			player = !first_player;
			pcolprintf(player," goes first...\n");
		}

		first_player = player;
		max_decktop = DECK_SIZE;
		prev_decktop = decktop;
		flags.end_of_game = 0;
		flags.store_melds = 0;
		flags.show_meld = 0;

		// Loop until someone wins
		for(moves=1;;++moves)
		{
			colprintf("\n~BB~FWMove %d:\n",moves);
			dbgprintf(NO_PLY,"decktop = %d, max_decktop = %d\n",
				decktop,max_decktop);
			assert(decktop <= max_decktop);

			if (flags.self_play) computerMove(player);
			else if (player == USER)
				userMove();
			else
				computerMove(COMPUTER);

			// If neither user went for the next card then increment
			// the decktop.
			if (!(moves % 2))
			{
 				if (decktop == prev_decktop) ++decktop;
				prev_decktop = decktop;
			}

			// See if anyone has knocked
			if (knock_player != NO_PLY || decktop == max_decktop)
			{
				if (flags.end_of_game) break;
				max_decktop = decktop;
				flags.end_of_game = true;

				/* After a knock we do one more move to let
				   the other player meld if possible. If the
				   1st player knocked let the 2nd draw one more
				   card so all players were dealt the same num 
				   of cards */
				if (knock_player == first_player) ++max_decktop;
			}
			player = !player;
			if (flags.self_play && play_delay_usec) 
				usleep(play_delay_usec);
		}
		colprintf("\n~BM~FW---{ Game ~FG%d~FW complete in ~FY%d~FW moves }---\n\n",
			game,moves);
		endOfGame();

		// Alternate which user starts each game
		self_play_start_player = !self_play_start_player;

		if (flags.prompt)
		{
			colprintf("\n~FTPress return for next game... ");
			fflush(stdout);
			getchar();
			// If user used 'p' command make sure we go back to
			// user prompt for new game.
			flags.self_play = flags.self_play_orig;
		}
	}
}



/*** Do layoffs, calculate scores and show winner ***/
void endOfGame(void)
{
	char *pname[2];
	char col[2];
	int val[2];
	int winner;
	int i;
	int j;

	if (flags.debug && flags.layoff)
	{
		for(i=0;i < NUM_PLAYERS;++i)
		{
			dbgprintf(i,"Melded set cards: ");
			SPEECH_OFF();
			for(j=0;j < meldsetscnt[i];++j)
				colprintf("%s ",cardString(meldsets[i][j]));
			putchar('\n');
			SPEECH_ON();
			dbgprintf(i,"Melded run cards: ");
			SPEECH_OFF();
			for(j=0;j < meldrunscnt[i];++j)
				colprintf("%s ",cardString(meldruns[i][j]));
			SPEECH_ON();
			putchar('\n');
		}
	}

	if (flags.layoff && knock_player != NO_PLY) 
		playerLayOff(!knock_player);

	for(i=0;i < NUM_PLAYERS;++i)
	{
		val[i] = handGetValue(hands[i]);
		pname[i] = (char *)player_name[flags.self_play][i];
		col[i] = player_col[i];
	}
	if (!flags.self_play)
	{
		pcolprintf(COMPUTER,"'s final hand:\n");
		handPrint(hands[COMPUTER],false);
	}

	colprintf("~BY~FMFinal hand values:\n");
	for(i=0;i < NUM_PLAYERS;++i)
		colprintf("   ~F%c%-8s:~RS %d\n",col[i],pname[i],val[i]);

	// Hand with the lowest score wins, so if player 0 hand is > than 
	// player 1 then winner is set to 1 and vice versa.
	if (val[0] == val[1]) colprintf("\n~BR~FW*** Game drawn ***\n\n");
	else
	{
		winner = (val[USER] > val[COMPUTER]) ? COMPUTER : USER;

		// See if player who didn't knock has stolen the game via 
		// layoffs.
		if (knock_player != NO_PLY && winner != knock_player)
		{
			if (flags.layoff_gin && !val[winner]) playerGin(winner);
			colprintf("\n~BM~FW~LI***~B%c %s steals the win! ~BM***\n\n",
				col[winner],pname[winner]);
		}
		else
		{
			if (!val[winner]) playerGin(winner);
			colprintf("\n~B%c~FW*** %s wins the game! ***\n\n",
				col[winner],pname[winner]);
		}
		score[winner] += (val[!winner] - val[winner]);
		if (gin_player != NO_PLY) score[winner] += GIN_ADD;
	}

	colprintf("~BM~FWCurrent scores:\n");
	for(i=0;i < NUM_PLAYERS;++i)
	{
		colprintf("   ~F%c%-8s:~RS %d ",col[i],pname[i],score[i]);
		if (gin_player == i) 
			colprintf("(~FY~LI+%d~RS for ~FRGoing Gin~RS)",GIN_ADD);
		putchar('\n');
	}

	// First player to reach 100 wins the match
	for(i=0;i < NUM_PLAYERS;++i)
	{
		if (score[i] >= MATCH_WIN_VAL)
		{
			colprintf("\n~BR~FW~LI<<<~B%c %s wins the match! ~BR>>>\n\n",
				player_col[i],player_name[flags.self_play][i]);
			exit(0);
		}
	}
}



/*** This is only so we can print ~RS to return terminal colours to normal.
     colprintf() does this silently at newlines. ***/
void sighandler(int sig)
{
	SPEECH_OFF();
	colprintf("\n\n~BR~FW~LI*** Exiting on signal %d ***\n\n",sig);
	exit(sig);
}
