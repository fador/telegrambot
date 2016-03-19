#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"
#include "irc.h"
#include "ircbot.h"

#define STATE_CONNECTED 0
#define STATE_OPEN 1
#define STATE_NICKSENT 2
#define STATE_JOINED 3


static const char userCmd[]="NICK "NICK"\r\nUSER "NICK" localhost localhost :This bot is running on an ESP8266\r\n";
static const char joinCmd[]="x\r\nJOIN "CHANNEL"\r\n\r\n";

static int state=STATE_CONNECTED;
static char lineBuf[1024];
static int lineBufPos;

void ICACHE_FLASH_ATTR ircSendmessage(struct espconn *conn, char *to, char *line) {
	char buff[1024];
	os_printf("Msg to %s: %s\n", to, line);
	strcpy(buff, "PRIVMSG ");
	strcat(buff, to);
	strcat(buff, " :");
	strcat(buff, line);
	strcat(buff, "\r\n");
	espconn_sent(conn, buff, os_strlen(buff));
}

//Parses an IRC line. line is the line, sans ending cr/lf.
static void ICACHE_FLASH_ATTR ircParseLine(struct espconn *conn, char *line) {
	char buff[1024];
	char *prefix=NULL;
	char *command=NULL;
	int cmdNum=-1;
	char *param[10];
	int noParam;
	char *p=line;
	char *e;
	int x;
	int forceEnd;

	os_printf("RECV: %s\n", line);

	//Parse line into prefix/command/param/... bits
	if (p[0]==':') {
		//Line has a prefix.
		p++;
		e=(char*)os_strstr(p, " ");
		if (e==NULL) return;
		*e=0; //End prefix with 0 byte
		prefix=p;
		p=e+1; //Parse following stuff after space.
	}
	while(*p==' ') p++; //skip spaces

	noParam=-1;
	forceEnd=0;
	while (*p!=0) {
		if (*p==':') {
			p++;
			forceEnd=1;
		}
		if (noParam==-1) {
			command=p;
			cmdNum=atoi(command);
		} else {
			param[noParam]=p;
		}
		noParam++;
		if (forceEnd) break;
		//Find end of this and start of next command
		e=(char*)os_strstr(p, " ");
		if (e==NULL) break;
		*e=0; //Convert space to NULL to end prev arg
		e++; //Move to start of next arg
		while (*e==' ') e++; //Skip spaces at start
		p=e;
	}
	if (command==NULL || command[0]==0) {
		return;
	}

//	os_printf("Prefix: %s, command: %s, noParam: %d\n", prefix, command, noParam);

  if (state==STATE_OPEN) {
    os_delay_us(500000);
    espconn_sent(conn, (char*)userCmd, sizeof(userCmd));
    os_printf("Sent nick.\n");
    state=STATE_NICKSENT;
  }
    
	if (os_strcmp(command, "PING")==0) {
		os_strcpy(buff, "PONG :");
		os_strcat(buff, param[0]);
		os_strcat(buff, "\r\n\r\n");
		espconn_sent(conn, buff, os_strlen(buff));
		os_printf("Sent pong.\n");
	} else if (os_strcmp(command, "PRIVMSG")==0) {
		//:Sprite_tm!~sprite_tm@spritesmods.com PRIVMSG #bottest :Okay, dit is fucked.
		//:Sprite_tm!~sprite_tm@spritesmods.com PRIVMSG espbot :Okay, dit is fucked.
		for (x=0; prefix[x]!=0 && prefix[x]!='!'; x++) buff[x]=prefix[x];
		buff[x]=0;
		ircbotParseMessage(conn, buff, param[0], param[1]);
	} else if (os_strcmp(command, "NOTICE")==0) {

	} else if (state==STATE_NICKSENT && os_strcmp(command, "MODE")==0) {
		espconn_sent(conn, (char*)joinCmd, sizeof(joinCmd));
		os_printf("Sent join command.\n");
		state=STATE_JOINED;
	}
}

static void ICACHE_FLASH_ATTR ircParseChar(struct espconn *conn, char c) {
	lineBuf[lineBufPos++]=c;
	if (lineBufPos>=sizeof(lineBuf)) lineBufPos--;

	if (lineBufPos>2 && lineBuf[lineBufPos-2]=='\r' && lineBuf[lineBufPos-1]=='\n') {
		lineBuf[lineBufPos-2]=0;
		ircParseLine(conn, lineBuf);
		lineBufPos=0;
	}
}

static void ICACHE_FLASH_ATTR ircRecvCb(void *arg, char *data, unsigned short len) {
	struct espconn *conn=(struct espconn *)arg;
	int x;
	for (x=0; x<len; x++) ircParseChar(conn, data[x]);
}

static void ICACHE_FLASH_ATTR ircConnectedCb(void *arg) {
	struct espconn *conn=(struct espconn *)arg;
	espconn_regist_recvcb(conn, ircRecvCb);
	os_printf("Connected.\n");
	lineBufPos=0;
	state=STATE_OPEN;
}

static void ICACHE_FLASH_ATTR ircReconCb(void *arg, sint8 err) {
	os_printf("Recon\n");
	ircInit();
}

static void ICACHE_FLASH_ATTR ircDisconCb(void *arg) {
	os_printf("Discon\n");
	ircInit();
}

static void ICACHE_FLASH_ATTR ircServerFoundCb(const char *name, ip_addr_t *ip, void *arg) {
	static esp_tcp tcp;
	struct espconn *conn=(struct espconn *)arg;
	if (ip==NULL) {
		os_printf("Huh? Nslookup of irc server failed :/ Trying again...\n");
		ircInit();
	}
	os_printf("Server at %d.%d.%d.%d\n",
		*((uint8 *)&ip->addr), *((uint8 *)&ip->addr + 1),
		*((uint8 *)&ip->addr + 2), *((uint8 *)&ip->addr + 3));

	conn->type=ESPCONN_TCP;
	conn->state=ESPCONN_NONE;
	conn->proto.tcp=&tcp;
	conn->proto.tcp->local_port=espconn_port();
	conn->proto.tcp->remote_port=6667;
	os_memcpy(conn->proto.tcp->remote_ip, &ip->addr, 4);

	espconn_regist_connectcb(conn, ircConnectedCb);
	espconn_regist_disconcb(conn, ircDisconCb);
	espconn_regist_reconcb(conn, ircReconCb);
	os_printf("Connecting to IRC server...\n");
	espconn_connect(conn);
}

void ircInit() {
	static struct espconn conn;
	static ip_addr_t ip;
	os_printf("Looking up IRC server...\n");
	espconn_gethostbyname(&conn, "irc.cc.tut.fi", &ip, ircServerFoundCb);
	state=STATE_CONNECTED;
}
