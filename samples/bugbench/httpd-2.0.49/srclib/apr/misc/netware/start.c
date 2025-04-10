/* Copyright 2000-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apr.h"
#include "apr_general.h"
#include "apr_pools.h"
#include "apr_signal.h"

#include "apr_arch_misc.h"       /* for WSAHighByte / WSALowByte */
#include "apr_arch_proc_mutex.h" /* for apr_proc_mutex_unix_setup_lock() */
#include "apr_arch_internal_time.h"


APR_DECLARE(apr_status_t) apr_app_initialize(int *argc, 
                                             const char * const * *argv, 
                                             const char * const * *env)
{
    /* An absolute noop.  At present, only Win32 requires this stub, but it's
     * required in order to move command arguments passed through the service
     * control manager into the process, and it's required to fix the char*
     * data passed in from win32 unicode into utf-8, win32's apr internal fmt.
     */
    return apr_initialize();
}

APR_DECLARE(apr_status_t) apr_initialize(void)
{
    apr_pool_t *pool;
    apr_status_t status;
    int iVersionRequested;
    WSADATA wsaData;
    int err;

    /* Register the NLM as using APR. If it is already
        registered then just return. */
    if (register_NLM(getnlmhandle()) != 0) {
        return APR_SUCCESS;
    }

    /* apr_pool_initialize() is being called from the library
        startup code since all of the memory resources belong 
        to the library rather than the application. */
    
    if (apr_pool_create(&pool, NULL) != APR_SUCCESS) {
        return APR_ENOPOOL;
    }

    apr_pool_tag(pool, "apr_initilialize");

    iVersionRequested = MAKEWORD(WSAHighByte, WSALowByte);
    err = WSAStartup((WORD) iVersionRequested, &wsaData);
    if (err) {
        return err;
    }
    if (LOBYTE(wsaData.wVersion) != WSAHighByte ||
        HIBYTE(wsaData.wVersion) != WSALowByte) {
        WSACleanup();
        return APR_EEXIST;
    }
    
    apr_signal_init(pool);
//    setGlobalPool((void*)pool);

    return APR_SUCCESS;
}

APR_DECLARE_NONSTD(void) apr_terminate(void)
{
    /* Unregister the NLM. If it is not registered
        then just return. */
    if (unregister_NLM(getnlmhandle()) != 0) {
        return;
    }

    /* apr_pool_terminate() is being called from the 
        library shutdown code since the memory resources
        belong to the library rather than the application */

    /* Just clean up the memory for the app that is going
        away. */
    netware_pool_proc_cleanup ();
    WSACleanup();
}

APR_DECLARE(void) apr_terminate2(void)
{
    apr_terminate();
}
