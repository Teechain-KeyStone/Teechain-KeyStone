#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include "message.h"
#include "sodium.h"
#include "report.h"

typedef unsigned char byte;

// Temporary channel handle
#define TEMPORARY_CHANNEL_ID "0000011111111111111111111111111111111111111111111111111111100000"

#define DEFAULT_HOSTNAME "127.0.0.1"
#define DEFAULT_PORT 8067
#define BUFFERLEN 4096

extern int client_sockfd;

void untrusted_teechain_exit();
void untrusted_teechain_init();
byte* untrusted_teechain_pubkey(size_t* len);
void untrusted_teechain_get_report(void* buffer, int ignore_valid);
int untrusted_teechain_read_reply(unsigned char* data, size_t len);
void send_exit_message();
void send_cmd_message(char* msg, size_t msg_len);

byte* untrusted_teechain_box(byte* msg, size_t size, size_t* finalsize);
void untrusted_teechain_unbox(unsigned char* buffer, size_t len);
void wait_for_send_ack();

#endif /* _NETWORK_H_ */