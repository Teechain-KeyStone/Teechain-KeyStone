#include <vector>
#include <map>
#include <string>
#include "app/eapp_utils.h"
// #include "string.h"
#include "syscall.h"
#include "malloc.h"
#include "edge_wrapper.h"
#include "sodium.h"
#include "hacks.h"
#include "channel.h"
#include "command.h"
#include "teechain.h"
#include "utils.h"

#define DEFAULT_HOSTNAME "127.0.0.1"

// remote hostname and port for the teechain node to connect to. 
static std::string remote_host(DEFAULT_HOSTNAME);
static int remote_port = 0;

// optional flags to define teechain node properties.
static bool initiator = false;
static bool use_monotonic_counters = false;
bool debug = false;
bool benchmark = false;
bool parse_err = false;

// parsed option index and arguments from cmdline
int opt_idx;
char *opt_arg;
std::map<char, int> opt_map;

char *prompt = "Usage: command [options]\n"
        "Teechain commands: \n"
        "quit:            Quit. \n"
        "ghost:           Creates a Teechain enclave (not yet a primary or backup). \n"
        "primary:         Assigns an existing ghost to become a primary node. \n"
        "                  Contacts localhost at port p to make assignment. \n"
        "setup_deposits:  Gives a primary node the information about the number of deposits the user wishes to make. \n"
        "                  The TEE then returns a set of bitcoin addresses the user should pay into. \n"
        "deposits_made:   Gives the primary node the transaction hash of the transaction that was generated by \n"
        "                  the ownwer and paid the deposits into the primary node's bitcoin account. This requires \n"
        "                  knowing the redeem scripts to spend and the account to return funds to on settlement.\n"
        "                  This is the information presented to counter party, requiring manual verification. \n"
        "add_deposit:     Assigns an unused (but verified) deposit to a channel. When adding a deposit to a channel \n"
        "                  the adding TEE adds the funds before requiring an ack from the remote party. This is \n"
        "                  an optimization, but doesn't break safety. \n"
        "remove_deposit:  Removes an assigned (but unspent) deposit from a channel. The removing TEE *must wait* \n"
        "                  for the remote TEE to ack the removal first, before removing. This is to ensure \n"
        "                  we don't break safety. \n"
        "create_channel:  Creates a channel with a remote party. \n"
        "                  Both sides of the channel must have create called. \n"
        "                  The initiating side will start the message exchange process. \n"
        "                  The add_backup command uses the same message exchange protocol as here. \n"
        "verify_deposits: The manual verification step that must be performed by the user to verify that the \n"
        "                  deposit transactions presented by the remote TEE do actually exist and are actually \n"
        "                  in the blockchain. \n"
        "                  This is called on an individual channel after creation, but before sending can occur. \n"
        "                  We don't yet wait for a secure ack from the remote party (i.e. for them to establish \n"
        "                  the channel on their side -- but this is fine). \n"
        "send:            Sends a payment across a channel to the counterparty. \n"
        "settle_channel:  Generate a transaction that closes a single channel and notifies the counterparty. \n"
        "                  This can only be called on the primary node. \n"
        "return_unused_deposits: Generate a transaction that returns the currently unused deposits. \n"
        "shutdown:        Settle each of the channels individually. Also generate a transaction that returns \n"
        "                  unused deposits back to the owner. \n";

void attest_and_establish_channel(){
    // TODO sizeof report
    char buffer[2048];
    attest_enclave((void*) buffer, server_pk, crypto_kx_PUBLICKEYBYTES);
    ocall_send_report(buffer, 2048);

    ocall_wait_for_client_pubkey(client_pk, crypto_kx_PUBLICKEYBYTES);
    channel_establish();
}

static void send_reply(char* msg) {
    size_t reply_size = channel_get_send_size(strlen(msg)+1);
    unsigned char* reply_buffer = (unsigned char*)malloc(reply_size);
    if(reply_buffer == NULL){
        ocall_print_buffer("Reply too large to allocate, no reply sent\n");
        // continue;
    }

    channel_send((unsigned char*)msg, strlen(msg)+1, reply_buffer);
    ocall_send_reply(reply_buffer,reply_size);

    free(reply_buffer);
}

static void usage() {
    ocall_print_buffer(prompt);
    parse_err = true;
}

static void opt_map_init(char *optstring) {
    char *p = optstring;
    while (*p != '\0') {
        if (p[1] == ':') {
            opt_map[*p] = 1;
        } else {
            opt_map[*p] = 0;
        }
        p++;
    }
}

static char parse_opt(std::vector<const char*> &opt_vec) {
    char ch;
    for (; opt_idx < opt_vec.size(); opt_idx++) {
        if (opt_vec[opt_idx][0] == '-') {
            ch = opt_vec[opt_idx][1];
            if (opt_map[opt_vec[opt_idx][1]]) {
                opt_arg = (char *)opt_vec[opt_idx + 1];
                opt_idx++;
            }
            opt_idx++;
            return ch;
        }
    }
    opt_idx = 0;
    return -1;
}

static void execute_command(const char *command, std::vector<const char*> &opt_vec) {
    if (streq(command, "primary")) {
        ecall_primary();
    }
}

void handle_messages() {
    int opt_ret;
    struct edge_data msg;
    char *cmd;
    while (1) {
        ocall_wait_for_message(&msg);
        CommandMsg* cmd_msg = (CommandMsg*)malloc(msg.size);
        size_t wordmsg_len;

        if (cmd_msg == NULL) {
            ocall_print_buffer("Message too large to store, ignoring\n");
            continue;
        }

        copy_from_shared(cmd_msg, msg.offset, msg.size);
        if (channel_recv((unsigned char*)cmd_msg, msg.size, &wordmsg_len) != 0) {
            free(cmd_msg);
            continue;
        }

        if (cmd_msg->msg_op == OP_QUIT) {
            ocall_print_buffer("Received exit, exiting\n");
            EAPP_RETURN(0);
        }

        char *token = strtok((char *)cmd_msg->msg, " ");
        cmd = token;
        std::vector<const char*> opt_vec;
        while (token != NULL) {
            opt_vec.push_back(token);
            token = strtok(NULL, " ");
        }
        
        while ((opt_ret = parse_opt(opt_vec)) != -1) {
            switch (opt_ret) {
                case 'm':
                    use_monotonic_counters = true;
                    break;
                case 'd':
                    debug = true;
                    break;
                case 'b':
                    benchmark = true;
                    break;
                case 'i':
                    initiator = true;
                    break;
                case 'r': {
                    char *host_and_port= opt_arg;
                    char *token;
                    const char *colon = ":";
                    token = strtok(host_and_port, colon);
                    remote_host = std::string(token);
                    token = strtok(NULL, colon);
                    remote_port = atoi(token);
                    break;
                }
                default:
                    usage();
            }
            if (parse_err) {
                parse_err = false;
                goto parse_err;
            }
        }

        execute_command(cmd, opt_vec);

parse_err:
        // Done with the message, free it
        free(cmd_msg);

    }
}

void EAPP_ENTRY eapp_entry() {
    edge_init();
    magic_random_init();
    channel_init();

    attest_and_establish_channel();

    opt_map_init("mdbir:");

    handle_messages();

    EAPP_RETURN(0);
}