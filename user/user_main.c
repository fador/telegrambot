#define USE_US_TIMER
#include "ets_sys.h"
#include "driver/uart.h"
#include "c_types.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "connection.h"
#include "io.h"
#include "user_network.h"
#include "configure.h"

void user_init(void)
{
  char *ssid = WLAN_SSID;
  char *password = WLAN_PASSWD;
  struct station_config stationConf;

  system_timer_reinit();
  
  os_memcpy(&stationConf.ssid, ssid, 32);
  os_memcpy(&stationConf.password, password, 32);
  uart_init(BIT_RATE_9600, BIT_RATE_9600);
  os_printf("SDK version:%s\n", system_get_sdk_version());
  wifi_set_opmode( STATION_MODE );
  wifi_set_phy_mode(PHY_MODE_11N);
  wifi_station_set_config(&stationConf);
  wifi_station_connect();
  
  network_init();
  
  os_printf("\nReady\n");
}
