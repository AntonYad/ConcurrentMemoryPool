#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_inst;
int count = 0;
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size)
{
	size_t fetch_count = 0;
	size_t index = SizeClass::Index(byte_size);
	SpanList* list = &_freeList[index];
	{
		std::unique_lock<SpinLock> lock(list->_lock);
		Span* span = GetOneSpan(list, byte_size);

		void* cur = span->_list;
		void* prev = cur;
		while (fetch_count < n && cur != nullptr)
		{
			prev = cur;
			cur = NEXT_OBJ(cur);
			++fetch_count;
		}

		start = span->_list;
		end = prev;
		NEXT_OBJ(end) = nullptr;

		span->_list = cur;
		span->_use_count += fetch_count;

		// 如果当前的span已经为空，则将span移除出来插入到最后面
		if (span->_list == nullptr)
		{
			list->Erase(span);
			list->Insert(list->_head, span);
		}
	}

	return fetch_count;
}

Span* CentralCache::GetOneSpan(SpanList* list, size_t byte_size)
{
	Span* span = list->Begin();
	while (span != list->End())
	{
		if (span->_list != nullptr)
			return span;

		span = span->_next;
	}

	const size_t pages = SizeClass::NumMovePage(byte_size);
	span = PageCache::GetInstance()->NewSpan(pages);

	// 切割分配到span对象内存，然后串到一起
	char* start = reinterpret_cast<char*>(span->_pageid << PAGE_SHIFT);
	char* prev = start;
	char* cur = start;
	char* end = cur + (pages << PAGE_SHIFT);
	while (cur < end)
	{
		char* next = cur + byte_size;
		NEXT_OBJ(cur) = next;
		prev = cur;
		cur = next;
	}

	NEXT_OBJ(prev) = nullptr;

	span->_list = start;
	span->_use_count = 0;
	span->_objsize = byte_size;

	list->Push(span);

	return span;
}

void CentralCache::ReleaseListToSpans(void* start, size_t byte_size)
{
	size_t index = SizeClass::Index(byte_size);
	SpanList* list = &_freeList[index];
	void* cur = start;
	std::unique_lock<SpinLock> lock(list->_lock);
	while (cur)
	{
		Span* span = PageCache::GetInstance()->MapObjectToSpan(cur);
		// span如果是空的，则将span移除出来插入到头上去
		if (span->_list == nullptr)
		{
			list->Erase(span);
			list->Push(span);
		}

		// 将对象插入到span
		void* next = NEXT_OBJ(cur);
		NEXT_OBJ(cur) = span->_list;
		span->_list = cur;

		cur = next;

		// 如果span中的对象已经全部回收回来，那么将span放回PageCache，并进行合并
		if (--span->_use_count == 0)
		{
			list->Erase(span);
			PageCache::GetInstance()->ReleaseSpanToPageCahce(span);
		}
	}
}