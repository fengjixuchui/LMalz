#pragma once
#ifdef _KERNEL_MODE	  // __R0
//头文件
#include <ntifs.h>
#include <intrin.h>
#else	  // __R3
//头文件
#include <Windows.h>
#include <winnt.h>

#endif 
#pragma warning(disable:4200)

typedef signed char         INT8, * PINT8;
typedef signed short        INT16, * PINT16;
typedef signed int          INT32, * PINT32;
typedef signed __int64      INT64, * PINT64;
typedef unsigned char       UINT8, * PUINT8;
typedef unsigned short      UINT16, * PUINT16;
typedef unsigned int        UINT32, * PUINT32;
typedef unsigned __int64    UINT64, * PUINT64;

typedef void* PVOID;

typedef void(*BUFFER_HANDLER)(PVOID, UINT64);

#pragma pack(1)
struct OR_SHA_IO_DATA
{
	UINT16 valid;				   //可以读取标志
	UINT16 prepare;				   //正在写入标志
	//UINT64 number;				   //包编号
	//UINT8 data_type;			   //包类型
	UINT16 Length;				   //有效数据长度
	long size;				   //整个数据区大小
	UINT8 data[0];				   //数据
};

static_assert(sizeof OR_SHA_IO_DATA == offsetof(OR_SHA_IO_DATA, data), "adddddd");
struct OR_SHA_IO_HEAD
{
	UINT8 initialization;
	UINT16 existing;	   //存在的包数量
	/*UINT64 number_i;
	UINT64 number_o;*/
	OR_SHA_IO_DATA data_area;

};

class OR_SHA_IO
{
	UINT64 bufferSize;
	OR_SHA_IO_HEAD* buffer;

public:
	OR_SHA_IO(PVOID io_buffer, UINT64 Size) :buffer((OR_SHA_IO_HEAD*)io_buffer), bufferSize(Size) {
		if (!buffer->initialization)
			buffer->data_area.size = (UINT32)Size - sizeof OR_SHA_IO_HEAD;
		buffer->initialization = 1;
	};

	void ReadData(BUFFER_HANDLER handler);
	PVOID PrepareData(UINT16 size);
	bool SendData(PVOID data);
	UINT16 GetExisting() { return buffer->existing; };

	PVOID getBuffer() { return buffer; };

};
#pragma pack()
