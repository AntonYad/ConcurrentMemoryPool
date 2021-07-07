#include "PageCache.h"

PageCache PageCache::_inst;

Span* PageCache::NewSpan(size_t k)
{
	std::lock_guard<SpinLock> lock(_lock);
	return _NewSpan(k);
}

Span* PageCache::_NewSpan(size_t k)
{
	// �����Ҫ����128��ҳ����ֱ����ϵͳ����
	if (k >= NPAGES)
	{
		// �˴�������Ҫ�洢һ��span�������ͷ�ʱ��֪������ڴ�ҳ�ж��
		void* ptr = SystemAlloc(k);
		Span* span = new Span;
		span->_pageid = reinterpret_cast<PageID>(ptr) >> PAGE_SHIFT;
		_id_span_map[span->_pageid] = span;

		return span;
	}

	SpanList* list = &_freelist[k];
	if (!list->Empty())
	{
		return list->Pop();
	}

	for (size_t i = k + 1; i < NPAGES; ++i)
	{
		SpanList* list = &_freelist[i];
		if (!list->Empty())
		{
			// ȡ��kλ�����span
			Span* span = list->Pop();

			// ���ѳ�һ��span���أ�����ԭ��span��Ӧ��λ�ã�����pageid��span��ӳ��
			Span* split = new Span;
			split->_pageid = span->_pageid + span->_n - k;
			split->_n = k;
			for (size_t i = 0; i < split->_n; ++i)
			{
				_id_span_map[split->_pageid + i] = split;
			}
			span->_n -= k;
			_freelist[span->_n].Push(span);

			return split;
		}
	}

	AllocPage(NPAGES-1);

	return _NewSpan(k);
}

void PageCache::AllocPage(size_t k)
{
	Span* span = new Span;
	void* ptr = SystemAlloc(k);

	span->_pageid = reinterpret_cast<PageID>(ptr) >> PAGE_SHIFT;
	span->_n = k;
	for (size_t i = 0;  i < k; ++i)
	{
		_id_span_map[span->_pageid+i] = span;
	}

	_freelist[k].Push(span);
}

Span* PageCache::MapObjectToSpan(void* obj)
{
	PageID id = reinterpret_cast<PageID>(obj) >> PAGE_SHIFT;
	auto it = _id_span_map.find(id);
	assert(it != _id_span_map.end());

	return it->second;
}

void PageCache::ReleaseSpanToPageCahce(Span* cur)
{
	if (cur->_n >= NPAGES)
	{
		void* ptr = (void*)(cur->_pageid << PAGE_SHIFT);
		SystemFree(ptr);
		_id_span_map.erase(cur->_pageid);
		delete cur;
		return;
	}

	auto pit = _id_span_map.find(cur->_pageid - 1);
	while (pit != _id_span_map.end())
	{
		Span* prev = pit->second;
		if (prev->_use_count != 0)
			break;

		if (prev->_n + cur->_n >= NPAGES)
		{
			break;
		}

		// ���ǰһ��spanû��ʹ���ˣ���ô������span���кϲ�
		SpanList* list = &_freelist[prev->_n];
		list->Erase(prev);
		prev->_n += cur->_n;
		for (size_t i = 0; i < cur->_n; ++i)
		{
			_id_span_map[cur->_pageid + i] = prev;
		}

		delete cur;
		cur = prev;

		// ������ǰ�ϲ�
		pit = _id_span_map.find(cur->_pageid - 1);
	}

	auto nit = _id_span_map.find(cur->_pageid + cur->_n);
	while (nit != _id_span_map.end())
	{
		Span* next = nit->second;
		if (next->_use_count != 0)
			break;

		if (next->_n + cur->_n >= NPAGES)
		{
			break;
		}

		// ���ǰһ��spanû��ʹ���ˣ���ô������span���кϲ�
		SpanList* list = &_freelist[next->_n];
		list->Erase(next);
		cur->_n += next->_n;
		for (size_t i = 0; i < next->_n; ++i)
		{
			_id_span_map[next->_pageid + i] = cur;
		}

		delete next;

		// ������ǰ�ϲ�
		nit = _id_span_map.find(cur->_pageid + cur->_n);
	}

	cur->_objsize = 0;
	cur->_list = nullptr;
	_freelist[cur->_n].Push(cur);
}





