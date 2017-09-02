/*
 * websocket.c
 */

#include "websocket.h"

ScmClass *WsClientClass;

static ScmObj callback_sym;

static int
gauche_dispatch_websocket(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
    switch (reason) {
    case LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH:
    case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
    case LWS_CALLBACK_WSI_CREATE:
    case LWS_CALLBACK_WSI_DESTROY:
    case LWS_CALLBACK_GET_THREAD_ID:
    case LWS_CALLBACK_PROTOCOL_INIT:
    case LWS_CALLBACK_ADD_POLL_FD:
    case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
    case LWS_CALLBACK_LOCK_POLL:
    case LWS_CALLBACK_UNLOCK_POLL:
        // ignore
        break;
    default:
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
    case LWS_CALLBACK_CLOSED:
    case LWS_CALLBACK_CLIENT_RECEIVE:
    case LWS_CALLBACK_CLIENT_WRITEABLE: {
        gwebsocket_client *ws =
            (gwebsocket_client *)lws_get_protocol(wsi)->user;
        ScmObj obj = WSCLIENT_BOX(ws);
        ScmObj proc = Scm_ForeignPointerAttrGet(
            SCM_FOREIGN_POINTER(obj), callback_sym, SCM_FALSE);
        if (SCM_BOOL_VALUE(proc)) {
            ScmObj in_uvec = SCM_FALSE;
            if (in != NULL) {
                in_uvec = Scm_MakeU8VectorFromArray(len, in);
            }
            ScmObj n =
                Scm_ApplyRec3(proc, obj, in_uvec, Scm_MakeInteger(reason));
            return Scm_GetInteger(n);
        }
        break;
    }
    }
    return 0;
}

gwebsocket_client *
make_websocket_client(
    const char *address,
    int port,
    int ssl_connection,
    const char *path,
    const char *host,
    const char *origin,
    ScmObj callback)
{
    gwebsocket_client *result =
        (gwebsocket_client *)malloc(sizeof(gwebsocket_client));

    if (result == NULL) {
        Scm_Error("cannot allocate <websocket-client>");
    }

    memset(&result->info, 0, sizeof(result->info));
    result->info.port = CONTEXT_PORT_NO_LISTEN;
    result->info.iface = NULL;
    result->info.ssl_cert_filepath = NULL;
    result->info.ssl_private_key_filepath = NULL;
    result->info.extensions = NULL;
    result->info.gid = -1;
    result->info.uid = -1;
    result->info.options = 0;
    struct lws_protocols *protocols =
        (struct lws_protocols *)malloc(sizeof(struct lws_protocols));

    if (protocols == NULL) {
        Scm_Error("cannot allocate lws_protocols");
    }

    protocols->name = "gauche-websocket";
    protocols->callback = gauche_dispatch_websocket;
    protocols->user = result;
    protocols->per_session_data_size = 0;
    protocols->rx_buffer_size = 0;
    protocols->id = 0;

    result->info.protocols = protocols;

    memset(&result->ccinfo, 0, sizeof(struct lws_client_connect_info));
    result->ccinfo.context = NULL; // to be set later
    result->ccinfo.address = address;
    result->ccinfo.port = port;
    result->ccinfo.ssl_connection = ssl_connection;
    result->ccinfo.path = path;
    result->ccinfo.host = host;
    result->ccinfo.origin = origin;

    Scm_ForeignPointerAttrSet(
        SCM_FOREIGN_POINTER(WSCLIENT_BOX(result)), callback_sym, callback);

    return result;
}

void
gwebsocket_client_connect(gwebsocket_client *ws)
{
    ws->ccinfo.context = lws_create_context(&ws->info);

    if (ws->ccinfo.context == NULL) {
        Scm_Error("cannot create lws_context");
    }

    ws->wsi = lws_client_connect_via_info(&ws->ccinfo);

    if (ws->wsi == NULL) {
        Scm_Error("connection error: %A", WSCLIENT_BOX(ws));
    }

    lws_callback_on_writable(ws->wsi);
}

int
gwebsocket_client_write(gwebsocket_client *ws, ScmObj msg)
{
    const char *input;
    size_t size;
    enum lws_write_protocol protocol;

    if (SCM_UVECTORP(msg)) {
        size = Scm_UVectorSizeInBytes(SCM_UVECTOR(msg));
        input = (const char *)SCM_UVECTOR_ELEMENTS(msg);
        protocol = LWS_WRITE_BINARY;
    } else if (SCM_STRINGP(msg)) {
        size = SCM_STRING_BODY_SIZE(SCM_STRING_BODY(msg));
        input = (const char *)Scm_GetStringConst(SCM_STRING(msg));
        protocol = LWS_WRITE_TEXT;
    } else {
        Scm_TypeError("websocket message", "uvector or string", msg);
    }

    unsigned char *buf = (unsigned char *)malloc(size + LWS_PRE);

    if (buf == NULL) {
        Scm_Error("cannot allocate buffer");
    }
    memcpy(buf + LWS_PRE, input, size);
    int result = lws_write(ws->wsi, buf + LWS_PRE, size, protocol);
    free(buf);
    return result;
}

static void
gwebsocket_client_print(ScmObj obj, ScmPort *out, ScmWriteContext *ctx)
{
    gwebsocket_client *ws = WSCLIENT_UNBOX(obj);
    Scm_Printf(
        out,
        "#<websocket-client %s://%s:%d%s>",
        (ws->ccinfo.ssl_connection ? "wss" : "ws"),
        ws->ccinfo.address,
        ws->ccinfo.port,
        ws->ccinfo.path);
}

static void
gwebsocket_client_cleanup(ScmObj obj)
{
    gwebsocket_client *ws = WSCLIENT_UNBOX(obj);
    if (ws->ccinfo.context) {
        lws_context_destroy(ws->ccinfo.context);
        ws->ccinfo.context = NULL;
    }

    if (ws->info.protocols) {
        free((void *)ws->info.protocols);
        ws->info.protocols = NULL;
    }

    free(ws);
}

/*
 * Module initialization function.
 */
extern void Scm_Init_websocketlib(ScmModule *);

void
Scm_Init_websocket(void)
{
    ScmModule *mod;

    lws_set_log_level(LLL_ERR | LLL_WARN, NULL);

    /* Register this DSO to Gauche */
    SCM_INIT_EXTENSION(websocket);

    /* Create the module if it doesn't exist yet. */
    mod = SCM_MODULE(SCM_FIND_MODULE("libwebsockets", TRUE));

    callback_sym = SCM_INTERN("calback");

    WsClientClass = Scm_MakeForeignPointerClass(
        mod,
        "<websocket-client>",
        gwebsocket_client_print,
        gwebsocket_client_cleanup,
        SCM_FOREIGN_POINTER_KEEP_IDENTITY | SCM_FOREIGN_POINTER_MAP_NULL);

    /* Register stub-generated procedures */
    Scm_Init_websocketlib(mod);
}
