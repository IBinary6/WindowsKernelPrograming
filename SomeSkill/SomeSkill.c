#include <ntddk.h>

/**
 * һЩ���ü��ɺ�֪ʶ
 */

 /**
  * ����x64��x86
  * ��32λ����������64λϵͳʱ
  * ��һ��WOW64��ϵͳ�������ã��������Ϊһ�����������ݲ㣬������Ӧ�ò�
  * ��32λ����ϵͳ���ã��ᱻ�����ϵͳ���أ�ʹ��ָ��ʱ��WOW64�ͻ��ָ�볤��ת��Ϊ���ʵĳ����ٽ����ںˣ���һ���̳�Ϊ��thunking��
  *
  * ϵͳĿ¼��
  * System32��64λ�������ļ���SysWOW64��32λ�ļ�
  * ��32λ������ʵ�System32Ŀ¼�ļ����ᱻ��ϵͳ�ض���SysWOW64���������У�
  * ���ȷʵ��Ҫ����System32Ŀ¼����Ҫ����API��Wow64DisableWow64FsRedirection
  * ע���Ҳ����Ӧ���ض������
  */
VOID Test_Redirection() // �û���ʹ��
{
	//���ڱ����ض���״̬
	//PVOID pOldValue = NULL;
	//BOOL bRet = Wow64DisableWow64FsRedirection(&pOldValue);//ȡ���ض���
	//if (bRet == TRUE)
	//{
	//	HANDLE hFile = CreateFile("C:\\Windows\\system32\\test.txt", GENERIC_READ, 0, NULL, CREATE_ALWAYS, 0, NULL);
	//	if (hFile != INVALID_HANDLE_VALUE)
	//	{
	//		CloseHandle(hFile);
	//		hFile = INVALID_HANDLE_VALUE;
	//	}
	//	Wow64RevertWow64FsRedirection(pOldValue);//�ָ��ض������
	//}
}

/**
 * PatchGuard����
 * ��ʱ���ϵͳ�ؼ�λ�ã���SSDT��ϵͳ������������GDT��ȫ����������IDT���ж���������ϵͳģ�飨ntoskrnl.exe����
 * ���ִ۸�ʱ��������������������һЩ����HOOK������64λ������
 */

 /**
  * ���Ƕ��
  * ������ֻ֧��32λ��Ƕ�������
  * ���Գ��õ� __asm int 3; �����жϣ���64λ�²������ã����������ֳɵ�API��KdBreakPoint();
  * ͨ������࣬�������Ч����һ����
  */

  /**
   * ��Ҫע���һЩ��
   * ��ʼ����ֵ���пգ��ͷŵ�
   */
VOID Test_Init()
{
	WCHAR strNameBuf[256] = { 0 }; //��ʼ����ø�ֵ
	PVOID pData = NULL;
	pData = ExAllocatePool(PagedPool, 1024);
	if (pData == NULL)
	{
		//��Դ����ʱ������ʱ����ʧ�ܵģ�������Ч���жϺ���Ҫ
		return;
	}//���û��������жϣ�memset�����˿�ָ�������
	memset(pData, 0, 1024);
	//�����ڴ��ʹ�ã�����ͷ�
	ExFreePool(pData);
	pData = NULL; //�ͷź�����ǰ�ָ��Ҳ��ֵΪ�գ�����ڴ��ͷź����ñ������������룬��ָ�뻹ָ������ڴ棬�ᵼ��POOL�����ƻ�������Ӧ�ò�Ķ��ƻ�
	//��ֻ�������ڴ�ʱ��ʹ��������Դ������������ָ��ȣ��ͷź�ҲӦ���ѱ����ÿ�
	//��Ч���ж�Ӧ�ð���ʹ����ں��������������϶�Ϊһ����Դ���ǲ����ŵ�
}

/**
 * ��Win10 14393��ʼ��������Ҫ��EVǩ����Ϊ�˼��ٳɱ�������������
 * ����һ���տ�������EVǩ������������û���κ�ҵ��ֻ�����������ûǩ��������
 */

/**
 * һ��������
 * ���纯�������˺ܶ��̣߳���ж��ʱ������Ҫֹͣ��Щ�̵߳�
 * �����unload������ʹ��KeWaitForMultipleObjects�����ȴ��߳��˳������������������Ҫһ���Ƿ�ҳ�ڴ��
 * ����������ڴ����ʧ��ʱ����û���ܹ��ȴ��߳��˳���������
 * �����������DriverEntry�а�������Դ�����
 */

/**
 * ΢���ṩ����֤���ߡ���Verifier
 * ����Ӧ���������жϡ����Ҽ�⡢�����޸����̣���Բ�ͬ�����ƶ���ͬ���ԣ���ֹ��������ģ��������һֱ����
 */

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	return STATUS_SUCCESS;
}