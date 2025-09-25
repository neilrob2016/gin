#include "globals.h"

#define NUM_CODES  21
#define RESET_CODE 0

extern char **environ;


void errprintf(const char *fmt, ...)
{
	va_list args;
	char *text;

	va_start(args,fmt);
	vasprintf(&text,fmt,args);

	colprintf("\07~BR~FWERROR:~RS %s",text);
#ifdef __APPLE__
	sayprintf("error %s",text);
#endif
	va_end(args);
	free(text);
}



/*** Output string with foreground in player colours ***/
void pcolprintf(int player, const char *fmt, ...)
{
	va_list args;
	char *text;

	va_start(args,fmt);
	vasprintf(&text,fmt,args);

	colprintf("~F%c%s%s",
		player_col[player],
		player_name[flags.self_play][player],
		text);
	va_end(args);
	free(text);
}



void dbgprintf(int player, const char *fmt, ...)
{
	if (!flags.debug) return;
	va_list args;
	char *text;

	va_start(args,fmt);
	vasprintf(&text,fmt,args);

	SPEECH_OFF();
	if (player == NO_PLY) colprintf("~FYDEBUG:~RS %s",text);
	else
	{
		colprintf("~FYDEBUG:~RS %s: %s",
			player_name[flags.self_play][player],text);
	}
	SPEECH_ON();
	va_end(args);
	free(text);
}



/*** Print a string in colour if it has embedded colour commands ***/
void colprintf(const char *fmt, ...)
{
	// Static so we can re-use memory allocated on a previous call avoiding
	// an alloc-free.
	static char *newfmt = NULL;
	static char *output = NULL;
	static int fmt_alloc = 0;
	static int out_alloc = 0;
#ifdef __APPLE__
	static char *spchtext = NULL;
	static int spch_alloc = 0;
	int spchpos = 0;
	bool do_speech = flags.speech;
	char c;
#endif
	const char *ansitag[NUM_CODES] = 
	{
		"RS","OL","UL","LI","RV",

		"FK","FR","FG","FY",
		"FB","FM","FT","FW",

		"BK","BR","BG","BY",
		"BB","BM","BT","BW"
	};
	const char *ansicode[NUM_CODES] =
	{
		/* Non colour actions */
		"\033[0m", "\033[1m", "\033[4m", "\033[5m", "\033[7m",

		/* Foreground colour */
		"\033[30m","\033[31m","\033[32m","\033[33m",
		"\033[34m","\033[35m","\033[36m","\033[37m",

		/* Background colour */
		"\033[40m","\033[41m","\033[42m","\033[43m",
		"\033[44m","\033[45m","\033[46m","\033[47m"
	};
	va_list args;
	char *s1;	
	char *s2;
	int len;
	int print_len;
	int reset_len;
	int out_len;
	int i;

	if (!(print_len = strlen(fmt))) return;

	// Allocate space for printf formatted string 
	do
	{
		va_start(args,fmt);
		len = print_len * 2;
		if (len > fmt_alloc && !(newfmt = (char *)realloc(newfmt,len)))
		{
			perror("ERROR: colprintf(): realloc() 1");
			exit(1);
		}
		fmt_alloc = len;

		/* It returns how long a string would have been if enough 
		   space had been available */
		print_len = vsnprintf(newfmt,len,fmt,args);
		va_end(args);
	} while(print_len > len / 2);

	// Now allocate space for our output string. * 5 because we might have
	// to put a reset before a load of newlines. +1 for \0 */
	if (len > (out_alloc - 1) / 5)
	{
		out_alloc = len * 5 + 1;
		if (!(output = (char *)realloc(output,out_alloc)))
		{
			perror("ERROR: colprintf(): realloc() 2");
			exit(1);
		}
	}
	reset_len = strlen(ansicode[RESET_CODE]);
	out_len = 0;
#ifdef __APPLE__
	if (do_speech && spch_alloc < out_alloc)
	{
		spch_alloc = out_alloc;
		spchtext = (char *)realloc(spchtext,spch_alloc+1);
		assert(spchtext);
	}
#endif
	// Parse ~ tags copying the formatted string into output as we do so 
	for(s1=newfmt,s2=output;*s1;++s1,++s2)
	{
		// Put a reset before every newline since the terminal will
		// make a mess otherwise 
		if (*s1 == '\n' && flags.colour)
		{
			memcpy(s2,ansicode[RESET_CODE],reset_len);
			s2 += reset_len;
			out_len += reset_len;
		}

		*s2 = *s1;
		if (*s1 == '~')
		{
			// Find colour tag and replace it with terminal code 
			for(i=0;i < NUM_CODES;++i)
			{
				if (!strncmp(s1+1,ansitag[i],2))
				{
					s1 += 2;
					if (flags.colour)
					{
						len = strlen(ansicode[i]);
						memcpy(s2,ansicode[i],len);
						s2 += (len - 1);
						out_len += len;
					}
					else --s2;
					break;
				}
			}
			if (i == NUM_CODES) ++out_len;
			continue;
		}
		++out_len;
#ifdef __APPLE__
		if (!do_speech) continue;

		/* Remove most punctuation from speech text and convert card 
		   mnemonics into phrases. eg Kc -> King of clubs
		   If we hit a '(' then stop adding as it'll be a (y/n) 
		   prompt */
		c = *s1;
		if (c == '\'' || c == '=' || c == ' ' || isalnum(c))
		{
			if (spchpos)
			{
				char *name = cardGetName(c,spchtext[spchpos-1],&len);
				if (len)
				{
					spch_alloc += len + 1;
					spchtext = (char *)realloc(spchtext,spch_alloc);
					assert(spchtext);
					// Overwrite 1st char
					--spchpos;
					memcpy(spchtext+spchpos,name,len);
					spchpos += len;
				}
				else spchtext[spchpos++] = c;
			}
			else spchtext[spchpos++] = c;
		}
		// Don't speak (y/n)
		else if (c == '(') do_speech = false;
		// Causes a pause in speech
		else if (c == ':') spchtext[spchpos++] = ',';
#endif
	}
	// Just in case there's something there 
	fflush(stdout);

	// Print it 
	write(STDOUT_FILENO,output,out_len);
#ifdef __APPLE__
	if (spchpos)
	{
		spchtext[spchpos] = 0;
		sayprintf(spchtext);
	}
#endif
}



void sayprintf(const char *fmt, ...)
{
#ifdef __APPLE__
	if (!flags.speech) return;

	va_list args;
	char *text;
	pid_t pid;

	// Could do system() but this is far more efficient. No idea how to use
	// MacOS speech API and is probably Objective C anyway.
	va_start(args,fmt);
	vasprintf(&text,fmt,args);
	va_end(args);

	switch((pid = fork()))
	{
	case -1:
		errprintf("fork(): %s\n",strerror(errno));
		return;
	case 0:
		break;
	default:
		/* Parent. Use waitpid() because if wait is off there may be
		   many speech processes running at the same time and we could
		   wait for the wrong one */
		free(text);
		if (flags.wait_for_speech) waitpid(pid,NULL,0);
		return;
	}

	// Child
	char *exec_argv[7];

	exec_argv[0] = "/usr/bin/say";
	exec_argv[1] = "-r";
	exec_argv[2] = speech_rate;
	if (voice)
	{
		exec_argv[3] = "-v";
		exec_argv[4] = voice;
		exec_argv[5] = text;
		exec_argv[6] = NULL;
		if (flags.debug)
		{
			dbgprintf(NO_PLY,"exec %s %s %s %s %s \"%s\"\n",
				exec_argv[0],
				exec_argv[1],
				exec_argv[2],
				exec_argv[3],
				exec_argv[4],
				exec_argv[5]);
		}
	}
	else
	{
		exec_argv[3] = text;
		exec_argv[4] = NULL;
		if (flags.debug)
		{
			dbgprintf(NO_PLY,"exec %s %s %s \"%s\"\n",
				exec_argv[0],
				exec_argv[1],
				exec_argv[2],
				exec_argv[3]);
		}
	}

	execve("/usr/bin/say",exec_argv,environ);
	perror("ERROR: execve(\"/usr/bin/say\")");
	exit(1);
#else
	// Stops unused param compile warning on Linux
	++fmt;
#endif
}
