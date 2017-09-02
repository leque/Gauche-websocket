/*
 * websocket.h
 */

/* Prologue */
#ifndef GAUCHE_WEBSOCKET_H
#define GAUCHE_WEBSOCKET_H

#include <gauche.h>
#include <gauche/extend.h>
#include <libwebsockets.h>

SCM_DECL_BEGIN

extern ScmClass *WsClientClass;

#define WSCLIENT_P(obj) SCM_XTYPEP(obj, WsClientClass)
#define WSCLIENT_UNBOX(obj) SCM_FOREIGN_POINTER_REF(gwebsocket_client *, obj)
#define WSCLIENT_BOX(ptr) Scm_MakeForeignPointer(WsClientClass, ptr)

typedef struct gwebsocket_client_rec {
    struct lws_context_creation_info info;
    struct lws_client_connect_info ccinfo;
    struct lws *wsi;
} gwebsocket_client;

gwebsocket_client *make_websocket_client(
    const char *address,
    int port,
    int ssl_connection,
    const char *path,
    const char *host,
    const char *origin,
    ScmObj callback);

void gwebsocket_client_connect(gwebsocket_client *ws);

int gwebsocket_client_write(gwebsocket_client *ws, ScmObj msg);

/* Epilogue */
SCM_DECL_END

#endif /* GAUCHE_WEBSOCKET_H */
