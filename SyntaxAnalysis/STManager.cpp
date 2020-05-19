/* STManager.cpp文件，对STManager类的函数定义
 *
 */

#include "STManager.h"
#include <algorithm>
#include <string>
#include <vector>

using namespace std;

STManager::STManager() { // 标准构造函数 
	reDeclareFuncName = "GLOBAL"; // 全局域
}

// 符号表管理器函数定义
 // int常量
bool STManager::pushItem(string id, string functionName, int num) {
	if (!isInsert(id, functionName)) // 重复定义不可插入
		return false;
	SymbolTableItem newItem(id, functionName);
	newItem.setItemKind(Con_Kind);
	newItem.setValueType(Int_Type);
	newItem.setConInt(num);
	globalSymbolTable.push_back(newItem); // 添加到符号表向量尾部
	return true;
}
// char常量
bool STManager::pushItem(string id, string functionName, char character) { 
	if (!isInsert(id, functionName))
		return false;
	SymbolTableItem newItem(id, functionName);
	newItem.setItemKind(Con_Kind);
	newItem.setValueType(Char_Type);
	newItem.setConChar(character);
	globalSymbolTable.push_back(newItem);
	return true;
}
// 数组
bool STManager::pushItem(string id, string functionName, VALUE_TYPE valueType, int len) {
	if (!isInsert(id, functionName))
		return false;
	SymbolTableItem newItem(id, functionName);
	newItem.setItemKind(Var_Kind);
	newItem.setValueType(valueType);
	newItem.setArrLen(len);
	globalSymbolTable.push_back(newItem);
	return true;
}
// 变量+参数
bool STManager::pushItem(string id, string functionName, ITEM_KIND itemKind, VALUE_TYPE valueType) {
	if (!isInsert(id, functionName))
		return false;
	SymbolTableItem newItem(id, functionName);
	newItem.setItemKind(itemKind);
	newItem.setValueType(valueType);
	globalSymbolTable.push_back(newItem);
	return true;
}
// 函数
bool STManager::pushItem(string id, string functionName, FUNC_TYPE funcType) {
	if (!isInsert(id, functionName))
		return false;
	SymbolTableItem newItem(id, functionName);
	newItem.setItemKind(Func_Kind);
	newItem.setFuncType(funcType);
	globalSymbolTable.push_back(newItem);
	return true;
}
// 检查是否可填表
bool STManager::isInsert(string id, string functionName) {
	for (unsigned int i = 0; i < globalSymbolTable.size(); i++) {
		SymbolTableItem item = globalSymbolTable.at(i);
		if (functionName == item.getFuncName() && id == item.getIdName()) // 作用域相同,名字也相同
			return false;
	}
	return true;
}

// 数组检查＜标识符＞‘[’＜表达式＞‘]’，返回-1未定义，-2下标越界，-3下标不确定是整型，其他值为符号表项号
int STManager::idArrExpCheck(string identifier, string funcName, bool surable, int index) {
	bool globalIndexOut = false; // 是全局数组但越界
	bool notglobalArr = false; // 不是全局数组
	bool isDefined = false; // 未定义标识符
	int order = -1;
	for (unsigned int i = 0; i < globalSymbolTable.size(); i++) {
		SymbolTableItem item = globalSymbolTable.at(i);
		if (item.getFuncName() == "GLOBAL" && item.getIdName() == identifier) { // 全局,标识符名相同
			isDefined = true; // 定义了
			if (item.getArrLen() > 0) { // 是数组
				if (surable) { // 数组下标值是确定的
					if (index >= item.getArrLen() || index < 0) { // 越界
						globalIndexOut = true;
					}
					else order = item.getOrder();
				}
				else order = item.getOrder();
			}
			else notglobalArr = true; // 不是数组
		}
		else if (item.getFuncName() == funcName && item.getIdName() == identifier) { // 属于函数作用域内
			isDefined = true;
			if (item.getArrLen() > 0) {
				if (surable) {
					if (index >= item.getArrLen() || index < 0) {//越界
						//myError.SemanticAnalysisError(ArrIndexOutOfRangeError, getLineNumber(), identifier);
						return -1;
					}
					else return item.getOrder();
				}
				else return item.getOrder();
			}
			else {//不是数组
				if (globalIndexOut) {
					break;
				}
				else {
					//myError.SemanticAnalysisError(TypeNotMatchError, getLineNumber(), identifier);
					return -1;
				}
			}
		}
	}

	if (globalIndexOut) {
		//myError.SemanticAnalysisError(ArrIndexOutOfRangeError, getLineNumber(), identifier);
		return -1;
	}
	if (notglobalArr) {
		//myError.SemanticAnalysisError(TypeNotMatchError, getLineNumber(), identifier);
		return -1;
	}
	//标识符未定义
	if (!isDefined) {
		//myError.SemanticAnalysisError(NotDefinitionError, getLineNumber(), identifier);
		return -1;
	}
	return order;
}

// 标识符检查因子中(不可以为void函数),表达式中，赋值语句右边,返回-1为未定义或其他标识符，其他值即为所在符号表的项号,通过项号可以查找该项
int STManager::idCheckInFactor(string identifier, string funcName) {
	bool foundInGlobal = false; // 表示在global中发现此结构存在问题
	bool isDefined = false;
	for (unsigned int i = 0; i < globalSymbolTable.size(); i++) {
		SymbolTableItem item = globalSymbolTable.at(i);
		if (item.getFuncName() == "GLOBAL") {//全局作用域
			if (item.getIdName() == identifier) {//标识符名字相同
				isDefined = true;
				if (item.getArrLen() > 0 || (item.getItemKind() == Func_Kind && item.getFuncType() == Void_Type)) 
					foundInGlobal = true;
				else return item.getOrder();
			}
		}
		else if (item.getFuncName() == funcName) { // 作用域相同(局部作用域)
			if (item.getIdName() == identifier) {
				isDefined = true;
				if (item.getArrLen() > 0) //为数组,报错
					return -1;
				else return item.getOrder();
			}
		}
	}
	if (foundInGlobal)
		return -1;
	if (!isDefined) 
		return -1;
	return -1;
}

// 语句中函数标识符检查，用于判断是否有该函数，返回函数类型
FUNC_TYPE STManager::idCheckInState(string identifier) {
	// 由于只能是函数,所以只需要分析全局的即可
	for (unsigned int i = 0; i < globalSymbolTable.size(); i++) {
		SymbolTableItem item = globalSymbolTable.at(i);
		if (item.getFuncName() == "GLOBAL") 
			if (item.getIdName() == identifier) 
				if (item.getItemKind() == Func_Kind)
					return item.getFuncType();
	}
	return NotDefine_Func; // 没有定义
}

// ＜标识符＞‘(’<值参数表>‘)’带参数和没有参数的函数调用检查,若是因子项中的(表达式中的,需要判断是否是有返回值)
// -1未定义，返回1代表个数不匹配，2代表类型不匹配，3代表不具有返回值
int STManager::funcCallCheck(string identifier, bool isInExp, vector<VALUE_TYPE> paramType) {
	bool isDefined = false; // 是否需要进行参数检查(函数未定义则不需要进行)
	vector<VALUE_TYPE> actualParam;
	for (unsigned int i = 0; i < globalSymbolTable.size(); i++) {
		SymbolTableItem item = globalSymbolTable.at(i);
		if (item.getFuncName() == "GLOBAL") 
			if (item.getIdName() == identifier)
				if (item.getItemKind() == Func_Kind) {//是函数
					isDefined = true;
					if (isInExp && item.getFuncType() == Void_Type) // 不具有返回值
						return 3;
					for (unsigned int j = i + 1; j < globalSymbolTable.size(); j++) {
						item = globalSymbolTable.at(j);
						if (item.getFuncName() == identifier && item.getItemKind() == Para_Kind)
							actualParam.push_back(item.getValueType());
						else break;
					}
					break;
				}
	}
	if (!isDefined)
		return -1;
	// 参数表考察
	if (paramType.size() == 0 && actualParam.size() == 0) // 参数个数都是0个
		return 0;
	if (paramType.size() != actualParam.size()) // 参数个数不匹配
		return 1;
	for (unsigned int i = 0; i < paramType.size(); i++) {
		VALUE_TYPE first = actualParam.at(i);
		VALUE_TYPE second = paramType.at(i);
		if (first != second) //参数类型不匹配
			return 2;
	}
	return 0; // 符合
}

// 对赋值语句中左边的标识符,以及scanf中单纯的标识符的检查(scanf实际就是对变量的赋值操作)
// 返回-1为未定义,-2为函数或数组的标识符,-3为常量不能被赋值, 其他值为符合，即为所在符号表的项号,通过项号可以查找该项
int STManager::checkAssignId(string identifier, string funcName) {
	bool isDefined = false; // 是否定义
	for (unsigned int i = 0; i < globalSymbolTable.size(); i++) {
		SymbolTableItem item = globalSymbolTable.at(i);
		if (item.getFuncName() == "GLOBAL") {
			if (item.getIdName() == identifier) {
				isDefined = true; // 被全局定义
				if (item.getItemKind() == Var_Kind && item.getArrLen() == 0) // 是全局变量,相当于已定义
					return item.getOrder();
				else if (item.getItemKind() == Con_Kind) return -3; // 全局常量
			}
		}
		else if (item.getFuncName() == funcName) { // 是函数内部的
			if (item.getIdName() == identifier) {
				isDefined = true;
				if ((item.getItemKind() == Var_Kind && item.getArrLen() == 0) 
					|| (item.getItemKind() == Para_Kind)) { // 是该函数内的变量或者参数
					return item.getOrder();
				}
				else if (item.getItemKind() == Con_Kind) return -3; // 该函数内的常量
			}
		}
	}
	if (!isDefined) 
		return -1;
	return -2;
}

// 无返回值函数return语句检查，true为无返回值函数
bool STManager::checkReturn(string funcName) {
	for (unsigned int i = 0; i < globalSymbolTable.size(); i++) {
		SymbolTableItem item = globalSymbolTable.at(i);
		if (item.getItemKind() == Func_Kind && item.getIdName() == funcName) {
			if (item.getFuncType() != Void_Type) // 不是无返回值函数
				return false;
			return true;
		}
	}
	return false;
}
// 有返回值函数return语句检查,不匹配返回false，匹配返回true
bool STManager::checkReturn(string funcName, VALUE_TYPE retType) {
	for (unsigned int i = 0; i < globalSymbolTable.size(); i++) {
		SymbolTableItem item = globalSymbolTable.at(i);
		if (item.getItemKind() == Func_Kind && item.getIdName() == funcName) {
			if ((item.getFuncType() == ReturnChar_Type && retType == Int_Type) || 
				(item.getFuncType() == ReturnInt_Type && retType  == Char_Type)) { // 函数返回类型与return类型不同
				return false;
			}
			return true;
		}
	}
	return false;
}

VALUE_TYPE STManager::getItemValueType(int order) { // 得到order对应的符号表项值类型
	return globalSymbolTable.at(order).getValueType();
}

/*
//加权----引用计数
void STManager::addWeight(int order, int weight) {
	SymbolTableItem item = globalSymbolTable.at(order);
	if ((item.getItemKind() == Var_Kind || item.getItemKind() == Para_Kind) && item.getFuncName() != "GLOBAL"
		&& item.getArrLen() == 0) {
		globalSymbolTable.at(order).addWeight(weight);
	}
}
*/