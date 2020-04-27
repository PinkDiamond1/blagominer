#include "network.h"

#include <curl/curl.h>

#include "logger.h"
#include "loggerCsv.h"
#include "error.h"
#include "accounts.h"
#include "filemonitor.h"
#include "picohttpparser.h"
#include "reference/diskcoin/DiskcoinMath.h"

#include "blagominer.h" // for use_debug, total_size

static std::map <u_long, unsigned long long> satellite_size; // Structure with volumes of satellite plots

void init_coinNetwork(std::shared_ptr<t_coin_info> coin)
{
	coin->network = std::make_shared<t_network_info>();
	coin->network->nodeaddr = "localhost";
	coin->network->nodeport = "8125"; // bhd: 8732
	coin->network->noderoot = "burst";
	coin->network->submitTimeout = 1000;
	coin->network->updateraddr = "localhost";
	coin->network->updaterport = "8125"; // bhd: 8732
	coin->network->updaterroot = "burst";
	coin->network->enable_proxy = false;
	coin->network->proxyport = "8125"; // bhd: 8732
	coin->network->send_interval = 100;
	coin->network->update_interval = 1000;
	coin->network->proxy_update_interval = 500;
	coin->network->network_quality = -1;
}

void hostname_to_ip(char const *const  in_addr, char* out_addr)
{
	struct addrinfo *result = nullptr;
	struct addrinfo *ptr = nullptr;
	struct addrinfo hints;
	DWORD dwRetval;
	struct sockaddr_in  *sockaddr_ipv4;

	RtlSecureZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	dwRetval = getaddrinfo(in_addr, NULL, &hints, &result);
	if (dwRetval != 0) {
		Log(L" getaddrinfo failed with error: %llu", dwRetval);
		WSACleanup();
		exit(-1);
	}
	for (ptr = result; ptr != nullptr; ptr = ptr->ai_next) {

		if (ptr->ai_family == AF_INET)
		{
			sockaddr_ipv4 = (struct sockaddr_in *) ptr->ai_addr;
			char str[INET_ADDRSTRLEN];
			inet_ntop(hints.ai_family, &(sockaddr_ipv4->sin_addr), str, INET_ADDRSTRLEN);
			//strcpy(out_addr, inet_ntoa(sockaddr_ipv4->sin_addr));
			strcpy_s(out_addr, 50, str);
			Log(L"Address: %S defined as %S", in_addr, out_addr);
		}
	}
	freeaddrinfo(result);
}


void proxy_i(std::shared_ptr<t_coin_info> coinInfo)
{
	const wchar_t* proxyName = coinInfo->coinname.c_str();
	int iResult;
	std::vector<char, heap_allocator<char>> buffer(1000, theHeap);
	std::vector<char, heap_allocator<char>> tmp_buffer(1000, theHeap);
	SOCKET ServerSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;
	struct addrinfo *result = nullptr;
	struct addrinfo hints;

	RtlSecureZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(nullptr, coinInfo->network->proxyport.c_str(), &hints, &result);
	if (iResult != 0) {
		gui->printToConsole(12, true, false, true, false, L"PROXY %s: getaddrinfo failed with error: %i", proxyName, iResult);
	}

	ServerSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ServerSocket == INVALID_SOCKET) {
		gui->printToConsole(12, true, false, true, false, L"PROXY %s: socket failed with error: %i", proxyName, WSAGetLastError());
		freeaddrinfo(result);
	}

	iResult = bind(ServerSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		gui->printToConsole(12, true, false, true, false, L"PROXY %s: bind failed with error: %i", proxyName, WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ServerSocket);
	}
	freeaddrinfo(result);
	BOOL l = TRUE;
	iResult = ioctlsocket(ServerSocket, FIONBIO, (unsigned long*)&l);
	if (iResult == SOCKET_ERROR)
	{
		Log(L"Proxy %s: ! Error ioctlsocket's: %i", proxyName, WSAGetLastError());
		gui->printToConsole(12, true, false, true, false, L"PROXY %s: ioctlsocket failed: %i", proxyName, WSAGetLastError());
	}

	iResult = listen(ServerSocket, 8);
	if (iResult == SOCKET_ERROR) {
		gui->printToConsole(12, true, false, true, false, L"PROXY %s: listen failed with error: %i", proxyName, WSAGetLastError());
		closesocket(ServerSocket);
	}
	Log(L"Proxy %s thread started", proxyName);

	while (!exit_flag)
	{
		struct sockaddr_in client_socket_address;
		int iAddrSize = sizeof(struct sockaddr_in);
		ClientSocket = accept(ServerSocket, (struct sockaddr *)&client_socket_address, (socklen_t*)&iAddrSize);

		char client_address_str[INET_ADDRSTRLEN];
		inet_ntop(hints.ai_family, &(client_socket_address.sin_addr), client_address_str, INET_ADDRSTRLEN);

		if (ClientSocket == INVALID_SOCKET)
		{
			if (WSAGetLastError() != WSAEWOULDBLOCK)
			{
				Log(L"Proxy %s:! Error Proxy's accept: %i", proxyName, WSAGetLastError());
				gui->printToConsole(12, true, false, true, false, L"PROXY %s: can't accept. Error: %i", proxyName, WSAGetLastError());
			}
		}
		else
		{
			RtlSecureZeroMemory(buffer.data(), buffer.size());
			do {
				RtlSecureZeroMemory(tmp_buffer.data(), tmp_buffer.size());
				iResult = recv(ClientSocket, tmp_buffer.data(), (int)(tmp_buffer.size() - 1), 0);
				strcat_s(buffer.data(), buffer.size(), tmp_buffer.data());
			} while (iResult > 0);

			unsigned long long get_accountId = 0;
			unsigned long long get_nonce = 0;
			unsigned long long get_deadline = 0;
			unsigned long long get_totalsize = 0;
			// locate HTTP header
			char *find = strstr(buffer.data(), "\r\n\r\n");
			if (find != nullptr)
			{
				const unsigned long long targetDeadlineInfo = getTargetDeadlineInfo(coinInfo);
				if (strstr(buffer.data(), "submitNonce") != nullptr)
				{

					char *startaccountId = strstr(buffer.data(), "accountId=");
					if (startaccountId != nullptr)
					{
						startaccountId = strpbrk(startaccountId, "0123456789");
						char *endaccountId = strpbrk(startaccountId, "& }\"");

						char *startnonce = strstr(buffer.data(), "nonce=");
						char *startdl = strstr(buffer.data(), "deadline=");
						char *starttotalsize = strstr(buffer.data(), "X-Capacity");
						if ((startnonce != nullptr) && (startdl != nullptr))
						{
							startnonce = strpbrk(startnonce, "0123456789");
							char *endnonce = strpbrk(startnonce, "& }\"");
							startdl = strpbrk(startdl, "0123456789");
							char *enddl = strpbrk(startdl, "& }\"");

							endaccountId[0] = 0;
							endnonce[0] = 0;
							enddl[0] = 0;

							get_accountId = _strtoui64(startaccountId, 0, 10);
							get_nonce = _strtoui64(startnonce, 0, 10);
							get_deadline = _strtoui64(startdl, 0, 10);

							if (starttotalsize != nullptr)
							{
								starttotalsize = strpbrk(starttotalsize, "0123456789");
								char *endtotalsize = strpbrk(starttotalsize, "& }\"");
								endtotalsize[0] = 0;
								get_totalsize = _strtoui64(starttotalsize, 0, 10);
								satellite_size.insert(std::pair <u_long, unsigned long long>(client_socket_address.sin_addr.S_un.S_addr, get_totalsize));
							}
							EnterCriticalSection(&coinInfo->locks->sharesLock);
							coinInfo->mining->shares.push_back(std::make_shared<t_shares>(
								client_address_str, get_accountId, get_deadline, get_nonce, get_deadline, coinInfo->mining->currentHeight, coinInfo->mining->currentBaseTarget));
							LeaveCriticalSection(&coinInfo->locks->sharesLock);

							gui->printToConsole(2, true, false, true, false, L"[%20llu|%-10s|Proxy ] DL found     : %s {%S}", get_accountId, proxyName,
								toWStr(get_deadline / coinInfo->mining->currentBaseTarget, 11).c_str(), client_address_str);
							Log(L"Proxy %s: received DL %llu from %S", proxyName, get_deadline / coinInfo->mining->currentBaseTarget, client_address_str);
							
							// We confirm
							RtlSecureZeroMemory(buffer.data(), buffer.size());
							size_t acc = Get_index_acc(get_accountId, coinInfo, targetDeadlineInfo);
							int bytes = sprintf_s(buffer.data(), buffer.size(), "HTTP/1.0 200 OK\r\nConnection: close\r\n\r\n{\"result\": \"proxy\",\"accountId\": %llu,\"deadline\": %llu,\"targetDeadline\": %llu}", get_accountId, get_deadline / coinInfo->mining->currentBaseTarget, coinInfo->mining->bests[acc].targetDeadline);
							iResult = send(ClientSocket, buffer.data(), bytes, 0);
							if (iResult == SOCKET_ERROR)
							{
								Log(L"Proxy %s: ! Error sending to client: %i", proxyName, WSAGetLastError());
								gui->printToConsole(12, true, false, true, false, L"PROXY %s: failed sending to client: %i", proxyName, WSAGetLastError());
							}
							else
							{
								if (use_debug)
								{
									gui->printToConsole(9, true, false, true, false, L"[%20llu|%-10s|Proxy ] DL confirmed to             %S",
										get_accountId, proxyName, client_address_str);
								}
								Log(L"Proxy %s: sent confirmation to %S", proxyName, client_address_str);
							}
						}
					}
				}
				else
				{
					if (strstr(buffer.data(), "getMiningInfo") != nullptr)
					{
						char* str_signature = getCurrentStrSignature(coinInfo);

						RtlSecureZeroMemory(buffer.data(), buffer.size());
						int bytes = sprintf_s(buffer.data(), buffer.size(), "HTTP/1.0 200 OK\r\nConnection: close\r\n\r\n{\"baseTarget\":\"%llu\",\"height\":\"%llu\",\"generationSignature\":\"%s\",\"targetDeadline\":%llu}", coinInfo->mining->currentBaseTarget, coinInfo->mining->currentHeight, str_signature, targetDeadlineInfo);
						delete[] str_signature;
						iResult = send(ClientSocket, buffer.data(), bytes, 0);
						if (iResult == SOCKET_ERROR)
						{
							Log(L"Proxy %s: ! Error sending to client: %i", proxyName, WSAGetLastError());
							gui->printToConsole(12, true, false, true, false, L"PROXY %s: failed sending to client: %i", proxyName, WSAGetLastError());
						}
						else if (loggingConfig.logAllGetMiningInfos)
						{
							Log(L"Proxy %s: sent update to %S: %S", proxyName, client_address_str, buffer.data());
						}
					}
					else
					{
						if ((strstr(buffer.data(), "getBlocks") != nullptr) || (strstr(buffer.data(), "getAccount") != nullptr) || (strstr(buffer.data(), "getRewardRecipient") != nullptr))
						{
							; // do not do anything, do not mistake, skip
						}
						else
						{
							find[0] = 0;
							//You can crash the miner when the proxy is enabled and you open the address in a browser.  wprintw("PROXY: %s\n", "Error", 0);
							gui->printToConsole(15, true, false, true, false, L"PROXY %s: %S", proxyName, buffer.data());
						}
					}
				}
			}
			iResult = closesocket(ClientSocket);
		}
		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::milliseconds(coinInfo->network->proxy_update_interval));
	}
}

void decreaseNetworkQuality(std::shared_ptr<t_coin_info> coin) {
	if (coin->network->network_quality < 0) {
		coin->network->network_quality = 0;
	}
	else if (coin->network->network_quality > 0) {
		coin->network->network_quality--;
	}
}

void increaseNetworkQuality(std::shared_ptr<t_coin_info> coin) {
	if (coin->network->network_quality < 0) {
		coin->network->network_quality = 100;
	}
	else if (coin->network->network_quality < 100) {
		coin->network->network_quality++;
	}
}


void __impl__send_i__sockets(std::vector<char, heap_allocator<char>>& buffer, std::shared_ptr<t_coin_info> coinInfo, std::vector<std::shared_ptr<t_session>>& tmpSessions, unsigned long long targetDeadlineInfo, std::shared_ptr<t_shares> share)
{
	const wchar_t* senderName = coinInfo->coinname.c_str();

	SOCKET ConnectSocket = INVALID_SOCKET;
	std::unique_ptr<SOCKET, void(*)(SOCKET*)> guardConnectSocket(&ConnectSocket, [](SOCKET* s) {closesocket(*s); });

	int iResult = 0;

	struct addrinfo *result = nullptr;
	struct addrinfo hints;

	RtlSecureZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo(coinInfo->network->nodeaddr.c_str(), coinInfo->network->nodeport.c_str(), &hints, &result);
	if (iResult != 0) {
		decreaseNetworkQuality(coinInfo);
		gui->printToConsole(12, true, false, true, false, L"[%20llu|%-10s|Sender] getaddrinfo failed with error: %i", share->account_id, senderName, iResult);
		return;
	}
	ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ConnectSocket == INVALID_SOCKET) {
		decreaseNetworkQuality(coinInfo);
		gui->printToConsole(12, true, false, true, false, L"SENDER %s: socket failed with error: %i", senderName, WSAGetLastError());
		freeaddrinfo(result);
		return;
	}
	setsockopt(ConnectSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&coinInfo->network->submitTimeout, sizeof(unsigned));
	iResult = connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		decreaseNetworkQuality(coinInfo);
		Log(L"Sender %s:! Error Sender's connect: %i", senderName, WSAGetLastError());
		gui->printToConsole(12, true, false, true, false, L"SENDER %s: can't connect. Error: %i", senderName, WSAGetLastError());
		freeaddrinfo(result);
		return;
	}
	else
	{
		freeaddrinfo(result);

		int bytes = 0;
		RtlSecureZeroMemory(buffer.data(), buffer.size());
		if (coinInfo->mining->miner_mode == 0)
		{
			bytes = sprintf_s(buffer.data(), buffer.size(), "POST /%s?requestType=submitNonce&secretPhrase=%s&nonce=%llu HTTP/1.0\r\nConnection: close\r\n\r\n",
				coinInfo->network->noderoot.c_str(), coinInfo->network->solopass.c_str(), share->nonce);
		}
		if (coinInfo->mining->miner_mode == 1)
		{
			unsigned long long total = total_size / 1024 / 1024 / 1024;
			for (auto It = satellite_size.begin(); It != satellite_size.end(); ++It) total = total + It->second;

			char const* format = "POST /%s?requestType=submitNonce&accountId=%llu&nonce=%llu&deadline=%llu%s HTTP/1.0\r\nHost: %s:%s\r\nX-Miner: Blago %S\r\nX-Capacity: %llu\r\n%sContent-Length: 0\r\nConnection: close\r\n\r\n";
			bytes = sprintf_s(buffer.data(), buffer.size(), format,
				coinInfo->network->noderoot.c_str(), share->account_id, share->nonce, share->best, coinInfo->network->sendextraquery.c_str(),
				coinInfo->network->nodeaddr.c_str(), coinInfo->network->nodeport.c_str(), version.c_str(), total, coinInfo->network->sendextraheader.c_str());
		}

		// Sending to server
		iResult = send(ConnectSocket, buffer.data(), bytes, 0);
		if (iResult == SOCKET_ERROR)
		{
			decreaseNetworkQuality(coinInfo);
			Log(L"Sender %s: ! Error deadline's sending: %i", senderName, WSAGetLastError());
			gui->printToConsole(12, true, false, true, false, L"SENDER %s: send failed: %i", senderName, WSAGetLastError());
			return;
		}
		else
		{
			increaseNetworkQuality(coinInfo);
			gui->printNetworkDeadlineSent(
				share->account_id,
				senderName,
				share->deadline);

			tmpSessions.push_back(std::make_shared<t_session>(ConnectSocket, share->deadline, *share));
			guardConnectSocket.release();

			Log(L"[%20llu] Sender %s: Setting bests targetDL: %10llu", share->account_id, senderName, share->deadline);
			coinInfo->mining->bests[Get_index_acc(share->account_id, coinInfo, targetDeadlineInfo)].targetDeadline = share->deadline;
			EnterCriticalSection(&coinInfo->locks->sharesLock);
			if (!coinInfo->mining->shares.empty()) {
				coinInfo->mining->shares.erase(coinInfo->mining->shares.begin());
			}
			LeaveCriticalSection(&coinInfo->locks->sharesLock);
		}
	}
}

void __impl__send_i__curl(std::vector<char, heap_allocator<char>>& buffer, std::shared_ptr<t_coin_info> coinInfo, std::vector<std::shared_ptr<t_session2>>& tmpSessions, unsigned long long targetDeadlineInfo, std::shared_ptr<t_shares> share)
{
	bool failed = false;

	const wchar_t* senderName = coinInfo->coinname.c_str();
	bool newBlock = false;

	// curl_easy_cleanup is safe here: we either own the handle and clean it up here,
	// or we release it and pass to the session queue and then don't clean it up here.
	std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
	if (!curl) {
		failed = true;
	}
	else {
		if (false) { // coinInfo->network->SKIP_PEER_VERIFICATION
			/*
			 * If you want to connect to a site who isn't using a certificate that is
			 * signed by one of the certs in the CA bundle you have, you can skip the
			 * verification of the server's certificate. This makes the connection
			 * A LOT LESS SECURE.
			 *
			 * If you have a CA cert for the server stored someplace else than in the
			 * default bundle, then the CURLOPT_CAPATH option might come handy for
			 * you.
			 */
			curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0L);
		}

		if (false) { // coinInfo->network->SKIP_HOSTNAME_VERIFICATION
			/*
			 * If the site you're connecting to uses a different host name that what
			 * they have mentioned in their server certificate's commonName (or
			 * subjectAltName) fields, libcurl will refuse to connect. You can skip
			 * this check, but this will make the connection less secure.
			 */
			curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 0L);
		}

		std::string root = "https://";
		root += coinInfo->network->nodeaddr;
		root += ":";
		root += coinInfo->network->nodeport;

		curl_easy_setopt(curl.get(), CURLOPT_URL, root.c_str());
		curl_easy_setopt(curl.get(), CURLOPT_CONNECT_ONLY, 1L);
		curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT_MS, coinInfo->network->submitTimeout);

		CURLcode res = curl_easy_perform(curl.get());

		curl_socket_t sockfd;
		if (res == CURLE_OK) {
			res = curl_easy_getinfo(curl.get(), CURLINFO_ACTIVESOCKET, &sockfd);
		}

		if (res == CURLE_OK) {
			fd_set writefds;
			FD_ZERO(&writefds);
			FD_SET(sockfd, &writefds);

			RtlSecureZeroMemory(buffer.data(), buffer.size());
			int bytes = 0;
			if (coinInfo->mining->miner_mode == 0)
			{
				bytes = sprintf_s(buffer.data(), buffer.size(), "POST /%s?requestType=submitNonce&secretPhrase=%s&nonce=%llu HTTP/1.0\r\nConnection: close\r\n\r\n",
					coinInfo->network->noderoot.c_str(), coinInfo->network->solopass.c_str(), share->nonce);
			}
			if (coinInfo->mining->miner_mode == 1)
			{
				unsigned long long total = total_size / 1024 / 1024 / 1024;
				for (auto It = satellite_size.begin(); It != satellite_size.end(); ++It) total = total + It->second;

				char const* format = "POST /%s?requestType=submitNonce&accountId=%llu&nonce=%llu&deadline=%llu%s HTTP/1.0\r\nHost: %s:%s\r\nX-Miner: Blago %S\r\nX-Capacity: %llu\r\n%sContent-Length: 0\r\nConnection: close\r\n\r\n";
				bytes = sprintf_s(buffer.data(), buffer.size(), format,
					coinInfo->network->noderoot.c_str(), share->account_id, share->nonce, share->best, coinInfo->network->sendextraquery.c_str(),
					coinInfo->network->nodeaddr.c_str(), coinInfo->network->nodeport.c_str(), version.c_str(), total, coinInfo->network->sendextraheader.c_str());
			}

			struct timeval tv;
			tv.tv_sec = coinInfo->network->submitTimeout / 1000;
			tv.tv_usec = (coinInfo->network->submitTimeout % 1000) * 1000;

			size_t total_sent = 0;
			do { // TODO: at some point of time, refactor that loop. also, copy&paste&adapt for RECV - maybe that's where 10060 come from?
				size_t sent = 0;
				res = curl_easy_send(curl.get(), buffer.data() + total_sent, bytes - total_sent, &sent);
				total_sent += sent;
				if (total_sent >= bytes)
					break;
				if (res == CURLE_AGAIN) {
					int res2 = select(0, NULL, &writefds, &writefds, &tv);
					if (res2 != 1) {
						res = CURLE_OPERATION_TIMEDOUT;
						break;
					}
					if (!FD_ISSET(sockfd, &writefds)) {
						res = CURLE_OPERATION_TIMEDOUT;
						break;
					}
				}
			} while (res == CURLE_AGAIN);
		}

		/* Check for errors */
		if (res != CURLE_OK) {
			decreaseNetworkQuality(coinInfo);
			Log(L"Sender %s: ! Error deadline's sending: %S", senderName, curl_easy_strerror(res));
			gui->printToConsole(12, true, false, true, false, L"SENDER %s: send failed: %S", senderName, curl_easy_strerror(res));
			failed = true;
		}
		else {
			increaseNetworkQuality(coinInfo);
			gui->printNetworkDeadlineSent(
				share->account_id,
				senderName,
				share->deadline);

			tmpSessions.push_back(std::make_shared<t_session2>(curl.get(), share->deadline, *share));
			curl.release();

			Log(L"[%20llu] Sender %s: Setting bests targetDL: %10llu", share->account_id, senderName, share->deadline);
			coinInfo->mining->bests[Get_index_acc(share->account_id, coinInfo, targetDeadlineInfo)].targetDeadline = share->deadline;
			EnterCriticalSection(&coinInfo->locks->sharesLock);
			if (!coinInfo->mining->shares.empty()) {
				coinInfo->mining->shares.erase(coinInfo->mining->shares.begin());
			}
			LeaveCriticalSection(&coinInfo->locks->sharesLock);
		}
	}
}

void send_i(std::shared_ptr<t_coin_info> coinInfo)
{
	const wchar_t* senderName = coinInfo->coinname.c_str();
	Log(L"Sender %s: started thread", senderName);

	std::vector<char, heap_allocator<char>> buffer(1000, theHeap);

	std::vector<std::shared_ptr<t_session>> tmpSessions;
	std::vector<std::shared_ptr<t_session2>> tmpSessions2;
	while (!exit_flag && !coinInfo->locks->stopRoundSpecificNetworkingThreads) {
		std::shared_ptr<t_shares> share;

		EnterCriticalSection(&coinInfo->locks->sharesLock);
		if (!coinInfo->mining->shares.empty()) {
			share = coinInfo->mining->shares.front();
		}
		LeaveCriticalSection(&coinInfo->locks->sharesLock);

		if (share == nullptr) {
			// No more data for now

			if (!tmpSessions.empty()) {
				EnterCriticalSection(&coinInfo->locks->sessionsLock);
				for (auto& session : tmpSessions) {
					coinInfo->network->sessions.push_back(session);
				}
				LeaveCriticalSection(&coinInfo->locks->sessionsLock);
				tmpSessions.clear();
			}
			if (!tmpSessions2.empty()) {
				EnterCriticalSection(&coinInfo->locks->sessions2Lock);
				for (auto& session : tmpSessions2) {
					coinInfo->network->sessions2.push_back(session);
				}
				LeaveCriticalSection(&coinInfo->locks->sessions2Lock);
				tmpSessions2.clear();
			}

			std::this_thread::yield();
			std::this_thread::sleep_for(std::chrono::milliseconds(coinInfo->network->send_interval));
			continue;
		}

		const unsigned long long targetDeadlineInfo = getTargetDeadlineInfo(coinInfo);
		// Disable the ball if it is larger than the current targetDeadline, relevant for the Proxy mode
		if (share->deadline > coinInfo->mining->bests[Get_index_acc(share->account_id, coinInfo, targetDeadlineInfo)].targetDeadline)
		{
			Log(L"[%20llu|%-10s|Sender] DL discarded : %llu > %llu",
				share->account_id, senderName, share->deadline,
				coinInfo->mining->bests[Get_index_acc(share->account_id, coinInfo, targetDeadlineInfo)].targetDeadline);
			if (use_debug)
			{
				gui->debugNetworkDeadlineDiscarded(
					share->account_id, senderName, share->deadline,
					coinInfo->mining->bests[Get_index_acc(share->account_id, coinInfo, targetDeadlineInfo)].targetDeadline);
			}
			EnterCriticalSection(&coinInfo->locks->sharesLock);
			if (!coinInfo->mining->shares.empty()) {
				coinInfo->mining->shares.erase(coinInfo->mining->shares.begin());
			}
			LeaveCriticalSection(&coinInfo->locks->sharesLock);
			continue;
		}

		if (share->height != coinInfo->mining->currentHeight) {
			Log(L"Sender %s: DL %llu from block %llu discarded. New block %llu",
				senderName, share->deadline, share->height, coinInfo->mining->currentHeight);
			EnterCriticalSection(&coinInfo->locks->sharesLock);
			if (!coinInfo->mining->shares.empty()) {
				coinInfo->mining->shares.erase(coinInfo->mining->shares.begin());
			}
			LeaveCriticalSection(&coinInfo->locks->sharesLock);
			continue;
		}

		if (coinInfo->network->usehttps)
			__impl__send_i__curl(buffer, coinInfo, tmpSessions2, targetDeadlineInfo, share);
		else
			__impl__send_i__sockets(buffer, coinInfo, tmpSessions, targetDeadlineInfo, share);
	}

	Log(L"Sender %s: All work done, shutting down.", senderName);
}


bool __impl__confirm_i__sockets(std::vector<char, heap_allocator<char>>& buffer, std::shared_ptr<t_coin_info> coinInfo, rapidjson::Document& output, char*& find, bool& nonJsonSuccessDetected, std::shared_ptr<t_session>& session) {
	bool failed = false;

	const wchar_t* confirmerName = coinInfo->coinname.c_str();

	SOCKET ConnectSocket;
	int iResult = 0;

	EnterCriticalSection(&coinInfo->locks->sessionsLock);
	if (!coinInfo->network->sessions.empty()) {
		session = coinInfo->network->sessions.front();
	}
	LeaveCriticalSection(&coinInfo->locks->sessionsLock);

	if (session == nullptr) {
		// No more data for now

		return true;
	}

	if (session->body.height != coinInfo->mining->currentHeight) {
		Log(L"Confirmer %s: DL %llu from block %llu discarded. New block %llu",
			confirmerName, session->deadline, session->body.height, coinInfo->mining->currentHeight);
		EnterCriticalSection(&coinInfo->locks->sessionsLock);
		if (!coinInfo->network->sessions.empty()) {
			iResult = closesocket(coinInfo->network->sessions.front()->Socket);
			Log(L"Confirmer %s: Close socket. Code = %i", confirmerName, WSAGetLastError());
			coinInfo->network->sessions.erase(coinInfo->network->sessions.begin());
		}
		LeaveCriticalSection(&coinInfo->locks->sessionsLock);
		return true;
	}

	const unsigned long long targetDeadlineInfo = getTargetDeadlineInfo(coinInfo);
		
	ConnectSocket = session->Socket;

	// Make socket blocking
	BOOL l = FALSE;
	iResult = ioctlsocket(ConnectSocket, FIONBIO, (unsigned long*)&l);
	if (iResult == SOCKET_ERROR)
	{
		decreaseNetworkQuality(coinInfo);
		Log(L"Confirmer %s: ! Error ioctlsocket's: %i", confirmerName, WSAGetLastError());
		gui->printToConsole(12, true, false, true, false, L"SENDER %s: ioctlsocket failed: %i", confirmerName, WSAGetLastError());
		return true;
	}
	RtlSecureZeroMemory(buffer.data(), buffer.size());
	iResult = recv(ConnectSocket, buffer.data(), (int)buffer.size(), 0); // TODO: no -1 for \0, no loop&reread...

	if (iResult == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK) // disconnect, silently re-send deadline
		{
			decreaseNetworkQuality(coinInfo);
			Log(L"Confirmer %s: ! Error getting confirmation for DL: %llu  code: %i", confirmerName, session->deadline, WSAGetLastError());
			EnterCriticalSection(&coinInfo->locks->sessionsLock);
			if (!coinInfo->network->sessions.empty()) {
				iResult = closesocket(coinInfo->network->sessions.front()->Socket);
				Log(L"Confirmer %s: Close socket. Code = %i", confirmerName, WSAGetLastError());
				coinInfo->network->sessions.erase(coinInfo->network->sessions.begin());
			}
			LeaveCriticalSection(&coinInfo->locks->sessionsLock);
			EnterCriticalSection(&coinInfo->locks->sharesLock);
			coinInfo->mining->shares.push_back(std::make_shared<t_shares>(
				session->body.file_name,
				session->body.account_id, 
				session->body.best, 
				session->body.nonce,
				session->body.deadline,
				session->body.height,
				session->body.baseTarget));
			LeaveCriticalSection(&coinInfo->locks->sharesLock);
		}
		failed = true;
	}
	else //got something from the server
	{
		increaseNetworkQuality(coinInfo);

		//received an empty string, re-send deadline
		if (buffer[0] == '\0')
		{
			Log(L"Confirmer %s: zero-length message for DL: %llu", confirmerName, session->deadline);
			EnterCriticalSection(&coinInfo->locks->sharesLock);
			coinInfo->mining->shares.push_back(std::make_shared<t_shares>(
				session->body.file_name,
				session->body.account_id, 
				session->body.best, 
				session->body.nonce,
				session->body.deadline,
				session->body.height,
				session->body.baseTarget));
			LeaveCriticalSection(&coinInfo->locks->sharesLock);
			failed = true;
		}
		else //received a pool response
		{
			find = strstr(buffer.data(), "{");
			if (find == nullptr)
			{
				find = strstr(buffer.data(), "\r\n\r\n");
				if (find != nullptr) find = find + 4;
				else find = buffer.data();
			}

			unsigned long long ndeadline;
			unsigned long long naccountId = 0;
			unsigned long long ntargetDeadline = 0;

			rapidjson::Document& answ = output;
			if (answ.Parse<0>(find).HasParseError())
			{
				if (strstr(find, "Received share") != nullptr)
				{
					failed = false;
					nonJsonSuccessDetected = true;
				}
				else //received an unrecognized answer
				{
					failed = true;

					int minor_version;
					int status = 0;
					const char *msg;
					size_t msg_len;
					struct phr_header headers[12];
					size_t num_headers = sizeof(headers) / sizeof(headers[0]);
					phr_parse_response(buffer.data(), strlen(buffer.data()), &minor_version, &status, &msg, &msg_len, headers, &num_headers, 0);

					if (status != 0)
					{
						std::string error_str(msg, msg_len);
						gui->printToConsole(6, true, false, true, false, L"%s: Server error: %d %S", confirmerName, status, error_str.c_str());
						Log(L"Confirmer %s: server error for DL: %llu", confirmerName, session->deadline);
						EnterCriticalSection(&coinInfo->locks->sharesLock);
						coinInfo->mining->shares.push_back(std::make_shared<t_shares>(
							session->body.file_name,
							session->body.account_id,
							session->body.best,
							session->body.nonce,
							session->body.deadline,
							session->body.height,
							session->body.baseTarget));
						LeaveCriticalSection(&coinInfo->locks->sharesLock);
					}
					else //got something incomprehensible
					{
						gui->printToConsole(7, true, false, true, false, L"%s: %S", confirmerName, buffer.data());
					}
				}
			}
			else
			{
				failed = false;
				nonJsonSuccessDetected = false;
			}
		}
		EnterCriticalSection(&coinInfo->locks->sessionsLock);
		if (!coinInfo->network->sessions.empty()) {
			iResult = closesocket(coinInfo->network->sessions.front()->Socket);
			Log(L"Confirmer %s: Close socket. Code = %i", confirmerName, WSAGetLastError());
			coinInfo->network->sessions.erase(coinInfo->network->sessions.begin());
		}
		LeaveCriticalSection(&coinInfo->locks->sessionsLock);
	}
	return failed;
}

// https://curl.haxx.se/libcurl/c/sendrecv.html
static int wait_on_socket(curl_socket_t sockfd, int for_recv, long timeout_ms)
{
	struct timeval tv;
	fd_set infd, outfd, errfd;
	int res;

	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec = (timeout_ms % 1000) * 1000;

	FD_ZERO(&infd);
	FD_ZERO(&outfd);
	FD_ZERO(&errfd);

	FD_SET(sockfd, &errfd); /* always check for error */

	if (for_recv) {
		FD_SET(sockfd, &infd);
	}
	else {
		FD_SET(sockfd, &outfd);
	}

	/* select() returns the number of signalled sockets or -1 */
	res = select((int)sockfd + 1, &infd, &outfd, &errfd, &tv);
	return res;
}

bool __impl__confirm_i__curl(std::vector<char, heap_allocator<char>>& buffer, std::shared_ptr<t_coin_info> coinInfo, rapidjson::Document& output, char*& find, bool& nonJsonSuccessDetected, std::shared_ptr<t_session2>& session) {
	bool failed = false;

	const wchar_t* confirmerName = coinInfo->coinname.c_str();

	int iResult = 0;

	EnterCriticalSection(&coinInfo->locks->sessions2Lock);
	if (!coinInfo->network->sessions2.empty()) {
		session = coinInfo->network->sessions2.front();
	}
	LeaveCriticalSection(&coinInfo->locks->sessions2Lock);

	if (session == nullptr) {
		// No more data for now

		return true;
	}

	if (session->body.height != coinInfo->mining->currentHeight) {
		Log(L"Confirmer %s: DL %llu from block %llu discarded. New block %llu",
			confirmerName, session->deadline, session->body.height, coinInfo->mining->currentHeight);
		EnterCriticalSection(&coinInfo->locks->sessions2Lock);
		if (!coinInfo->network->sessions2.empty()) {
			curl_easy_cleanup(coinInfo->network->sessions2.front()->curl);
			Log(L"Confirmer %s: Close socket.", confirmerName);
			coinInfo->network->sessions2.erase(coinInfo->network->sessions2.begin());
		}
		LeaveCriticalSection(&coinInfo->locks->sessions2Lock);
		return true;
	}

	const unsigned long long targetDeadlineInfo = getTargetDeadlineInfo(coinInfo);

	CURL* curl = session->curl;
	curl_socket_t sockfd;
	CURLcode res = curl_easy_getinfo(curl, CURLINFO_ACTIVESOCKET, &sockfd);

	if (res == CURLE_OK) {
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(sockfd, &readfds);

		RtlSecureZeroMemory(buffer.data(), buffer.size());

		size_t total_recvd = 0;
		do {
			size_t recvd = 0;
			res = curl_easy_recv(curl, buffer.data() + total_recvd, buffer.size() - total_recvd, &recvd); // TODO: bug: no '-1' for terminator

			if (res == CURLE_AGAIN)
				if (wait_on_socket(sockfd, 1, coinInfo->network->submitTimeout))
					continue;
				else
					res = CURLE_OPERATION_TIMEDOUT;

			if (res != CURLE_OK)
				break;

			if (recvd == 0)
				break;

			total_recvd += recvd;

		} while (res == CURLE_AGAIN);
	}

	/* Check for errors */
	if (res != CURLE_OK) {
		decreaseNetworkQuality(coinInfo);
		Log(L"Confirmer %s: ! Error getting confirmation for DL: %llu  error: %S", confirmerName, session->deadline, curl_easy_strerror(res));
		failed = true;

		EnterCriticalSection(&coinInfo->locks->sessions2Lock);
		if (!coinInfo->network->sessions2.empty()) {
			curl_easy_cleanup(coinInfo->network->sessions2.front()->curl);
			Log(L"Confirmer %s: Close socket.", confirmerName);
			coinInfo->network->sessions2.erase(coinInfo->network->sessions2.begin());
		}
		LeaveCriticalSection(&coinInfo->locks->sessions2Lock);
		EnterCriticalSection(&coinInfo->locks->sharesLock);
		coinInfo->mining->shares.push_back(std::make_shared<t_shares>(
			session->body.file_name,
			session->body.account_id,
			session->body.best,
			session->body.nonce,
			session->body.deadline,
			session->body.height,
			session->body.baseTarget));
		LeaveCriticalSection(&coinInfo->locks->sharesLock);
	}
	else //got something from the server
	{
		increaseNetworkQuality(coinInfo);

		//Received an empty string, re-send deadline
		if (buffer[0] == '\0')
		{
			Log(L"Confirmer %s: zero-length message for DL: %llu", confirmerName, session->deadline);
			EnterCriticalSection(&coinInfo->locks->sharesLock);
			coinInfo->mining->shares.push_back(std::make_shared<t_shares>(
				session->body.file_name,
				session->body.account_id,
				session->body.best,
				session->body.nonce,
				session->body.deadline,
				session->body.height,
				session->body.baseTarget));
			LeaveCriticalSection(&coinInfo->locks->sharesLock);
			failed = true;
		}
		else //received a pool response
		{
			find = strstr(buffer.data(), "{");
			if (find == nullptr)
			{
				find = strstr(buffer.data(), "\r\n\r\n");
				if (find != nullptr) find = find + 4;
				else find = buffer.data();
			}

			unsigned long long ndeadline;
			unsigned long long naccountId = 0;
			unsigned long long ntargetDeadline = 0;

			rapidjson::Document& answ = output;
			if (answ.Parse<0>(find).HasParseError())
			{
				if (strstr(find, "Received share") != nullptr)
				{
					failed = false;
					nonJsonSuccessDetected = true;
				}
				else //received an unrecognized answer
				{
					failed = true;

					int minor_version;
					int status = 0;
					const char *msg;
					size_t msg_len;
					struct phr_header headers[12];
					size_t num_headers = sizeof(headers) / sizeof(headers[0]);
					phr_parse_response(buffer.data(), strlen(buffer.data()), &minor_version, &status, &msg, &msg_len, headers, &num_headers, 0);

					if (status != 0)
					{
						std::string error_str(msg, msg_len);
						gui->printToConsole(6, true, false, true, false, L"%s: Server error: %d %S", confirmerName, status, error_str.c_str());
						Log(L"Confirmer %s: server error for DL: %llu", confirmerName, session->deadline);
						EnterCriticalSection(&coinInfo->locks->sharesLock);
						coinInfo->mining->shares.push_back(std::make_shared<t_shares>(
							session->body.file_name,
							session->body.account_id,
							session->body.best,
							session->body.nonce,
							session->body.deadline,
							session->body.height,
							session->body.baseTarget));
						LeaveCriticalSection(&coinInfo->locks->sharesLock);
					}
					else //got something incomprehensible
					{
						gui->printToConsole(7, true, false, true, false, L"%s: %S", confirmerName, buffer.data());
					}
				}
			}
			else
			{
				failed = false;
				nonJsonSuccessDetected = false;
			}
		}
		EnterCriticalSection(&coinInfo->locks->sessions2Lock);
		if (!coinInfo->network->sessions2.empty()) {
			curl_easy_cleanup(coinInfo->network->sessions2.front()->curl);
			Log(L"Confirmer %s: Close socket.", confirmerName);
			coinInfo->network->sessions2.erase(coinInfo->network->sessions2.begin());
		}
		LeaveCriticalSection(&coinInfo->locks->sessions2Lock);
	}
	return failed;
}

void confirm_i(std::shared_ptr<t_coin_info> coinInfo) {
	const wchar_t* coinName = coinInfo->coinname.c_str();
	Log(L"Confirmer %s: started thread", coinName);

	SOCKET ConnectSocket;
	int iResult = 0;
	std::vector<char, heap_allocator<char>> buffer(1000, theHeap);

	while (!exit_flag && !coinInfo->locks->stopRoundSpecificNetworkingThreads) {

		std::shared_ptr<t_session> session;
		std::shared_ptr<t_session2> session2;

		const unsigned long long targetDeadlineInfo = getTargetDeadlineInfo(coinInfo);
		
		unsigned long long ndeadline;
		unsigned long long naccountId = 0;
		unsigned long long ntargetDeadline = 0;

		char* find;
		bool nonJsonSuccessDetected;
		rapidjson::Document answ;
		bool failedOrNoData;

		if (coinInfo->network->usehttps)
			failedOrNoData = __impl__confirm_i__curl(buffer, coinInfo, answ, find, nonJsonSuccessDetected, session2);
		else
			failedOrNoData = __impl__confirm_i__sockets(buffer, coinInfo, answ, find, nonJsonSuccessDetected, session);

		t_session sessionX_(NULL, -1, t_shares("", 0, 0, 0, 0, 0, 0));
		t_session* sessionX = &sessionX_;
		if (!failedOrNoData)
			if (coinInfo->network->usehttps) {
				sessionX_.body = session2->body;
				sessionX_.deadline = session2->deadline;
			}
			else {
				sessionX_.body = session->body;
				sessionX_.deadline = session->deadline;
			}

		// burst.ninja        {"requestProcessingTime":0,"result":"success","block":216280,"deadline":304917,"deadlineString":"3 days, 12 hours, 41 mins, 57 secs","targetDeadline":304917}
		// pool.burst-team.us {"requestProcessingTime":0,"result":"success","block":227289,"deadline":867302,"deadlineString":"10 days, 55 mins, 2 secs","targetDeadline":867302}
		// proxy              {"result": "proxy","accountId": 17930413153828766298,"deadline": 1192922,"targetDeadline": 197503}
		if (!failedOrNoData && !nonJsonSuccessDetected)
		{
			if (answ.IsObject())
			{
				if (answ.HasMember("deadline")) {
					if (answ["deadline"].IsString())	ndeadline = _strtoui64(answ["deadline"].GetString(), 0, 10);
					else
						if (answ["deadline"].IsInt64()) ndeadline = answ["deadline"].GetInt64();
					Log(L"Confirmer %s: confirmed deadline: %llu", coinName, ndeadline);

					if (answ.HasMember("targetDeadline")) {
						if (answ["targetDeadline"].IsString())	ntargetDeadline = _strtoui64(answ["targetDeadline"].GetString(), 0, 10);
						else
							if (answ["targetDeadline"].IsInt64()) ntargetDeadline = answ["targetDeadline"].GetInt64();
					}
					if (answ.HasMember("accountId")) {
						if (answ["accountId"].IsString())	naccountId = _strtoui64(answ["accountId"].GetString(), 0, 10);
						else
							if (answ["accountId"].IsInt64()) naccountId = answ["accountId"].GetInt64();
					}

					if ((naccountId != 0) && (ntargetDeadline != 0))
					{
						EnterCriticalSection(&coinInfo->locks->bestsLock);
						coinInfo->mining->bests[Get_index_acc(naccountId, coinInfo, targetDeadlineInfo)].targetDeadline = ntargetDeadline;
						LeaveCriticalSection(&coinInfo->locks->bestsLock);

						gui->printNetworkDeadlineConfirmed(true, naccountId, coinName, ndeadline);

						// TODO: somehow deduplicate log math
						// and yes, I've CONSIDERED putting the logging it into printNetworkDeadlineConfirmed, and I'm still hesitant
						unsigned long long days = (ndeadline) / (24 * 60 * 60);
						unsigned hours = (ndeadline % (24 * 60 * 60)) / (60 * 60);
						unsigned min = (ndeadline % (60 * 60)) / 60;
						unsigned sec = ndeadline % 60;
						Log(L"[%20llu] %s confirmed DL: %10llu %5llud %02u:%02u:%02u", naccountId, coinName, ndeadline, days, hours, min, sec);

						Log(L"[%20llu] %s set targetDL: %10llu", naccountId, coinName, ntargetDeadline);
						if (use_debug) {
							gui->debugNetworkTargetDeadlineUpdated(naccountId, coinName, ntargetDeadline);
						}
					}
					else {
						gui->printNetworkDeadlineConfirmed(true, sessionX->body.account_id, coinName, ndeadline);

						// TODO: somehow deduplicate log math
						// and yes, I've CONSIDERED putting the logging it into printNetworkDeadlineConfirmed, and I'm still hesitant
						unsigned long long days = (ndeadline) / (24 * 60 * 60);
						unsigned hours = (ndeadline % (24 * 60 * 60)) / (60 * 60);
						unsigned min = (ndeadline % (60 * 60)) / 60;
						unsigned sec = ndeadline % 60;
						Log(L"[%20llu] %s confirmed DL: %10llu %5llud %02u:%02u:%02u", sessionX->body.account_id, coinName, ndeadline, days, hours, min, sec);
					}
					if (ndeadline < coinInfo->mining->deadline || coinInfo->mining->deadline == 0)  coinInfo->mining->deadline = ndeadline;

					if (ndeadline != sessionX->deadline)
					{
						// TODO: 4398046511104, 240, etc - that are COIN PARAMETERS, these should not be HARDCODED
						Log(L"Confirmer %s: Calculated and confirmed deadlines don't match. Fast block or corrupted file? Response: %S", coinName, find);
						std::thread{ Csv_Fail, coinInfo, sessionX->body.height, sessionX->body.file_name, sessionX->body.baseTarget,
							4398046511104 / 240 / sessionX->body.baseTarget, sessionX->body.nonce, sessionX->deadline, ndeadline, find }.detach();
						std::thread{ increaseConflictingDeadline, coinInfo, sessionX->body.height, sessionX->body.file_name }.detach();
						gui->printToConsole(6, false, false, true, false,
							L"----Fast block or corrupted file?----\n%s sent deadline:\t%llu\nServer's deadline:\t%llu \n----",
							coinName, sessionX->deadline, ndeadline);
					}
				}
				else {
					if (answ.HasMember("errorDescription")) {
						// TODO: 4398046511104, 240, etc - that are COIN PARAMETERS, these should not be HARDCODED
						Log(L"Confirmer %s: Deadline %llu sent with error: %S", coinName, sessionX->deadline, find);
						std::thread{ Csv_Fail, coinInfo, sessionX->body.height, sessionX->body.file_name, sessionX->body.baseTarget,
								4398046511104 / 240 / sessionX->body.baseTarget, sessionX->body.nonce, sessionX->deadline, 0, find }.detach();
						std::thread{ increaseConflictingDeadline, coinInfo, sessionX->body.height, sessionX->body.file_name }.detach();
						if (sessionX->deadline <= targetDeadlineInfo) {
							Log(L"Confirmer %s: Deadline should have been accepted (%llu <= %llu). Fast block or corrupted file?", coinName, sessionX->deadline, targetDeadlineInfo);
							gui->printToConsole(6, false, false, true, false,
								L"----Fast block or corrupted file?----\n%s sent deadline:\t%llu\nTarget deadline:\t%llu \n----",
								coinName, sessionX->deadline, targetDeadlineInfo);
						}
						if (answ["errorCode"].IsInt()) {
							gui->printToConsole(15, true, false, true, false, L"[ERROR %i] %s: %S", answ["errorCode"].GetInt(), coinName, answ["errorDescription"].GetString());
							if (answ["errorCode"].GetInt() == 1004) {
								gui->printToConsole(12, true, false, true, false, L"%s: You need change reward assignment and wait 4 blocks (~16 minutes)", coinName); //error 1004
							}
						}
						else if (answ["errorCode"].IsString()) {
							gui->printToConsole(15, true, false, true, false, L"[ERROR %S] %s: %S", answ["errorCode"].GetString(), coinName, answ["errorDescription"].GetString());
							if (answ["errorCode"].GetString() == "1004") {
								gui->printToConsole(12, true, false, true, false, L"%s: You need change reward assignment and wait 4 blocks (~16 minutes)", coinName); //error 1004
							}
						}
						else {
							gui->printToConsole(15, true, false, true, false, L"[ERROR] %s: %S", coinName, answ["errorDescription"].GetString());
						}
					}
					else {
						gui->printToConsole(15, true, false, true, false, L"%s: %S", coinName, find);
					}
				}
			}
		}
		else if (!failedOrNoData && nonJsonSuccessDetected)
		{
				coinInfo->mining->deadline = coinInfo->mining->bests[Get_index_acc(sessionX->body.account_id, coinInfo, targetDeadlineInfo)].DL; //maybe better iter-> deadline?
																			// if(deadline > iter->deadline) deadline = iter->deadline;
				std::thread{ increaseMatchingDeadline, sessionX->body.file_name }.detach();
				gui->printNetworkDeadlineConfirmed(false, sessionX->body.account_id, coinName, sessionX->deadline); // TODO: why pretty-time is disabled here?
		}

		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::milliseconds(coinInfo->network->send_interval));
	}

	Log(L"Confirmer %s: All work done, shutting down.", coinName);
}


void updater_i(std::shared_ptr<t_coin_info> coinInfo)
{
	const wchar_t* updaterName = coinInfo->coinname.c_str();
	if (coinInfo->network->updateraddr.length() <= 3) {
		Log(L"Updater %s: GMI: ERROR in UpdaterAddr", updaterName);
		exit(2);
	}
	while (!exit_flag) {
		if (pollLocal(coinInfo) && coinInfo->mining->enable && !coinInfo->mining->newMiningInfoReceived) {
			setnewMiningInfoReceived(coinInfo, true);
		}

		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::milliseconds(coinInfo->network->update_interval));
	}
}

bool __impl__pollLocal__sockets(std::shared_ptr<t_coin_info> coinInfo, rapidjson::Document& output, std::string& rawResponse) {
	bool failed = false;

	const wchar_t* updaterName = coinInfo->coinname.c_str();
	bool newBlock = false;

	std::vector<char, heap_allocator<char>> buffer(1001, theHeap); // TODO: catch bad_alloc, quit via ShowMemErrorExit()

	int iResult;
	struct addrinfo *result = nullptr;
	struct addrinfo hints;
	SOCKET UpdaterSocket = INVALID_SOCKET;

	RtlSecureZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo(coinInfo->network->updateraddr.c_str(), coinInfo->network->updaterport.c_str(), &hints, &result);
	if (iResult != 0) {
		decreaseNetworkQuality(coinInfo);
		Log(L"*! GMI %s: getaddrinfo failed with error: %i", updaterName, WSAGetLastError());
		failed = true;
	}
	else {
		UpdaterSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (UpdaterSocket == INVALID_SOCKET)
		{
			decreaseNetworkQuality(coinInfo);
			Log(L"*! GMI %s: socket function failed with error: %i", updaterName, WSAGetLastError());
			failed = true;
		}
		else {
			const unsigned t = 1000;
			setsockopt(UpdaterSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&t, sizeof(unsigned));
			//Log("*Connecting to server: %S:%S", updateraddr.c_str(), updaterport.c_str());
			iResult = connect(UpdaterSocket, result->ai_addr, (int)result->ai_addrlen);
			if (iResult == SOCKET_ERROR) {
				decreaseNetworkQuality(coinInfo);
				Log(L"*! GMI %s: connect function failed with error: %i", updaterName, WSAGetLastError());
				failed = true;
			}
			else {
				int bytes = sprintf_s(buffer.data(), buffer.size(), "POST /%s?requestType=getMiningInfo HTTP/1.0\r\nHost: %s:%s\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",
					coinInfo->network->updaterroot.c_str(), coinInfo->network->updateraddr.c_str(), coinInfo->network->updaterport.c_str());
				iResult = send(UpdaterSocket, buffer.data(), bytes, 0);
				if (iResult == SOCKET_ERROR)
				{
					decreaseNetworkQuality(coinInfo);
					Log(L"*! GMI %s: send request failed: %i", updaterName, WSAGetLastError());
					failed = true;
				}
				else {
					RtlSecureZeroMemory(buffer.data(), buffer.size());
					size_t  pos = 0;
					iResult = 0;
					do {
						iResult = recv(UpdaterSocket, &buffer[pos], (int)(buffer.size() - pos - 1), 0);
						if (iResult > 0) pos += (size_t)iResult;
					} while (iResult > 0);
					if (iResult == SOCKET_ERROR)
					{
						decreaseNetworkQuality(coinInfo);
						Log(L"*! GMI %s: get mining info failed:: %i", updaterName, WSAGetLastError());
						failed = true;
					}
					else {
						increaseNetworkQuality(coinInfo);
						if (loggingConfig.logAllGetMiningInfos) {
							Log(L"* GMI %s: Received: %S", updaterName, Log_server(buffer.data()).c_str());
						}

						rawResponse = { buffer.begin(), buffer.end() };
						rapidjson::Document& gmi = output;

						// locate HTTP header
						char const* find = strstr(buffer.data(), "\r\n\r\n");
						if (find == nullptr) {
							Log(L"*! GMI %s: error message from pool: %S", updaterName, Log_server(buffer.data()).c_str());
							failed = true;
						}
						else if (loggingConfig.logAllGetMiningInfos && gmi.Parse<0>(find).HasParseError()) {
							Log(L"*! GMI %s: error parsing JSON message from pool", updaterName);
							failed = true;
						}
						else if (!loggingConfig.logAllGetMiningInfos && gmi.Parse<0>(find).HasParseError()) {
							Log(L"*! GMI %s: error parsing JSON message from pool: %S", updaterName, Log_server(buffer.data()).c_str());
							failed = true;
						}
					}
				}
			}
		}
		iResult = closesocket(UpdaterSocket);
		freeaddrinfo(result);
	}

	return failed;
}

typedef std::vector<char, heap_allocator<char>> MemoryStruct;
static size_t __impl__pollLocal__curl__readcallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	MemoryStruct *mem = (MemoryStruct *)userp;

	size_t oldsize = mem->size() - 1;
	size_t delta = size * nmemb;
	mem->resize(oldsize + delta + 1); // TODO: devise a way to use HeapReAlloc back again
	memcpy(&(*mem)[oldsize], contents, delta);
	*(--mem->end()) = 0;

	return delta;
}
bool __impl__pollLocal__curl(std::shared_ptr<t_coin_info> coinInfo, rapidjson::Document& output, std::string& rawResponse) {
	bool failed = false;

	const wchar_t* updaterName = coinInfo->coinname.c_str();
	bool newBlock = false;
	
	// TODO: fixup: extract outside like it was before? maybe?
	std::vector<char, heap_allocator<char>> buffer(theHeap); // TODO: catch bad_alloc, quit via ShowMemErrorExit()
	buffer.resize(1000);

	MemoryStruct chunk(1, theHeap);

	CURL* curl = curl_easy_init();
	if (!curl) {
		failed = true;
	}
	else {
		if (false) { // coinInfo->network->SKIP_PEER_VERIFICATION
			/*
			 * If you want to connect to a site who isn't using a certificate that is
			 * signed by one of the certs in the CA bundle you have, you can skip the
			 * verification of the server's certificate. This makes the connection
			 * A LOT LESS SECURE.
			 *
			 * If you have a CA cert for the server stored someplace else than in the
			 * default bundle, then the CURLOPT_CAPATH option might come handy for
			 * you.
			 */
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		}

		if (false) { // coinInfo->network->SKIP_HOSTNAME_VERIFICATION
			/*
			 * If the site you're connecting to uses a different host name that what
			 * they have mentioned in their server certificate's commonName (or
			 * subjectAltName) fields, libcurl will refuse to connect. You can skip
			 * this check, but this will make the connection less secure.
			 */
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		}

		int bytes = sprintf_s(buffer.data(), buffer.size(), "https://%s:%s/%s?requestType=getMiningInfo",
			coinInfo->network->updateraddr.c_str(), coinInfo->network->updaterport.c_str(), coinInfo->network->updaterroot.c_str());
		curl_easy_setopt(curl, CURLOPT_URL, buffer.data());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, ""); // wee need to send a POST but body may be left empty
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __impl__pollLocal__curl__readcallback);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, coinInfo->network->submitTimeout);

		/* Perform the request, res will get the return code */
		CURLcode res = curl_easy_perform(curl);

		/* Check for errors */
		if (res != CURLE_OK) {
			decreaseNetworkQuality(coinInfo);
			Log(L"*! GMI %s: get mining info failed:: %S", updaterName, curl_easy_strerror(res));
			failed = true;
		}
		else {
			increaseNetworkQuality(coinInfo);
			if (loggingConfig.logAllGetMiningInfos) {
				Log(L"* GMI %s: Received: %S", updaterName, Log_server(chunk.data()).c_str());
			}

			rawResponse.assign(chunk.begin(), chunk.end()); // TODO: not so 'raw', that's just the response BODY with no headers
			rapidjson::Document& gmi = output;
			if (loggingConfig.logAllGetMiningInfos && gmi.Parse<0>(chunk.data(), chunk.size()).HasParseError()) {
				Log(L"*! GMI %s: error parsing JSON message from pool", updaterName);
				failed = true;
			}
			else if (!loggingConfig.logAllGetMiningInfos && gmi.Parse<0>(chunk.data(), chunk.size()).HasParseError()) {
				Log(L"*! GMI %s: error parsing JSON message from pool: %S", updaterName, Log_server(chunk.data()).c_str());
				failed = true;
			}
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}

	return failed;
}

bool pollLocal(std::shared_ptr<t_coin_info> coinInfo) {
	std::string rawResponse;
	rapidjson::Document gmi;
	bool failed;

	if (coinInfo->network->usehttps)
		failed = __impl__pollLocal__curl(coinInfo, gmi, rawResponse);
	else
		failed = __impl__pollLocal__sockets(coinInfo, gmi, rawResponse);

	if (failed) return false;
	if (!gmi.IsObject()) return false;

	const wchar_t* updaterName = coinInfo->coinname.c_str();
	bool newBlock = false;

	if (gmi.HasMember("baseTarget")) {
		if (gmi["baseTarget"].IsString())	coinInfo->mining->baseTarget = _strtoui64(gmi["baseTarget"].GetString(), 0, 10);
		else
			if (gmi["baseTarget"].IsInt64())coinInfo->mining->baseTarget = gmi["baseTarget"].GetInt64();
	}

	uint64_t height = INTMAX_MAX;
	if (gmi.HasMember("height")) {
		if (gmi["height"].IsString())	setHeight(coinInfo, height = _strtoui64(gmi["height"].GetString(), 0, 10));
		else
			if (gmi["height"].IsInt64()) setHeight(coinInfo, height = gmi["height"].GetInt64());
	}

	if (gmi.HasMember("generationSignature")) {
		setStrSignature(coinInfo, gmi["generationSignature"].GetString());
		char sig[33];
		size_t sigLen = xstr2strr(sig, 33, gmi["generationSignature"].GetString());

		if (coinInfo->mining->enableDiskcoinGensigs) {
			std::array<uint8_t, 32> tmp;
			std::copy(sig, sig + 32, tmp.data());
			auto newsig = diskcoin_generate_gensig_aes128(height, tmp);
			std::copy(newsig->begin(), newsig->end(), sig);
		}

		bool sigDiffer = signaturesDiffer(coinInfo, sig);
										
		if (sigLen <= 1) {
			Log(L"*! GMI %s: Node response: Error decoding generationsignature: %S", updaterName, Log_server(rawResponse.c_str()).c_str());
		}
		else if (sigDiffer) {
			newBlock = true;
			setSignature(coinInfo, sig);
			if (!loggingConfig.logAllGetMiningInfos) {
				Log(L"*! GMI %s: Received new mining info: %S", updaterName, Log_server(rawResponse.c_str()).c_str());
			}
		}
	}
	if (gmi.HasMember("targetDeadline")) {
		unsigned long long newTargetDeadlineInfo = 0;
		if (gmi["targetDeadline"].IsString()) {
			newTargetDeadlineInfo = _strtoui64(gmi["targetDeadline"].GetString(), 0, 10);
		}
		else {
			newTargetDeadlineInfo = gmi["targetDeadline"].GetInt64();
		}
		if (loggingConfig.logAllGetMiningInfos || newBlock) {
			Log(L"*! GMI %s: newTargetDeadlineInfo: %llu", updaterName, newTargetDeadlineInfo);
			Log(L"*! GMI %s: my_target_deadline: %llu", updaterName, coinInfo->mining->my_target_deadline);
		}
		if ((newTargetDeadlineInfo > 0) && (newTargetDeadlineInfo < coinInfo->mining->my_target_deadline)) {
			setTargetDeadlineInfo(coinInfo, newTargetDeadlineInfo);
			if (loggingConfig.logAllGetMiningInfos || newBlock) {
				Log(L"*! GMI %s: Target deadline from pool is lower than deadline set in the configuration. Updating targetDeadline: %llu", updaterName, newTargetDeadlineInfo);
			}
		}
		else {
			setTargetDeadlineInfo(coinInfo, coinInfo->mining->my_target_deadline);
			if (loggingConfig.logAllGetMiningInfos || newBlock) {
				Log(L"*! GMI %s: Using target deadline from configuration: %llu", updaterName, coinInfo->mining->my_target_deadline);
			}
		}
	}
	else {
		setTargetDeadlineInfo(coinInfo, coinInfo->mining->my_target_deadline);
		Log(L"*! GMI %s: No target deadline information provided. Using target deadline from configuration: %llu", updaterName, coinInfo->mining->my_target_deadline);
	}

	return newBlock;
}


// Helper routines taken from http://stackoverflow.com/questions/1557400/hex-to-char-array-in-c
int xdigit(char const digit) {
	int val;
	if ('0' <= digit && digit <= '9') val = digit - '0';
	else if ('a' <= digit && digit <= 'f') val = digit - 'a' + 10;
	else if ('A' <= digit && digit <= 'F') val = digit - 'A' + 10;
	else val = -1;
	return val;
}

size_t xstr2strr(char *buf, size_t const bufsize, const char *const in) {
	if (!in) return 0; // missing input string

	size_t inlen = (size_t)strlen(in);
	if (inlen % 2 != 0) inlen--; // hex string must even sized

	size_t i, j;
	for (i = 0; i<inlen; i++)
		if (xdigit(in[i])<0) return 0; // bad character in hex string

	if (!buf || bufsize<inlen / 2 + 1) return 0; // no buffer or too small

	for (i = 0, j = 0; i<inlen; i += 2, j++)
		buf[j] = xdigit(in[i]) * 16 + xdigit(in[i + 1]);

	buf[inlen / 2] = '\0';
	return inlen / 2 + 1;
}
