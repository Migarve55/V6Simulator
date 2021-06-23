#ifndef MESSAGES_H
#define MESSAGES_H

#define NUMBEROFMSGS 64

#define MESSAGES_FILE "messages.txt"

typedef struct {
  int number;
  char format[120];
} DEBUG_MESSAGES;

extern DEBUG_MESSAGES DebugMessages[NUMBEROFMSGS];

int Messages_Get_Pos(int number);
int Messages_Load_Messages(int, char *);

#endif

