#include "ntddk.h"

/**
 * Ӧ�ò����ں˲�ͨ��
 * ���û���Ӧ�ü���DLLģ����õ���������ͬ
 * ��Ҫ���ں�����ͨ�ţ���Ҫ���豸����Ϊ����
 * �豸����������ں��б�¶��Ӧ�ò㣬��Ӧ�ò�������ļ�һ��������
 */

#define DEV_NAME L"\\Device\\CO_WORK_DRIVER"
#define SYM_NAME L"\\??\\MY_CO_WORK_DRIVER"
#define SDDL_SYM L"D:P(A;;GA;;;WD)" //��ȫ��������D:P��ͷ�Ӷ�����������еķ��ţ�����Ϊ�����κ��û����ʸ��豸�İ�ȫ�ַ������������


 // ��Ӧ�ò����������һ���ַ�����
#define  CWK_DVC_SEND_STR \
	(ULONG)CTL_CODE( \
	FILE_DEVICE_UNKNOWN, \
	0x911,METHOD_BUFFERED, \
	FILE_WRITE_DATA)

// ��������ȡһ���ַ���
#define  CWK_DVC_RECV_STR \
	(ULONG)CTL_CODE( \
	FILE_DEVICE_UNKNOWN, \
	0x912,METHOD_BUFFERED, \
	FILE_READ_DATA)

// ����һ���������������ַ���
#define CWK_STR_LEN_MAX 512
typedef struct {
	LIST_ENTRY list_entry;
	char buf[CWK_STR_LEN_MAX];
} CWK_STR_NODE, * PCWK_STR_NODE;

// ��������һ������������֤��������İ�ȫ��
KSPIN_LOCK g_cwk_lock;
// һ���¼�����ʶ�Ƿ����ַ�������ȡ
KEVENT  g_cwk_event;
// �����и�����ͷ
LIST_ENTRY g_cwk_str_list;

#define MEM_TAG 'cwkr'

// �����ڴ沢��ʼ��һ������ڵ�
PCWK_STR_NODE cwkMallocStrNode()
{
	PCWK_STR_NODE ret = ExAllocatePoolWithTag(
		NonPagedPool, sizeof(CWK_STR_NODE), MEM_TAG);
	if (ret == NULL)
		return NULL;
	return ret;
}

NTSTATUS CoWorkDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DbgPrint("Enter CoWorkDispatch\n");
	NTSTATUS status = STATUS_SUCCESS;
	//��ȡirp����ջ
	PIO_STACK_LOCATION  irpsp = IoGetCurrentIrpStackLocation(pIrp);
	ULONG ret_len = 0;

	while (pDevObj)
	{
		if (irpsp->MajorFunction == IRP_MJ_CREATE || irpsp->MajorFunction == IRP_MJ_CLOSE)
		{
			// ���ɺ͹ر��������һ�ɼ򵥵ط��سɹ��Ϳ���
			// �ˡ��������ۺ�ʱ�򿪺͹رն����Գɹ���
			break;
		}

		if (irpsp->MajorFunction == IRP_MJ_DEVICE_CONTROL)
		{
			// ����DeviceIoControl��
			PVOID buffer = pIrp->AssociatedIrp.SystemBuffer;//��ȡ������
			ULONG inlen = irpsp->Parameters.DeviceIoControl.InputBufferLength;//���뻺��������
			ULONG outlen = irpsp->Parameters.DeviceIoControl.OutputBufferLength;//�������������
			ULONG len;
			PCWK_STR_NODE str_node;
			switch (irpsp->Parameters.DeviceIoControl.IoControlCode)
			{
			case CWK_DVC_SEND_STR:
				ASSERT(buffer != NULL);
				ASSERT(outlen == 0);

				// ��ȫ�ı��̬��֮һ:������뻺��ĳ��ȶ��ڳ��ȳ���Ԥ�ڵģ���
				// �Ϸ��ش���
				if (inlen > CWK_STR_LEN_MAX)
				{
					status = STATUS_INVALID_PARAMETER;
					break;
				}

				// ��ȫ�ı��̬��֮��������ַ����ĳ��ȣ���Ҫʹ��strlen!���ʹ
				// ��strlen,һ�������߹�������û�н��������ַ������ᵼ���ں���
				// �����ʷǷ��ڴ�ռ��������
				DbgPrint("strnlen = %llu\r\n", strnlen((char*)buffer, inlen));
				if (strnlen((char*)buffer, inlen) == inlen)
				{
					// �ַ���ռ���˻����������м�û�н����������̷��ش���
					status = STATUS_INVALID_PARAMETER;
					break;
				}

				// ���ڿ�����Ϊ���뻺���ǰ�ȫ���Ҳ�������ġ�����ڵ㡣
				str_node = cwkMallocStrNode();
				if (str_node == NULL)
				{
					// ������䲻���ռ��ˣ�������Դ����Ĵ���
					status = STATUS_INSUFFICIENT_RESOURCES;
					break;
				}

				// ǰ���Ѿ�����˻������е��ַ�����ȷ���Ⱥ��ʶ��Һ��н�����
				// ������������ʲô�����������ַ����԰�ȫ�Զ��Բ����ǳ���Ҫ��
				strncpy(str_node->buf, (char*)buffer, CWK_STR_LEN_MAX);
				// ���뵽����ĩβ����������֤��ȫ�ԡ�
				ExInterlockedInsertTailList(&g_cwk_str_list, (PLIST_ENTRY)str_node, &g_cwk_lock);
				// InsertTailList(&g_cwk_str_list, (PLIST_ENTRY)str_node);
				// ��ӡ
				DbgPrint((char*)buffer);
				// ��ô���ھͿ�����Ϊ��������Ѿ��ɹ�����Ϊ�ո��Ѿ�������һ
				// ������ô���������¼��������������Ѿ���Ԫ���ˡ�
				KeSetEvent(&g_cwk_event, 0, TRUE);
				break;
			case CWK_DVC_RECV_STR:
				ASSERT(buffer != NULL);
				ASSERT(inlen == 0);
				// Ӧ��Ҫ������ַ������Դˣ���ȫ��Ҫ�����������Ҫ�㹻����
				if (outlen < CWK_STR_LEN_MAX)
				{
					status = STATUS_INVALID_PARAMETER;
					break;
				}
				while (1)
				{
					// ���뵽����ĩβ����������֤��ȫ�ԡ�
					str_node = (CWK_STR_NODE*)ExInterlockedRemoveHeadList(&g_cwk_str_list, &g_cwk_lock);
					// str_node = RemoveHeadList(&g_cwk_str_list);
					if (str_node != NULL)
					{
						// ��������£�ȡ�����ַ������ǾͿ�������������С�Ȼ��
						// ��������ͷ����˳ɹ���
						strncpy((char*)buffer, str_node->buf, CWK_STR_LEN_MAX);
						ret_len = strnlen(str_node->buf, CWK_STR_LEN_MAX) + 1;
						ExFreePool(str_node);
						break;
					}
					else
					{
						// ���ںϷ���Ҫ���ڻ�������Ϊ�յ�����£��ȴ��¼�����
						// ������Ҳ����˵�������������û���ַ�������ͣ�����ȴ�
						// ������Ӧ�ó���Ҳ�ᱻ����ס��DeviceIoControl�ǲ��᷵��
						// �ġ�����һ���оͻ᷵�ء�����������������֪ͨ��Ӧ�á�
						KeWaitForSingleObject(&g_cwk_event, Executive, KernelMode, 0, 0);
					}
				}
				break;
			default:
				// ������������ǲ����ܵ�����δ֪������һ�ɷ��طǷ���������
				status = STATUS_INVALID_PARAMETER;
				break;
			}
		}
		break;
	}

	//�ַ��з�������
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	DbgPrint("Leave CoWorkDispatch\n");
}

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	DbgPrint("Enter DriverUnload\n");
	//ɾ���豸�ͷ���
	UNICODE_STRING ustrSymName;
	RtlInitUnicodeString(&ustrSymName, SYM_NAME);
	IoDeleteSymbolicLink(&ustrSymName);
	if (pDriverObject->DeviceObject)
	{
		IoDeleteDevice(pDriverObject->DeviceObject);
	}
	// �ͷŷ�����������ں��ڴ档
	PCWK_STR_NODE str_node;
	while (TRUE)
	{
		str_node = ExInterlockedRemoveHeadList(
			&g_cwk_str_list, &g_cwk_lock);
		// str_node = RemoveHeadList(&g_cwk_str_list);
		if (str_node != NULL)
			ExFreePool(str_node);
		else
			break;
	};

	DbgPrint("Leave DriverUnload\n");
}

NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT pDevObj = NULL;
	UNICODE_STRING ustrDevName, ustrSymName;

	RtlInitUnicodeString(&ustrDevName, DEV_NAME);
	RtlInitUnicodeString(&ustrSymName, SYM_NAME);
	status = IoCreateDevice(pDriverObject, 0, &ustrDevName, FILE_DEVICE_UNKNOWN, 0, FALSE, &pDevObj);
	if (!NT_SUCCESS(status))
	{
		return status;
	}
	status = IoCreateSymbolicLink(&ustrSymName, &ustrDevName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
		return status;
	}

	return status;
}

VOID Init()
{
	// ��ʼ���¼�����������ͷ��
	KeInitializeEvent(&g_cwk_event, SynchronizationEvent, TRUE);
	KeInitializeSpinLock(&g_cwk_lock);
	InitializeListHead(&g_cwk_str_list);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING RegistryPath)
{
	DbgPrint("Enter DriverEntry\n");
	NTSTATUS status = STATUS_SUCCESS;

	pDriverObject->DriverUnload = DriverUnload;
	for (ULONG i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		//ע��ַ�����
		pDriverObject->MajorFunction[i] = CoWorkDispatch;
	}

	//��ʼ����������
	Init();
	//�����豸����
	status = CreateDevice(pDriverObject);

	DbgPrint("Leave DriverEntry\n");
	return status;
}