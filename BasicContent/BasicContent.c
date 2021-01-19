#include "ntifs.h"
#include "ntddk.h"
#include "ntstrsafe.h"

/**
 * ������̳��û�����
 */

 /**
  * �ڴ����
  * ����Ӧ�ò㣬��malloc�Լ�new�������ڶ��Ϸ����ڴ�
  * ���ǻ��������ڴ��ϸ�С���ȵķָ�жѹ���������������Ҫȥ����һҳ���ҳ�����ڴ棬�ٷָ����
  * ��֮���ƣ��ں����гأ�Pool���ĸ�����Դ��������ڴ棬WDK�ṩһЩ�к���������
  */
VOID Test_Pool()
{
	//��һ������PoolType��ʾ��������ڴ棬���õ���NonPagedPool��PagedPool
	//�Ƿ�ҳ�ڴ�һ�����ڸ�IRQL�����ڵ���DISPATCH_LEVEL���Ĵ����У�������Ҫ���ݴ����IRQL��ѡ����ʵ��ڴ�����
	//���ͻ��кܶ࣬�Ƚ���Ҫ��ע�Ļ���NonPagedPoolExecute��NonPagedPoolNx
	//�Ƿ�ҳ�����ڴ�����Ϊ��ִ�У���ζ�ſ���д�������ָ���ִ�У��Դ���©���Ĵ��룬����ʹ�û������������ִ��ָ��
	//NonPagedPoolNx��������Դ��治��Ҫ��ִ�����ԵķǷ�ҳ�ڴ棬Execute�����п�ִ�����Ե����ͣ�����ͨ�Ƿ�ҳ���͵ȼ�
	//�ڶ�������Ϊ�����С
	//������Tagһ�������Ų����⣬���ڴ�й©������ͨ���鿴ϵͳ��Tag��־��Ӧ�ڴ��С���ҵ������������ڴ��
	PVOID address;//ִ�гɹ����ط����ڴ���׵�ַ��ʧ�ܷ���NULL
	address = ExAllocatePoolWithTag(PagedPool, 128, 0);//����ҪTag����ʹ��ExAllocatePool����
	if (address == NULL)
	{
		DbgPrint("AllocatePool Failed.");
		return;
	}
	//�ڴ�ʹ�ú��ͷ�
	ExFreePoolWithTag(address, 0);
}
/**
 * ĳЩ�����У���Ҫ��Ƶ�ʵ�����̶���С���ڴ�
 * ʹ��ExAllocatePoolЧ�ʲ����ߣ�������������ڴ���Ƭ
 * Ϊ������ܣ���һ�� �����б� ���ڴ���䷽��
 * ���ȣ���ʼ��һ�������б���������ڴ���С
 * �����ڲ���ά���ڴ�ʹ��״̬��ͨ�����ƻ������˵��һ���Զ���صķ�ʽ���ڴ���ж��ι���
 */
BOOLEAN Test_Lookaside()
{
	//�Ƿ�ҳ�ڴ�ʾ��
	PNPAGED_LOOKASIDE_LIST pLookAsideList = NULL;
	BOOLEAN bSucc = FALSE;
	BOOLEAN bInit = FALSE;
	PVOID pFirstMemory = NULL;
	PVOID pSecondMemory = NULL;

	do
	{
		//�����ڴ�
		pLookAsideList = ExAllocatePoolWithTag(NonPagedPool, sizeof(NPAGED_LOOKASIDE_LIST), 'test');
		if (pLookAsideList == NULL)
		{
			break;
		}
		memset(pLookAsideList, 0, sizeof(NPAGED_LOOKASIDE_LIST));
		//��ʼ�������б���� �ڶ������������ֱ��Ƿ�����ͷ��ڴ�ĺ���ָ�룬�����Զ��壬����NULLΪʹ��ϵͳĬ�Ϻ���
		ExInitializeNPagedLookasideList(pLookAsideList, NULL, NULL, 0, 128, 'test', 0);
		bInit = TRUE;
		//�����ڴ�
		pFirstMemory = ExAllocateFromNPagedLookasideList(pLookAsideList);
		if (pFirstMemory == NULL)
		{
			break;
		}
		pSecondMemory = ExAllocateFromNPagedLookasideList(pLookAsideList);
		if (pSecondMemory == NULL)
		{
			break;
		}
		DbgPrint("First: %p , Second: %p\n", pFirstMemory, pSecondMemory);
		//�ͷŵ�һ���ڴ�
		ExFreeToNPagedLookasideList(pLookAsideList, pFirstMemory);
		pFirstMemory = NULL;
		//�ٴη���
		pFirstMemory = ExAllocateFromNPagedLookasideList(pLookAsideList);
		if (pFirstMemory == NULL)
		{
			break;
		}
		DbgPrint("ReAllocate First: %p \n", pFirstMemory);
		bSucc = TRUE;

	} while (FALSE);

	if (pFirstMemory != NULL)
	{
		ExFreeToNPagedLookasideList(pLookAsideList, pFirstMemory);
		pFirstMemory = NULL;
	}
	if (pSecondMemory != NULL)
	{
		ExFreeToNPagedLookasideList(pLookAsideList, pSecondMemory);
		pSecondMemory = NULL;
	}
	if (bInit == TRUE)
	{
		ExDeleteNPagedLookasideList(pLookAsideList);
		bInit = FALSE;
	}
	if (pLookAsideList != NULL)
	{
		ExFreePoolWithTag(pLookAsideList, 'test');
		pLookAsideList = NULL;
	}
	return bSucc;
}

/**
 * Windows��һ�ж���Ϊ�������������̡��̡߳������ȶ��Ƕ���
 * ����û�̬������Ҫ����һ���ں˶��󣬶����ڴ��ַ�����ں�̬��ַ�ռ䣬���û�̬�����޷�����
 * ����Ҫ���(HANDLE),�������ͬ�ں˶���ƾ֤���û�̬������Լ�Ӳ����ں˶���
 * ���ֻ�ڵ�ǰ�����������壬��Ϊ��ͬ�����и��Եľ�����ں�̬�У���һ��ϵͳ�ľ����������SYSTEM���̣�ֻ��һ���������ں���������ʹ�øñ�
 * ���������ĸ�������һ������F1�ڽ���P1��ִ�У���һ�����H1����һ������F2��Ҫ�ڽ���P2ʹ��H1������ͻ���ִ���
 * �����ڴ���H1���ʱ��ָ���������Ϊ�ں˾���������Ϳ��Կ����ʹ����
 */
BOOLEAN Test_Handle()
{
	BOOLEAN bSucc = FALSE;
	HANDLE hCreateEvent = NULL;
	PVOID pCreateEventObject = NULL;
	HANDLE hOpenEvent = NULL;
	PVOID pOpenEventObject = NULL;

	do
	{
		OBJECT_ATTRIBUTES obj_attr = { 0 };
		UNICODE_STRING uNameString = { 0 };
		RtlInitUnicodeString(&uNameString, L"\\BaswNamedObjects\\TestEvent");
		InitializeObjectAttributes(&obj_attr, &uNameString, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
		ZwCreateEvent(&hCreateEvent, EVENT_ALL_ACCESS, &obj_attr, SynchronizationEvent, FALSE);
		if (hCreateEvent == NULL)
		{
			break;
		}
		ObReferenceObjectByHandle(hCreateEvent, EVENT_ALL_ACCESS, *ExEventObjectType, KernelMode, &pCreateEventObject, NULL);
		if (pCreateEventObject == NULL)
		{
			break;
		}
		ZwOpenEvent(&hOpenEvent, EVENT_ALL_ACCESS, *ExEventObjectType, KernelMode, &pCreateEventObject, NULL);
		if (hOpenEvent == NULL)
		{
			break;
		}
		ObReferenceObjectByHandle(hOpenEvent, EVENT_ALL_ACCESS, *ExEventObjectType, KernelMode, &pOpenEventObject, NULL);
		if (pOpenEventObject == NULL)
		{
			break;
		}
		DbgPrint("Create Handle: %p, Create Pointer = %p \n", hCreateEvent, pCreateEventObject);
		DbgPrint("Open Handle: %p, Open Pointer = %p \n", hOpenEvent, pOpenEventObject);
		bSucc = TRUE;

	} while (FALSE);

	//�ͷ�
	if (pCreateEventObject != NULL)
	{
		ObDereferenceObject(pCreateEventObject);
		pCreateEventObject = NULL;
	}
	if (hCreateEvent != NULL)
	{
		ZwClose(hCreateEvent);
		hCreateEvent = NULL;
	}
	if (pOpenEventObject != NULL)
	{
		ObDereferenceObject(pOpenEventObject);
		pOpenEventObject = NULL;
	}
	if (hOpenEvent != NULL)
	{
		ZwClose(hOpenEvent);
		hOpenEvent = NULL;
	}

	return bSucc;
}

/**
 * ע�����һ���ļ���Windows\System32\config�µ��ļ������ڴ�ӳ��ķ�ʽӳ�䵽�ں˿ռ䣬Ȼ����һ��HIVE�ķ�ʽ��֯����
 * ֮ǰ��Ŀ�ж��ù����򵥹�һ��
 */
VOID Test_Reg_Create(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	//�����µ�ע������ֻ����IRQLΪPASSIVE_LEVEL�����У�Zw��ͷ����һ�㶼�ǣ�
	OBJECT_ATTRIBUTES objAttr = { 0 };
	HANDLE hkey = NULL;
	ULONG ulDisposition = 0;
	UNREFERENCED_PARAMETER(DriverObject);//���߱��������Ѿ�ʹ���˸ñ��������ؼ�⾯��
	InitializeObjectAttributes(&objAttr, RegistryPath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
	ZwCreateKey(&hkey, KEY_WRITE, &objAttr, 0, NULL, REG_OPTION_NON_VOLATILE, &ulDisposition);
	if (hkey != NULL)
	{
		//�޸ļ�
		UNICODE_STRING usValueName = { 0 };
		ULONG ulNewStartValue = 2;
		RtlInitUnicodeString(&usValueName, L"Start");
		ZwSetValueKey(hkey, &usValueName, 0, REG_DWORD, (PVOID)&ulNewStartValue, sizeof(ulNewStartValue));

		//��ѯ
		ULONG ulRetSize = 0;
		NTSTATUS nStatus = ZwQueryValueKey(hkey, &usValueName, KeyValuePartialInformation, NULL, 0, &ulRetSize);
		if (nStatus == STATUS_BUFFER_TOO_SMALL && ulRetSize != 0)//�ռ䲻��
		{
			//ulRetSize������������С��ȷ�������С�������ڴ沢�ٴβ�ѯ
			ULONG ulStartValue = 0;
			PKEY_VALUE_PARTIAL_INFORMATION pData = ExAllocatePoolWithTag(PagedPool, ulRetSize, 'DriF');
			if (pData != NULL)
			{
				memset(pData, 0, ulRetSize);
				nStatus = ZwQueryValueKey(hkey, &usValueName, KeyValuePartialInformation, (PVOID)pData, ulRetSize, &ulRetSize);
			}
			//�ͷ��ڴ�
			ExFreePoolWithTag(pData, 'Drif');
			pData = NULL;
		}

		//�رպ��ͷ�
		ZwClose(hkey);
		hkey = NULL;
	}
}

/**
 * �ļ�����
 * ·��ǰ�ӡ�\\??\\������Ϊ����ʱʹ�õ�ʱ����·��������C:����һ���������Ӷ�����������ڡ�\\??\\��·����
 * ������֮ǰ��Ӧ�ò�д����ʱ������Դ�������и��Ƴ�����·����VS�ǰ���������ʺŵ������в��ɼ�������һֱ��ȡ�����ļ�
 */
VOID Test_File(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	HANDLE file_handle = NULL;
	IO_STATUS_BLOCK io_status;
	//�ȳ�ʼ�������ļ�·����OBJECT_ATTRIBUTES
	OBJECT_ATTRIBUTES objAttr;
	UNICODE_STRING ufile_name = RTL_CONSTANT_STRING(L"\\??\\C:\\Test.txt");
	InitializeObjectAttributes(&objAttr, &ufile_name, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	//���ļ�
	ZwCreateFile(&file_handle, GENERIC_READ | GENERIC_WRITE, &objAttr, &io_status, NULL,
		FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN_IF, FILE_NON_DIRECTORY_FILE | FILE_RANDOM_ACCESS | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	//�ر��ļ�
	ZwClose(file_handle);
}
//��������ʾ��
NTSTATUS MyCopyFile(PUNICODE_STRING target_path, PUNICODE_STRING source_path)
{
	//�ļ����
	HANDLE target = NULL;
	HANDLE source = NULL;

	//����������
	PVOID buffer = NULL;
	LARGE_INTEGER offset = { 0 };
	IO_STATUS_BLOCK io_status = { 0 };

	NTSTATUS status;
	do
	{
		//���ļ����
		OBJECT_ATTRIBUTES objAttr_target;
		OBJECT_ATTRIBUTES objAttr_source;
		UNICODE_STRING u_target = *target_path;
		UNICODE_STRING u_source = *source_path;
		InitializeObjectAttributes(&objAttr_target, &u_target, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
		InitializeObjectAttributes(&objAttr_source, &u_source, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
		ZwCreateFile(&target, GENERIC_READ | GENERIC_WRITE, &objAttr_target, &io_status, NULL,
			FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN_IF, FILE_NON_DIRECTORY_FILE | FILE_RANDOM_ACCESS | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
		ZwCreateFile(&source, GENERIC_READ | GENERIC_WRITE, &objAttr_source, &io_status, NULL,
			FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN_IF, FILE_NON_DIRECTORY_FILE | FILE_RANDOM_ACCESS | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
		if (target == NULL || source == NULL)
		{
			break;
		}

		//��buffer�����ڴ�
		int length = 4 * 1024;
		buffer = ExAllocatePool(PagedPool, length);
		if (buffer == NULL)
		{
			break;
		}
		memset(buffer, 0, length);

		//ѭ����д
		while (1)
		{
			length = 4 * 1024;//ÿ����Ҫ���ĳ���
			status = ZwReadFile(source, NULL, NULL, NULL, &io_status, buffer, length, &offset, NULL);
			if (!NT_SUCCESS(status))
			{
				//�ж��Ƿ����
				if (status == STATUS_END_OF_FILE)
				{
					status = STATUS_SUCCESS;
					break;
				}
			}
			length = io_status.Information;//ʵ�ʶ����ĳ���
			//д���ļ�
			status = ZwWriteFile(target, NULL, NULL, NULL, &io_status, buffer, length, &offset, NULL);
			if (!NT_SUCCESS(status))
			{
				break;
			}
			//�ƶ�ƫ����������ѭ����ֱ������
			offset.QuadPart += length;
		}

	} while (0);

	//������Դ
	if (target != NULL)
	{
		ZwClose(target);
	}
	if (source != NULL)
	{
		ZwClose(source);
	}
	if (buffer != NULL)
	{
		ExFreePool(buffer);
	}

	return STATUS_SUCCESS;

}

/**
 * �����еȴ���ͣ�ٻ�ʹ����ϵͳ��ס��Ӧ������һ���߳�����һЩ���ڡ���ʱ�Ĳ���
 * ���������ɵ��߳�һ����ϵͳ�̣߳����ڽ���ΪSystem
 */
VOID MyThreadProc(PVOID context)
{
	PUNICODE_STRING str = (PUNICODE_STRING)context;
	DbgPrint("Print something in my thread.");
	PsTerminateSystemThread(STATUS_SUCCESS);//�����Լ�
}
VOID Test_Thread()
{
	UNICODE_STRING str = { 0 };
	RtlInitUnicodeString(&str, L"Hello!");
	HANDLE thread = NULL;
	NTSTATUS status = STATUS_SUCCESS;
	status = PsCreateSystemThread(&thread, 0, NULL, NULL, NULL, MyThreadProc, (PVOID)&str);
	//����д�������и����⣬��MyThreadProcִ��ʱ������������ִ������ˣ���ջ��str�����Ѿ��ͷţ��ᵼ������������Ҫ��str����ȫ�ֿռ�
	//�����ں�����ϵȴ��߳̽��������
	if (!NT_SUCCESS(status))
	{
		//������
	}
	//�رվ��
	ZwClose(thread);
}
//ͬ���¼�


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = TRUE;

	//�����б�
	Test_Lookaside();


	return status;
}