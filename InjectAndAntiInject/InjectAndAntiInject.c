#include <ntddk.h>
#include <wdm.h>
/**
 * ����ע�����ע��
 *
 * ����ע��һ���Խ���ΪĿ�꣬��д��Dllģ�����һ��SHELLCODE��Ȼ��ͨ��ĳ���ֶΣ�����Ŀ����̣���Ŀ�����ִ����δ���
 * һ���Ŀ���У�
 * ��Ȩ
 * ��ɱ
 * ��Ϊ���
 * ������Ϣ��ȡ
 * ����ע�뷽ʽ��
 * ����ע�룬ָ��ע����̵���ĳЩϵͳAPIʱ��������ЩAPI�ڲ�����һЩ�����������DLL����SHELL��չע�롢���뷨ע�롢��Ϣ����ע�롢SPIע��
 * ����ע�룬ָ��ע����̼�ʹ��ִ���κβ�����Ҳ�ᱻǿ��ע������DLL����������Զ�߳�ע�롢APCע��
 */
int g_int_0 = 0;
/**
 * ����ע��ʱ����ϵͳ���ƣ�ϵͳΪһ���ֹ���Ԥ������չ�ӿڣ�Ϊϵͳ���ƻ򲹳���������������߿��Ա�дDLL����ϵͳ���Լ��
 * ��������Ӧ����ʱ���������չ�������õ����DLL
 * ��ʵ�ù��ߣ�PCHunter��
 *
 * AppInitע�룺
 * ��ע���Windows��ֵ�£���һ��AppInit_DLLs������ڸ����������Ҫע���DLLȫ·������ͬ��ֵ��LoadAppInit_DLLsΪ1��ʾ��������
 * AppInitע�����User32.dll�������̼���User32.dllʱ���ͻ��������ָ����DLL�����̣�ֻ�����GUIӦ�ã�
 *
 * SPIע�룺
 * �����ṩ�߽ӿڣ���ϵͳΪ��չ���繦�������һ�׽ӿ�
 * �����߿��Ա�дһ����ѭ�淶��DLL������һ�����̶ֹ��ĺ�����ͬʱҲҪ��DLL��Ϣд��ע�����Ӧλ�ã�WinSock2��
 *
 * ��Ϣʱ��ע�룺
 * ϵͳ�ṩ���׻�����Ϣ�¼���ע�뷽ʽ����Ϣ���Ӻ��¼�����
 * ��Ϣ�����ǳ�����ʹ��SetWindowsHookEx��������Ӧ�ģ��¼�����ʹ��SetWinEventHook��ʹ�÷�������
 * ������ֻ�ܼ��ͳһ�����µĽ���
 */
int g_int_1 = 1;
/**
 * ����ע��Ŀ���Խ�ǿ��һ�����һ����һ�����
 *
 * Զ�߳�ע�룺
 * Ӧ�ÿ����У�������Ҫ�����̣߳�ϵͳ��һ�׻��ƣ��������ߴ���һ���������������̵��е��̣߳���Զ�߳�
 * ʹ�ú���CreateRemoteThread����Զ���߳�
 * ����Զ�̲߳�����������̿ռ��У�ÿ�����̾�����������Ľ��̿ռ䣬����IpStartAddress������ָ���̺߳�����ַ��Ҳ����ָ��Ŀ����̵ĵ�ַ�ռ�
 *
 * APCע�룺
 * APC���첽�������ã���ϵͳΪ���첽���һЩ������¼����ṩ�����߳�������ǿ��صĻ���
 * ������׼����һ��������Ȼ��ͨ��ϵͳ�ṩ��APC���ƣ���ָ���߳��ں���ʱ��ִ�иú���
 * APC��Ϊ�ں˺��û�̬���ں�̬APC�ַ�Ϊ����APC����ͨAPC
 * �����ں�APC��Ҫ������IO��ɵ�������ͨ�ں�APC�����������̹߳���
 * �û�̬APC��غ���ֻ��һ������QueueUserAPC
 * 
 * ���ӽ���ע�룺
 * ΢��Detours��ʹ�õľ��Ǹ��ӽ���ע�뷽ʽ��һ��ԭ��Ϊ
 * �����̵���CreateProcess����������CREATE_SUSPENDED��ǣ�����ʽ�����ӽ��̣����ӽ��̾�����Ҫ��ע��Ľ���
 * �ӽ��̴������ܱ�ǿ��ƣ��ӽ����е�������û�����У�����PE������û�жԽ��̽��г�ʼ���������ӽ��̴�����ͣ״̬
 * �����̿����޸��ӽ����ڴ���Ϣ���磬Ϊ�ӽ��̵��������DLL���������ӽ���д��ShellCode��Ȼ���޸�EIPָ��öδ��롢���ӽ���ĳ�ؼ�API����Hook��API������ʱ����ע��Ĵ���
 * ����ResumeThread���ָ��ӽ���ִ��
 * �����ڽٳ�
 */
 //APCע�뺯��ԭ��
 //DWORD WINAPI QueueUserAPC(_In_ PAPCFUNC pfnAPC, _In_ HANDLE hThread, _In_ ULONG_PTR dwData);
int g_int_2 = 2;
/**
 * ��ע��
 * 
 * ��ֹ����ע��
 * ����������Ϣ�����ڴ�
 *	����ڴ棬�޸Ķ�Ӧ��Ϣ��ɾ��ע����Ϣ
 * 
 * ����ע����Ϣ����ע���
 * �����������Ϊ��ֹAppInitע��
 * 
 * ����ע����Ϣ���ڴ���
 * ����DLL����˳��©����������ľ���
 * ��������Ϊ��ntdll��LdrLoadDll�������йҹ�
 * ���ڰ�ȫ�����ͨ����ͨ���ļ�������������ֹ��������������������ļ����޸Ļ������ļ�
 * 
 * ��ֹ����ע��
 * ���Բο��������̷�ʽ
 */

NTSTATUS RegistryCallback(PVOID CallbackContext, PVOID Argument1, PVOID Argument2);

VOID DriverUnload(PDRIVER_OBJECT pDrObj);


LARGE_INTEGER	g_CmCookies = { 0 };

UNICODE_STRING	g_AppInitValueName = { 0 };

NTSTATUS DriverEntry(PDRIVER_OBJECT pDrvObj, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	do
	{
		UNREFERENCED_PARAMETER(pRegistryPath);
		//KdBreakPoint();
		RtlInitUnicodeString(&g_AppInitValueName, L"AppInit_DLLs");
		if (STATUS_SUCCESS != CmRegisterCallback(RegistryCallback, NULL, &g_CmCookies))
		{
			break;
		}
		pDrvObj->DriverUnload = DriverUnload;
		nStatus = TRUE;

	} while (FALSE);
	return nStatus;
}

VOID DriverUnload(PDRIVER_OBJECT pDrObj)
{
	UNREFERENCED_PARAMETER(pDrObj);
	CmUnRegisterCallback(g_CmCookies);
}

NTSTATUS RegistryCallback(PVOID CallbackContext, PVOID Argument1, PVOID Argument2)
{
	NTSTATUS nStatus = STATUS_SUCCESS;
	do
	{
		UNREFERENCED_PARAMETER(CallbackContext);
		if (ExGetPreviousMode() == KernelMode)
		{
			break;
		}
		switch ((REG_NOTIFY_CLASS)(ULONGLONG)Argument1)
		{
		case RegNtPreQueryValueKey:
		{
			//TODO�������������ӶԽ������ֵĹ���
			REG_QUERY_VALUE_KEY_INFORMATION* pInfo = (REG_QUERY_VALUE_KEY_INFORMATION*)Argument2;
			if (pInfo == NULL)
			{
				break;
			}
			if (pInfo->ValueName == NULL || pInfo->ValueName->Buffer == NULL)
			{
				break;
			}
			if (0 != RtlCompareUnicodeString(pInfo->ValueName, &g_AppInitValueName, TRUE))
			{
				break;
			}
			//����Value����,ʵ���ϣ�������Ӧ��ͨ��pInfo->Object,���ObQueryNameString����ȫ·��
			//������Ϊ�ϸ�
			__try
			{
				*pInfo->ResultLength = 0;
			}
			except(EXCEPTION_EXECUTE_HANDLER)
			{

			}
			nStatus = STATUS_CALLBACK_BYPASS;
			break;
		}
		default:
		{
			break;
		}
		}
	} while (FALSE);
	return nStatus;
}
