#ifndef __PAGE_CACHE_H__
#define __PAGE_CACHE_H__

#include "Common.h"

class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_inst;
	}

	// 向系统申请k页内存
	void AllocPage(size_t k);

	// 申请一个新的span
	Span* _NewSpan(size_t k);
	Span* NewSpan(size_t k);

	// 获取从对象到span的映射
	Span* MapObjectToSpan(void* obj);

	// 释放空闲span回到Pagecache，并合并相邻的span
	void ReleaseSpanToPageCahce(Span* span);
private:
	SpanList _freelist[NPAGES];

private:
	PageCache()
	{}

	PageCache(const PageCache&) = delete;
	static PageCache _inst;

	std::unordered_map<PageID, Span*> _id_span_map;
	SpinLock _lock;
};


#endif // __PAGE_CACHE_H__