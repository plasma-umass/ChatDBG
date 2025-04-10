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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "apr_md5.h"
#include "apr_xlate.h"
#include "apr_general.h"
#include "test_apu.h"

int cur; 

struct testcase {
    const char *s;
    const char *digest;
};

struct testcase testcases[] =
{
    {"Jeff was here!",
     "\xa5\x25\x8a\x89\x11\xb2\x9d\x1f\x81\x75\x96\x3b\x60\x94\x49\xc0"},
    {"01234567890aBcDeFASDFGHJKLPOIUYTR"
     "POIUYTREWQZXCVBN  LLLLLLLLLLLLLLL",
     "\xd4\x1a\x06\x2c\xc5\xfd\x6f\x24\x67\x68\x56\x7c\x40\x8a\xd5\x69"},
    {"111111118888888888888888*******%%%%%%%%%%#####"
     "142134u8097289720432098409289nkjlfkjlmn,m..   ",
     "\xb6\xea\x5b\xe8\xca\x45\x8a\x33\xf0\xf1\x84\x6f\xf9\x65\xa8\xe1"},
    {"01234567890aBcDeFASDFGHJKLPOIUYTR"
     "POIUYTREWQZXCVBN  LLLLLLLLLLLLLLL"
     "01234567890aBcDeFASDFGHJKLPOIUYTR"
     "POIUYTREWQZXCVBN  LLLLLLLLLLLLLLL"
     "1",
     "\xd1\xa1\xc0\x97\x8a\x60\xbb\xfb\x2a\x25\x46\x9d\xa5\xae\xd0\xb0"}
};

static void try(const void *buf, apr_size_t bufLen, apr_xlate_t *xlate,
                const void *digest)
{
    int i;
    apr_md5_ctx_t context;
    unsigned char hash[APR_MD5_DIGESTSIZE];
    
    printf("Trying translation %d\n", cur + 1);

    STD_TEST_NEQ("    apr_md5_init", apr_md5_init(&context))

    if (xlate) {
#if APR_HAS_XLATE
        STD_TEST_NEQ("    apr_md5_set_xlate", 
                     apr_md5_set_xlate(&context, xlate))
#else
        printf("    Didn't expect a translation handle! Not fatal.\n");
#endif
    }
    
    STD_TEST_NEQ("    apr_md5_update", apr_md5_update(&context, buf, bufLen))
    STD_TEST_NEQ("    apr_md5_final", apr_md5_final(hash, &context))

    printf("     (MD5 hash : ");
    for (i = 0; i < APR_MD5_DIGESTSIZE; i++) {
        printf("%02x",hash[i]);
    }
    
    printf(")\n");

    printf("%-60s", "    Checking hash against expected");
    if (memcmp(hash, digest, APR_MD5_DIGESTSIZE)) {
        /* This is a fatal error...report on stderr */
        fprintf(stderr, "The digest is not as expected!\n");
#if 'A' != 0x41
        fprintf(stderr,
                "Maybe you didn't tell me what character sets "
                "to translate between?\n"
                "The expected digest is based on the string "
                "being in ASCII.\n");
#endif
    }
    printf("OK\n");
}

int main(int argc, char **argv)
{
    apr_status_t rv;
    apr_xlate_t *xlate = NULL;
    apr_pool_t *pool;
    const char *src = NULL, *dst = NULL;

    switch(argc) {
    case 1:
        break;
    case 3:
        src = argv[1];
        dst = argv[2];
        break;
    default:
        fprintf(stderr,
                "Usage: %s [src-charset dst-charset]\n",
                argv[0]);
        exit(1);
    }

    rv = apr_initialize();
    assert(!rv);
    atexit(apr_terminate);

    printf("APR MD5 Test\n============\n\n");
    STD_TEST_NEQ("Creating pool", apr_pool_create(&pool, NULL))

    if (src) {
#if APR_HAS_XLATE
        STD_TEST_NEQ("Opening xlate functions", 
                     apr_xlate_open(&xlate, dst, src, pool))
#else
        /* This isn't a fatal error, so just report it... */
        printf("APR doesn't implement translation for this "
               "configuration.\n");
#endif
    }

    for (cur = 0; cur < sizeof(testcases) / sizeof(testcases[0]); cur++) {
        try(testcases[cur].s, strlen(testcases[cur].s), xlate,
            testcases[cur].digest);
    }

    printf("\nMD5 Test passed.\n");    
    return 0;
}
