#include "espconn.h"

void ICACHE_FLASH_ATTR ircSendmessage(struct espconn *conn, char *to, char *line);
void telegramInit();
void ICACHE_FLASH_ATTR hostFoundCb(const char *name, ip_addr_t *ip, void *arg);

#define NICK "esp8266bot"
//#define CHANNEL "#bottest"
#define CHANNEL "#ilma"
