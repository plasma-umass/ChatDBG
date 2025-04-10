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

#include <stdio.h>
#include <stdlib.h>

#include "apr_general.h"
#include "apr_uuid.h"


int main(int argc, char **argv)
{
    apr_uuid_t uuid;
    apr_uuid_t uuid2;
    char buf[APR_UUID_FORMATTED_LENGTH + 1];
    int retcode = 0;

    apr_initialize();
    atexit(apr_terminate);

    apr_uuid_get(&uuid);
    apr_uuid_format(buf, &uuid);
    printf("UUID: %s\n", buf);

    apr_uuid_parse(&uuid2, buf);
    if (memcmp(&uuid, &uuid2, sizeof(uuid)) == 0)
        printf("Parse appears to work.\n");
    else {
        printf("ERROR: parse produced a different UUID.\n");
        retcode = 1;
    }

    apr_uuid_format(buf, &uuid2);
    printf("parsed/reformatted UUID: %s\n", buf);

    /* generate two of them quickly */
    apr_uuid_get(&uuid);
    apr_uuid_get(&uuid2);
    apr_uuid_format(buf, &uuid);
    printf("UUID 1: %s\n", buf);
    apr_uuid_format(buf, &uuid2);
    printf("UUID 2: %s\n", buf);

    return retcode;
}
