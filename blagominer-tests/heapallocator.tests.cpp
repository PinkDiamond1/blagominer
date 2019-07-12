#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "heapallocator.h"

TEST(HeapAllocator, WinapiHeapValidateWorks) {
	HANDLE hHeap = GetProcessHeap();

	EXPECT_EQ(false, HeapValidate(hHeap, 0, reinterpret_cast<LPCVOID>(-1)));

	void* ptr = HeapAlloc(hHeap, 0, 10);
	EXPECT_EQ(true, HeapValidate(hHeap, 0, ptr));

	HeapFree(hHeap, 0, ptr);
	EXPECT_EQ(false, HeapValidate(hHeap, 0, ptr));
}

TEST(HeapAllocator, AllocatesBlockFromHeap) {
	HANDLE hHeap = GetProcessHeap();
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	heap_allocator<char> tested = { hHeap, 0 };

	char* ptr = tested.allocate(10);

	EXPECT_EQ(true, HeapValidate(hHeap, 0, ptr));

	HeapFree(hHeap, 0, ptr);
}

TEST(HeapAllocator, FreesBlockFromHeap) {
	HANDLE hHeap = GetProcessHeap();
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	heap_allocator<char> tested = { hHeap, 0 }; // HEAP_ZERO_MEMORY

	char* ptr = tested.allocate(10);

	tested.deallocate(ptr, 10);

	EXPECT_EQ(false, HeapValidate(hHeap, 0, ptr));
}

TEST(HeapAllocator, ThrowsWhenFailsToAllocate) {
	HANDLE hHeap = GetProcessHeap();
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	heap_allocator<char> tested = { hHeap, 0 }; // HEAP_ZERO_MEMORY

	// this tests _that_ the expected exception is thrown
	EXPECT_THROW({
		try
		{
			char* ptr = tested.allocate(INT64_MAX);
		}
		catch (const std::bad_alloc& e)
		{
			// EXPECT_...
			throw;
		}
	}, std::bad_alloc);
}

TEST(HeapAllocator, PassesFlagsToHeapAlloc) {
	HANDLE hHeap = GetProcessHeap();
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	heap_allocator<char> tested1 = { hHeap, 0 };
	heap_allocator<char> tested2 = { hHeap, HEAP_ZERO_MEMORY };

	char* ptr = tested1.allocate(10240);

	ptr[0] = 0x00;
	ptr[1] = 0x00;
	ptr[2] = 0x00;
	ptr[3] = 0x00;
	ptr[4] = 0xAA;
	ptr[5] = 0xFF;

	HeapFree(hHeap, 0, ptr);

	char* ptr2 = tested1.allocate(10240);
	//ASSERT_EQ(ptr, ptr2);

	bool any = false;
	for (size_t i = 0; i < 10240 && !any; ++i) any = ptr[i] != 0;
	EXPECT_TRUE(any);

	HeapFree(hHeap, 0, ptr2);

	char* ptr3 = tested2.allocate(10240);
	//ASSERT_EQ(ptr, ptr3);

	any = false;
	for (size_t i = 0; i < 10240 && !any; ++i) any = ptr[i] != 0;
	EXPECT_FALSE(any);

	HeapFree(hHeap, 0, ptr3);
}
