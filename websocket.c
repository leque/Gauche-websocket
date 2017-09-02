/*
 * websocket.c
 */

#include "websocket.h"

/*
 * The following function is a dummy one; replace it for
 * your C function definitions.
 */

ScmObj test_websocket(void)
{
    return SCM_MAKE_STR("websocket is working");
}

/*
 * Module initialization function.
 */
extern void Scm_Init_websocketlib(ScmModule*);

void Scm_Init_websocket(void)
{
    ScmModule *mod;

    /* Register this DSO to Gauche */
    SCM_INIT_EXTENSION(websocket);

    /* Create the module if it doesn't exist yet. */
    mod = SCM_MODULE(SCM_FIND_MODULE("libwebsockets", TRUE));

    /* Register stub-generated procedures */
    Scm_Init_websocketlib(mod);
}
