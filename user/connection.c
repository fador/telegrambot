#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"
#include "connection.h"
#include "message.h"
#include "configure.h"

#define BUFFERLEN 1024

#define GETUPDATE "GET /bot" BOT_API_KEY "/getUpdates?limit=1&timeout=100&offset=%d HTTP/1.1\r\nHost: api.telegram.org\r\nConnection: close\r\n\r\n\0"
static char sendBuf[128];

static char buffer[BUFFERLEN];
static int bufferpos = 0;
ip_addr_t ip;
static struct espconn conn;
static int update_id = 0;

static void ICACHE_FLASH_ATTR recvCb(void *arg, char *data, unsigned short len) {
  struct espconn *conn=(struct espconn *)arg;
  int x;
  for (x=0; x<len; x++) os_printf("%c", data[x]);
  if(bufferpos + len >= BUFFERLEN) return;
  
  os_memcpy(&buffer[bufferpos], data, len);
  bufferpos += len;
}

static void ICACHE_FLASH_ATTR connectedCb(void *arg) {
  struct espconn *conn=(struct espconn *)arg;
  espconn_regist_recvcb(conn, recvCb);
  os_printf("Connected.\n");
  os_sprintf(sendBuf, GETUPDATE, update_id);
  espconn_secure_sent(conn, (char*)sendBuf, os_strlen(sendBuf));
}

static void ICACHE_FLASH_ATTR reconCb(void *arg, sint8 err) {
  os_printf("Recon\n");
}

static void ICACHE_FLASH_ATTR disconCb(void *arg) {
  os_printf("Discon\n");
  if(bufferpos) {
    int uid = parseReply(buffer, bufferpos);
    if(uid > 0) {
      update_id = uid + 1;
    }
  }
  os_delay_us(500000);
  bufferpos = 0;
  espconn_gethostbyname(&conn, "api.telegram.org", &ip, hostFoundCb);
}

static void ICACHE_FLASH_ATTR connectWithIp(void *arg) {
  static esp_tcp tcp;
  struct espconn *conn=(struct espconn *)arg;
  
  const char ip[4] = {192, 168, 0, 106};
  
  os_printf("Server at %d.%d.%d.%d\n",
    *((uint8 *)&ip), *((uint8 *)&ip + 1),
    *((uint8 *)&ip + 2), *((uint8 *)&ip + 3));

  conn->type=ESPCONN_TCP;
  conn->state=ESPCONN_NONE;
  conn->proto.tcp=&tcp;
  conn->proto.tcp->local_port=espconn_port();
  conn->proto.tcp->remote_port=8000;
  os_memcpy(conn->proto.tcp->remote_ip, &ip, 4);

  espconn_regist_connectcb(conn, connectedCb);
  espconn_regist_disconcb(conn, disconCb);
  espconn_regist_reconcb(conn, reconCb);
  os_printf("Connecting to server...\n");
  espconn_secure_ca_disable(ESPCONN_CLIENT);
  espconn_secure_cert_req_disable(ESPCONN_CLIENT);
  espconn_secure_set_size(ESPCONN_CLIENT,16384/2); 
  espconn_secure_connect(conn);
}

void ICACHE_FLASH_ATTR hostFoundCb(const char *name, ip_addr_t *ip, void *arg) {
  static esp_tcp tcp;
  struct espconn *conn=(struct espconn *)arg;
  if (ip==NULL) {
    os_printf("Huh? Nslookup of server failed :/ Trying again...\n");
    telegramInit();
  }
  /*
  os_printf("Server at %d.%d.%d.%d\n",
    *((uint8 *)&ip->addr), *((uint8 *)&ip->addr + 1),
    *((uint8 *)&ip->addr + 2), *((uint8 *)&ip->addr + 3));
*/
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
  os_printf("Connecting to server...\n");
  espconn_secure_ca_disable(ESPCONN_CLIENT);
  espconn_secure_cert_req_disable(ESPCONN_CLIENT);
  espconn_secure_set_size(ESPCONN_CLIENT,16384/2); 
  espconn_secure_connect(conn);
}

void ICACHE_FLASH_ATTR telegramInit() {
  
  bufferpos = 0;
  os_printf("Looking up server...\n");
  espconn_gethostbyname(&conn, "api.telegram.org", &ip, hostFoundCb);
}
