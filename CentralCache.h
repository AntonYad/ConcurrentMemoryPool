#ifndef __CENTRAL_CACHE_H__
#define __CENTRAL_CACHE_H__

#include "Common.h"

// 设计为单例模式
class CentralCache
{
public:
	static CentralCache* GetInstance(){
		return &_inst;
	}

	// 从中心缓存获取一定数量的对象给thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size);

	// 将一定数量的对象释放到span跨度
	void ReleaseListToSpans(void* start, size_t byte_size);

	// 从page cache获取一个span
	Span* GetOneSpan(SpanList* list, size_t byte_size);
private:
	// 中心缓存自由链表
	SpanList _freeList[NLISTS];
private:
	CentralCache() = default;
	CentralCache(const CentralCache&) = delete;
	CentralCache& operator=(const CentralCache&) = delete;

	// 单例对象
	static CentralCache _inst;
};


#endif // __CENTRAL_CACHE_H__