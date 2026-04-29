#include "wifi_manager.h"
#include <stdbool.h>
static bool g_conn=false; static const char *g_ip="192.168.4.1"; static const char *g_mode="ap";
void wifi_manager_start(void){g_conn=false;} const char *wifi_manager_get_ip(void){return g_ip;} bool wifi_manager_is_connected(void){return g_conn;} const char *wifi_manager_get_mode(void){return g_mode;}
