/* Copyright 2001-2004 The Apache Software Foundation
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
/* This is derived from material copyright RSA Data Security, Inc.
 * Their notice is reproduced below in its entirety.
 *
 * Copyright (C) 1990-2, RSA Data Security, Inc. Created 1990. All
 * rights reserved.
 *
 * RSA Data Security, Inc. makes no representations concerning either
 * the merchantability of this software or the suitability of this
 * software for any particular purpose. It is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 */


#include "apr.h"
#include "apr_general.h"
#include "apr_file_io.h"
#include "apr_time.h"
#include "apr_md4.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * This is a MD4 test program based on the code published in RFC 1320.
 * When run as ./testmd4 -x it should produce the following output:

MD4 test suite:
MD4 ("") = 31d6cfe0d16ae931b73c59d7e0c089c0
MD4 ("a") = bde52cb31de33e46245e05fbdbd6fb24
MD4 ("abc") = a448017aaf21d8525fc10ae87aa6729d
MD4 ("message digest") = d9130a8164549fe818874806e1c7014b
MD4 ("abcdefghijklmnopqrstuvwxyz") = d79e1c308aa5bbcdeea8ed63df412da9
MD4 ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") = 043f8582f241db351ce627e153e7f0e4
MD4 ("12345678901234567890123456789012345678901234567890123456789012345678901234567890") = e33b4ddc9c38f2199c3e7b164fcc0536

*/

/* Length of test block, number of test blocks.
 */
#define TEST_BLOCK_LEN 1000
#define TEST_BLOCK_COUNT 1000

apr_pool_t *local_pool;
apr_file_t *in, *out, *err;

/* Prints a message digest in hexadecimal.
 */
static void MDPrint (unsigned char digest[APR_MD4_DIGESTSIZE])
{
    unsigned int i;

    for (i = 0; i < APR_MD4_DIGESTSIZE; i++)
        apr_file_printf(out, "%02x", digest[i]);
}

/* Digests a string and prints the result.
 */
static void MDString(char *string)
{
    apr_md4_ctx_t context;
    unsigned char digest[APR_MD4_DIGESTSIZE];
    unsigned int len = strlen(string);

    apr_md4_init(&context);
    apr_md4_update(&context, (unsigned char *)string, len);
    apr_md4_final(digest, &context);

    apr_file_printf (out, "MD4 (\"%s\") = ", string);
    MDPrint(digest);
    apr_file_printf (out, "\n");
}

/* Measures the time to digest TEST_BLOCK_COUNT TEST_BLOCK_LEN-byte
     blocks.
 */
static void MDTimeTrial(void)
{
    apr_md4_ctx_t context;
    apr_time_t endTime, startTime;
    apr_interval_time_t timeTaken;
    unsigned char block[TEST_BLOCK_LEN], digest[APR_MD4_DIGESTSIZE];
    unsigned int i;

    apr_file_printf(out, "MD4 time trial. Digesting %d %d-byte blocks ...", 
                     TEST_BLOCK_LEN, TEST_BLOCK_COUNT);

    /* Initialize block */
    for (i = 0; i < TEST_BLOCK_LEN; i++)
        block[i] = (unsigned char)(i & 0xff);

    /* Start timer */
    startTime = apr_time_now();

    /* Digest blocks */
    apr_md4_init(&context);
    for (i = 0; i < TEST_BLOCK_COUNT; i++)
        apr_md4_update(&context, block, TEST_BLOCK_LEN);

    apr_md4_final(digest, &context);

    /* Stop timer */
    endTime = apr_time_now();
    timeTaken = endTime - startTime;

    apr_file_printf(out, " done\n");
    apr_file_printf(out, "Digest = ");
    MDPrint(digest);

    apr_file_printf(out, "\nTime = %" APR_TIME_T_FMT " seconds\n", timeTaken);
    apr_file_printf(out, "Speed = % " APR_TIME_T_FMT " bytes/second\n",
                    TEST_BLOCK_LEN * TEST_BLOCK_COUNT/timeTaken);
}

/* Digests a reference suite of strings and prints the results.
 */
static void MDTestSuite(void)
{
    apr_file_printf(out, "MD4 test suite:\n");

    MDString("");
    MDString("a");
    MDString("abc");
    MDString("message digest");
    MDString("abcdefghijklmnopqrstuvwxyz");
    MDString("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    MDString("12345678901234567890123456789012345678901234567890123456789012345678901234567890");
}

/* Digests a file and prints the result.
 */
static void MDFile(char *filename)
{
    apr_file_t *file;
    apr_md4_ctx_t context;
    apr_size_t len = 1024;
    unsigned char buffer[1024], digest[APR_MD4_DIGESTSIZE];

    if (apr_file_open(&file, filename, APR_READ, APR_OS_DEFAULT, local_pool) 
        != APR_SUCCESS)
        apr_file_printf(err, "%s can't be opened\n", filename);
    else {
        apr_md4_init(&context);
        while (apr_file_read(file, buffer, &len) != APR_SUCCESS)
        {
            apr_md4_update(&context, buffer, len);
            len = 1024;
        }
        apr_md4_final(digest, &context);

        apr_file_close(file);

        apr_file_printf(out, "MD4 (%s) = ", filename);
        MDPrint(digest);
        apr_file_printf(out, "\n");
    }
}

/* Digests the standard input and prints the result.
 */
static void MDFilter(void)
{
    apr_md4_ctx_t context;
    apr_size_t len = 16;
    unsigned char buffer[16], digest[16];

    apr_md4_init(&context);
    while (apr_file_read(in, buffer, &len) != APR_SUCCESS)
    {
        apr_md4_update(&context, buffer, len);
        len = 16;
    }
    apr_md4_update(&context, buffer, len);
    apr_md4_final(digest, &context);

    MDPrint(digest);
    apr_file_printf(out, "\n");
}


/* Main driver.

   Arguments (may be any combination):
     -sstring - digests string
     -t       - runs time trial
     -x       - runs test script
     filename - digests file
     (none)   - digests standard input
 */
int main (int argc, char **argv)
{
    int i;

    apr_initialize();
    atexit(apr_terminate);

    if (apr_pool_create(&local_pool, NULL) != APR_SUCCESS)
        exit(-1);

    apr_file_open_stdin(&in, local_pool); 
    apr_file_open_stdout(&out, local_pool); 
    apr_file_open_stderr(&err, local_pool); 

    if (argc > 1)
    {
        for (i = 1; i < argc; i++)
            if (argv[i][0] == '-' && argv[i][1] == 's')
                MDString(argv[i] + 2);
            else if (strcmp(argv[i], "-t") == 0)
                MDTimeTrial();
            else if (strcmp (argv[i], "-x") == 0)
                MDTestSuite();
            else
                MDFile(argv[i]);
    }
    else
        MDFilter();

    return 0;
}
