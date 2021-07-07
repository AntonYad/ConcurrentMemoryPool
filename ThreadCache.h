#ifndef __THREAD_CACHE_H__
#define __THREAD_CACHE_H__

#include "Common.h"

class ThreadCache
{
public:
	// ������ͷ��ڴ����
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);

	// �����Ļ����ȡ����
	void* FetchFromCentralCache(size_t index, size_t size);
	// �ͷŶ���ʱ���������ʱ�������ڴ�ص����Ķ�
	void ListTooLong(FreeList* list, size_t size);
private:
	FreeList _freeList[NLISTS];	   // ��������
};

static __declspec(thread) ThreadCache* tls_threadcache = nullptr;

#endif // __THREAD_CACHE_H__