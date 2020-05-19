/* CodeGenerator.h : 此头文件是代码生成相关的定义
 * 包括中间代码生成，MIPS代码生成
 */

#pragma once

#include <iostream>
#include <map>
#include <vector>
#include "SymbolTableItem.h"

 /*-------宏定义------*/
#define TEMP_REGISTER 6 // 临时变量寄存器$t4~$t9

using namespace std;

/*------常量定义------*/
const int tempStackMax = 100; // 设置临时参数栈最大为100字节(最多有25个参数)
const unsigned int dataBaseAddr = 0x10010000; // data段起始地址

//extern vector<SymbolTableItem> globalSymbolTable; // 来自外部的符号表

/*------代码生成函数声明------*/
string generateLabel();			// 生成标签并返回
string generateVar();				// 生成临时变量并返回
string generateStrLabel();         // 生成字符串标签
bool isStringDigit(string);	    // 判断字符串是否是数字串
int strToInt(string);				// 字符串转为整数

// 表达式的相关计算处理
void turnToPostfixExp(vector<PostfixItem>, vector<PostfixItem>&); // 转化为后缀表达式
string calculateExp(vector<PostfixItem>&, bool&, VALUE_TYPE&, int&, bool, vector<FourYuanItem>&, string); // 计算后缀表达式值
void objExpLeftR(vector<PostfixItem>&, bool&, string&, int&); // 计算后缀表达式左右两项

void writeItmCodeToFile(string, vector<FourYuanItem>, map<string, unsigned>&);			// 将中间代码写入到文件中
void generateMipsCode(string, bool);			// 将中间代码翻译成MIPS汇编语言代码

void generateData(ofstream&);		// 生成data段
void generateText(ofstream&, vector<FourYuanItem>);		// 生成text段

void getAddrOfGlobal(string, string, ofstream&);
void getAddrOfLocal(string, string, string, ofstream&);
void getAddrOfTemp(int, string, ofstream&);
void helpAssignStatement(FourYuanItem, ofstream&);
int getGlobalVarSumSpace();
void initializeStack(string, ofstream&);
void helpFunctionDef(string, ofstream&);
void helpBJump(FourYuanItem, ofstream&);
void helpReturn(ofstream&);
void cancelEscapeChar(string&);    // 取消字符串中的转义字符

void generateOpItmArr(vector<FourYuanItem>&);
/*
void op_initializeStack(string, ofstream&);
void op_helpAssignStatement(FourYuanItem, ofstream&);
void op_helpFunctionDef(string, ofstream&);
void op_helpBJump(FourYuanItem, ofstream&);
void op_helpReturn(ofstream&, string);
void op_generateText(ofstream&);
*/