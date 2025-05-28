#include "orhv.h"

segment_descriptor_32* ALhvGDTgetHostGDT()
{
	segment_descriptor_register_64 gdtr = { 0 };
	_sgdt(&gdtr);
	auto ret = ALhvMMallocateMemory(0x1000);
	//auto tss = ALvmMMallocateMemory(0x1000);
	if (ret && gdtr.base_address)
	{
		memcpy(ret, (PVOID)gdtr.base_address, 0x1000);
		
		return (segment_descriptor_32*)(ret);
	}
	return 0;
}