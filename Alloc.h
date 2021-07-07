#include"ThreadCache.h"
#include "PageCache.h"
#include "Common.h"

static inline void* ConcurrentAlloc(size_t size)
{
	if (size > MAX_BYTES)
	{
		size_t align_size = SizeClass::_RoundUp(size, PAGE_SHIFT);
		size_t pagenum = align_size >> PAGE_SHIFT;
		Span* span = PageCache::GetInstance()->NewSpan(pagenum);
		// ���span���б�ǣ���ֹspan���ϲ���
		span->_objsize = align_size;
		span->_use_count = -1;
		span->_n = pagenum;

		return (void*)((span->_pageid) << PAGE_SHIFT);
	}
	else
	{	
		// �̵߳�һ�������ڴ棬�򿪱�һ��ThreadCache����
		if (tls_threadcache == nullptr)
			tls_threadcache = new ThreadCache;

		return tls_threadcache->Allocate(size);
	}
}

static inline void ConcurrentFree(void *p)
{
	Span* span = PageCache::GetInstance()->MapObjectToSpan(p);
	size_t size = span->_objsize;
	if (size > MAX_BYTES)
	{
		PageCache::GetInstance()->ReleaseSpanToPageCahce(span);
	}
	else
	{
		tls_threadcache->Deallocate(p, size);
	}
}