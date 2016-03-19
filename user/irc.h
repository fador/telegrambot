#include "espconn.h"

void ICACHE_FLASH_ATTR ircSendmessage(struct espconn *conn, char *to, char *line);
void ircInit();

#define NICK "esp8266bot"
//#define CHANNEL "#bottest"
#define CHANNEL "#ilma"
