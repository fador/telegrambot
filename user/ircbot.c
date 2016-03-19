#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"
#include "ircbot.h"
#include "irc.h"
#include "io.h"

typedef struct {
	char name[16];
	int timeSince;
} UserSeen;

#define NOUSERSSEEN 32
UserSeen users[NOUSERSSEEN];


static ETSTimer timer;

//ETSTimer *timer, void *arg
void ICACHE_FLASH_ATTR ircbotSecTimer(void) {
	int i;
	for (i=0; i<NOUSERSSEEN; i++) users[i].timeSince++;
}

void ICACHE_FLASH_ATTR ircbotAddSeen(char *nick) {
	int i, maxp=0;
	for (i=0; i<NOUSERSSEEN; i++) {
		if ((os_strncmp(users[i].name, nick, 16)==0) || users[i].name[0]==0) {
			os_printf("addSeen: nick %s to slot %d\n", nick, i);
			strncpy(users[i].name, nick, 16);
			users[i].timeSince=0;
			return;
		}
		if (users[i].timeSince>users[maxp].timeSince) maxp=i;
	}
	strncpy(users[maxp].name, nick, 16);
	users[maxp].timeSince=0;
}

void ICACHE_FLASH_ATTR ircbotParseMessage(struct espconn *conn, char *from, char *to, char *txt) {
	char *replUser;
	char *msg=txt;
	char buff[2048];
	int i;
	//Take note of existence of user
	ircbotAddSeen(from);

	//See if this message is meant for me
	if (to[0]=='#') {
//		os_printf("Message is to chan and contains %s\n", txt);
		//Channel: grab attention of bot either by prefixing the message with nick: or a !
		if (os_strncmp(msg, NICK, os_strlen(NICK))==0) {
			msg=txt+os_strlen(NICK)+1;
			while (*msg==' ') msg++;
		} else if (msg[0]=='!') {
			//Skip over '!'
			msg++;
		} else {
			return;
		}
		replUser=to;
	} else {
		replUser=from;
	}

	//Default response
	strcpy(buff, msg);
	strcat(buff, " indeed, dear ");
	strcat(buff, from);

	if (os_strncmp(msg, "dice", 4)==0) {
		strcpy(buff, "I just rolled a pair of dice. They both came up 7. Hmm, my power supply must be faulty.");
	} else if (os_strncmp(msg, "badum", 5)==0) {
		strcpy(buff, "*tssssssh*");
	} else if (os_strncmp(msg, "led", 3)==0) {
		i=atoi(msg+4);
		ioLed(i);
		os_sprintf(buff, "Led is now %s.", i?"on":"off");
	} else if (os_strncmp(msg, "seen", 4)==0) {
		char *nick=msg+5;
		i=0;
		while(nick[i]!=0 && nick[i]!=' ') i++;
		nick[i]=0;
		os_sprintf(buff, "Sorry, I've not seen %s.", nick);
		for (i=0; i<NOUSERSSEEN; i++) {
			if (os_strncmp(users[i].name, nick, 16)==0) {
				//todo: 1 minute*s*?
				os_sprintf(buff, "Yeah, %s was here %i minutes ago.", nick, users[i].timeSince/60);
			}
		}
	} else if (os_strncmp(msg, "whack ", 6)==0) {
		char *target=msg+6;
		for (i=0; ((target[i]!=' ') && (target[i]!=0)); i++) ;
		target[i]=0;
		if (msg[0]==0) return;
		const char *animal[]={"trout", "squid", "dolphin", "otter", "herring", "sea star", "piece of corral", \
				"U-boot", "flying fish", "can o' tuna", "blowfish"};
		const char *mode[]={"angry", "red", "supersized", "miniscule", "weird-looking", "green", "algae-covered", \
				"irate", "scared"};
		const int an[]={1,0,0,0,0,0,1,1,0};
		i=system_get_time();
		int animalIdx=i%11;
		int modeIdx=(i>>4)%9;
//		os_printf("whack %i %i\n", modeIdx, animalIdx);
		os_sprintf(buff, "\001ACTION whacks %s with %s %s %s.\001", target, an[modeIdx]?"an":"a", mode[modeIdx], animal[animalIdx]);
	} else if (os_strncmp(msg, "apologize to ", 13)==0) {
		os_sprintf(buff, "Sorry, %s.", msg+13);
	} else if ((char*)os_strstr(msg, "?")!=NULL) {
		i=system_get_time()&3;
		strcpy(buff, "According to my crystal ball, ");
		if (i==0) strcat(buff, "yes.");
		if (i==1) strcat(buff, "no.");
		if (i==2) strcat(buff, "maybe.");
		if (i==3) strcat(buff, "... What's a guru meditation?");
	}

	ircSendmessage(conn, replUser, buff);
}

void ICACHE_FLASH_ATTR ircbotInit() {
	int i;
	for (i=0; i<NOUSERSSEEN; i++) users[i].name[0]=0;
	os_timer_disarm(&timer);
	os_timer_setfn(&timer, ircbotSecTimer, NULL);
	os_timer_arm(&timer, 1000, 1);
}

