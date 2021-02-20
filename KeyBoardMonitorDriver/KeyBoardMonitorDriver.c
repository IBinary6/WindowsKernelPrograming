#include <ntddk.h>
#include <wdm.h>
#include <ntddkbd.h>
#include <windef.h>

/**
 * ����������
 *
 * ����ctrl2cap
 */


extern POBJECT_TYPE* IoDriverObjectType;//ȫ�ֱ�����ͷ�ļ��в����ڣ���������á�ע����Ҫ����Ϊ*�������Ǵ����

#define KBD_DRIVER_NAME L"\\Driver\\Kbdclass" //������

ULONG gC2pKeyCount = 0;//���̰����¼�����

NTSTATUS
ObReferenceObjectByName(	//δ�������������������ʹ��
	PUNICODE_STRING OjbectName,
	ULONG Attributes,
	PACCESS_STATE AccessState,
	ACCESS_MASK DesiredAccess,
	POBJECT_TYPE ObjectType,
	KPROCESSOR_MODE AccessMode,
	PVOID ParseContext,
	PVOID* Object
);

//ת��ɨ����Ϊʵ�ʼ�λ
unsigned char asciiTbl[] = {
	0x00, 0x1B, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x2D, 0x3D, 0x08, 0x09,	//normal
		0x71, 0x77, 0x65, 0x72, 0x74, 0x79, 0x75, 0x69, 0x6F, 0x70, 0x5B, 0x5D, 0x0D, 0x00, 0x61, 0x73,
		0x64, 0x66, 0x67, 0x68, 0x6A, 0x6B, 0x6C, 0x3B, 0x27, 0x60, 0x00, 0x5C, 0x7A, 0x78, 0x63, 0x76,
		0x62, 0x6E, 0x6D, 0x2C, 0x2E, 0x2F, 0x00, 0x2A, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0x38, 0x39, 0x2D, 0x34, 0x35, 0x36, 0x2B, 0x31,
		0x32, 0x33, 0x30, 0x2E,
		0x00, 0x1B, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x2D, 0x3D, 0x08, 0x09,	//caps
		0x51, 0x57, 0x45, 0x52, 0x54, 0x59, 0x55, 0x49, 0x4F, 0x50, 0x5B, 0x5D, 0x0D, 0x00, 0x41, 0x53,
		0x44, 0x46, 0x47, 0x48, 0x4A, 0x4B, 0x4C, 0x3B, 0x27, 0x60, 0x00, 0x5C, 0x5A, 0x58, 0x43, 0x56,
		0x42, 0x4E, 0x4D, 0x2C, 0x2E, 0x2F, 0x00, 0x2A, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0x38, 0x39, 0x2D, 0x34, 0x35, 0x36, 0x2B, 0x31,
		0x32, 0x33, 0x30, 0x2E,
		0x00, 0x1B, 0x21, 0x40, 0x23, 0x24, 0x25, 0x5E, 0x26, 0x2A, 0x28, 0x29, 0x5F, 0x2B, 0x08, 0x09,	//shift
		0x51, 0x57, 0x45, 0x52, 0x54, 0x59, 0x55, 0x49, 0x4F, 0x50, 0x7B, 0x7D, 0x0D, 0x00, 0x41, 0x53,
		0x44, 0x46, 0x47, 0x48, 0x4A, 0x4B, 0x4C, 0x3A, 0x22, 0x7E, 0x00, 0x7C, 0x5A, 0x58, 0x43, 0x56,
		0x42, 0x4E, 0x4D, 0x3C, 0x3E, 0x3F, 0x00, 0x2A, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0x38, 0x39, 0x2D, 0x34, 0x35, 0x36, 0x2B, 0x31,
		0x32, 0x33, 0x30, 0x2E,
		0x00, 0x1B, 0x21, 0x40, 0x23, 0x24, 0x25, 0x5E, 0x26, 0x2A, 0x28, 0x29, 0x5F, 0x2B, 0x08, 0x09,	//caps + shift
		0x71, 0x77, 0x65, 0x72, 0x74, 0x79, 0x75, 0x69, 0x6F, 0x70, 0x7B, 0x7D, 0x0D, 0x00, 0x61, 0x73,
		0x64, 0x66, 0x67, 0x68, 0x6A, 0x6B, 0x6C, 0x3A, 0x22, 0x7E, 0x00, 0x7C, 0x7A, 0x78, 0x63, 0x76,
		0x62, 0x6E, 0x6D, 0x3C, 0x3E, 0x3F, 0x00, 0x2A, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0x38, 0x39, 0x2D, 0x34, 0x35, 0x36, 0x2B, 0x31,
		0x32, 0x33, 0x30, 0x2E
};
// flags for keyboard status
#define	S_SHIFT				1
#define	S_CAPS				2
#define	S_NUM				4
static int kb_status = S_NUM;
void __stdcall print_keystroke(UCHAR sch)
{
	UCHAR	ch = 0;
	int		off = 0;

	if ((sch & 0x80) == 0)	//make
	{
		if ((sch < 0x47) ||
			((sch >= 0x47 && sch < 0x54) && (kb_status & S_NUM))) // Num Lock
		{
			ch = asciiTbl[off + sch];
		}

		switch (sch)
		{
		case 0x3A:
			kb_status ^= S_CAPS;
			break;

		case 0x2A:
		case 0x36:
			kb_status |= S_SHIFT;
			break;

		case 0x45:
			kb_status ^= S_NUM;
		}
	}
	else		//break
	{
		if (sch == 0xAA || sch == 0xB6)
			kb_status &= ~S_SHIFT;
	}

	if (ch >= 0x20 && ch < 0x7F)
	{
		DbgPrint("%C \n", ch);
	}
}


/**
 * �ڴ��ڹ��������У�ʹ�����������飬һ���������й����豸����һ��������ʵ�豸
 * ������������ӳ�������ã��õ������豸ָ��ʱ�Ϳ����ҵ���ʵ�豸ָ��
 * ��ʵ����û�б�Ҫ�����ɹ����豸ʱ�����Ը��豸ָ�����ⳤ�ȵ��豸��չ
 * ���ԣ����ﶨ��һ���ṹ����Ϊ�豸��չ��ͨ������չ����д���ݣ��ͱ����˸�ָ����Ϣ
 */
typedef struct _C2P_DEV_EXT
{
	// ����ṹ�Ĵ�С
	ULONG NodeSize;
	// �����豸����
	PDEVICE_OBJECT pFilterDeviceObject;
	// ͬʱ����ʱ�ı�����
	KSPIN_LOCK IoRequestsSpinLock;
	// ���̼�ͬ������  
	KEVENT IoInProgressEvent;
	// �󶨵��豸����
	PDEVICE_OBJECT TargetDeviceObject;
	// ��ǰ�ײ��豸����
	PDEVICE_OBJECT LowerDeviceObject;
} C2P_DEV_EXT, * PC2P_DEV_EXT;

NTSTATUS
c2pDevExtInit(
	IN PC2P_DEV_EXT devExt,
	IN PDEVICE_OBJECT pFilterDeviceObject,
	IN PDEVICE_OBJECT pTargetDeviceObject,
	IN PDEVICE_OBJECT pLowerDeviceObject)
{
	//���豸��չ���м�¼������Ϣ
	memset(devExt, 0, sizeof(C2P_DEV_EXT));
	devExt->NodeSize = sizeof(C2P_DEV_EXT);
	devExt->pFilterDeviceObject = pFilterDeviceObject;
	KeInitializeSpinLock(&(devExt->IoRequestsSpinLock));
	KeInitializeEvent(&(devExt->IoInProgressEvent), NotificationEvent, FALSE);
	devExt->TargetDeviceObject = pTargetDeviceObject;
	devExt->LowerDeviceObject = pLowerDeviceObject;
	return(STATUS_SUCCESS);
}

//����������KbdClass��Ȼ������������豸
NTSTATUS
c2pAttachDevices(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
)
{
	NTSTATUS status = 0;

	UNICODE_STRING uniNtNameString;
	PC2P_DEV_EXT devExt;
	PDEVICE_OBJECT pFilterDeviceObject = NULL;
	PDEVICE_OBJECT pTargetDeviceObject = NULL;
	PDEVICE_OBJECT pLowerDeviceObject = NULL;

	PDRIVER_OBJECT KbdDriverObject = NULL;

	RtlInitUnicodeString(&uniNtNameString, KBD_DRIVER_NAME);//��ʼ���������ַ���
	//ͨ���������򿪶���
	status = ObReferenceObjectByName(
		&uniNtNameString,
		OBJ_CASE_INSENSITIVE,
		NULL,
		0,
		*IoDriverObjectType,
		KernelMode,
		NULL,
		&KbdDriverObject
	);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Attach: open driver object by name failed."));
		return status;
	}

	ObDereferenceObject(KbdDriverObject);//����ObReferenceObjectByName��ʹ���������������ӣ���Ҫ������

	//�豸���е�һ���豸
	pTargetDeviceObject = KbdDriverObject->DeviceObject;
	//�����豸��
	while (pTargetDeviceObject)
	{
		//���ɹ����豸
		status = IoCreateDevice(
			DriverObject,
			sizeof(C2P_DEV_EXT), //��д�豸��չ����Ҫ�Ĵ�С
			NULL,
			pTargetDeviceObject->DeviceType,
			pTargetDeviceObject->Characteristics,
			FALSE,
			&pFilterDeviceObject
		);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("Attach: create filter device failed."));
			return status;
		}
		//�󶨹����豸����ʵ�豸������pLowerDeviceObjcet���հ󶨺�õ�����һ����ʵ�豸����
		pLowerDeviceObject = IoAttachDeviceToDeviceStack(pFilterDeviceObject, pTargetDeviceObject);
		//ʧ�������֮ǰ�������˳�
		if (!pLowerDeviceObject)
		{
			KdPrint(("Attach: attach device failed."));
			IoDeleteDevice(pFilterDeviceObject);
			pFilterDeviceObject = NULL;
			return status;
		}

		//�豸��չ																				
		devExt = (PC2P_DEV_EXT)(pFilterDeviceObject->DeviceExtension);						 //����Ϊ�󶨵Ĺ����豸����StackSize��ֵ
		c2pDevExtInit(																		 //msdn�����豸�����StackSize�ֶν����У���ȷָ����
			devExt,																			 //��ʹ��IoAttachDevice��IoAttachDeviceToDeviceStack��һ���豸����ʱ
			pFilterDeviceObject,															 //IO���������Զ��󶨵��豸�������ú��ʵ�StackSizeֵ
			pTargetDeviceObject,															 //ֻ�е�ǰ����ʹ��IoGetDeviceObjectPointer��ȡ���豸����
			pLowerDeviceObject																 //����Ҫ��ʾ���������豸����StackSize+1
		);																					 //���Կ��Բ���Ҫд
																							 //���ο� https://blog.csdn.net/cssxn/article/details/103165667��
		//׼������һ���豸
		pFilterDeviceObject->DeviceType = pLowerDeviceObject->DeviceType;								   //  ��
		pFilterDeviceObject->Characteristics = pLowerDeviceObject->Characteristics;
		//pFilterDeviceObject->StackSize = pLowerDeviceObject->StackSize + 1; //��仰���Բ���Ҫ��������� ������հ�
		pFilterDeviceObject->Flags |= pLowerDeviceObject->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE);
		pTargetDeviceObject = pTargetDeviceObject->NextDevice; //�ƶ�����һ���豸
	}

	return status;
}

//����豸
VOID
c2pDetach(IN PDEVICE_OBJECT pDeviceObject)
{
	PC2P_DEV_EXT devExt;
	BOOLEAN NoRequestsOutstanding = FALSE;
	devExt = (PC2P_DEV_EXT)pDeviceObject->DeviceExtension;
	__try
	{
		__try
		{
			IoDetachDevice(devExt->TargetDeviceObject);
			devExt->TargetDeviceObject = NULL;
			IoDeleteDevice(pDeviceObject);
			devExt->pFilterDeviceObject = NULL;
			DbgPrint(("Detach Finished\n"));
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {}
	}
	__finally {}
	return;
}

/**
 * ���ַ�����
 */
NTSTATUS
c2pDispatchGeneral(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
)
{
	//û�õ��ķַ�������ֱ��skipȻ����IoCallDriver��IRP���͵���ʵ�豸���� 
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(((PC2P_DEV_EXT)DeviceObject->DeviceExtension)->LowerDeviceObject, Irp);//ʹ��Ԥ�ȱ������豸��չ�е�ָ��
}

#define LCONTROL ((USHORT)0x1D) 
#define CAPS_LOCK ((USHORT)0x3A) 
NTSTATUS // ����һ��IRP��ɻص�������ԭ��
c2pReadComplete(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Context
)
{
	PIO_STACK_LOCATION IrpSp;
	ULONG buf_len = 0;
	PUCHAR buf = NULL;
	size_t i, numKeys;

	//�ṹ���л�ȡ������Ϣ
	PKEYBOARD_INPUT_DATA KeyData;

	IrpSp = IoGetCurrentIrpStackLocation(Irp);

	// �������ʧ���ˣ���ô��ȡ��һ������Ϣ��û����ġ�
	if (NT_SUCCESS(Irp->IoStatus.Status))
	{
		// ��ö�������ɺ�����Ļ�����
		buf = Irp->AssociatedIrp.SystemBuffer;
		KeyData = (PKEYBOARD_INPUT_DATA)buf;
		// �������������ĳ��ȡ�һ���˵����ֵ�ж೤��������
		// Information�С�
		buf_len = Irp->IoStatus.Information;
		numKeys = buf_len / sizeof(KEYBOARD_INPUT_DATA);
		for (i = 0; i < numKeys; ++i)
		{
			//DbgPrint("ctrl2cap: %2x\r\n", buf[i]);
			DbgPrint("\n");
			DbgPrint("ScanCode: %x ", KeyData->MakeCode);
			DbgPrint("%s\n", KeyData->Flags ? "Up" : "Down");
			print_keystroke((UCHAR)KeyData->MakeCode);

			if (KeyData->MakeCode == CAPS_LOCK)
			{
				KeyData->MakeCode = LCONTROL;
			}
		}

	}
	//�����������-1
	gC2pKeyCount--;

	if (Irp->PendingReturned)
	{
		IoMarkIrpPending(Irp);
	}
	return Irp->IoStatus.Status;
}

NTSTATUS
c2pRead(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
)
{
	NTSTATUS status = STATUS_SUCCESS;
	PC2P_DEV_EXT devExt;
	PIO_STACK_LOCATION currentIrpStack;
	KEVENT waitEvent;
	KeInitializeEvent(&waitEvent, NotificationEvent, FALSE);

	if (Irp->CurrentLocation == 1)
	{
		ULONG ReturnedInformation = 0;
		KdPrint(("Dispatch encountered bogus current location\n"));
		status = STATUS_INVALID_DEVICE_REQUEST;
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = ReturnedInformation;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return(status);
	}

	// ȫ�ֱ�������������1
	gC2pKeyCount++;

	// �õ��豸��չ��Ŀ����֮��Ϊ�˻����һ���豸��ָ�롣
	devExt = (PC2P_DEV_EXT)DeviceObject->DeviceExtension;

	// ���ûص���������IRP������ȥ�� ֮����Ĵ���Ҳ�ͽ����ˡ�
	// ʣ�µ�������Ҫ�ȴ���������ɡ�
	currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
	IoCopyCurrentIrpStackLocationToNext(Irp);//���Ƶ�ǰջ�ռ�
	IoSetCompletionRoutine(Irp, c2pReadComplete, DeviceObject, TRUE, TRUE, TRUE);//������ɻص�
	return  IoCallDriver(devExt->LowerDeviceObject, Irp);
}

NTSTATUS
c2pPower(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
)
{
	//��Դ���IRP����
	PC2P_DEV_EXT devExt;
	devExt = (PC2P_DEV_EXT)DeviceObject->DeviceExtension;
	//����ͨ�ַ���skip�������ƣ����ȵ���PoStartNextPowerIrp��֮��ʹ��PoCallDriver����IoCallDriver
	PoStartNextPowerIrp(Irp);
	IoSkipCurrentIrpStackLocation(Irp);
	return PoCallDriver(devExt->LowerDeviceObject, Irp);
}

NTSTATUS
c2pPnP(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
)
{
	//�豸���ʱ�ķַ����⴦��
	PC2P_DEV_EXT devExt;
	PIO_STACK_LOCATION irpStack;
	NTSTATUS status = STATUS_SUCCESS;
	KIRQL oldIrql;
	KEVENT event;

	// �����ʵ�豸��
	devExt = (PC2P_DEV_EXT)(DeviceObject->DeviceExtension);
	irpStack = IoGetCurrentIrpStackLocation(Irp);

	switch (irpStack->MinorFunction)
	{
	case IRP_MN_REMOVE_DEVICE:
		KdPrint(("IRP_MN_REMOVE_DEVICE\n"));

		// ���Ȱ�������ȥ
		IoSkipCurrentIrpStackLocation(Irp);
		IoCallDriver(devExt->LowerDeviceObject, Irp);
		// Ȼ�����󶨡�
		IoDetachDevice(devExt->LowerDeviceObject);
		// ɾ�������Լ����ɵ������豸��
		IoDeleteDevice(DeviceObject);
		status = STATUS_SUCCESS;
		break;

	default:
		// �����������͵�IRP��ȫ����ֱ���·����ɡ� 
		IoSkipCurrentIrpStackLocation(Irp);
		status = IoCallDriver(devExt->LowerDeviceObject, Irp);
	}
	//û��Ҫ���Ļ���δ���IRP��WindowsϵͳҪ��ж���豸ʱϵͳ����Ӧ���Ѿ������������δ��IRP
	return status;
}

/**
 * �������Ǵ�����һ��������û��ɵ�״̬
 * �������ƴ�������һ���ȴ�5�룬Ҳ����һ����������������Ϊ��δ����)
 * ��ֹδ������û��ɵķ�������ʹ��gC2pKeyCount����һ��ȫ�ּ�������
 * ��һ����������ʱ������+1�����ʱ-1��ֻ������������ɣ��Ż�����ȴ�
 * ���ս���ǣ�ֻ��һ�����������£�ж�ع��̲Ž���
 */
#define  DELAY_ONE_MICROSECOND  (-10) //����ʱ��Ϊ���� https://blog.csdn.net/lqk1985/article/details/2541867
#define  DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)
#define  DELAY_ONE_SECOND (DELAY_ONE_MILLISECOND*1000)
VOID
c2pUnload(PDRIVER_OBJECT DriverObject)
{
	PDEVICE_OBJECT DeviceObject;
	PDEVICE_OBJECT OldDeviceObject;
	PC2P_DEV_EXT devExt;

	LARGE_INTEGER lDelay;
	PRKTHREAD CurrentThread;
	lDelay = RtlConvertLongToLargeInteger(100 * DELAY_ONE_MILLISECOND);
	CurrentThread = KeGetCurrentThread();
	//��ǰ�߳�����Ϊ��ʵʱģʽ�����ٶ����������Ӱ��
	KeSetPriorityThread(CurrentThread, LOW_REALTIME_PRIORITY);

	UNREFERENCED_PARAMETER(DriverObject);
	KdPrint(("Driver Unloading..."));

	//���������豸�������
	DeviceObject = DriverObject->DeviceObject;
	while (DeviceObject)
	{
		c2pDetach(DeviceObject);
		DeviceObject = DeviceObject->NextDevice;
	}
	ASSERT(NULL == DriverObject->DeviceObject);

	while (gC2pKeyCount)
	{
		//�ں���sleep
		KeDelayExecutionThread(KernelMode, FALSE, &lDelay);
	}
	KdPrint(("Driver Unload OK!"));
	return;
}

PDRIVER_OBJECT gDriverObject = NULL;


/**
 * HOOK�ַ�����
 *
 * ���������ٻ�ͨ����������ֱ�Ӱ������豸�ķ�ʽ����Ϊ�����ױ�����
 * ������޸���������ķַ�����ָ�����������󣬽ػ���ٵ��þɵĺ�����ʹ����������������
 */
NTSTATUS
MyDispatch(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
)
{
	//��һЩ�ػ����
}
NTSTATUS
HookKbdclassDispatch(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
)
{
	NTSTATUS status = STATUS_SUCCESS;

	UNICODE_STRING uniNtNameString;
	PDRIVER_OBJECT KbdDriverObject = NULL;

	RtlInitUnicodeString(&uniNtNameString, KBD_DRIVER_NAME);//��ʼ���������ַ���
	//ͨ���������򿪶���
	status = ObReferenceObjectByName(
		&uniNtNameString,
		OBJ_CASE_INSENSITIVE,
		NULL,
		0,
		*IoDriverObjectType,
		KernelMode,
		NULL,
		&KbdDriverObject
	);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Attach: open driver object by name failed."));
		return status;
	}

	ObDereferenceObject(KbdDriverObject);//����ObReferenceObjectByName��ʹ���������������ӣ���Ҫ������

	//����ԭ�зַ�����ָ��
	ULONG i;
	PDRIVER_DISPATCH OldDispatchFunctions[IRP_MJ_MAXIMUM_FUNCTION + 1];

	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		OldDispatchFunctions[i] = KbdDriverObject->MajorFunction[i];
		//����ԭ�ӽ����������滻Ϊ�Լ��ķַ�����
		InterlockedExchangePointer(&KbdDriverObject->MajorFunction[i], MyDispatch);
	}

	return status;
}

/**
 * �������µĶ˿�����
 *
 * �滻�ַ�����ָ��ķ�����Ȼ������
 * ����һ���ֶΣ���ֱ��Ѱ��һ�����ڶ˿������ж�ȡ���뻺�����ĺ�����ʵ�������������ṩ��
 * ͨ��Hook�ú���ʵ�ֹ���
 *
 * KbdClass����Ϊ����������
 * ������ͨ��ָͳ��һ���豸�������������������º�ʵ��Ӳ����������������Ϊ�˿�����
 *
 * ��������һ����������ʱ������һ��MakeCode�����������жϣ��ɿ�ʱ������һ��BreakCode�������ж�
 * �����жϵ��¼����жϷ������̱�ִ�У����յ���i8042prt��I8042KeyboardInterruptService��ִ��
 * �Ӷ˿ڶ���������ɨ���룬����KEYBOARD_INPUT_DATA�У����˽ṹ����i8042prt���������ݶ����У�һ���жϷ�һ������
 * ���յ���KeInsertQueueDpc�����и��ദ����ӳٹ��̵���
 *
 * ����ҵ���I8042KeyboardInterruptService�е��õ��������Ļص��������Ϳ��Ի�ȡ����������
 * �ؼ����ڶ�λ����ָ�룺
 *		����ָ��Ӧ�ñ�����i8042prt���ɵ��豸���Զ����豸��չ��
 *		������ʼ��ַӦ�����ں�ģ��KbdClass��
 *		�ں�ģ��KbdClass���ɵ�һ���豸����ָ��Ҳ�������豸��չ�У�������Ҫ�ҵĺ���ָ��֮ǰ
 */
 //ps2�Ķ˿�����
#define PS2_DRIVER_NAME  L"\\Driver\\i8042prt"
 //usb�Ķ˿�����
#define USB_DRIVER_NAME  L"\\Driver\\Kbdhid"
//Ҫ�����Ļص��������Ͷ���
typedef VOID(_stdcall* KEYBOARDCLASSSERVIECALLBACK)(
	IN PDEVICE_OBJECT DeviceObject,
	IN PKEYBOARD_INPUT_DATA InputDataStart,
	IN PKEYBOARD_INPUT_DATA InputDataEnd,
	IN OUT PULONG InputDataConsumed
	);
//����һ���ṹ���Լ�ȫ�ֱ��������������Ļص�����
typedef struct _KBD_CALLBACK
{
	PDEVICE_OBJECT classDeviceObject;
	KEYBOARDCLASSSERVIECALLBACK serviceCallBack;
}KBD_CALLBACK, * PKBD_CALLBACK;
KBD_CALLBACK gKbdCallBack = { 0 };
//�滻�ĺ���
VOID
MyCallBackFunction(
	PDEVICE_OBJECT DeviceObject,  //��������
	PKEYBOARD_INPUT_DATA InputDataStart,  //
	PKEYBOARD_INPUT_DATA InputDataEnd,
	PULONG InputDataConsumed
)
{
	DbgPrint("keyup %d", InputDataStart->MakeCode);
	DbgPrint("keydown %d", InputDataEnd->MakeCode);
}
//�����ص�����
NTSTATUS
SearchServiceCallBack(IN PDRIVER_OBJECT DriverObject)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	int i = 0;
	UNICODE_STRING uniNtNameString;
	PDEVICE_OBJECT pTargetDeviceObject = NULL;
	PDRIVER_OBJECT KbdDriverObject = NULL;
	PDRIVER_OBJECT KbdhidDriverObject = NULL;
	PDRIVER_OBJECT Kbd8042DriverObject = NULL;
	PDRIVER_OBJECT UsingDriverObject = NULL;
	PDEVICE_OBJECT UsingDeviceObject = NULL;
	PVOID KbdDriverStart = NULL;
	ULONG KbdDriverSize = 0;
	PVOID UsingDeviceExt = NULL;
	PVOID* AddrServiceCallBack = 0;

	//��USB���ͼ���
	RtlInitUnicodeString(&uniNtNameString, USB_DRIVER_NAME);
	status = ObReferenceObjectByName(
		&uniNtNameString,
		OBJ_CASE_INSENSITIVE,
		NULL,
		0,
		IoDriverObjectType,
		KernelMode,
		NULL,
		&KbdhidDriverObject
	);
	if (!NT_SUCCESS(status))
	{
		DbgPrint(("Search: open usb keyboard failed."));
	}
	else
	{
		ObDereferenceObject(KbdhidDriverObject);
	}
	//��PS/2���ͼ���
	RtlInitUnicodeString(&uniNtNameString, PS2_DRIVER_NAME);
	status = ObReferenceObjectByName(
		&uniNtNameString,
		OBJ_CASE_INSENSITIVE,
		NULL,
		0,
		IoDriverObjectType,
		KernelMode,
		NULL,
		&Kbd8042DriverObject
	);
	if (!NT_SUCCESS(status))
	{
		DbgPrint(("Search: open ps/2 keyboard failed."));
	}
	else
	{
		ObDereferenceObject(Kbd8042DriverObject);
	}
	//ͬʱ���ڻ��߶�û�ҵ�������ʧ��
	if (Kbd8042DriverObject && KbdhidDriverObject)
	{
		return STATUS_UNSUCCESSFUL;
	}
	if (!KbdhidDriverObject && !Kbd8042DriverObject)
	{
		return STATUS_UNSUCCESSFUL;
	}
	//ʹ�ô��ڵĶ���
	UsingDriverObject = Kbd8042DriverObject ? Kbd8042DriverObject : KbdhidDriverObject;
	//�ҵ������������µ�һ���豸����
	UsingDeviceObject = UsingDriverObject->DeviceObject;
	//��ȡ���豸������豸��չ�����ˣ�����չ��Ӧ�ð�����Ҫ�ҵĺ���ָ��
	UsingDeviceExt = UsingDeviceObject->DeviceExtension;
	//��õ�ַ�����ڴ���չ�б����һ����������KbdClass�еĵ�ַ

	//������KbdClass
	RtlInitUnicodeString(&uniNtNameString, KBD_DRIVER_NAME);
	status = ObReferenceObjectByName(
		&uniNtNameString,
		OBJ_CASE_INSENSITIVE,
		NULL,
		0,
		IoDriverObjectType,
		KernelMode,
		NULL,
		&KbdDriverObject
	);
	if (!NT_SUCCESS(status))
	{
		DbgPrint(("Search: open kbd driver failed."));
		return STATUS_UNSUCCESSFUL;
	}
	else
	{
		ObDereferenceObject(KbdDriverObject);
		//��ÿ�ʼ��ַ�ʹ�С
		KbdDriverStart = KbdDriverObject->DriverStart;
		KbdDriverSize = KbdDriverObject->DriverSize;
	}
	//����kbdclass�������豸������Щ�豸�У���һ���ᱣ���ڶ˿��������豸��չ��
	pTargetDeviceObject = KbdDriverObject->DeviceObject;
	PBYTE DeviceExt;
	while (pTargetDeviceObject)
	{
		DeviceExt = (PBYTE)UsingDeviceExt;
		//�����豸��չ�µ�ÿһ��ָ��
		for (; i < 4096; i++, DeviceExt += sizeof(PBYTE))
		{
			PVOID tmp;
			if (!MmIsAddressValid(DeviceExt)) //ntddk.h����ǰ�������������Ҳ���
			{
				break;
			}
			//�ҵ������д��ȫ�ֱ����У������������Ƿ����ҵ����ҵ�ֱ������
			if (gKbdCallBack.classDeviceObject && gKbdCallBack.serviceCallBack)
			{
				status = STATUS_SUCCESS;
				break;
			}
			//�˿��������豸��չ�У��ҵ������������豸���󣬼�¼���������豸����
			tmp = *(PVOID*)DeviceExt;
			if (tmp == pTargetDeviceObject)
			{
				gKbdCallBack.classDeviceObject = (PDEVICE_OBJECT)tmp;
				DbgPrint(("classDeviceObject %8x\n", tmp));
				continue;
			}
			//����豸��չ���ҵ�һ����ַλ��KbdClass�����У�����������Ϊ��������Ҫ�ҵĻص�������ַ
			if (
				(tmp > KbdDriverStart) &&
				(tmp < (PBYTE)KbdDriverStart + KbdDriverSize) &&
				MmIsAddressValid(tmp)
				)
			{
				//��¼����
				gKbdCallBack.serviceCallBack = (KEYBOARDCLASSSERVIECALLBACK)tmp;
				AddrServiceCallBack = (PVOID*)DeviceExt;
				DbgPrint(("serviceCallBack:%8x AddrServiceCallBack: %8x\n", tmp, AddrServiceCallBack));
			}

		}
		pTargetDeviceObject = pTargetDeviceObject->NextDevice;//����������һ���豸
	}

	//�ɹ��ҵ����滻Ϊ�Լ��Ļص����������˳ɹ��ػ�
	if (AddrServiceCallBack && gKbdCallBack.serviceCallBack)
	{
		DbgPrint(("Hook KeyboardClassServiceCallback\n"));
		*AddrServiceCallBack = MyCallBackFunction;
	}

	return status;
}

/**
 * ͨ�������жϷ�����
 *
 * �����˿��������ײ���
 * ���PS/2���̣�32λϵͳ�£�
 * ���ڷ����������뽹���Ƶ������ʱ��ע��һ���жϷ���ӹܼ����жϣ�����0x93
 * ֮�󰴼��Ͳ�ͨ������������������ȫ������жϷ���������ɨ���벢���������
 *
 * ֮ǰ֪��_asm int 3��һ���ж�ָ�int n������nΪ�жϺţ����Դ�������жϣ�����ж��ֽ��쳣��
 * �����ı�����ʹCPU��ִͣ�У�����ת���жϴ������У��жϴ������׵�ַ�����ڽ���IDT�ı���
 *
 * ������Ӳ���ж�һ���ΪIRQ��ĳ��IRQ����ʲôӲ��ͨ������ȷ�涨������IRQ1����PS/2����
 * һ��IRQһ����Ҫһ���жϴ���������������Ҫһ��IRQ�ŵ��жϺŵĶ�Ӧ��ϵ
 * ��IOAIPC���ֺ󣬶�Ӧ��ϵ��ÿ����޸ģ���Windows�ϣ�PS/2�����¼�����һ�㶼��int 0x93
 * ���ԣ��޸�int 0x93��IDT�б���ĺ�����ַ���޸�Ϊ�Լ��ĺ�������ô���ܹ��Ƚػ�����ж�
 *
 * �޸�IDT
 * ���ں˳����У������޸�IDT��IDT�ڴ��ַ������������ͨ��ָ��sidt��ȡ
 * ��Ҫע����ǣ��ڶ��CPU�ϣ�ÿ�����Ķ����Լ���IDT��������Ҫ��ÿ���˻�ȡIDT����֤���븲��ÿ����
 *
 */
 //������ȷһ�����ж���λ������Ԥ�ȶ��弸����ȷ֪�����ȵı���
typedef unsigned char P2C_U8;
typedef unsigned short P2C_U16;
typedef unsigned long P2C_U32;
//ͨ��sidtָ���õ����½ṹ�壬ע��1�ֽڶ���
#pragma pack(push,1)
typedef struct P2C_IDTR_
{
	P2C_U16 limit;//��Χ
	P2C_U32 base;//����ַ
}P2C_IDTR, * PP2C_IDTR;
#pragma  pack(pop)
//��ָ������ṹ�壬������IDT��ַ
void* p2cGetIdt()
{
	P2C_IDTR idtr;
	_asm sidt idtr //���ָ���ȡIDTλ��
	return (void*)idtr.base;
}
//��ȡIDT��ַ�󣬸��ڴ�ռ���һ�����飬ÿ��Ԫ��Ϊ���½ṹ
#pragma pack(push,1)
typedef struct P2C_IDT_ENTRY_
{
	P2C_U16 offset_low;
	P2C_U16 selector;
	P2C_U8 reserved;
	P2C_U8 type : 4;			   //��ð�ŵ���Ϊλ��
	P2C_U8 always0 : 1;			   //��ʾ��Ա�������1�ֽڣ�ð�ź����ֱ�ʾ����λ��
	P2C_U8 dpl : 2;				   //�����ĸ���Ա��ռ4/1/2/1λ
	P2C_U8 present : 1;			   //ʵ��ռ�ռ�λ1�ֽ�
	P2C_U16 offset_high;
}P2C_IDT_ENTRY, * PP2C_IDT_ENTRY;
#pragma  pack(pop)
//дһ�������滻�жϷ����ַ������ִ�к���ת��ԭ���жϺ���
void* g_p2c_old;//��¼ԭ�жϷ�����
__declspec(naked)/*����һ���㺯����MS��C���������������ɺ������ָ��*/ p2cInterruptProc()
{
	__asm
	{
		pushad					//�������мĴ���״̬
		pushfd					//
		call p2cUserFilter		//�����Լ��ĺ���

		popfd					//�ָ����мĴ���
		popad					//
		jmp g_p2c_old			//��ת��ԭ�����жϷ������
	}
}
//�괦������ȡ���ݸߵ��ֽڲ��֣����ߴӸߵ��ֽڲ����������
#define P2C_MAKELONG(low,high) \
((P2C_U32)(((P2C_U16)((P2C_U32)(low) & 0xffff)) | \
((P2C_U32)((P2C_U16)((P2C_U32)(high) & 0xffff))) << 16))
#define P2C_LOW16_OF_32(data) \
((P2C_U16)(((P2C_U32)data) & 0xffff))
#define P2C_HIGH16_OF_32(data) \
((P2C_U16)(((P2C_U32)data) >> 16))
//�޸�IDT���е�0x93��޸�Ϊp2cInterruptProc
void p2cHookInt93(BOOLEAN hook_or_unhook)
{
	PP2C_IDT_ENTRY idt_addr = (PP2C_IDT_ENTRY)p2cGetIdt();
	idt_addr += 0x93;
	if (hook_or_unhook)
	{
		//��¼ԭ�жϺ�����ַ
		g_p2c_old = (void*)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high);
		//�滻Ϊp2cInterruptProc
		idt_addr->offset_low = P2C_LOW16_OF_32(p2cInterruptProc);
		idt_addr->offset_high = P2C_HIGH16_OF_32(p2cInterruptProc);
	}
	else
	{
		//ȡ��hook
		idt_addr->offset_low = P2C_LOW16_OF_32(g_p2c_old);
		idt_addr->offset_high = P2C_HIGH16_OF_32(g_p2c_old);
	}
}

/**
 * ���淽�����õ���0x93�жϺ�
 * ʵ������������жϺŶ�Ӧ��ϵ���Ա����ã�Ӧʹ��HalGetInterruptVector����ȡ����жϺţ�Ȼ�������滻
 * ����������QQ��PS/2�����˴�ʩ��
 *
 * �����Ը��ӵײ㣬ֱ���ö˿ڲ�������
 * PS/2�������ݶ˿�Ϊ0x60��ֱ�Ӷ�ȡ�˿ھ��ܶ������ݣ�ǰ���Ǽ��̱��봦�ڿɶ�״̬)
 * ������û�жԶ˿ڶ�ȡ�������ƣ�ֱ���û��Ϳ��Զ�ȡ��һ�ζ�ȡһ�ֽ�
 */
#define OBUFFER_FULL 0x02
#define IBUFFER_FULL 0x01
 //�ȴ������̿ɶ�
ULONG p2cWaitForKbRead()
{
	int i = 100;
	P2C_U8 mychar;//һ���ֽڵȴ���ȡ
	do
	{
		_asm in al, 0x64	//��ȡ��������˿�
		_asm mov mychar, al
		KeStallExecutionProcessor(50);
		if (!(mychar & OBUFFER_FULL)) break; //��֤�Ƿ��ж�Ӧ��־
	} while (i--);
	if (i)	return TRUE;
	return FALSE;
}
//�ȴ������̿�д
ULONG p2cWaitForKbWrite()
{
	int i = 100;
	P2C_U8 mychar;//һ���ֽڵȴ���ȡ
	do
	{
		_asm in al, 0x64
		_asm mov mychar, al
		KeStallExecutionProcessor(50);
		if (!(mychar & IBUFFER_FULL)) break;
	} while (i--);
	if (i)	return TRUE;
	return FALSE;
}
//�Ӷ˿ڲ�������
void p2cUserFilter()
{
	static P2C_U8 sch_pre = 0;
	P2C_U8 sch;
	//�ɶ�ʱ��ȡ�˿��е�ɨ����
	p2cWaitForKbRead();
	_asm in al, 0x60 //��ȡ���̶˿�
	_asm mov sch, al
	//�Ѷ���������д�ض˿ڣ��Ա���������������ȡ��������뱻��ȡ���������дһ�����ݣ�
	if (sch_pre != sch)
	{
		sch_pre = sch;
		_asm mov al, 0xd2
		_asm out 0x64, al
		p2cWaitForKbWrite();
		_asm mov al, sch
		_asm out 0x60, al
	}
	//ɨ��������󣬼������ݶ˿��о�û�����ֵ�ˣ� ���þɵ��жϴ���Ҳ��û�����壬������д���˶˿���
	//��д�ؼ���ʱͬ���Ǵ����жϣ��ٴν����жϴ�������������ѭ��
	//�����������ʹ��һ����̬����sch_pre��¼ǰһ�ε�ֵ����������ֵ��������
}


NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;

	//ָ����ͨ�ַ�����
	for (ULONG i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		pDriverObject->MajorFunction[i] = c2pDispatchGeneral;
	}

	//ָ����Ҫʹ�õ�����ַ�����
	pDriverObject->MajorFunction[IRP_MJ_READ] = c2pRead;//��ȡ������Ϣ
	pDriverObject->MajorFunction[IRP_MJ_POWER] = c2pPower;//
	pDriverObject->MajorFunction[IRP_MJ_PNP] = c2pPnP;//���弴�÷ַ������������豸���ʱ�����⴦��

	//ж�غ���
	pDriverObject->DriverUnload = c2pUnload;
	gDriverObject = pDriverObject;//��¼Ϊȫ�ֱ���

	//�������豸
	status = c2pAttachDevices(pDriverObject, pRegistryPath);

	return status;
}