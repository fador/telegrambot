#pragma once

void ICACHE_FLASH_ATTR replyMessage(int message_id, char* username, char *message);

int ICACHE_FLASH_ATTR parseReply(char *data, int len);