#include "ntddk.h"

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

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = TRUE;

	//�����б�
	Test_Lookaside();


	return status;
}