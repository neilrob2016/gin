#ifndef __APPLE__
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#define VERSION "20250925"

enum en_suits
{
	NO_SUIT,
	CLUBS,
	SPADES,
	DIAMONDS,
	HEARTS,
	NUM_SUITS
};


enum en_types
{
	NO_TYPE,
	ACE,
	TWO,
	THREE,
	FOUR,
	FIVE,
	SIX,
	SEVEN,
	EIGHT,
	NINE,
	TEN,
	JACK,
	QUEEN,
	KING,
	NUM_TYPES
};

enum en_algo
{
	ALGO_MELD_SET_THEN_RUN,
	ALGO_MELD_RUN_THEN_SET
};

enum en_player
{
	NO_PLY = -1,
	USER,
	COMPUTER,
	NUM_PLAYERS
};

#define VALID_CARD(C)    (C.suit != NO_SUIT)
#define SAME_CARD(C1,C2) (C1.suit == C2.suit && C1.type == C2.type)
#define PRINT_INFO()     (flags.self_play || flags.debug)

#define DOUBLE_ADJUST_MULT 0.75

#define DECK_SIZE  52
#define HAND_SIZE  10
#define GIN_ADD    25
#define PLAY_DELAY 1.0

#define SPEECH_RATE  "180"
#define SPEECH_OFF() flags.speech = 0
#define SPEECH_ON()  flags.speech = flags.speech_orig

typedef struct
{
	uint8_t suit;
	uint8_t type;
} t_card;

typedef struct
{
	int value;
	int algo;
} t_handval;

typedef struct
{
	// Command line
	unsigned colour          : 1;
	unsigned speech          : 1;
	unsigned wait_for_speech : 1;
	unsigned self_play       : 1;
	unsigned self_play_orig  : 1;
	unsigned prompt          : 1;
	unsigned layoff          : 1;
	unsigned layoff_gin      : 1;
	unsigned debug           : 1;
	unsigned version         : 1;

	// Runtime
	unsigned colour_save : 1;
	unsigned speech_orig : 1;
	unsigned end_of_game : 1;
	unsigned show_meld   : 1;
	unsigned melded      : 1;
	unsigned meld_asap   : 1;
	unsigned store_melds : 1;
} t_flags;

// Globals
#ifdef MAINFILE
#define EXTERN 
t_card invalid_card = { NO_SUIT, NO_TYPE };
//                             - A,1,2,3,4,5,6,7,8,9, J, Q, K
int typeval[NUM_TYPES] = { 0,1,2,3,4,5,6,7,8,9,10,10,10,10 };
const char *player_name[2][NUM_PLAYERS] =
{
	{ "User", "Computer" }, // Normal play with user
	{ "Alice","Bob"  }      // Self play
};
const char *player_col = "TG";
#else
#define EXTERN extern
extern t_card invalid_card;
extern const char *player_name[2][NUM_PLAYERS];
extern int typeval[NUM_TYPES];
extern const char *player_col;
#endif

EXTERN t_card deck[DECK_SIZE];
EXTERN t_card hands[NUM_PLAYERS][HAND_SIZE];
EXTERN t_card seencards[NUM_PLAYERS][DECK_SIZE];
EXTERN t_card meldsets[NUM_PLAYERS][HAND_SIZE];
EXTERN t_card meldruns[NUM_PLAYERS][HAND_SIZE];
EXTERN int meldsetscnt[NUM_PLAYERS];
EXTERN int meldrunscnt[NUM_PLAYERS];
EXTERN int seencnt[NUM_PLAYERS];
EXTERN int score[NUM_PLAYERS];
EXTERN int decktop;
EXTERN int max_decktop;
EXTERN int knock_player;
EXTERN int gin_player;
EXTERN int play_delay_usec;
EXTERN float double_adjust_mult;

// Command line
EXTERN t_flags flags;
EXTERN int min_knock_val;
EXTERN char *speech_rate;
EXTERN char *voice;

// main.c
void version(void);

// deck.c
void deckInit(void);
void deckPrint(void);

// hand.c
void handsDeal(void);
void handSortSet(t_card *hand);
void handSortRun(t_card *hand);
void handPrint(t_card *hand, bool print_next);
int  handGetValue(t_card *hand);
int  handShiftLeft(t_card *hand);
int  handGetCardCount(t_card *hand);
void handCopy(t_card *from, t_card *to);

// card.c
char *cardString(t_card card);
char *cardGetName(char c, char prev, int *len);

// player.c
void playersInit(void);
void playerGin(int player);
void playerKnock(int player);
void playerLayOff(int player);
bool playerPushSeen(int player, t_card card);
void playerPopSeen(int player);
int  playerHasSeenCard(int player, t_card card);
int  playerHasSeenCardType(int player, int type);

// user.c
bool userFirst(void);
void userMove(void);

// computer.c
void computerMove(int player);

// printf.c
void errprintf(const char *fmt, ...);
void pcolprintf(int player, const char *fmt, ...);
void dbgprintf(int player, const char *fmt, ...);
void colprintf(const char *fmt, ...);
void sayprintf(const char *fmt, ...);
