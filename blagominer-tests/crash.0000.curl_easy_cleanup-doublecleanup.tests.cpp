#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <curl/curl.h>

typedef std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> autocurlpointer;

// Story:

// curl_easy_cleanup crashed. I managed to catch the crash with debugger, problem was
// somewhere in cleanup of 'multi' interface, pointer was 0x00000000. That pointer
// was taken from structure held under curl handle.
// This most probably indicates that:
// (1) curl handles are not safe against repeated cleanup.

TEST(Diag_Crash_curleasycleanup_doublecall_DeathTest, Test00001) {
	// Test idea:
	// Check what happens when curl_easy_cleanup is called multiple times on the same handle.
	// Can it crash badly?
	// If this test passes, then yes, IT CRASHES BADLY.

	ASSERT_DEATH({
		autocurlpointer testedHandle(curl_easy_init(), &curl_easy_cleanup);

		curl_easy_cleanup(testedHandle.get()); // first cleanup
		curl_easy_cleanup(testedHandle.get()); // got: SEH: Access Violation
	}, "");

	// Analysis:

	// it's pretty obvious. curl handles are dangerous and must be used very carefully.
	// great care must be used to ensure that these are both CLEANED UP and that's done ONLY ONCE.
	// sender/confirmer/queues must be carefully reviewed to ensure that no handle is cleaned up twice or more.
	// this is especially important for the thread-interrupt feature:
	// the interrupter and the interrupted must not both attempt to perform the cleanups.
}
