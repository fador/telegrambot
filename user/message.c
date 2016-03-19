#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"
#include "connection.h"
#include "configure.h"
#include <json/jsonparse.h>

static ip_addr_t ip;
static struct espconn conn;
#define SENDMSG "GET /bot" BOT_API_KEY "/sendMessage?chat_id=%d&text=%s HTTP/1.1\r\nHost: api.telegram.org\r\nConnection: close\r\n\r\n"
char outmessage[256];
bool sendStatus = false;
 
static void ICACHE_FLASH_ATTR connectedCb(void *arg) {
  struct espconn *conn=(struct espconn *)arg;
  os_printf("Reply sent %d\n", os_strlen(outmessage));
  espconn_secure_sent(conn, (char*)outmessage, os_strlen(outmessage));
  os_printf("Reply done\n");
  sendStatus = true;
}

static void ICACHE_FLASH_ATTR reconCb(void *arg, sint8 err) {
  //os_printf("Recon\n");
  sendStatus = true;
}

static void ICACHE_FLASH_ATTR disconCb(void *arg) {
  sendStatus = true;
}

void ICACHE_FLASH_ATTR connectAndSend(const char *name, ip_addr_t *ip, void *arg)
{
  static esp_tcp tcp;
  struct espconn *conn=(struct espconn *)arg;
  if (ip==NULL) {
    os_printf("Huh? Nslookup of server failed :/ Trying again...\n");
  }

  conn->type=ESPCONN_TCP;
  conn->state=ESPCONN_NONE;
  conn->proto.tcp=&tcp;
  conn->proto.tcp->local_port=espconn_port();
  conn->proto.tcp->remote_port=443;
  os_memcpy(conn->proto.tcp->remote_ip, &ip->addr, 4);
  
  espconn_create(conn);
  espconn_regist_connectcb(conn, connectedCb);
  espconn_regist_disconcb(conn, disconCb);
  espconn_regist_reconcb(conn, reconCb);
  os_printf("Connecting to server for reply...\n");
  espconn_secure_ca_disable(ESPCONN_CLIENT);
  espconn_secure_cert_req_disable(ESPCONN_CLIENT);
  espconn_secure_set_size(ESPCONN_CLIENT,16384/2); 
  espconn_secure_connect(conn); 
  
}


void ICACHE_FLASH_ATTR replyMessage(int id, char* username, char *message)
{
  char messageBuffer[128];
  bool sendReply = false;
  if(os_strcmp(message, "hi") == 0 || os_strcmp(message, "Hi") == 0) {
    os_strcpy(messageBuffer, "hello%20");
    os_strcat(messageBuffer, username);
    os_strcat(messageBuffer, "!");
    sendReply = true;
  } else {
    os_strcpy(messageBuffer, "umm, ok");
    sendReply = true;
  }
  
  os_sprintf(outmessage, SENDMSG, id, messageBuffer);
  if(sendReply) {
    os_printf("Sending reply..\n");
    espconn_gethostbyname(&conn, "api.telegram.org", &ip, connectAndSend);
  }
}

int ICACHE_FLASH_ATTR parseReply(char *data, int len)
{  
  int update_id = 0;
  int id = 0;
  int type;
  os_printf("Start parsing\n");
  int startPos = 0;
  char *msgPos = (char *)os_strstr(data, "\r\n\r\n");
  if(msgPos == NULL) return -1;
  startPos = msgPos - data;
  for(;startPos < len && data[startPos] != '['; startPos++) { }
  startPos++;
  
  char textbuf[1024];
  char username[128];
  struct jsonparse_state parser;
  jsonparse_setup(&parser, &data[startPos], len-startPos-2);
  while ((type = jsonparse_next(&parser)) != 0) {
    if (type == JSON_TYPE_PAIR_NAME) {
      if (jsonparse_strcmp_value(&parser, "update_id") == 0) {
        jsonparse_next(&parser);
        jsonparse_next(&parser);
        update_id = jsonparse_get_value_as_int(&parser);
      } else if (jsonparse_strcmp_value(&parser, "id") == 0) {
        jsonparse_next(&parser);
        jsonparse_next(&parser);
        id = jsonparse_get_value_as_int(&parser);
      } else if (jsonparse_strcmp_value(&parser, "text") == 0) {
        jsonparse_next(&parser);
        jsonparse_next(&parser);
        jsonparse_copy_value(&parser, textbuf, 1024);
      } else if (jsonparse_strcmp_value(&parser, "username") == 0) {
        jsonparse_next(&parser);
        jsonparse_next(&parser);
        jsonparse_copy_value(&parser, username, 128);
      }
    }
  }
  if(id) {
    sendStatus = false;
    ets_wdt_disable();
    replyMessage(id, username, textbuf);
    while(!sendStatus) {
      os_delay_us(1024*16);      
    }
    ets_wdt_enable();
  }
  return update_id;
}