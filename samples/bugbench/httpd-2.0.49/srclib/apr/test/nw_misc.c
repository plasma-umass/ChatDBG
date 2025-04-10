#include <netware.h>
#include <screen.h>
#include "test_apr.h"

void _NonAppStop( void )
{
    pressanykey();
}

static void test_not_impl(CuTest *tc)
{
    CuNotImpl(tc, "Test not implemented on this platform yet");
}

