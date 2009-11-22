#include <stdlib.h>

#include "unittest/framework/TestSuite.h"
#include "unittest/suites/HelloWorldTest.h"

TestSuite ** getTestSuites() {
	static TestSuite * testSuites[2];
	
	testSuites[0] = HelloWorldTest_suite();
	testSuites[1] = NULL;
	
	return testSuites;
}
