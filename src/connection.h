/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _listener_h
#define _listener_h

#include "server.h"
#include "request.h"
#include "state.h"
#include "proxy.h"
#include "io.h"
#include "adt/list.h"
#include "adt/hash.h"
#include "tnetstrings.h"

#define DELIVER_OUTSTANDING_MSGS 16

extern int CONNECTION_STACK;
extern int BUFFER_SIZE;
extern int MAX_CONTENT_LENGTH;
extern int MAX_CHUNK_SIZE;
extern int MAX_CREDITS;
extern int DEFAULT_TTL;

enum {
    CONN_TYPE_HTTP=1,
    CONN_TYPE_MSG,
    CONN_TYPE_SOCKET
};

enum deliverTaskState {
    DT_NOT_CREATED = 0,
    DT_RUNNING,
    DT_DYING,
    DT_DEAD
};
struct Connection;

typedef int (*deliver_function)(struct Connection *c, tns_value_t *data);

typedef struct Deliver_message {
    deliver_function deliver;
    tns_value_t *data;
} Deliver_message;

typedef struct Connection {
    Request *req;

    IOBuf *iob;
    IOBuf *proxy_iob;

    // if SNI is used, then the connection has its own cert
    int use_sni;
    x509_crt own_cert;
    pk_context pk_key;

    int rport;
    State state;
    struct httpclient_parser *client;
    int close;
    int type;
    hash_t *filter_state;
    char remote[IPADDR_SIZE+1];
    Handler *handler;
    volatile enum deliverTaskState deliverTaskStatus;
    list_t *deliverQueue;
    int deliverBytesPending;
    Rendez deliverRendez;
    Rendez uploadRendez;
    int sendCredits;
    int closing;
} Connection;

void Connection_destroy(Connection *conn);
Connection *Connection_create(Server *srv, int fd, int rport,
                              const char *remote);

int Connection_accept(Connection *conn);

void Connection_task(void *v);

struct Handler;
int Connection_send_to_handler(Connection *conn, Handler *handler, char *body, int content_len, hash_t *altheaders);

int Connection_deliver_raw(Connection *conn, bstring buf);

int Connection_deliver(Connection *conn, bstring buf);

int Connection_read_header(Connection *conn, Request *req);

void Connection_init();

void Connection_deliver_task(void *v);
int Connection_deliver_enqueue(Connection *conn, deliver_function f,
                                             tns_value_t *d);

#endif
