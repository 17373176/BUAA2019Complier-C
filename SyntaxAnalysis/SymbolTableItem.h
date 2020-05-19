/* SymbolTableItem.h : 此文件包含SymbolTableItem类。
 * SymbolTableItem符号表项，识别目标：C语言
 */

#pragma once // 用来保证只会被编译一次

#include <string>
#include <vector>
#include "ConstEnumDefine.h"

using namespace std;

class SymbolTableItem
{
private:
	static int numberCount;    // 符号表项的数量，静态变量，用于计数

	string idName;             // 标识符或名字
	int order;				   // 符号表所在的项号,从0开始，等于numberCount
	string functionName;	   // 符号表所属的函数, 全局符号表作用域名为"GLOBAL"
	ITEM_KIND itemKind;		   // 符号表项类型
	VALUE_TYPE valueType;	   // 值类型
	FUNC_TYPE functionType;	   // 函数类型

	int conIntValue;		   // 常量整数
	char conCharValue;		   // 常量字符
	int length;				   // 数组的长度, 如果是变量对应为0
	int weight;				   // 只针对函数内部的简单变量以及参数有效

public:
	SymbolTableItem(string id, string funcName); // 初始化构造函数

	// 获得符号表项内容
	int getOrder() { return order; }
	string getIdName() { return idName; }
	string getFuncName() { return functionName; }
	ITEM_KIND getItemKind() { return itemKind; }
	VALUE_TYPE getValueType() { return valueType; }
	FUNC_TYPE getFuncType() { return functionType; }
	int getArrLen() { return length; }
	int getConInt() { return conIntValue; }
	char getConChar() { return conCharValue; }
	//int getWeight() { return weight; }

	// set设置符号表项
	void setItemKind(ITEM_KIND kind) { itemKind = kind; }
	void setValueType(VALUE_TYPE type) { valueType = type; }
	void setArrLen(int len) { length = len; }
	void setConInt(int value) { conIntValue = value; }
	void setConChar(char value) { conCharValue = value; }
	void setFuncType(FUNC_TYPE type) { functionType = type; }
	//void addWeight(int num) { weight += num; }
};



