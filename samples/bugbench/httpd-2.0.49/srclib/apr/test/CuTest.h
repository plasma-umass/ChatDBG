/*
 * Copyright (c) 2002-2006 Asim Jalis
 *
 * This library is released under the zlib/libpng license as described at
 *
 * http://www.opensource.org/licenses/zlib-license.html
 *
 * Here is the statement of the license:
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software in a
 * product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */
/*
 * This file has been modified from the original distribution.
 */

#ifndef CU_TEST_H
#define CU_TEST_H

#include <setjmp.h>
#include <stdarg.h>

/* CuString */

char* CuStrAlloc(int size);
char* CuStrCopy(const char* old);

#define CU_ALLOC(TYPE)		((TYPE*) malloc(sizeof(TYPE)))

#define HUGE_STRING_LEN	8192
#define STRING_MAX		256
#define STRING_INC		256

typedef struct
{
	int length;
	int size;
	char* buffer;
} CuString;

void CuStringInit(CuString* str);
CuString* CuStringNew(void);
void CuStringRead(CuString* str, char* path);
void CuStringAppend(CuString* str, const char* text);
void CuStringAppendChar(CuString* str, char ch);
void CuStringAppendFormat(CuString* str, const char* format, ...);
void CuStringResize(CuString* str, int newSize);

/* CuTest */

typedef struct CuTest CuTest;

typedef void (*TestFunction)(CuTest *);

struct CuTest
{
	char* name;
	TestFunction function;
        int notimpl;
	int failed;
	int ran;
	char* message;
	jmp_buf *jumpBuf;
};

void CuInit(int argc, char *argv[]);
void CuTestInit(CuTest* t, char* name, TestFunction function);
CuTest* CuTestNew(char* name, TestFunction function);
void CuFail(CuTest* tc, const char* message);
void CuNotImpl(CuTest* tc, const char* message);
void CuAssert(CuTest* tc, const char* message, int condition);
void CuAssertTrue(CuTest* tc, int condition);
void CuAssertStrEquals(CuTest* tc, const char* expected, const char* actual);
void CuAssertStrNEquals(CuTest* tc, const char* expected, const char* actual,
                        int n);
void CuAssertIntEquals(CuTest* tc, int expected, int actual);
void CuAssertPtrEquals(CuTest* tc, const void* expected, const void* actual);
void CuAssertPtrNotNull(CuTest* tc, const void* pointer);

void CuTestRun(CuTest* tc);

/* CuSuite */

#define MAX_TEST_CASES	1024	

#define SUITE_ADD_TEST(SUITE,TEST)	CuSuiteAdd(SUITE, CuTestNew(#TEST, TEST))

typedef struct
{
	char *name;
	int count;
	CuTest* list[MAX_TEST_CASES]; 
	int failCount;
	int notimplCount;

} CuSuite;


void CuSuiteInit(CuSuite* testSuite, char* name);
CuSuite* CuSuiteNew(char* name);
void CuSuiteAdd(CuSuite* testSuite, CuTest *testCase);
void CuSuiteAddSuite(CuSuite* testSuite, CuSuite* testSuite2);
void CuSuiteRun(CuSuite* testSuite);
void CuSuiteSummary(CuSuite* testSuite, CuString* summary);
void CuSuiteOverView(CuSuite* testSuite, CuString* details);
void CuSuiteDetails(CuSuite* testSuite, CuString* details);

typedef struct
{
	char *name;
	int count;
	CuSuite* list[MAX_TEST_CASES]; 
} CuSuiteList;


CuSuiteList* CuSuiteListNew(char* name);
void CuSuiteListAdd(CuSuiteList* testSuite, CuSuite *testCase);
void CuSuiteListRun(CuSuiteList* testSuite);
void CuSuiteListRunWithSummary(CuSuiteList* testSuite);
void CuSuiteListSummary(CuSuiteList* testSuite, CuString* summary);
/* Print details of test suite results; returns total number of
 * tests which failed. */
int CuSuiteListDetails(CuSuiteList* testSuite, CuString* details);
#endif /* CU_TEST_H */

