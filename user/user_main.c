/*
  Copyright (c) 2016, Marko Viitanen (Fador)

  Permission to use, copy, modify, and/or distribute this software for any purpose 
  with or without fee is hereby granted, provided that the above copyright notice 
  and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH 
  REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
  AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, 
  INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
  LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
  OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
  PERFORMANCE OF THIS SOFTWARE.

*/

#define USE_US_TIMER
#include "ets_sys.h"
#include "driver/uart.h"
#include <c_types.h>
#include <user_interface.h>
#include <osapi.h>
#include <espconn.h>
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
