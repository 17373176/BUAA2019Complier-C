/* ConstEnumDefine.h : 此头文件有宏定义，定义全局常量、枚举类型、各种类型和结构体
 *
 * SYM类别码、ITEM_KIND符号表项类型、VALUE_TYPE值类型、FUNC_TYPE函数返回类型、ERRORSYM错误类别码、
 * SYM_to_str[SYMBOL_NUM]类别码映射字符串数组、wordNameArr[KEY_WORD_NUM]关键字保留字单词数组、
 * syntaxWord[SYNTAX_ELE_NUM]语法成分数组、syntaxError[SYNTAX_ERROR_NUM]语法错误标识码、
 * errorSym_to_o[SYNTAX_ERROR_NUM] 错误类型详细信息输出、errorSym_to_str[SYNTAX_ERROR_NUM]错误类别输出、
 * 四元式结构体、中间代码类型、中缀表达式转波兰表达式
 *
 */

#pragma once

#include <iostream>
#include <string>

 /*-------宏定义------*/
#define TEXT_LEN 512 * 512 * 4   // 输入文本代码长度
#define SYNTAX_WORD_NUM 1024 * 4 // 识别单词个数
#define KEY_WORD_NUM 40          // 关键字保留字个数
#define WORD_LEN 100             // 单词长度 
#define SYMBOL_NUM 40            // 单词类别码个数
#define SYNTAX_ELE_NUM 40        // 语法成分个数
#define SYNTAX_ERROR_NUM 40      // 错误类别个数

#define GLOBAL_FUNCNAME "GLOBAL" // 全局域，函数域由函数名决定

#define ITM_INSTR_NUM 40		 // 中间代码指令数

#define MIPS_INSTR_NUM 40		 // MIPS指令数

using namespace std;

/*------常量定义------*/
enum SYM { // 类别码
	IDENFR, INTCON, CHARCON, STRCON, CONSTTK, INTTK, CHARTK, VOIDTK, MAINTK,
	IFTK, ELSETK, DOTK, WHILETK, FORTK, SCANFTK, PRINTFTK, RETURNTK, PLUS,
	MINU, MULT, DIV, LSS, LEQ, GRE, GEQ, EQL, NEQ, ASSIGN, SEMICN, COMMA,
	LPARENT, RPARENT, LBRACK, RBRACK, LBRACE, RBRACE, INEXIST, JUMP // 错误处理需要跳过的
};

enum ITEM_KIND { // 符号表项类型，变量(数组由length决定)，常量，参数，函数等种类
	Con_Kind, Var_Kind, Para_Kind, Func_Kind
};

enum VALUE_TYPE { // 值类型
	Int_Type, Char_Type, String_Type, Empty_Type
};

enum FUNC_TYPE { // 函数返回类型
	Void_Type, ReturnInt_Type, ReturnChar_Type, NotDefine_Func
};

enum ERRORSYM { // 错误类别码
	a_ErrorS, b_ErrorS, c_ErrorS, d_ErrorS, e_ErrorS, f_ErrorS, g_ErrorS, h_ErrorS,
	i_ErrorS, j_ErrorS, k_ErrorS, l_ErrorS, m_ErrorS, n_ErrorS, o_ErrorS, my_ErrorS
};

const string SYM_to_str[SYMBOL_NUM] = { // 类别码映射字符串数组
	"IDENFR", "INTCON", "CHARCON", "STRCON", "CONSTTK", "INTTK", "CHARTK", "VOIDTK", "MAINTK",
	"IFTK", "ELSETK", "DOTK", "WHILETK", "FORTK", "SCANFTK", "PRINTFTK", "RETURNTK", "PLUS",
	"MINU", "MULT", "DIV", "LSS", "LEQ", "GRE", "GEQ", "EQL", "NEQ", "ASSIGN", "SEMICN",
	"COMMA", "LPARENT", "RPARENT", "LBRACK", "RBRACK", "LBRACE", "RBRACE", "INEXIST"
};

const string wordNameArr[KEY_WORD_NUM] = { // 关键字保留字单词数组
	"const", "int", "char", "void", "main", "if", "else",
	"do", "while", "for", "scanf", "printf", "return"
};

const string errorSym_to_str[SYNTAX_ERROR_NUM] = { // 错误类别输出
	"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "my"
};

/* 四元式结构体设计、语句类型
 * 1.值参传入 push x, push y
 * 2.调用函数 call add
 * 3.赋值语句 i = ret, i = t1 + 1
 * 4.条件判断 x == y, x<=y
 * 5.纯标签Label1:
 * 6.跳转语句 jump label1, bnz label2 ...
 * 7.函数返回 ret x, ret
 * 8.函数声明 int x
 * 9.参数表 param x
 * 10.print "stringaaa", print 'c', print 123, print a
 * 11.read int a, read char c
 */
enum ItmCodeType { // 中间代码类型
	ValueParamDeliver, // 值参传递
	FunctionCall,
	AssignState,
	Label,
	FunctionDef,
	ParamDef,
	Jump, // 无条件跳转
	BEZ, // 等于
	BNZ, // 不等于
	BLZ, // 小于
	BLEZ, // 小于等于
	BGZ, // 大于
	BGEZ, // 大于等于
	ReadChar,
	ReadInt,
	PrintStr,
	PrintChar,
	PrintInt,
	PrintId,
	ReturnInt,
	ReturnChar,
	ReturnId,
	ReturnEmpty,
	EndProcedure
};

const string itmInstrArr[ITM_INSTR_NUM] = {
	"push", "FunctionCall", "AssignState", "Label", "FunctionDef", "ParamDef", "Jump",
	"BEZ", "BNZ", "BLZ", "BLEZ", "BGZ", "BGEZ", "ReadChar", "ReadInt", "PrintStr", "PrintChar", 
	"PrintInt", "PrintId", "ReturnInt", "ReturnChar", "ReturnId", "ReturnEmpty", "EndProcedure"
};

// 四元式结构体
struct FourYuanItem {
	ItmCodeType type;			  // 项类型
	VALUE_TYPE valueType;		  // 参数值类型,print语句表达式输出时用
	FUNC_TYPE funcType;			  // 如果是函数，函数类型
	string target;				  // 目标结果标识符Id
	bool isTargetArr;			  // 目标结果是数组
	bool isRightArr;			  // 等号右边是数组，数组取值
	bool isPrintCharId;			  // 判断打印的是不是char类型的id
	string left;				  // 左边操作数或者id，当是数组取值时存放数组id
	string index1;				  // 数组1下标
	string right;				  // 右边操作数或者id
	string index2;				  // 数组2下标
	char op;					  // 运算符
};

// 中缀表达式-->逆波兰(后缀表达式),栈中的项
struct PostfixItem {
	VALUE_TYPE type;
	string str;					  // 字符串名称
	int number;					  // 整数或者字符对应的ascii码
	bool isCharVar;				  // 是否是char变量或char型数组的某元素
	bool isOperator;			  // 如果是char常量,判断是不是运算符
};