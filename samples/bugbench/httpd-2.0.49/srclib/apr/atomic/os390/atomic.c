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
#include "apr_atomic.h"

#if APR_HAS_THREADS

apr_int32_t apr_atomic_add(volatile apr_atomic_t *mem, apr_int32_t val) 
{
    apr_atomic_t old, new_val; 

    old = *mem;   /* old is automatically updated on cs failure */
    do {
        new_val = old + val;
    } while (__cs(&old, (cs_t *)mem, new_val)); 

    return new_val;
}

apr_uint32_t apr_atomic_cas(volatile apr_atomic_t *mem, apr_uint32_t swap, 
                            apr_uint32_t cmp)
{
    apr_uint32_t old = cmp;
    
    __cs(&old, (cs_t *)mem, swap);
    return old; /* old is automatically updated from mem on cs failure */
}

#endif /* APR_HAS_THREADS */
