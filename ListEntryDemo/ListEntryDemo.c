#include "ntddk.h"

/**
 * ����
 * ʹ�ý϶��Ϊ˫������WDK�ж������һ���ڵ㶨��ΪLIST_ENTRY
 */

 //LIST_ENTRYֻ��������ָ��ָ��ǰ��ڵ㣬ʹ��ʱ��Ҫ������Ϊ��Ա����һ���ṹ��
typedef struct _TestListEntry
{
	VOID* m_data;
	LIST_ENTRY m_list_entry;//ÿ���ڵ��Flink��Blink��ָ��ǰ����һ���ڵ��Flink
} TestListEntry, * PTestListEntry;
//һ��ᶨ��һ������ͷ�ڵ㣬�������κ����ݣ�ֻ��һ��LIST_ENTRY

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = TRUE;

	//��ʼ��ͷ�ڵ�ʹ��Flink��Blinkָ������
	LIST_ENTRY ListHeader = { 0 };
	InitializeListHead(&ListHeader);

	//����ڵ�
	TestListEntry EntryA = { 0 };
	TestListEntry EntryB = { 0 };
	TestListEntry EntryC = { 0 };
	EntryA.m_data = 'A';
	EntryB.m_data = 'B';
	EntryC.m_data = 'C';
	InsertHeadList(&ListHeader, &EntryB.m_list_entry);
	InsertHeadList(&ListHeader, &EntryA.m_list_entry);
	InsertTailList(&ListHeader, &EntryC.m_list_entry);

	//�����ڵ㣬ͨ��Flink��ǰ���
	PLIST_ENTRY pListEntry = NULL;
	pListEntry = ListHeader.Flink;
	while (pListEntry != &ListHeader)
	{
		PTestListEntry pTestListEntry = CONTAINING_RECORD(pListEntry, TestListEntry, m_list_entry);
		DbgPrint("ListPtr = %p, Entry = %p, Tag = %c \n", pListEntry, pTestListEntry, (CHAR)pTestListEntry->m_data);
		pListEntry = pListEntry->Flink;
	}

	//�ڵ��Ƴ���ͷβ�Ƴ�/ָ���Ƴ�
	PLIST_ENTRY RemoveEntryA = RemoveHeadList(&ListHeader);
	DbgPrint("RemoveEntryA = %p", RemoveEntryA);
	PLIST_ENTRY RemoveEntryC = RemoveTailList(&ListHeader);
	DbgPrint("RemoveEntryC = %p", RemoveEntryC);
	BOOLEAN isEmpty = RemoveEntryList(&EntryB.m_list_entry);
	DbgPrint("After remove EntryB, List is Empty: %d", isEmpty);
	BOOLEAN isListEmpty = IsListEmpty(&ListHeader);
	DbgPrint("List is Empty: %d", isListEmpty);

	return status;
}