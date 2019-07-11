#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "reference/BurstMath.h"
#include "hexstring.h"

#include "ctaes.h"

TEST(RnD_BurstMath_CalcDeadline_QueLap, dccONburst641159WithGenSigFromDump) {
	// based on data from LIVE debugging session
	// 23:20:17 * GMI: Received: HTTP/1.0 200 OK\r\nX-Ratelimit-Limit: 3\r\nX-Ratelimit-Remaining: 2\r\nX-Ratelimit-Reset: 1\r\nDate: Wed, 10 Jul 2019 21:20:17 GMT\r\nContent-Length: 151\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n{"baseTarget":34819,"generationSignature":"ed377f7ea5f857c565baa9e44aa94f5c8162c2fdf430d6eccdac4b856c6b5b68","height":641159,"targetDeadline":31536000}
	/*
		HEIGHT: [00007FF76D13EF60]      87 C8 09 00 = latest
		SIG:    [00007FF76D13F020]      = totally different
		00007FF76D13F020  BC EA A3 39 BD 7C D1 B1 33 74 9B F3 9F 3E E6 51
		00007FF76D13F030  8C C9 4E A3 16 FF 7C 49 CD 66 F6 D2 39 A3 9E F5
		BAS:    [00007FF76D13F150]      03 88 00 00 ... (34819) = latest
		NON:    rcx                     E0 0E CB 01 18 77 12 B6 ]+1  (res1 was the best)
		ACC:    ....                    6E680B79EFB72AB7 = ok
		SCOOP:  [00007FF76D13F014]      49 03 00 00 = noidea if it's synced or not
		  => DL     000015DE5AA6ECBA
	*/

	auto const height = 0x09C887; // 641159
	//auto currentSignature = HexString::from("ed377f7ea5f857c565baa9e44aa94f5c8162c2fdf430d6eccdac4b856c6b5b68"); // gensig from the pool
	auto currentSignature = HexString::from("BCEAA339BD7CD1B133749BF39F3EE6518CC94EA316FF7C49CD66F6D239A39EF5"); // 'corrected' gensig used by the miner
	uint64_t currentBaseTarget = 0x00008803; // 34819
	uint64_t account_nr = 7955621360090688183;

	{
		uint64_t nonce_nr = 0xB612771801CB0EE0 + 1;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 0x0349
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_EQ(deadline, 0x000015DE5AA6ECBA/*+000000000000604D*/); // 7B F6 B7 BD C3 61 9E 0B 33 74 9B F3 9F 3E E6 51
	}
	{
		uint64_t nonce_nr = 0xB612771801CB0EE0 + 0;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 0x0349
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_EQ(deadline, 0xD2FD1CE8ED9F53FE / currentBaseTarget); // FE 53 9F ED E8 1C FD D2 00 00 00 00 00 00 00 00
	}
	{
		uint64_t nonce_nr = 0xB612771801CB0EE0 + 2;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 0x0349
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_EQ(deadline, 0x2C39F0C1A3A30488 / currentBaseTarget); // 88 04 A3 A3 C1 F0 39 2C 80 02 00 00 00 00 00 00
	}
	{
		uint64_t nonce_nr = 0xB612771801CB0EE0 + 3;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 0x0349
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_EQ(deadline, 0x3280915CBDEC5885 / currentBaseTarget); // 85 58 EC BD 5C 91 80 32 00 00 00 00 00 CC 1D 00
	}
}

TEST(RnD_DcminerUnUPXed_sanityChecks, coreInterop) {
	// https://www.codeproject.com/Articles/1045674/Load-EXE-as-DLL-Mission-Possible
	// ^^ actually, I think that neither global data, nor imports are important
	// I'm pretty sure that the hashing function we need have no dependencies
	// and if it is true, then no preparations are needed

	std::string modpath = "D:\\w\\q\\dcminer\\dcminer\\dcminer-unpacked.exe";
	HMODULE module = LoadLibraryA(modpath.c_str());

	size_t offs_cstr_knownMessage = 0x0000000000089A50;
	std::string referencedata_knownMessage = "PROXY: socket failed with error: %ld\n";

	size_t offs_cstr_seedFormat = 0x000000000008AA70;
	std::string referencedata_seedFormat = "DISKCOIN%-08d";

	std::string sanityCheck_knownMessage = (char*)((uint8_t*)module + offs_cstr_knownMessage);
	std::string sanityCheck_hashSeedFormat = (char*)( (uint8_t*)module + offs_cstr_seedFormat);

	// check if the offsets hit the expected data
	ASSERT_EQ(referencedata_knownMessage, sanityCheck_knownMessage);
	ASSERT_EQ(referencedata_seedFormat, sanityCheck_hashSeedFormat);

	size_t offs_code_hasher1 = 0x000000000000C910;
	std::vector<uint8_t> referencedata_hasher1 = HexString::from("48895C2410488974241848897C2420554154415541564157488BEC4883EC6048");
	// 00007FF76D0AC910  48 89 5C 24 10 48 89 74 24 18 48 89 7C 24 20 55  H.\$.H.t$.H.|$ U  
	// 00007FF76D0AC920  41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60 48  ATAUAVAWH.ìH.ì`H  
	auto tmp_h1 = (uint8_t*)module + offs_code_hasher1;
	std::vector<uint8_t> sanityCheck_hasher1 = { tmp_h1, tmp_h1 + referencedata_hasher1.size() };
	
	size_t offs_code_hasher2 = 0x000000000000CC70;
	std::vector<uint8_t> referencedata_hasher2 = HexString::from("488BC4488958085556574154415541564157488D68A14881ECD00000000F2970");
	// 00007FF76D0ACC70  48 8B C4 48 89 58 08 55 56 57 41 54 41 55 41 56  H.ÄH.X.UVWATAUAV  
	// 00007FF76D0ACC80  41 57 48 8D 68 A1 48 81 EC D0 00 00 00 0F 29 70  AWH.h¡H.ìÐ....)p  
	auto tmp_h2 = (uint8_t*)module + offs_code_hasher2;
	std::vector<uint8_t> sanityCheck_hasher2 = { tmp_h2, tmp_h2 + referencedata_hasher2.size() };

	// check if 'fingerprints' of the code are as expected
	ASSERT_EQ(referencedata_hasher1, sanityCheck_hasher1);
	ASSERT_EQ(referencedata_hasher2, sanityCheck_hasher2);

	FreeLibrary(module);
}

TEST(RnD_DcminerUnUPXed_sanityChecks, stringHasher) {

	std::string modpath = "D:\\w\\q\\dcminer\\dcminer\\dcminer-unpacked.exe";
	HMODULE module = LoadLibraryA(modpath.c_str());

	size_t offs_code_hasher1 = 0x000000000000C910;

	auto stringhasher = reinterpret_cast<void(*)(uint8_t* output, char* input)>((uint8_t*)module + offs_code_hasher1);

	char seed[] = "DISKCOIN641187\x20\x20";
	std::vector<uint8_t> referencedata_hasher1_output = HexString::from("B257A631E5203832CC45CCCC33330000");
	// 0000005DFFBFF590  B2 57 A6 31 E5 20 38 32 CC 45 CC CC 33 33 00 00  ²W¦1å 82ÌEÌÌ33..
	// 0000005DFFBFF5A0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................  
	
	std::vector<uint8_t> buff1(2048, 'X');

	// rdx = &char[16] string data, seed
	// rcx = &char[] output, probably 16bytes
	// call 0x00007FF76D0AC910 = stringhasher	0xC910

	stringhasher(buff1.data(), seed); // WHOA. It writes the buffer all the way down to 0xB0=176th element !!!

	for (size_t i = 176; i < 2048; ++i)
		ASSERT_EQ('X', buff1[i]);

	std::vector<uint8_t> buff1head = { buff1.data(), buff1.data() + referencedata_hasher1_output.size() };
	ASSERT_EQ(referencedata_hasher1_output, buff1head);

	FreeLibrary(module);
}

TEST(RnD_DcminerUnUPXed_sanityChecks, blobHasherWithoutStringHasherNoPaddingFails) {

	std::string modpath = "D:\\w\\q\\dcminer\\dcminer\\dcminer-unpacked.exe";
	HMODULE module = LoadLibraryA(modpath.c_str());

	size_t offs_code_hasher2 = 0x000000000000CC70;

	auto blobhasher = reinterpret_cast<void(*)(uint8_t* key, void* unknown, uint8_t* input, uint8_t* output)>((uint8_t*)module + offs_code_hasher2);

	// below: the hasher1 output SHOULD be just 16b, and I captured 'reference' of 32b, and the hasher1 turns out to write FAR MORE.
	// there's a difference in the tail bytes between what I captured as hasher1 output and what hasher1 emitted,
	// but since there is NO VISIBLE DIFFERENCE in hasher2 output, so for the first test, I assume the key is 16b,
	// and the whole tail is just ignored. Reference data is 16b, it it reads any further, we'll get broken output.
	std::vector<uint8_t> referencedata_hasher1_output = HexString::from("B257A631E5203832CC45CCCC33330000");
	// 0000005DFFBFF590  B2 57 A6 31 E5 20 38 32 CC 45 CC CC 33 33 00 00  ²W¦1å 82ÌEÌÌ33..
	// 0000005DFFBFF5A0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................  
	std::vector<uint8_t> gensig = HexString::from("1d949950a77b309782cda1b2ec735c7b8cbef7234d58867e53396e0e8ca1e16d");
	// 00007FF76D13F020  1D 94 99 50 A7 7B 30 97 82 CD A1 B2 EC 73 5C 7B  ...P§{0..Í¡²ìs\{  
	// 00007FF76D13F030  8C BE F7 23 4D 58 86 7E 53 39 6E 0E 8C A1 E1 6D  .¾÷#MX.~S9n..¡ám  
	std::vector<uint8_t> referencedata_hasher2_output = HexString::from("419493CB098376B1D2E52AD0CFDF983C7FF2917213D07FF65EB2FF1A6B18386E");
	// 00007FF76D13F020  41 94 93 CB 09 83 76 B1 D2 E5 2A D0 CF DF 98 3C  A..Ë..v±Òå*ÐÏß.<  
	// 00007FF76D13F030  7F F2 91 72 13 D0 7F F6 5E B2 FF 1A 6B 18 38 6E  .ò.r.Ð.ö^²ÿ.k.8n  

	std::vector<uint8_t> buff2(2048, 'X');

	// r9  = &char[16] input: sig0
	// r8  = &char[16] output: new sig0
	// rdx = same as rcx, but ignored&unused
	// rcx = &char[] input: seed hash, probably 16bytes
	// call 0x00007FF76D0ACC70 = sighasher		0xCC70
	// r9  = &char[16] input: sig1
	// r8  = &char[16] output: new sig1
	// rdx = same as rcx, but ignored&unused
	// rcx = &char[] input: seed hash, probably 16bytes
	// call 0x00007FF76D0ACC70 = sighasher		0xCC70

	blobhasher(referencedata_hasher1_output.data(), 0, buff2.data() + 0x00, gensig.data() + 0x00);
	for (size_t i = 0x10; i < 2048; ++i)
		ASSERT_EQ('X', buff2[i]);

	blobhasher(referencedata_hasher1_output.data(), 0, buff2.data() + 0x10, gensig.data() + 0x10);
	for (size_t i = 0x20; i < 2048; ++i)
		ASSERT_EQ('X', buff2[i]);

	// output is NOT AS EXPECTED:
	//	[0x00000000]	0xa2 '˘'	unsigned char
	//	[0x00000001]	0x0a '\n'	unsigned char
	//	[0x00000002]	0x06 '\x6'	unsigned char
	//	[0x00000003]	0xb0 '°'	unsigned char
	//	[0x00000004]	0xe8 'č'	unsigned char
	//	[0x00000005]	0x72 'r'	unsigned char
	//	[0x00000006]	0x01 '\x1'	unsigned char
	//	[0x00000007]	0xaa 'Ş'	unsigned char
	//	[0x00000008]	0x94 '”'	unsigned char
	//	[0x00000009]	0x18 '\x18'	unsigned char
	//	[0x0000000a]	0xca 'Ę'	unsigned char
	//	[0x0000000b]	0xe9 'é'	unsigned char
	//	[0x0000000c]	0xd9 'Ů'	unsigned char
	//	[0x0000000d]	0x81 ''	unsigned char
	//	[0x0000000e]	0x29 ')'	unsigned char
	//	[0x0000000f]	0x6c 'l'	unsigned char
	//	....

	std::vector<uint8_t> buff2head = { buff2.data(), buff2.data() + referencedata_hasher2_output.size() };
	ASSERT_NE(referencedata_hasher2_output, buff2head);

	FreeLibrary(module);
}

TEST(RnD_DcminerUnUPXed_sanityChecks, blobHasherWithoutStringHasherZeroPaddingFails) {

	std::string modpath = "D:\\w\\q\\dcminer\\dcminer\\dcminer-unpacked.exe";
	HMODULE module = LoadLibraryA(modpath.c_str());

	size_t offs_code_hasher2 = 0x000000000000CC70;

	auto blobhasher = reinterpret_cast<void(*)(uint8_t* key, void* unknown, uint8_t* input, uint8_t* output)>((uint8_t*)module + offs_code_hasher2);

	// below: ok.. no-padding test proved there is a problem. Let's try following the data I captured
	// and let's zero-pad it, so there is no trash further in the buffer. Let's see if the output
	// is now different from the no-padding case. That will prove that `hasher2` reads further than 16b.
	std::vector<uint8_t> referencedata_hasher1_output = HexString::from("B257A631E5203832CC45CCCC33330000");
	for (size_t x = 16; x < 2048; ++x) referencedata_hasher1_output.push_back(0x00);
	// 0000005DFFBFF590  B2 57 A6 31 E5 20 38 32 CC 45 CC CC 33 33 00 00  ²W¦1å 82ÌEÌÌ33..  
	// 0000005DFFBFF5A0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................  
	std::vector<uint8_t> gensig = HexString::from("1d949950a77b309782cda1b2ec735c7b8cbef7234d58867e53396e0e8ca1e16d");
	// 00007FF76D13F020  1D 94 99 50 A7 7B 30 97 82 CD A1 B2 EC 73 5C 7B  ...P§{0..Í¡²ìs\{  
	// 00007FF76D13F030  8C BE F7 23 4D 58 86 7E 53 39 6E 0E 8C A1 E1 6D  .¾÷#MX.~S9n..¡ám  
	std::vector<uint8_t> referencedata_hasher2_output = HexString::from("419493CB098376B1D2E52AD0CFDF983C7FF2917213D07FF65EB2FF1A6B18386E");
	// 00007FF76D13F020  41 94 93 CB 09 83 76 B1 D2 E5 2A D0 CF DF 98 3C  A..Ë..v±Òå*ÐÏß.<  
	// 00007FF76D13F030  7F F2 91 72 13 D0 7F F6 5E B2 FF 1A 6B 18 38 6E  .ò.r.Ð.ö^²ÿ.k.8n  

	std::vector<uint8_t> buff2(2048, 'X');

	blobhasher(referencedata_hasher1_output.data(), 0, buff2.data() + 0x00, gensig.data() + 0x00);
	for (size_t i = 0x10; i < 2048; ++i)
		ASSERT_EQ('X', buff2[i]);

	blobhasher(referencedata_hasher1_output.data(), 0, buff2.data() + 0x10, gensig.data() + 0x10);
	for (size_t i = 0x20; i < 2048; ++i)
		ASSERT_EQ('X', buff2[i]);

	// output is NOT AS EXPECTED:
	std::vector<uint8_t> buff2head = { buff2.data(), buff2.data() + referencedata_hasher2_output.size() };
	ASSERT_NE(referencedata_hasher2_output, buff2head);

	// output is DIFFERENT than from no-padding case:
	//	[0x00000000]	0xd5 'Ő'	unsigned char
	//	[0x00000001]	0x40 '@'	unsigned char
	//	[0x00000002]	0xa7 '§'	unsigned char
	//	[0x00000003]	0x50 'P'	unsigned char
	//	[0x00000004]	0x58 'X'	unsigned char
	//	[0x00000005]	0xb4 '´'	unsigned char
	//	[0x00000006]	0x1c '\x1c'	unsigned char
	//	[0x00000007]	0xa0 ' '	unsigned char
	//	[0x00000008]	0x8b '‹'	unsigned char
	//	[0x00000009]	0xd8 'Ř'	unsigned char
	//	[0x0000000a]	0x17 '\x17'	unsigned char
	//	[0x0000000b]	0x70 'p'	unsigned char
	//	[0x0000000c]	0x3c '<'	unsigned char
	//	[0x0000000d]	0x77 'w'	unsigned char
	//	[0x0000000e]	0xaa 'Ş'	unsigned char
	//	[0x0000000f]	0xc1 'Á'	unsigned char
	//	....
	std::vector<uint8_t> othertestresult_nopadding = HexString::from("a20a06b0e87201aa9418cae9d981296c");
	std::vector<uint8_t> buff2head2 = { buff2.data(), buff2.data() + othertestresult_nopadding.size() };
	ASSERT_NE(othertestresult_nopadding, buff2head2);

	FreeLibrary(module);
}

TEST(RnD_DcminerUnUPXed_sanityChecks, blobHasherWithStringHasherOnSeparateBufferFails) {

	std::string modpath = "D:\\w\\q\\dcminer\\dcminer\\dcminer-unpacked.exe";
	HMODULE module = LoadLibraryA(modpath.c_str());

	size_t offs_code_hasher1 = 0x000000000000C910;
	size_t offs_code_hasher2 = 0x000000000000CC70;

	auto stringhasher = reinterpret_cast<void(*)(uint8_t* output, char* input)>((uint8_t*)module + offs_code_hasher1);
	auto blobhasher = reinterpret_cast<void(*)(uint8_t* key, void* unknown, uint8_t* input, uint8_t* output)>((uint8_t*)module + offs_code_hasher2);

	// below: ok.. zero-padding proved that PROBABLY the input should be much longer,
	// but let's prepare the zero-padded buffer once again and let's try crossing out another option:
	// theoretically, `hasher1` and `hasher2` may share some internal state. Let's see if running `hasher1`
	// has ANY effect on `hasher2`. It should not, and the output should be same as in zero-padded case.
	std::vector<uint8_t> referencedata_hasher1_output = HexString::from("B257A631E5203832CC45CCCC33330000");
	for (size_t x = 16; x < 2048; ++x) referencedata_hasher1_output.push_back(0x00);
	// 0000005DFFBFF590  B2 57 A6 31 E5 20 38 32 CC 45 CC CC 33 33 00 00  ²W¦1å 82ÌEÌÌ33..  
	// 0000005DFFBFF5A0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................  
	std::vector<uint8_t> gensig = HexString::from("1d949950a77b309782cda1b2ec735c7b8cbef7234d58867e53396e0e8ca1e16d");
	// 00007FF76D13F020  1D 94 99 50 A7 7B 30 97 82 CD A1 B2 EC 73 5C 7B  ...P§{0..Í¡²ìs\{  
	// 00007FF76D13F030  8C BE F7 23 4D 58 86 7E 53 39 6E 0E 8C A1 E1 6D  .¾÷#MX.~S9n..¡ám  
	std::vector<uint8_t> referencedata_hasher2_output = HexString::from("419493CB098376B1D2E52AD0CFDF983C7FF2917213D07FF65EB2FF1A6B18386E");
	// 00007FF76D13F020  41 94 93 CB 09 83 76 B1 D2 E5 2A D0 CF DF 98 3C  A..Ë..v±Òå*ÐÏß.<  
	// 00007FF76D13F030  7F F2 91 72 13 D0 7F F6 5E B2 FF 1A 6B 18 38 6E  .ò.r.Ð.ö^²ÿ.k.8n  

	// this block here rimply runs the `hasher1` and discards the output
	// if this has ANY impact on `hasher2`, then it means the hashing funcs
	// have some shared internal state that carries over
	{
		char seed[] = "DISKCOIN641187\x20\x20";
		std::vector<uint8_t> buff1(2048, 'X');
		stringhasher(buff1.data(), seed); // now that I think of it, I've already seen the 0xB0=176 size somewhere..

		// already tested in stringHasher tests, but just to make 100% sure that here `hasher1` behaves the same way
		for (size_t i = 176; i < 2048; ++i) ASSERT_EQ('X', buff1[i]);
		std::vector<uint8_t> buff1head = { buff1.data(), buff1.data() + 0x10 };
		ASSERT_EQ(HexString::from("B257A631E5203832CC45CCCC33330000"), buff1head);
	}

	std::vector<uint8_t> buff2(2048, 'X');

	blobhasher(referencedata_hasher1_output.data(), 0, buff2.data() + 0x00, gensig.data() + 0x00);
	for (size_t i = 0x10; i < 2048; ++i)
		ASSERT_EQ('X', buff2[i]);

	blobhasher(referencedata_hasher1_output.data(), 0, buff2.data() + 0x10, gensig.data() + 0x10);
	for (size_t i = 0x20; i < 2048; ++i)
		ASSERT_EQ('X', buff2[i]);

	// output is NOT AS EXPECTED:
	std::vector<uint8_t> buff2head = { buff2.data(), buff2.data() + referencedata_hasher2_output.size() };
	ASSERT_NE(referencedata_hasher2_output, buff2head);

	// output is SAME as in no-padding case:
	//	[0x00000000]	0xd5 'Ő'	unsigned char
	//	[0x00000001]	0x40 '@'	unsigned char
	//	[0x00000002]	0xa7 '§'	unsigned char
	//	[0x00000003]	0x50 'P'	unsigned char
	//	[0x00000004]	0x58 'X'	unsigned char
	//	[0x00000005]	0xb4 '´'	unsigned char
	//	[0x00000006]	0x1c '\x1c'	unsigned char
	//	[0x00000007]	0xa0 ' '	unsigned char
	//	[0x00000008]	0x8b '‹'	unsigned char
	//	[0x00000009]	0xd8 'Ř'	unsigned char
	//	[0x0000000a]	0x17 '\x17'	unsigned char
	//	[0x0000000b]	0x70 'p'	unsigned char
	//	[0x0000000c]	0x3c '<'	unsigned char
	//	[0x0000000d]	0x77 'w'	unsigned char
	//	[0x0000000e]	0xaa 'Ş'	unsigned char
	//	[0x0000000f]	0xc1 'Á'	unsigned char
	//	....
	std::vector<uint8_t> othertestresult_zeropadding = HexString::from("d540a75058b41ca08bd817703c77aac1");
	std::vector<uint8_t> buff2head2 = { buff2.data(), buff2.data() + othertestresult_zeropadding.size() };
	ASSERT_EQ(othertestresult_zeropadding, buff2head2);

	FreeLibrary(module);
}

TEST(RnD_DcminerUnUPXed_sanityChecks, blobHasherWithStringHasherOnSharedBufferWorks) {

	std::string modpath = "D:\\w\\q\\dcminer\\dcminer\\dcminer-unpacked.exe";
	HMODULE module = LoadLibraryA(modpath.c_str());

	size_t offs_code_hasher1 = 0x000000000000C910;
	size_t offs_code_hasher2 = 0x000000000000CC70;

	auto stringhasher = reinterpret_cast<void(*)(uint8_t* output, char* input)>((uint8_t*)module + offs_code_hasher1);
	auto blobhasher = reinterpret_cast<void(*)(uint8_t* key, void* unknown, uint8_t* input, uint8_t* output)>((uint8_t*)module + offs_code_hasher2);

	// alright. it seems that `hasher1` outputs MORE SIGNIFICANT data and probably the WHOLE data matters.
	// I have no idea how I managed to capture only first 16bytes and all-zeroes later, but it looks like
	// the capture was done in the middle of work, when the full `hasher1` output was not completed yet.
	// For example, maybe I captured it after a single hashing round? Actually, that's quite possible.
	// so.. using `referencedata_hasher1_output` is not an option.
	//std::vector<uint8_t> referencedata_hasher1_output = HexString::from("B257A631E5203832CC45CCCC33330000");
	// 0000005DFFBFF590  B2 57 A6 31 E5 20 38 32 CC 45 CC CC 33 33 00 00  ²W¦1å 82ÌEÌÌ33..  
	// 0000005DFFBFF5A0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................  
	std::vector<uint8_t> gensig = HexString::from("1d949950a77b309782cda1b2ec735c7b8cbef7234d58867e53396e0e8ca1e16d");
	// 00007FF76D13F020  1D 94 99 50 A7 7B 30 97 82 CD A1 B2 EC 73 5C 7B  ...P§{0..Í¡²ìs\{  
	// 00007FF76D13F030  8C BE F7 23 4D 58 86 7E 53 39 6E 0E 8C A1 E1 6D  .¾÷#MX.~S9n..¡ám  
	std::vector<uint8_t> referencedata_hasher2_output = HexString::from("419493CB098376B1D2E52AD0CFDF983C7FF2917213D07FF65EB2FF1A6B18386E");
	// 00007FF76D13F020  41 94 93 CB 09 83 76 B1 D2 E5 2A D0 CF DF 98 3C  A..Ë..v±Òå*ÐÏß.<  
	// 00007FF76D13F030  7F F2 91 72 13 D0 7F F6 5E B2 FF 1A 6B 18 38 6E  .ò.r.Ð.ö^²ÿ.k.8n  

	// Since we can't trust in the captured data for `referencedata_hasher1_output`,
	// and since we KNOW that `hasher1` and `hasher2` does not share any internal state,
	// let's just re-generate the whole `hasher1` result and use it as input to `hasher2`
	// Also, let's try checking if `hasher1` does not write further than out buffers.
	// Since hasher1 buff uses 'X' as control padding, let's change the buffer,
	// and change the padding here - if output is wrong, then it means we've lost
	// some part of hasher's state when copying data between buffers.
	std::vector<uint8_t> hasher1_output(2048, 'Z');
	{
		char seed[] = "DISKCOIN641187\x20\x20";
		std::vector<uint8_t> buff1(2048, 'X');
		stringhasher(buff1.data(), seed); // now that I think of it, I've already seen the 0xB0=176 size somewhere..

		// already tested in stringHasher tests, but just to make 100% sure that here `hasher1` behaves the same way
		for (size_t i = 176; i < 2048; ++i) ASSERT_EQ('X', buff1[i]);
		std::vector<uint8_t> buff1head = { buff1.data(), buff1.data() + 0x10 };
		ASSERT_EQ(HexString::from("B257A631E5203832CC45CCCC33330000"), buff1head);

		// copy ONLY those bytes that hasher1 wrote
		std::copy(buff1.begin(), buff1.begin() + 176, hasher1_output.begin());
		// hm.. hey. wasn't 176 related to the size of shabalcontext?
		for (size_t i = 176; i < 2048; ++i) ASSERT_EQ('Z', hasher1_output[i]);
	}

	std::vector<uint8_t> buff2(2048, 'X');

	blobhasher(hasher1_output.data(), 0, buff2.data() + 0x00, gensig.data() + 0x00);
	for (size_t i = 0x10; i < 2048; ++i)
		ASSERT_EQ('X', buff2[i]);

	blobhasher(hasher1_output.data(), 0, buff2.data() + 0x10, gensig.data() + 0x10);
	for (size_t i = 0x20; i < 2048; ++i)
		ASSERT_EQ('X', buff2[i]);

	// input data `referencedata_hasher1_output` tailed with 00 or FF - no difference
	// output is FULLY AS EXPECTED:
	//	[0x00000000]	0x41 'A'	unsigned char
	//	[0x00000001]	0x94 '”'	unsigned char
	//	[0x00000002]	0x93 '“'	unsigned char
	//	[0x00000003]	0xcb 'Ë'	unsigned char
	//	[0x00000004]	0x09 '\t'	unsigned char
	//	[0x00000005]	0x83 ''	unsigned char
	//	[0x00000006]	0x76 'v'	unsigned char
	//	[0x00000007]	0xb1 '±'	unsigned char
	//	[0x00000008]	0xd2 'Ň'	unsigned char
	//	[0x00000009]	0xe5 'ĺ'	unsigned char
	//	[0x0000000a]	0x2a '*'	unsigned char
	//	[0x0000000b]	0xd0 'Đ'	unsigned char
	//	[0x0000000c]	0xcf 'Ď'	unsigned char
	//	[0x0000000d]	0xdf 'ß'	unsigned char
	//	[0x0000000e]	0x98 ''	unsigned char
	//	[0x0000000f]	0x3c '<'	unsigned char
	//	....

	std::vector<uint8_t> buff2head = { buff2.data(), buff2.data() + referencedata_hasher2_output.size() };
	ASSERT_EQ(referencedata_hasher2_output, buff2head);

	FreeLibrary(module);
}

std::vector<uint8_t> diskcoin_generate_gensig_binload(size_t serverHeight, std::vector<uint8_t>& serverGenSig)
{
	std::string modpath = "D:\\w\\q\\dcminer\\dcminer\\dcminer-unpacked.exe";
	HMODULE module = LoadLibraryA(modpath.c_str());

	size_t offs_code_hasher1 = 0x000000000000C910;
	size_t offs_code_hasher2 = 0x000000000000CC70;

	// warning: that's `hasher1` and `hasher2`, I just came up with a bit better names
	auto initContext = reinterpret_cast<void(*)(uint8_t* output, char const* input)>((uint8_t*)module + offs_code_hasher1);
	auto hash128bits = reinterpret_cast<void(*)(uint8_t const* key, void* unknown, uint8_t const* input, uint8_t* output)>((uint8_t*)module + offs_code_hasher2);

	std::ostringstream cvt("                ");
	cvt << "DISKCOIN";
	cvt << serverHeight;
	std::string seed = cvt.str();

	std::vector<uint8_t> context(176, 'X');
	initContext(context.data(), seed.c_str());

	std::vector<uint8_t> newGenSig(0x20, 'X');
	hash128bits(context.data(), 0, newGenSig.data() + 0x00, serverGenSig.data() + 0x00);
	hash128bits(context.data(), 0, newGenSig.data() + 0x10, serverGenSig.data() + 0x10);

	FreeLibrary(module);

	return newGenSig;
}

std::vector<uint8_t> diskcoin_generate_gensig_aes128(size_t serverHeight, std::vector<uint8_t>& serverGenSig)
{
	std::ostringstream cvt("                ");
	cvt << "DISKCOIN";
	cvt << serverHeight;
	std::string seed = cvt.str();

	AES128_ctx ctx;
	AES128_init(&ctx, (unsigned char*)seed.c_str());

	std::vector<uint8_t> newGenSig(0x20, 'X');

	AES128_decrypt(&ctx, 1, newGenSig.data() + 0x00, serverGenSig.data() + 0x00);
	AES128_decrypt(&ctx, 1, newGenSig.data() + 0x10, serverGenSig.data() + 0x10);

	return newGenSig;
}

TEST(RnD_DcminerUnUPXed_GenerateGensig, dccONburst641159) {
	// based on data from LIVE debugging session
	// 23:20:17 * GMI: Received: HTTP/1.0 200 OK\r\nX-Ratelimit-Limit: 3\r\nX-Ratelimit-Remaining: 2\r\nX-Ratelimit-Reset: 1\r\nDate: Wed, 10 Jul 2019 21:20:17 GMT\r\nContent-Length: 151\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n{"baseTarget":34819,"generationSignature":"ed377f7ea5f857c565baa9e44aa94f5c8162c2fdf430d6eccdac4b856c6b5b68","height":641159,"targetDeadline":31536000}
	/*
		HEIGHT: [00007FF76D13EF60]      87 C8 09 00 = latest
		SIG:    [00007FF76D13F020]      = totally different
		00007FF76D13F020  BC EA A3 39 BD 7C D1 B1 33 74 9B F3 9F 3E E6 51
		00007FF76D13F030  8C C9 4E A3 16 FF 7C 49 CD 66 F6 D2 39 A3 9E F5
		BAS:    [00007FF76D13F150]      03 88 00 00 ... (34819) = latest
		NON:    rcx                     E0 0E CB 01 18 77 12 B6 ]+1  (res1 was the best)
		ACC:    ....                    6E680B79EFB72AB7 = ok
		SCOOP:  [00007FF76D13F014]      49 03 00 00 = noidea if it's synced or not
		  => DL     000015DE5AA6ECBA
	*/

	auto const height = 0x09C887; // 641159
	auto serverGenSig = HexString::from("ed377f7ea5f857c565baa9e44aa94f5c8162c2fdf430d6eccdac4b856c6b5b68"); // gensig from the pool
	uint64_t currentBaseTarget = 0x00008803; // 34819
	uint64_t account_nr = 7955621360090688183;

	auto currentSignature = diskcoin_generate_gensig_aes128(height, serverGenSig);

	auto referencedata_ExpectedSignature = HexString::from("BCEAA339BD7CD1B133749BF39F3EE6518CC94EA316FF7C49CD66F6D239A39EF5"); // 'corrected' gensig used by the miner
	EXPECT_EQ(referencedata_ExpectedSignature, currentSignature);
}
