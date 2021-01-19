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
LIST_ENTRY g_ListHeader = { 0 };
VOID Test_List()
{
	//��ʼ��ͷ�ڵ�ʹ��Flink��Blinkָ������
	InitializeListHead(&g_ListHeader);

	//����ڵ�
	TestListEntry EntryA = { 0 };
	TestListEntry EntryB = { 0 };
	TestListEntry EntryC = { 0 };
	EntryA.m_data = 'A';
	EntryB.m_data = 'B';
	EntryC.m_data = 'C';
	InsertHeadList(&g_ListHeader, &EntryB.m_list_entry);
	InsertHeadList(&g_ListHeader, &EntryA.m_list_entry);
	InsertTailList(&g_ListHeader, &EntryC.m_list_entry);

	//�����ڵ㣬ͨ��Flink��ǰ���
	PLIST_ENTRY pListEntry = NULL;
	pListEntry = g_ListHeader.Flink;
	while (pListEntry != &g_ListHeader)
	{
		PTestListEntry pTestListEntry = CONTAINING_RECORD(pListEntry, TestListEntry, m_list_entry);
		DbgPrint("ListPtr = %p, Entry = %p, Tag = %c \n", pListEntry, pTestListEntry, (CHAR)pTestListEntry->m_data);
		pListEntry = pListEntry->Flink;
	}

	//�ڵ��Ƴ���ͷβ�Ƴ�/ָ���Ƴ�
	PLIST_ENTRY RemoveEntryA = RemoveHeadList(&g_ListHeader);
	DbgPrint("RemoveEntryA = %p", RemoveEntryA);
	PLIST_ENTRY RemoveEntryC = RemoveTailList(&g_ListHeader);
	DbgPrint("RemoveEntryC = %p", RemoveEntryC);
	BOOLEAN isEmpty = RemoveEntryList(&EntryB.m_list_entry);
	DbgPrint("After remove EntryB, List is Empty: %d", isEmpty);
	BOOLEAN isListEmpty = IsListEmpty(&g_ListHeader);
	DbgPrint("List is Empty: %d", isListEmpty);
}

/**
 * ���������Ľṹ���ǻ����ٶ��߳�ͬ�����⣬��Ҫʹ����
 * ���������ں����ṩ�ĸ�IRQL������ͬ���Լ���ռ��ʽ����ĳ����Դ
 */
KSPIN_LOCK g_my_spin_lock = { 0 };
VOID Init_Lock()
{
	//��ʼ��������
	KeInitializeSpinLock(&g_my_spin_lock);
}
VOID Test_Lock()
{
	//ʹ��
	KIRQL irql;//�жϼ���
	KeAcquireSpinLock(&g_my_spin_lock, &irql);//KeAcquireSpinLock����ߵ�ǰ�жϼ��𣬽��ɵ��жϼ��𱣴浽irql
	// do something ֻ�е��߳�ִ���м䲿�ִ��룬Ҳ����ֻ��һ���߳��ܻ��������
	KeReleaseSpinLock(&g_my_spin_lock, irql);
}
//һ��ʹ�������ڲ���ǰ���û�ȡ����ִ�в������ͷ�������LIST_ENTRY�Ĳ����У����Դ���һ������ȥ
VOID Op_List_With_Lock()
{
	TestListEntry EntryTest = { 0 };
	EntryTest.m_data = '0';
	//��ͨ����
	//InsertHeadList(&g_ListHeader, &EntryTest);
	//��������
	ExInterlockedInsertHeadList(&g_ListHeader, &EntryTest, &g_my_spin_lock);
	//�����Ƴ�
	PLIST_ENTRY pRemoveEntry = NULL;
	pRemoveEntry = ExInterlockedRemoveHeadList(&g_ListHeader, &g_my_spin_lock);
}
VOID Test_Lock_In_List()
{
	//��ʼ��ͷ�ڵ�
	InitializeListHead(&g_ListHeader);
	//��ʼ��������
	Init_Lock();

	//��������
	Op_List_With_Lock();
}
//���������������ܸ��ã�������ѭ�ȵȴ����Ȼ�ȡԭ��
KSPIN_LOCK g_my_queue_spin_lock = { 0 };
VOID Init_Queue_Lock()
{
	//��ʼ����ʽ����ͨ��������ʽ��ͬ
	KeInitializeSpinLock(&g_my_queue_spin_lock);
}
VOID Test_Queue_Lock()
{
	//ʹ�ò�ͬ
	KLOCK_QUEUE_HANDLE my_lock_queue_handle;//����һ�����ݽṹΨһ�ı�ʾһ������������
	KeAcquireInStackQueuedSpinLock(&g_my_queue_spin_lock, &my_lock_queue_handle);
	// do something
	KeReleaseInStackQueuedSpinLock(&g_my_queue_spin_lock, &my_lock_queue_handle);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = TRUE;

	//����ʹ��
	//Test_List();

	//��Ӧ��
	Test_Lock_In_List();

	return status;
}