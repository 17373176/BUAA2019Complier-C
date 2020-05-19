/* SymbolTableItem.cpp文件，对SymbolTableItem类的函数定义
 */

#include "SymbolTableItem.h"
#include <vector>

using namespace std;

int SymbolTableItem::numberCount = 0;

//全局符号表
vector<SymbolTableItem> globalSymbolTable;

SymbolTableItem::SymbolTableItem(string id, string funcName) { // 构造函数的定义
	order = numberCount++;
	idName = id;
	functionName = funcName;
	length = 0; // 默认为0,默认是变量不是数组
	weight = 0;
}