#pragma once
#include "common-pragmas.h"
#include "stdafx.h"

#include <stdexcept>
#include <iomanip>

class heap_alloc_failed : public std::bad_alloc
{
	std::string msg;

public:
	heap_alloc_failed(std::string const& msg) : msg(msg) {}

	virtual char const* what() const
	{
		return msg.c_str();
	}
};

template <typename T>
class heap_allocator
{
public:
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;

	heap_allocator() : heapHandle(INVALID_HANDLE_VALUE), flags(0) {}
	heap_allocator(HANDLE heapHandle, DWORD flags) : heapHandle(heapHandle), flags(flags) {}
	~heap_allocator() {}

	template <class U> friend class heap_allocator;
	template <class U> struct rebind { typedef heap_allocator<U> other; };
	template <class U> heap_allocator(const heap_allocator<U>& other) : heapHandle(other.heapHandle), flags(other.flags) { }

	pointer address(reference x) const { return &x; }
	const_pointer address(const_reference x) const { return &x; }
	size_type max_size() const throw() { return size_t(-1) / sizeof(value_type); }

	pointer allocate(size_type n, const_pointer hint = 0)
	{
		auto ptr = HeapAlloc(heapHandle, flags, n * sizeof(T));
		if (ptr == nullptr)
		{
			std::ostringstream tmp;
			tmp << "Could not allocate " << n << " from heap " << "0x" << std::hex << std::setw(2 * sizeof(void*)) << std::setfill('0') << heapHandle;
			throw heap_alloc_failed(tmp.str());
		}

		return static_cast<pointer>(ptr);
	}

	void deallocate(pointer p, size_type n)
	{
		HeapFree(heapHandle, 0, p);
	}

	void construct(pointer p, const T& val)
	{
		new(static_cast<void*>(p)) T(val);
	}

	void construct(pointer p)
	{
		new(static_cast<void*>(p)) T();
	}

	void destroy(pointer p)
	{
		p->~T();
	}

private:
	HANDLE heapHandle;
	DWORD flags;
};
