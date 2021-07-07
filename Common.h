#ifndef __COMMON_H__
#define __COMMON_H__

#include <iostream>
#include <exception>
#include <algorithm>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <atomic>

#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

using std::cout;
using std::endl;

static const size_t MAX_BYTES = 64 * 1024;
static const size_t NLISTS = 184;
static const size_t PAGE_SHIFT = 12;
static const size_t NPAGES = 129;

inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage*(1 << PAGE_SHIFT),
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// brk mmap等
#endif

	if (ptr == nullptr)
		throw std::bad_alloc();

	return ptr;
}

inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	// sbrk unmmap等
#endif
}

/*
class SpinLock
{
public:
	void lock()
	{
		bool expect = false;
		while (!_flag.compare_exchange_weak(expect, true))
		{
			expect = false;
		}
	}

	void unlock()
	{
		_flag.store(false);
	}

	SpinLock()
		:_flag(false)
	{}

	SpinLock(const SpinLock&) = delete;
	SpinLock& operator=(const SpinLock&) = delete;

private:
	std::atomic<bool> _flag;
};
*/


class SpinLock
{
public:
	void lock()
	{
		_mtx.lock();
	}

	void unlock()
	{
		_mtx.unlock();
	}

	SpinLock() = default;

	SpinLock(const SpinLock&) = delete;
	SpinLock& operator=(const SpinLock&) = delete;

private:
	std::mutex _mtx;
};

typedef size_t PageID;
struct Span
{
	PageID        _pageid = 0;				 // Starting page number
	size_t        _n = 0;					 // Number of pages in span
	Span*         _next = nullptr;           // Used when in link list
	Span*         _prev = nullptr;           // Used when in link list

	void*		  _list = nullptr;
	size_t		  _use_count = 0;
	size_t		  _objsize = 0;
};

struct SpanList
{
	Span* _head;
	SpinLock _lock;

	Span* Begin()
	{
		return _head->_next;
	}

	Span* End()
	{
		return _head;
	}

	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	void Insert(Span* pos, Span* newspan)
	{
		assert(pos);
		Span* prev = pos->_prev;

		// prev newspan pos
		prev->_next = newspan;
		newspan->_prev = prev;

		newspan->_next = pos;
		pos->_prev = newspan;
	}

	void Erase(Span* pos)
	{
		assert(pos && pos != _head);
		Span* prev = pos->_prev;
		Span* next = pos->_next;
		prev->_next = next;
		next->_prev = prev;

		pos->_next = nullptr;
		pos->_prev = nullptr;
	}

	Span* Pop()
	{
		Span* first = _head->_next;
		Erase(first);
		return first;
	}

	void Push(Span* newspan)
	{
		Insert(_head->_next, newspan);
	}

	bool Empty()
	{
		return _head->_next == _head;
	}
};

//////////////////////////////////////////////////////////////
// 自由链表
static inline void*& NEXT_OBJ(void* obj)
{
	return *(reinterpret_cast<void**>(obj));
}

class FreeList
{
public:
	bool Empty() const{
		return _list == nullptr;
		//return _size == 0;
	}

	size_t Size() const{
		return _size;
	}

	size_t MaxSize() const{
		return _maxsize;
	}

	void SetMaxSize(size_t newsize)
	{
		_maxsize = newsize;
	}

	// [start, end]
	void PushRange(void* start, void* end, size_t n)
	{
		NEXT_OBJ(end) = _list;
		_list = start;
		_size += n;
	}

	// [start, end]
	void PopRange(void*& start, void*& end, size_t n)
	{
		assert(n <= _size);

		void* cur = _list;
		for (size_t i = 1; i < n; ++i)
		{
			cur = NEXT_OBJ(cur);
		}

		start = _list;
		end = cur;

		_list = NEXT_OBJ(end);
		NEXT_OBJ(end) = nullptr;

		_size -= n;
	}

	void Push(void* obj)
	{
		NEXT_OBJ(obj) = _list;
		_list = obj;
		++_size;
	}

	void* Pop() 
	{
		assert(_list != nullptr);
		void* obj = _list;
		_list = NEXT_OBJ(_list);
		--_size;

		return obj;
	}
private:
	void*	_list = nullptr;
	size_t	_size = 0;
	size_t	_maxsize = 1;
};

////////////////////////////////////////////////////////////
// size及相关处理

class SizeClass
{
public:
	// 控制在12%左右的内碎片浪费
	// [1,128]					8byte对齐	 freelist[0,16)
	// [129,1024]				16byte对齐	 freelist[16,72)
	// [1025,8*1024]			128byte对齐	 freelist[72,128)
	// [8*1024+1,64*1024]		512byte对齐  freelist[128,240)

	static inline size_t _RoundUp(size_t bytes, size_t align)
	{
		return (((bytes)+align - 1) & ~(align - 1));
	}

	// 对齐大小计算，浪费大概在1%-12%左右
	static inline size_t RoundUp(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);

		if (bytes <= 128){
			return _RoundUp(bytes, 8);
		}
		else if (bytes <= 1024){
			return  _RoundUp(bytes, 16);
		}
		else if (bytes <= 8192){
			return  _RoundUp(bytes, 128);
		}
		else if (bytes <= 65536){
			return  _RoundUp(bytes, 1024);
		}

		return -1;
	}

	static inline size_t _Index(size_t bytes, size_t align_shift)
	{
		return ((bytes + (1<<align_shift) - 1) >> align_shift) - 1;
	}

	// 映射的自由链表的位置
	static inline size_t Index(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);

		// 每个区间有多少个链
		static int group_array[4] = {16, 56, 56, 112};
		if (bytes <= 128){
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024){
			return _Index(bytes - 128, 4) + group_array[0];
		}
		else if (bytes <= 8192){
			return _Index(bytes - 1024, 7) + group_array[1] + group_array[0];
		}
		else if (bytes <= 65536){
			return _Index(bytes - 8192, 9) + group_array[2] + group_array[1] + group_array[0];
		}

		assert(false);

		return -1;
	}

	static size_t NumMoveSize(size_t size)
	{
		if (size == 0)
			return 0;

		int num = static_cast<int>(MAX_BYTES / size);
		if (num < 2)
			num = 2;

		if (num > 512)
			num = 512;

		return num;
	}

	// 计算一次向系统获取几个页
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);
		size_t npage = num*size;

		npage >>= 12;
		if (npage == 0)
			npage = 1;

		return npage;
	}
};



#endif // __COMMON_H__