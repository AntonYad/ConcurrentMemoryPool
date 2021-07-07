#include "ThreadCache.h"
#include "Alloc.h"

#include <vector>

void TestSizeClass()
{
	cout << SizeClass::Index(128 - 8) << endl;
	cout << SizeClass::Index(128) << endl;

	cout << SizeClass::Index(1024 - 16) << endl;
	cout << SizeClass::Index(1024) << endl;

	cout << SizeClass::Index(1024 * 8 - 128) << endl;
	cout << SizeClass::Index(1024 * 8) << endl;

	cout << SizeClass::Index(1024 * 64 - 512) << endl;
	cout << SizeClass::Index(1024 * 64) << endl;
	cout << endl;

	cout << SizeClass::RoundUp(127) << endl;
	cout << SizeClass::RoundUp(1023) << endl;
	cout << SizeClass::RoundUp(1024 * 8 - 1) << endl;
	cout << SizeClass::RoundUp(1024 * 64 - 1) << endl;
}

void TestThreadCache()
{
	ThreadCache cache;
	std::vector<void*> v;
	for (size_t i = 0; i < 10000; ++i)
	{
		v.push_back(cache.Allocate(10));
		cout << "alloc:"<<v.back() << endl;
	}

	for (size_t i = v.size(); i > 0; --i)
	{
		cout << "dealloc:" << v[i-1] << endl;
		cache.Deallocate(v[i-1], 10);
	}
}

void TestLargeAlloc()
{
	void* ptr1 = ConcurrentAlloc(100 << PAGE_SHIFT);
	ConcurrentFree(ptr1);

	void* ptr2 = ConcurrentAlloc(129 << PAGE_SHIFT);
	ConcurrentFree(ptr2);
}

//int main()
//{
//	TestLargeAlloc();
//
//	return 0;
//}