/* STManager.h : 此头文件用于符号表管理，添加符号表项以及检查符号表
 * 下面包括填符号表,查符号表语义分析相关的操作函数
 */

#pragma once

#include <string>
#include <vector>
#include "SymbolTableItem.h"
#include "CodeGenerator.h"

using namespace std;

extern vector<SymbolTableItem> globalSymbolTable; // 符号表

class STManager
{
private:
	string reDeclareFuncName; // 所在域，声明的函数名称
	SYM relation; // 关系运算符

public:
	STManager();

	vector<SymbolTableItem> getTable(); // 获得符号表

	// 填符号表函数,采用函数重载
	bool pushItem(string id, string functionName, int num); // 常量int
	bool pushItem(string id, string functionName, char ch); // 常量char
	bool pushItem(string id, string functionName, VALUE_TYPE valueType, int size); // 变量数组
	bool pushItem(string id, string functionName, FUNC_TYPE funcType); // 函数
	bool pushItem(string id, string functionName, ITEM_KIND itemKind, VALUE_TYPE valueType); // 变量与参数
	
	// 检查符号表
	bool isInsert(string id, string functionName); // 检查是否可以填表
	
	int idCheckInFactor(string identifier, string funcName); // 因子项中标识符检查
	
	FUNC_TYPE idCheckInState(string identifier); // 语句中标识符检查
	//＜标识符＞‘[’＜表达式＞‘]’因子项与赋值语句中检查数组
	int idArrExpCheck(string identifier, string funcName, bool surable, int index = 0);
	//＜标识符＞‘(’<值参数表>‘)’函数调用检查，若是因子项中的(表达式中的,需要判断是否是有返回值)
	int funcCallCheck(string identifier, bool isInExp, vector<VALUE_TYPE> paramType);
	//类型检查
	/*bool checkTypeMatch(VALUE_TYPE s_type, VALUE_TYPE e_type) {
		if (s_type == Char_Type && e_type == Int_Type) {
			//myError.SemanticAnalysisError(AssignIntToCharError, getLineNumber(), "");
		}
	}*/
	//对赋值语句单纯的标识符的检查
	int checkAssignId(string identifier, string funcName);

	bool checkReturn(string funcName); // 返回语句检查,无返回值的return
	bool checkReturn(string funcName, VALUE_TYPE retType); // 有返回值的

	// 获得符号表内私有变量
	VALUE_TYPE getItemValueType(int order); // 得到order对应的符号表项值类型

	//void addWeight(int order, int weight); // 优化部分的函数, 需要检查是否符合要求
};

