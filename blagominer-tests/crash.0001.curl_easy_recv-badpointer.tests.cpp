#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <curl/curl.h>

typedef std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> autocurlpointer;

// Story:

// curl_easy_recv crashed. I managed to catch the crash with debugger, problem was
// in `sendf` function on `data->` pointer, data pointer was 0x00000000. That pointer
// was taken from structure held under curl handle.
// This most probably indicates that:
// (1) curl is not really thread-safe, and/or safe against accessing closed handles
// (2) handle had been cleaned up while it still was being read by confirm_i
// Actually, after reviewing the docs, it's apparent that while previous cleanup/disconnect approach was fine for sockets
// (closing a socket does not crash send/recv even if these actively use the socket), but it not usable with curl handles.
// 'easy' api is not safe in this setup. 'multi' must be used instead.

TEST(Diag_Crash_curleasyrecv_BadPointer_DeathTest, Test00001) {
	// Test idea:
	// Check what happens when curl_easy_recv uses a handle in parallel with another thread trying to close it.
	// Can it crash badly?
	// If this test passes, then yes, IT CRASHES BADLY.

	// Possible approaches:
	// 1) create a handle, clean it up, give cleaned-up handle to curl_easy_recv
	// If it does not crash, try simulating thread race:
	// 2) create socket, make it start listening, create handle to that socket, ensure curl_easy_recv is blocked on reading data from it, try closing the curl handle
	// If it does not crash, try actual aggressive race:
	// repeatedly run curl_easy_recv in a low-prio thread, repeatedly run curl_easy_cleanup in a high-prio thread, randomize sleeps and yields in hi-prio thread, see if low-prio crashes

	ASSERT_DEATH({
		autocurlpointer testedHandle(curl_easy_init(), &curl_easy_cleanup);
		curl_easy_cleanup(testedHandle.get());

		char buff[1];
		size_t n;
		curl_easy_recv(testedHandle.get(), buff, 1, &n); // got: SEH: Access violation
	}, "");

	// Analysis:

	// it's a bit inobvious why this test is meaningful, but looking at how confirm_i is implemented helps.
	// the confirm_i only synchronizes the moment of fetching a `session` object from the queue.
	// later, it uses curl handle from that session without any synchronization. this means that when the
	// interrupt/cleanup thread tries to access the sessions, it most often is free to do so
	// and in the WORST possible case it will clean up the handle right after the confirmer_i grabbed
	// the session from the queue. So, the worst case is:
	// - confirm gets the session,
	// - cleanup cleans it up,
	// - confimer invokes curl_easy_recv
	// and from curl point of view, this is exactly what this test performs.
	// Since now we know the test crashes badly, there are only some solutions:
	// 1) add more synchronization so cleanup does not occur before reading..
	// - split the cleanup into close-sockets and the real-cleanup, do the real-cleanup only after it's known to be safe
	// - try using curl 'multi' to handle sending/receiving asynchronously on a single thread so there's no race at all
	// - or finally, drop curl and try finding another library more suitable to the case
	// Until more problems are discovered, for now, I choose second option.
	// 1) not a good idea: we want to be able to INTERRUPT at (almost) any time, adding more sync will slow it down or prevent it at all
	// 3) curl 'multi' is single-threaded so adding/removing handles from it requires sync. also, since it's "perform" loops as long as
	// there is something to handle, it MAY be hard to guarantee time restrictions. "May", because maybe there is a variation on "perform"
	// that allows some time limits. However, combining send+confirm+update in one thread MAY have some negative impact on performance,
	// as the 'update' part is used in a "polling" fashion and is called repeatedly. That's just one connection to handle, so probably
	// it will be negligble.
	// 4) too much work? although boost::beast looks NICE
	// 2) current code works, the problem is only in cleanup. If during interrupt we only forcibly break the socket connection,
	// and leave the curl handle still initialized, it should work perfectly safe. Later, actual cleanup of sessions will ensure sender/confirmer
	// threads are stuck on session synchronization, and problem solved, as long as curl does not crash when handle is alive but the socket is dead.
}

TEST(Diag_Crash_curleasyrecv_BadPointer_DeathTest, Test00002) {
	// Test idea:
	// Check what happens when curl_easy_recv uses an initialized handle which had its socket shutdown.

	autocurlpointer testedHandle(curl_easy_init(), &curl_easy_cleanup);
	ASSERT_NE(testedHandle, nullptr);

	curl_easy_setopt(testedHandle.get(), CURLOPT_URL, "http://www.example.com");
	curl_easy_setopt(testedHandle.get(), CURLOPT_CONNECT_ONLY, 1L);
	curl_easy_setopt(testedHandle.get(), CURLOPT_TIMEOUT_MS, 1000);

	CURLcode res = curl_easy_perform(testedHandle.get());
	ASSERT_EQ(res, CURLE_OK);

	curl_socket_t sockfd;
	res = curl_easy_getinfo(testedHandle.get(), CURLINFO_ACTIVESOCKET, &sockfd);
	ASSERT_EQ(res, CURLE_OK);

	// alright, we're connected

	// break the actual connection, but leave the curl handle alive
	//shutdown(sockfd, SD_xxx);
	closesocket(sockfd);

	char buff[1];
	size_t n = -123;
	res = curl_easy_recv(testedHandle.get(), buff, 1, &n); // yay, no crash

	EXPECT_NE(res, CURLE_OK); // got: CURLE_RECV_ERROR(56)
	EXPECT_TRUE(n == 0 || n == -123); // got: 0

	// Analysis:

	// Actually, that awesome. socket was forcibly closed, but the API didn't crash,
	// and even it zero'ed out the bytes-received output variable properly.
	// We have a way of actually terminating that is safe for curl.
}
