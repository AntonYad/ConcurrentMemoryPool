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

	// ��ϵͳ����kҳ�ڴ�
	void AllocPage(size_t k);

	// ����һ���µ�span
	Span* _NewSpan(size_t k);
	Span* NewSpan(size_t k);

	// ��ȡ�Ӷ���span��ӳ��
	Span* MapObjectToSpan(void* obj);

	// �ͷſ���span�ص�Pagecache�����ϲ����ڵ�span
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