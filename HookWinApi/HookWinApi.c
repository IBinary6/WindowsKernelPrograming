/**
 * Windows�ں˹ҹ�
 *
 * ϵͳ������������ҹ� SSDT HOOK
 * ����������ҹ� Export Table HOOK
 * �жϹҹ� IDT/IOAPIC HOOK
 * �ַ������ҹ� Dispatch Function HOOK
 * ϵͳ������ڹҹ� Syscall Entry HOOK
 * �����ҹ� Inline HOOK
 * ���Թҹ�	Debug HOOK
 */

 /**
  * SSDT
  * ϵͳ��������������Windows�ں���һ�����ݽṹ
  * �������ں˵�����һϵ�й��û�̬���õĺ�����ַ
  * �û�ģʽ������ϵͳ���ã�����ͨ������ָ������ںˣ���ͨ����������ϵͳ�����������ҵ���Ӧϵͳ�����ṩ����
  * ���ַ�ɴ��ں��е����ĽṹKeServiceDescriptorTable�л�ã���Ҫ��¼�˻�ַ�ͺ�������
  * �ṹ������Ҫ��עServiceTableBase��ָ��һ��ָ�����飬������ÿ��Ԫ�ض���ϵͳ����������ָ�룬�����ž�������
  * ͨ���޸���������еĺ���ָ�룬���ԴﵽһЩĿ�ģ���64λ�л�������򣬷����иĶ���ֱ������
  * �������������sh_ssdt_hook.h���л�࣬��Ҫ32λ����
  */

  /**
   * ����������ҹ�
   * ����һЩ�ں˺���û�зŵ�ϵͳ������������
   * ����ԭʼ��ַ���滻��תָ������Ҫ�Ĵ�����ٵ��û�ԭ���ĵ�ַ
   * ���ӹҹ�IoCallDriver����xtbl_hook.h
   */

#include <ntddk.h>

NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath)
{
	return STATUS_SUCCESS;
}