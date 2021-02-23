#include <ntifs.h>
#include <ntddk.h>

/**
 * ��������
 *
 * ��ȫ�������һ����Ҫ���ξ��Ǳ����������ȷ�Ժ�������
 * ���ں˶���ı�������Ҫ�ǽ��̱������̺߳ͻ�����������
 *
 * �ں˶������ں�̬��һ���ڴ棬��ϵͳ���估ά���������˶���������Ϣ
 * ������̶��������ں˶��������˺ͽ�����ص���Ϣ����ִ���ļ���������ҳ��ָ�롢���̷���������ڴ��
 * �ں˶����Ϊ�����������ں˶���һ�����̷��ʵĶ����������������
 * �����������͵ģ����绥����������¼�����
 *
 * �ں˶������ڴ��ϵ����ݽṹû�о��幫����������ͨ��WinDbg���.pdb���ű����鿴
 * ������Ҫ����32λ��win7ϵͳ���ں˶���ṹ
 *
 * �ں˶���ṹ��Ҫ����������ɣ�����ͷ��������
 * WinDbg�Ͽ���ʹ��dt������ʾ�ṹ����
 *
 * Ҫ����һ���ں˶�������Ҫ�򿪣�������򵥵ı������Ƿ�ֹ�˶��󱻶�������
 * ���Թҹ����ں˶������غ������ڴ��������жϵ�ǰ��Ҫ�򿪵��ں˶������Ƿ�����Ҫ�����Ķ���
 * ����ǣ����жϽ����Ƿ������Σ�����Ƕ����������ֹ�������
 * ���ڲ�ͬ���ں˶���Ҫ�ҹ���ͬ�����������¼�����ʹ��NtOpenEvent�������ڴ�ӳ�����ʹ��NtOpenSection
 */

#define PROTECT_NAME L"Global\\ProtectEvent" //��Ҫ�������ں˶�����

typedef NTSTATUS(_stdcall* PHOOK_NtOpenEvent) (PHANDLE EventHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes);

PHOOK_NtOpenEvent g_pOrgin_NtOpenEvent = NULL;//hookʱ��¼ԭ������ַ

UNICODE_STRING g_strProtectEventName = { //�����Ķ�����
	sizeof(PROTECT_NAME) - 2,
	sizeof(PROTECT_NAME),
	PROTECT_NAME
};

BOOLEAN IsSafeProcess(HANDLE processId)
{
	//�Զ����ж��Ƿ�Ϊ��ȫ���̣�һЩϵͳ����Ҫ������ȥ
	return TRUE;
}

//���Ӵ�����
NTSTATUS _stdcall HOOK_NtOpenEvent(
	PHANDLE EventHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes
)
{
	NTSTATUS status = STATUS_SUCCESS;
	HANDLE hCurrentProcessId = 0;
	do
	{
		if (ExGetPreviousMode() != UserMode)
		{
			break;//ֻ����UserMode
		}
		if (ObjectAttributes == NULL ||
			ObjectAttributes->ObjectName == NULL ||
			ObjectAttributes->ObjectName->Buffer == NULL ||
			ObjectAttributes->ObjectName->Length == 0)
		{
			break;//��������
		}
		if (RtlCompareUnicodeString(ObjectAttributes->ObjectName, &g_strProtectEventName, TRUE) != 0)
		{
			break;//���ֲ�ƥ������
		}
		hCurrentProcessId = PsGetCurrentProcessId();
		//��鵱ǰ�����Ƿ��ǰ�ȫ���̣������Զ��壩
		if (IsSafeProcess(hCurrentProcessId) == FALSE)
		{
			status = STATUS_ACCESS_DENIED;//���ǰ�ȫ���̣��ܾ�����
		}
		break;
	} while (FALSE);
	if (status != STATUS_ACCESS_DENIED)
	{
		//�ύ��ԭ����
		status = g_pOrgin_NtOpenEvent(EventHandle, DesiredAccess, ObjectAttributes);
	}
	return status;
}

/**
 * ��ֹ�ܱ������󱻷Ƿ��򿪽����ǵ�һ��
 *
 * ���������������Ի���ں˶�����
 * Windows�ṩ��һ�־�����Ʋ��������԰�һ�����̾�����еľ�����Ƶ���һ�����̵ľ����
 * ����ΪDuplicateHandle��Ҫ��ǰ���̱���ӵ��Դ���̺�Ŀ����̵�PROCESS_DUP_HANDLEȨ��
 * ���ԣ���ͨ���򿪲�����Ҳ�ܻ�ȡ������
 *
 * ���˼·�У�
 * �ҹ��ں˶���Ĳ��������������¼����󣬹ҹ�NtSetEvent�Ⱥ������ж��Ƿ��ǰ�ȫ�����ڲ����ں˶���
 * �ҹ�������ƺ�����NtDuplicateObject�����ж��Ƿ��Ǹ��Ƶ���ȫ�����У��������������
 * ���ڸ��ƾ����ҪȨ�ޣ����Թҹ�NtOpenProcess��������ֹ�ǰ�ȫ���̻�ȡ��Ȩ��
 */

NTSTATUS ProtectCopyOp()
{
	return STATUS_SUCCESS;
}

/**
 * ���˴򿪺͸��Ʋ�����������ͨ���̳�����ȡ���
 *
 * ��Vistaϵͳ��ʼ�����������б�AttributeList���������ڴ����ӽ��̹�����ָ��ĳ�����̼̳о��
 * ����������ͨ�������б�ʽ���ڴ����ӽ���ʱ��ָ���Ӱ�ȫ�����м̳о������������������ڴ����ӽ��̹����У�û�й�����������������ļ̳�
 * ��Ȼû�йҹ�Ŀ�꣬���Ǽ̳й����У�Ҳ�漰�˽��̾��Ȩ�ޣ�����ͨ������PROCESS_DUP_HANDLEȨ�޽��
 * ����һ���취�ǣ�
 * ͨ��֮ǰ�˽���ں˶���ṹ������ͷ���г�Ա��ʾ��������TypeIndex��ͬʱ��Ҳ��ObTypeIndexTable������±�
 * ������ÿ��Ԫ��Ҳ��Ҳ���ں˶���ϵͳ��ʹ���ں˶�������ʾһ���������͵�
 * �ڶ���ṹ����һ��TypeInfo��������_OBJECT_TYPE_INITIALIZER�ṹ�壬��һ����Ա��OpenProcedure
 * �˳�Ա��һ������ָ�룬��ʾ��һ���ں˶��󱻡���ȡ������ǣ�ϵͳ�ͻ��������ں˶�����Ӧ��Type���󡱵�OpenProcedure����
 * ���ң�����Ļ�ȡָ�����������з�ʽ��ȡʱ���ᷢ�������ԣ�hook�ú��������Խ���󲿷�����
 * ��������δ�������������Բ�ͬ�����¿��ܲ�ͬ
 *
 */

#define PROTECT_NAME_2 L"\\Global\\ProtectEvent" //��Ҫ�������ں˶�����
#define PROTECT_NAME_2_BASE L"����BaseNamedObjectsProtectEvent" //NT��ʽ����Ҫ�������ں˶�����
#define OPENPROCEDURE_OFFSET (0x5c)	//OpenProcedureƫ��

UNICODE_STRING g_strProtectEventName_2 = { //�����Ķ�����
	sizeof(PROTECT_NAME_2) - 2,
	sizeof(PROTECT_NAME_2),
	PROTECT_NAME_2
};
UNICODE_STRING g_strProtectEventName_2_Base = { //�����Ķ�����
	sizeof(PROTECT_NAME_2_BASE) - 2,
	sizeof(PROTECT_NAME_2_BASE),
	PROTECT_NAME_2_BASE
};

typedef enum _OB_OPEN_REASON
{
	//�Ը��ַ�ʽ��ȡ�����
	ObCreateHandle,
	ObOpenHandle,
	ObDuplicateHandle,
	ObInheritHandle,
	ObMaxOpenReason
}OB_OPEN_REASON;

//�������������Լ�ȫ�ֱ�����¼ԭ����
typedef NTSTATUS(_stdcall* PHOOK_OpenProcedure)(
	IN OB_OPEN_REASON OpenReason,
	IN KPROCESSOR_MODE AccessMode,
	IN PEPROCESS Proccess OPTIONAL,
	IN PVOID Object,
	IN PACCESS_MASK GrandedAccess,
	IN ULONG HandleCount
	);
PHOOK_OpenProcedure g_pOrigin_OpenProcedure = NULL;

BOOLEAN ObjectHookOpenEvent(PVOID pHookFunc, PVOID* pOldFunc)
{
	BOOLEAN bSucc = FALSE;
	do
	{
		PVOID pEventTypeObj = NULL;
		PVOID* pHookAddress = NULL;
		if (ExEventObjectType == NULL)
		{
			break;
		}
		pEventTypeObj = (PVOID)*ExEventObjectType;
		//��Object_Type��ͨ��Ӳ���붨λ��OpenProcedure��ַ
		pHookAddress = (PVOID*)((UCHAR*)pEventTypeObj + OPENPROCEDURE_OFFSET);
		if (pHookAddress == NULL)
		{
			break;
		}
		//����ԭ����
		if (pOldFunc != NULL)
		{
			*pOldFunc = *pHookAddress;
		}
		//Hook����
		InterlockedExchangePointer(pHookAddress, pHookFunc);
		bSucc = TRUE;
	} while (FALSE);
	return bSucc;
}

NTSTATUS _stdcall HOOK_OpenProcedure_Event(
	IN OB_OPEN_REASON OpenReason,
	IN KPROCESSOR_MODE AccessMode,
	IN PEPROCESS Proccess OPTIONAL,
	IN PVOID Object,
	IN PACCESS_MASK GrandedAccess,
	IN ULONG HandleCount
)
{
	NTSTATUS status = STATUS_SUCCESS;
	POBJECT_NAME_INFORMATION pObjNameInfo = NULL;
	do
	{
		ULONG uReturnLen = 0;
		NTSTATUS nRet = STATUS_UNSUCCESSFUL;
		HANDLE hCurrentProcessId = NULL;
		//������
		if (OpenReason != ObInheritHandle && AccessMode != UserMode)
		{
			break;//���ڼ̳в�����������ģʽ����������ֻ����UserMode
		}
		if (GrandedAccess == NULL)
		{
			break;
		}
		if (Object == NULL)
		{
			break;
		}
		ObQueryNameString(Object, NULL, 0, &uReturnLen);
		if (uReturnLen == 0)
		{
			break;
		}
		pObjNameInfo = (POBJECT_NAME_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, uReturnLen, '1');
		if (pObjNameInfo == NULL)
		{
			break;
		}
		memset(pObjNameInfo, 0, uReturnLen);
		nRet = ObQueryNameString(Object, pObjNameInfo, uReturnLen, &uReturnLen);
		if (!NT_SUCCESS(nRet))
		{
			break;
		}
		if (pObjNameInfo->Name.Buffer == NULL || pObjNameInfo->Name.Length == 0)
		{
			break;
		}
		if (RtlCompareUnicodeString(&pObjNameInfo->Name, &g_strProtectEventName_2, TRUE) &&
			RtlCompareUnicodeString(&pObjNameInfo->Name, &g_strProtectEventName_2_Base, TRUE))
		{
			break;
		}
		hCurrentProcessId = PsGetCurrentProcessId();
		if (IsSafeProcess(hCurrentProcessId) == TRUE)
		{
			break;
		}
		status = STATUS_ACCESS_DENIED; //�ߵ������ʾ�Ƿǰ�ȫ�����ڻ�ȡ���
		break;
	} while (FALSE);
	if (pObjNameInfo != NULL)
	{
		ExFreePoolWithTag(pObjNameInfo, '1');
		pObjNameInfo = NULL;
	}
	if (status != STATUS_ACCESS_DENIED && g_pOrigin_OpenProcedure != NULL)
	{
		//�ύ��ԭ����
		status = g_pOrigin_OpenProcedure(OpenReason, AccessMode, Proccess, Object, GrandedAccess, HandleCount);
	}
	return status;
}

VOID HOOK_GET_HANDLE()
{
	ObjectHookOpenEvent((PVOID)HOOK_OpenProcedure_Event, (PVOID*)&g_pOrigin_OpenProcedure);
}

/**
 * �����о�����δ��ں˶���Ƕȱ�������
 *
 * �ڰ�ȫ����У����̺��ں�ģ���໥��ϣ��κ�һ���ƻ���������һ�����쳣
 * ���ڹ����ں�ģ���ѶȽϴ����Ը�ƫ���ڹ�����ȫ����ϲ����
 *
 * ����ԭ��
 * ���̶���Ҳ��һ���ں˶�������ͨ������ķ�ʽ���ؽ��̾����ȡ����һ���̶��ϱ������̶���
 * ���⣬����������ں˶��󣬽��̶��󱣻���������
 * �Ӳ����ں˶���Ƕȣ��Խ��̲�������������Ҫ�࣬�����ȡ���̵������С�ö�ٽ������̡߳��ȴ�һ�������˳���
 * ���һζ��ֹ�ǰ�ȫ���̣���һ��ȫ�Ƕ�����̣��򿪽��̾�������ܻᵼ��һЩ�����ļ�������
 * ���ԣ����ڽ��̣�Ӧ������Ƕ�����������������Ҫָ����ͨ�����̾�������޸ġ�д��������ݻ������ֹ����
 *
 * ���ں˶���ͬ������Ҫ��ͬ��Ȩ�ޣ���������Ͽ���������˼·��
 * ����Ȩ����Ҫ��Ϊ�ƶ�Ȩ�޺��޸�Ȩ�ޣ����ڷǰ�ȫ���̣�����ֻ��Ȩ�ޣ��ܹ���һЩ��ѯ���������޷��ƻ�����
 * ����Ȩ�޿���ͨ������˵��Hook����NtOpenProcess�е�DesiredAccess�����������޸�OpenProcedure������GrandedAcess����
 *
 * ����OpenProcedure����δ�������ڲ�ͬ�����ռ�����ƫ�������Ҫ���Ĵ���ʱ�������
 * ��Vistaϵͳ��ʼ��ϵͳ������ObRegisterCallbacks����������������Լ�ػ�ȡ���̺��߳̾����������ֹ�����ȡ���޸�Ȩ��
 */
//��������ҹ�OpenProcedure��ͬ�Ĺ���
NTSTATUS ObRegisterCallbacks(_In_ POB_CALLBACK_REGISTRATION CallbackRegistration, _Outptr_ PVOID* RegistrationHandle);

/**
 * ����ͨ�����̾���ƻ����̣����кܶ෽ʽ
 * ���磺
 * ���������ڵ��̣߳���֮�������̣߳����߳��в���APC��
 * �����������������ں˶���
 * �������̵�Windows����
 * ����������Ҫ���ص��ļ���ע���
 * ����̷�����ƭ��Ϣ����ϵͳ�ػ�������ƭ�����˳�
 * ͨ��DLLע�����
 * ����ϵͳ�ṩ��Exe�ٳֻ��ض�����ƿ��ƻ��ֹ������������IFEO����
 * ����������0day©��
 */

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	return STATUS_SUCCESS;
}