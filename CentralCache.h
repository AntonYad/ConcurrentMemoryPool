#ifndef __CENTRAL_CACHE_H__
#define __CENTRAL_CACHE_H__

#include "Common.h"

// ���Ϊ����ģʽ
class CentralCache
{
public:
	static CentralCache* GetInstance(){
		return &_inst;
	}

	// �����Ļ����ȡһ�������Ķ����thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size);

	// ��һ�������Ķ����ͷŵ�span���
	void ReleaseListToSpans(void* start, size_t byte_size);

	// ��page cache��ȡһ��span
	Span* GetOneSpan(SpanList* list, size_t byte_size);
private:
	// ���Ļ�����������
	SpanList _freeList[NLISTS];
private:
	CentralCache() = default;
	CentralCache(const CentralCache&) = delete;
	CentralCache& operator=(const CentralCache&) = delete;

	// ��������
	static CentralCache _inst;
};


#endif // __CENTRAL_CACHE_H__