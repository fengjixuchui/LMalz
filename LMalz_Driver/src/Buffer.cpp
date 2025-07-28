#include "Buffer.h"
#define __R3
#include "F:\g_un\ALJT\Code\omri\include\debug\debug.h"

void OR_SHA_IO::ReadData(BUFFER_HANDLER handler)
{
	if (InterlockedDecrement16((SHORT volatile*)&buffer->existing) >= 0)	 //����Լ���С��0
	{
		for (PUINT8 i = (PUINT8)&buffer->data_area; i < (PUINT8)buffer + bufferSize;)
		{
			OR_SHA_IO_DATA* data_p = (OR_SHA_IO_DATA*)(i);
			if (InterlockedCompareExchange16((SHORT volatile*)(&data_p->valid), 0, 1))
			{
				handler(data_p->data, data_p->Length);
				//_mm_sfence(); // �ڴ�����
				//InterlockedCompareExchange16((SHORT volatile*)(&data_p->prepare), 0, 1);
				//data_p->prepare = 0;
				InterlockedExchange16((SHORT volatile*)&data_p->prepare, 0);
				_mm_mfence(); // �ڴ�����


			}
			i = data_p->data;
			i += data_p->size;
		}
	}
	else
	{
		InterlockedIncrement16((SHORT volatile*)&buffer->existing);
	}
	return;
}

bool OR_SHA_IO::SendData(PVOID data)
{
	OR_SHA_IO_DATA* data_p = (OR_SHA_IO_DATA*)((UINT64)data - sizeof OR_SHA_IO_DATA);
	if (data_p->data != data)
		return 0;
	//data_p->number = InterlockedIncrement64((LONG64 volatile*)&buffer->number_o) - 1;
	//data_p->valid = 1;
	InterlockedExchange16((SHORT volatile*)&data_p->valid, 1);
	InterlockedIncrement16((SHORT volatile*)&buffer->existing);
	_mm_mfence(); // �ڴ�����
	return 1;
}
PVOID OR_SHA_IO::PrepareData(UINT16 size)
{

	for (PUINT8 i = (PUINT8)&buffer->data_area; i < (PUINT8)buffer + bufferSize;)
	{
		//ALdbgStoput();
		OR_SHA_IO_DATA* data_p = (OR_SHA_IO_DATA*)(i);
		//ALdbgStoput("%p size:%p", data_p, data_p->size);												 
		if (!InterlockedCompareExchange16((SHORT volatile*)(&data_p->prepare), 1, 0))
		{
			//ALdbgStoput();
			if (data_p->size >= size + sizeof(OR_SHA_IO_DATA))//�ɷֽ�
			{
				//ALdbgStoput();
				//ALdbgStop();
				OR_SHA_IO_DATA* data_p_2 = (OR_SHA_IO_DATA*)(data_p->data + size);
				//data_p_2->Length = 0;
				InterlockedExchange16((SHORT volatile*)&data_p_2->Length, 0);
				//data_p_2->valid = 0;
				InterlockedExchange16((SHORT volatile*)&data_p_2->valid, 0);
				//data_p_2->prepare = 0;
				InterlockedExchange16((SHORT volatile*)&data_p_2->prepare, 0);

				//data_p_2->size = data_p->size - size - offsetof(OR_SHA_IO_DATA, data);
				InterlockedExchange((long volatile*)&data_p_2->size, data_p->size - size - offsetof(OR_SHA_IO_DATA, data));

				//data_p->size = size;
				InterlockedExchange((long volatile*)&data_p->size, size);

				//data_p->Length = size;
				InterlockedExchange16((SHORT volatile*)&data_p->Length, size);

				//data_p->data_type = type;

				//memcpy(data_p->data, data, size);
				return data_p->data;
			}
			else if (data_p->size >= size)//����
			{
				//ALdbgStoput();
				//data_p->data_type = type;
				//data_p->Length = size;			   //��С������Ч���ȱ��
				InterlockedExchange16((SHORT volatile*)&data_p->Length, size);

				//data_p->number = InterlockedIncrement64((LONG64 volatile*)&buffer->number_o) - 1;
				//memcpy(data_p->data, data, size);
				return data_p->data;
			}
			else		 //������,��������ѭ����ϲ��ݹ�
			{
				//ALdbgStoput("%p size:%p", data_p, data_p->size);													//�ϲ�
				OR_SHA_IO_DATA* data_p_2 = (OR_SHA_IO_DATA*)(data_p->data + data_p->size);
				if ((UINT64)data_p_2 < (UINT64)((PUINT8)buffer + bufferSize))			 //[bug�޸�] //!!!!!!!!!!!!!!  �������ѿ�,û�ж��Ƿ�Խ��
				{
					if (!InterlockedCompareExchange16((SHORT volatile*)(&data_p_2->prepare), 1, 0))		  //�����һ��Ҳ����Ч��δ��׼����
					{
						//ALdbgStoput("%p size:%p", data_p, data_p->size);													//�ϲ�
						//data_p->size += data_p_2->size + offsetof(OR_SHA_IO_DATA, data); //������һ����С
						InterlockedAdd((LONG volatile*)&data_p->size, data_p_2->size + offsetof(OR_SHA_IO_DATA, data));
						//data_p->prepare = 0;  //ʹ��ǰ��Ч
						InterlockedExchange16((SHORT volatile*)&data_p->prepare, 0);
						return PrepareData(size);		   //�ݹ�
					}
					else //��һ����Ч  ���ͷŵ�ǰ,Ѱ����һ����Ч
					{
						//ALdbgStoput("%p size:%p", data_p, data_p->size);												 
						//data_p->prepare = 0;
						InterlockedExchange16((SHORT volatile*)&data_p->prepare, 0);

					}
				}
			}
		}
		i = data_p->data;
		i += data_p->size;
	}
	//ȫ����Ч
	return 0;
}