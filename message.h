#ifndef __MESSAGE_H__
#define __MESSAGE_H__
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MSG_WARN    "Warn"
#define MSG_ERR     "Error"

#define SHOW_LOG(IO, PORT, MSG_TYPE, FORMAT, ...) \
	do { \
		char time[50]; \
		char *endchar; \
		const char *command = "date +%Y-%m-%d-%T"; \
		FILE *pp; \
		pp = popen(command, "r"); \
		if (!pp || fgets(time, sizeof(time), pp) == NULL) { \
			if (PORT < 0) fprintf(IO, "[portd] "); \
			else          fprintf(IO, "[portd(%d)] ", PORT); \
			fprintf(IO, "%s: Fail to get local time.\n", MSG_TYPE); \
		} else { \
			if((endchar = strchr(time, '\n'))) \
				*endchar = '\0'; \
			if (PORT < 0) fprintf(IO, "[portd] "); \
			else          fprintf(IO, "[portd(%d) %s] ", PORT, time); \
			fprintf(IO, "%s: ", MSG_TYPE); \
			fprintf(IO, FORMAT, ##__VA_ARGS__); \
		} \
		if (pp) \
			pclose(pp);	\
	} while (0);

#endif