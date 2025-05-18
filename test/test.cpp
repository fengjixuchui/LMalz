// test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#define __R3
#include "F:\g_un\ALJT\Code\omri\include\debug\debug.h"
//static inline void printchar(char* str, char c) {
//	*str = c;
//	++(str);
//}
//
//#define PAD_RIGHT 1
//#define PAD_ZERO  2
//
//static inline int prints(char* out, const char* string, int width, int pad) {
//	register int pc = 0;
//	char         padchar = ' ';
//
//	if (width > 0) {
//		register int         len = 0;
//		register const char* ptr;
//		for (ptr = string; *ptr; ++ptr)
//			++len;
//		if (len >= width)
//			width = 0;
//		else
//			width -= len;
//		if (pad & PAD_ZERO)
//			padchar = '0';
//	}
//	if (!(pad & PAD_RIGHT)) {
//		for (; width > 0; --width) {
//			printchar(out, padchar);
//			++pc;
//		}
//	}
//	for (; *string; ++string) {
//		printchar(out, *string);
//		++pc;
//	}
//	for (; width > 0; --width) {
//		printchar(out, padchar);
//		++pc;
//	}
//
//	return pc;
//}
//
///* the following should be enough for 32 bit int */
//#define PRINT_BUF_LEN 64
//
//static inline int printi(char* out, intptr_t i, int b, int sg, int width, int pad, int letbase) {
//	char            print_buf[PRINT_BUF_LEN];
//	register char* s;
//	register int    t, neg = 0, pc = 0;
//	register size_t u = i;
//
//	if (i == 0) {
//		print_buf[0] = '0';
//		print_buf[1] = '\0';
//		return prints(out, print_buf, width, pad);
//	}
//
//	if (sg && b == 10 && i < 0) {
//		neg = 1;
//		u = (size_t)-i;
//	}
//
//	s = print_buf + PRINT_BUF_LEN - 1;
//	*s = '\0';
//
//	while (u) {
//		t = u % b;
//		if (t >= 10)
//			t += letbase - '0' - 10;
//		*--s = (char)(t + '0');
//		u /= b;
//	}
//
//	if (neg) {
//		if (width && (pad & PAD_ZERO)) {
//			printchar(out, '-');
//			++pc;
//			--width;
//		}
//		else {
//			*--s = '-';
//		}
//	}
//
//	return pc + prints(out, s, width, pad);
//}
//
//static inline int print(char* out, const char* format, void** varg) {
//	register int width, pad;
//	register int pc = 0;
//	char         scr[2];
//
//	for (; *format != 0; ++format) {
//		if (*format == '%') {
//			++format;
//			width = pad = 0;
//			if (*format == '\0')
//				break;
//			if (*format == '%')
//				goto out;
//			if (*format == '-') {
//				++format;
//				pad = PAD_RIGHT;
//			}
//			while (*format == '0') {
//				++format;
//				pad |= PAD_ZERO;
//			}
//			for (; *format >= '0' && *format <= '9'; ++format) {
//				width *= 10;
//				width += *format - '0';
//			}
//			if (*format == 's') {
//				register char* s = *((char**)varg++);
//				//ALdbgPutValue(varg);
//
//				ALdbgPut("%s", s);
//
//				pc += prints(out, s ? s : "(null)", width, pad);
//				continue;
//			}
//			if (*format == 'd') {
//				pc += printi(out, *(int*)(varg++), 10, 1, width, pad, 'a');
//				continue;
//			}
//			if (*format == 'x') {
//				pc += printi(out, *(int*)(varg++), 16, 0, width, pad, 'a');
//				continue;
//			}
//			if (*format == 'X') {
//				pc += printi(out, *(int*)(varg++), 16, 0, width, pad, 'A');
//				continue;
//			}
//			if (*format == 'u') {
//				pc += printi(out, *(int*)(varg++), 10, 0, width, pad, 'a');
//				continue;
//			}
//			if (*format == 'p') {
//				printchar(out, '0');
//				printchar(out, 'x');
//				pc += 2;
//				pc += printi(out, *(intptr_t*)(varg++), 16, 0, sizeof(void*) * 2, PAD_ZERO, 'A');
//				continue;
//			}
//			if (*format == 'c') {
//				/* char are converted to int then pushed on the stack */
//				scr[0] = *(char*)(varg++);
//				scr[1] = '\0';
//				pc += prints(out, scr, width, pad);
//				continue;
//			}
//		}
//		else {
//		out:
//			printchar(out, *format);
//			++pc;
//		}
//	}
//	if (out)
//		*out = '\0';
//	return pc;
//}
//
//static char g_errstr[256] = { 0 };
//static int g_errstr_number = 0;
//char* ALhvErrSetString(const char* format, ...)
//{
//
//	register void** varg = (void**)(&format) + 1;
//	ALdbgPut("%s", *varg);
//	ALdbgPutValue(varg);
//	g_errstr_number = print(g_errstr, format, varg);
//	ALdbgPutValue(varg);
//	return g_errstr;
//}
//char* ALhvErrAddString(const char* format, ...)
//{
//
//	register void** varg = (void**)(&format) + 1;
//	g_errstr_number += print(g_errstr + g_errstr_number, format, varg);
//	return g_errstr + g_errstr_number;
//}

static inline void printchar(char** str, char c) {
	**str = c;
	++(*str);
}

#define PAD_RIGHT 1
#define PAD_ZERO  2

//static inline int prints(char** out, const char* string, int width, int pad) {
static int prints(char** out, const char* string, int width, int pad) {
	register int pc = 0;
	char         padchar = ' ';
	if (width > 0) {
		register int         len = 0;
		register const char* ptr;
		for (ptr = string; *ptr; ++ptr)
			++len;
		if (len >= width)
			width = 0;
		else
			width -= len;
		if (pad & PAD_ZERO)
			padchar = '0';
	}
	if (!(pad & PAD_RIGHT)) {
		for (; width > 0; --width) {
			printchar(out, padchar);
			++pc;
		}
	}

	for (; *string; ++string) {
		printchar(out, *string);
		++pc;
	}


	for (; width > 0; --width) {
		printchar(out, padchar);
		++pc;
	}

	return pc;
}

/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 64

static inline int printi(char** out, intptr_t i, int b, int sg, int width, int pad, int letbase) {
	char            print_buf[PRINT_BUF_LEN];
	register char* s;
	register int    t, neg = 0, pc = 0;
	register size_t u = i;

	if (i == 0) {
		print_buf[0] = '0';
		print_buf[1] = '\0';
		return prints(out, print_buf, width, pad);
	}

	if (sg && b == 10 && i < 0) {
		neg = 1;
		u = (size_t)-i;
	}

	s = print_buf + PRINT_BUF_LEN - 1;
	*s = '\0';

	while (u) {
		t = u % b;
		if (t >= 10)
			t += letbase - '0' - 10;
		*--s = (char)(t + '0');
		u /= b;
	}

	if (neg) {
		if (width && (pad & PAD_ZERO)) {
			printchar(out, '-');
			++pc;
			--width;
		}
		else {
			*--s = '-';
		}
	}

	return pc + prints(out, s, width, pad);
}

static inline int print(char** out, const char* format, void** varg) {
	register int width, pad;
	register int pc = 0;
	char         scr[2];

	for (; *format != 0; ++format) {
		if (*format == '%') {
			++format;
			width = pad = 0;
			if (*format == '\0')
				break;
			if (*format == '%')
				goto out;
			if (*format == '-') {
				++format;
				pad = PAD_RIGHT;
			}
			while (*format == '0') {
				++format;
				pad |= PAD_ZERO;
			}
			for (; *format >= '0' && *format <= '9'; ++format) {
				width *= 10;
				width += *format - '0';
			}
			if (*format == 's') {
				register char* s = *((char**)varg++);
				pc += prints(out, s ? s : "(null)", width, pad);
				continue;
			}
			if (*format == 'd') {
				pc += printi(out, *(int*)(varg++), 10, 1, width, pad, 'a');
				continue;
			}
			if (*format == 'x') {
				pc += printi(out, *(int*)(varg++), 16, 0, width, pad, 'a');
				continue;
			}
			if (*format == 'X') {
				pc += printi(out, *(int*)(varg++), 16, 0, width, pad, 'A');
				continue;
			}
			if (*format == 'u') {
				pc += printi(out, *(int*)(varg++), 10, 0, width, pad, 'a');
				continue;
			}
			if (*format == 'p') {
				printchar(out, '0');
				printchar(out, 'x');
				pc += 2;
				pc += printi(out, *(intptr_t*)(varg++), 16, 0, sizeof(void*) * 2, PAD_ZERO, 'A');
				continue;
			}
			if (*format == 'c') {
				/* char are converted to int then pushed on the stack */
				scr[0] = *(char*)(varg++);
				scr[1] = '\0';
				pc += prints(out, scr, width, pad);
				continue;
			}
		}
		else {
		out:
			printchar(out, *format);
			++pc;
		}
	}
	if (out)
		**out = '\0';
	return pc;
}
static char g_errstr[256] = { 0 };
static int g_errstr_number = 0;
char* ALhvErrSetString(const char* format, ...)
{

	register void** varg = (void**)(&format) + 1;
	auto a = g_errstr;
	g_errstr_number = print(&a, format, varg);
	return g_errstr;
}
char* ALhvErrAddString(const char* format, ...)
{

	register void** varg = (void**)(&format) + 1;
	auto a = g_errstr + g_errstr_number;
	g_errstr_number += print(&a, format, varg);
	return g_errstr;
}
#define ALhvSetErr(a,...) ALhvErrSetString( __FUNCTION__ "(%d):\t" a,__LINE__, __VA_ARGS__)
#define ALhvAddErr(a,...) ALhvErrAddString("\nFROM:" __FUNCTION__ "(%d):\t" a,__LINE__, __VA_ARGS__)
bool test1()
{
	ALhvSetErr("申请内存失败号码%d", 111555);
	//ALdbgPut("%s", g_errstr);

	return 0;
}
bool test2()
{
	if (!test1())
	{
		ALhvAddErr("内存映射失败",123);
		//ALdbgPut("%s", g_errstr);

		return 0;

	}
	return 0;
}


int main()
{
	if (!test2())
	{
		ALhvAddErr("执行测试er时出现错误%d", 12613);
		//ALdbgPut("%s", g_errstr);


	}


	ALdbgPut("%s", g_errstr);

	ALdbgPut("%d", g_errstr_number);
    std::cout << "Hello World!\n";
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
