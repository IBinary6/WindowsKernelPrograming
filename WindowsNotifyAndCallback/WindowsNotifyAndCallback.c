#include <ntddk.h>

/**
 * Windows֪ͨ���ص�
 *
 * ϵͳ�ں��ṩ��һϵ���¼�֪ͨ�����Լ��ص�����
 * �¼�֪ͨ��Ҫ���ڼ��ϵͳ��ĳһ�¼�����
 * �ص����Ƹ��౻������Ӧϵͳ��ĳһ������״̬�������Ա�����ʵ���ں�ģ��֮���ͨ��
 *
 * �¼�֪ͨ�������ɿ�����д��һЩ������ͨ���ں�API�Ѻ����;����¼������󶨣�ע�ᣩ�����¼���������Щ�����ͻᱻϵͳ����
 * �������д�������֪ͨ�������߳�֪ͨ������ģ��֪ͨ��ע������֪ͨ��
 * 
 * �ص�����Ϊ�����ṩ��һ��ͨ�÷��������ͺͽ���ĳ��ͨ��
 * ��Щͨ�����ʱϵͳĳ����״̬�����仯��������ͨ�棬Ҳ����ʱ�������Զ���ĳ��������������
 * ϵͳ������һЩ�ص��е�Դ״̬�仯�ص���ϵͳʱ��仯�ص�
 * ��Ҫ����»���CALLBACK_OBJECT�ص�����Ļص�����
 * �ص��������ʵ�ַ��ͺͽ���ͨ��Ĺؼ�������Ψһ������һ���ص�
 * ����һ���������ں˶��󣨸о����ǿ���������ͨ�ŵ��豸����
 * ����ϵͳ�е�Դ״̬�ص��Ķ�����Ϊ��\Callback\PowerState
 * ��Ҫ���ոûص�ʱ�����Ե���ExCreateCallback�����򿪶��󣬻᷵��һ���ص�����ָ��
 * ֮�����ExRegisterCallback��һ���ص�����ע�ᵽ�ö�����
 */

 /*����ԭ������*/
void  DriverUnload(__in struct _DRIVER_OBJECT* DriverObject);

/**
 * ���������¼��ص���أ������Ļ���һ�����ļ�������Ŀ�û�����룩
 */
VOID CreateProcessNotifyEx(__inout PEPROCESS  Process,
	__in HANDLE  ProcessId,
	__in_opt PPS_CREATE_NOTIFY_INFO  CreateInfo);


typedef NTSTATUS(_stdcall* PPsSetCreateProcessNotifyRoutineEx)(
	IN PCREATE_PROCESS_NOTIFY_ROUTINE_EX  NotifyRoutine,
	IN BOOLEAN  Remove
	);

/*ȫ�ֱ�������*/
PPsSetCreateProcessNotifyRoutineEx	g_pPsSetCreateProcessNotifyRoutineEx = NULL;
BOOLEAN g_bSuccRegister = FALSE;

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	//_asm int 3;
	do
	{
		UNICODE_STRING uFuncName = { 0 };
		DriverObject->DriverUnload = DriverUnload;
		RtlInitUnicodeString(&uFuncName, L"PsSetCreateProcessNotifyRoutineEx");
		//��̬��ȡ������ַ
		g_pPsSetCreateProcessNotifyRoutineEx = (PPsSetCreateProcessNotifyRoutineEx)MmGetSystemRoutineAddress(&uFuncName);
		if (g_pPsSetCreateProcessNotifyRoutineEx == NULL)
		{
			break;
		}
		if (STATUS_SUCCESS != g_pPsSetCreateProcessNotifyRoutineEx(CreateProcessNotifyEx, FALSE))
		{
			//���ܷ���STATUS_ACCESS_DENIED��ʱ��Ϊ֪ͨ��������ģ��PEͷû�б�����IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY��־
			//����취ʱ�����ν�ѡ��/inegritycheck
			break;
		}
		g_bSuccRegister = TRUE;
		nStatus = STATUS_SUCCESS;
	} while (FALSE);

	return nStatus;
	return STATUS_SUCCESS;
}

void  DriverUnload(__in struct _DRIVER_OBJECT* DriverObject)
{
	if (g_bSuccRegister && g_pPsSetCreateProcessNotifyRoutineEx)
	{
		//�ڲ�ʹ��ʱ��������Ӧ���Ƴ�֪ͨ
		g_pPsSetCreateProcessNotifyRoutineEx(CreateProcessNotifyEx, TRUE);
		g_bSuccRegister = FALSE;
	}
	return;
}

VOID CreateProcessNotifyEx(
	__inout PEPROCESS  Process,
	__in HANDLE  ProcessId,
	__in_opt PPS_CREATE_NOTIFY_INFO  CreateInfo
)
{
	HANDLE hParentProcessID = NULL;/*������ID*/
	HANDLE hPareentThreadID = NULL;/*�����̵��߳�ID*/
	HANDLE hCurrentThreadID = NULL;/*�ص�����CreateProcessNotifyEx��ǰ�߳�ID*/
	hCurrentThreadID = PsGetCurrentThreadId();
	if (CreateInfo == NULL)
	{
		/*�����˳�*/
		DbgPrint("CreateProcessNotifyEx [Destroy][CurrentThreadId: %p][ProcessId = %p]\n", hCurrentThreadID, ProcessId);
		return;
	}
	/*��������*/
	hParentProcessID = CreateInfo->CreatingThreadId.UniqueProcess;
	hPareentThreadID = CreateInfo->CreatingThreadId.UniqueThread;

	DbgPrint("CreateProcessNotifyEx [Create][CurrentThreadId: %p][ParentID %p:%p][ProcessId = %p,ProcessName=%wZ]\n",
		hCurrentThreadID, hParentProcessID, hPareentThreadID, ProcessId, CreateInfo->ImageFileName);
	return;
}

/**
 * һЩ���������������ĵ��ԣ�ͨ��ɱ��ɾ�������������ֻ����
 * һ����˵�����ಡ��Ҳ��sys�ļ���ʽ���ڣ�Ҳ������ע����У���ֻɾ����������Ϣ�ǲ�����
 * ���������ܶ඼��ע��ϵͳ�ػ��¼�֪ͨ�����Դ״̬�仯�ص�
 * ��֪ͨ��ص������У�����������ע�ᵽϵͳ�У�������һ��sys�ļ�������������Ϣд��ע���
 * ��Ҳֻ��һЩ�򵥵�Ӧ�ã�ʵ��������ܸ�����
 */