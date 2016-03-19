#define USE_US_TIMER
#include "ets_sys.h"
#include "c_types.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "irc.h"
#include "ircbot.h"
#include "io.h"
#include "user_network.h"

void user_init(void)
{
  char *ssid = ""; // Wifi SSID
  char *password = ""; // Wifi Password
  struct station_config stationConf;
  
  system_timer_reinit();
  
  os_memcpy(&stationConf.ssid, ssid, 32);
  os_memcpy(&stationConf.password, password, 32);
  wifi_set_opmode( STATION_MODE );
  wifi_set_phy_mode(PHY_MODE_11N);
  wifi_station_set_config(&stationConf);
  wifi_station_connect();
  
	network_init();
	

	os_printf("\nReady\n");
}
