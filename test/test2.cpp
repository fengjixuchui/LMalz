#include <windows.h>
#include <intrin.h>
#include <stdio.h>

int main() {
    // 通过Windows API获取当前逻辑处理器号
    DWORD win_core = GetCurrentProcessorNumber();

    // 通过CPUID获取APIC ID（示例：Leaf 0x01的EBX高24位）
    int regs[4];
    __cpuid(regs, 1);
    DWORD apic_id = (regs[1] >> 24) & 0xFF;

    printf("Windows Logical Core: %u\n", win_core);
    printf("CPUID APIC ID: %u\n", apic_id);

    return 0;
}