#pragma once

#include <iostream>
#include <string>
#include <cctype>
#include <vector>
#include <map>
#include "STManager.h"

using namespace std;

struct Token_Sym { // 类别码和相应单词
	SYM symbol; // 单词类别
	string word; // 单词
	int lines; // 单词所在行数
	Token_Sym() { this->symbol = INEXIST; this->lines = 0; }
};

struct Exp_ret { // 表达式类型
	string name; // 表达式返回的标识符名--->统一规定,表达式都需要有一个返回变量名(临时变量)
	bool surable; // 是否确定值
	VALUE_TYPE type; // 值类型
	int num;
	char ch;
	bool isEmpty; // 是否是空的
	Exp_ret() { this->surable = false;  this->type = Empty_Type; this->isEmpty = true; this->num = -1; this->ch = '\0'; }
};

/*------函数声明------*/
char getNextCh();                       // 获取下一个字符
void backIndex();                       // 退回到上一个字符
bool isAlnum(char ch);                  // 判断是否符合文法里的字母数字，包括下划线
bool isChar(char ch);                   // 判断是否符合文法里的字符，包括下划线
bool isNum(char ch);					 // 判断是否为数字
bool isString(char ch);                 // 判断是否符合文法里字符串中的字符
int isReserve();                        // 判断token中的字符串是保留字还是标识符
int getNum(string);						 // 字符串转数字
void initial();                         // 对字符数组单词符都清空
SYM getSymbol();                        // 获取相应的类型
SYM getPreSymbol();                     // 获取预读类型
string getTokenSymWord();               // 获得当前打印单词
string getWord();						 // 获得当前单词
string getWord(int);					 // 获得index下标处的单词
int getNowSymLines();                   // 获得当前单词所在行数
void backPreSymbol();                   // 回退预读类型
void printSymbol(SYM);                  // 打印识别单词的类别码
void printSyntax(int);                  // 打印识别单词的语法成分
void analysisError(int, int);           // 代码分析错误显示地输出，参数<错误分析单词或语法，代码错误所在行>
void errorHandling(int, ERRORSYM);      // 错误处理，输出<错误所在行，错误类别>

/*------语法分析部分------*/
void syntaxProcedure();                 // ＜程序＞
void syntaxFunc();                      // <函数>
void syntaxcConstDeclare(string);      // ＜常量说明＞
void syntaxConstDefine(string);        // ＜常量定义＞
void constIntDefine(string);           // 扩展迭代-INT常量定义
void constCharDefine(string);          // 扩展迭代-CHAR常量定义
void syntaxVarDeclare(string);         // ＜变量说明＞
bool syntaxVarDefine(SYM, string);     // ＜变量定义＞
void syntaxReFuncDefine(string);       // ＜有返回值函数定义＞
void syntaxNoReFuncDefine();           // ＜无返回值函数定义＞
bool syntaxDeclareHead();              // ＜声明头部＞
void syntaxParamTable(string);         // ＜参数表＞
void syntaxCompStatement(string);      // ＜复合语句＞
void syntaxStatementLine(string, bool, vector<FourYuanItem>&);      // ＜语句列＞
void syntaxStatement(string, bool, vector<FourYuanItem>&);          // <语句>
void syntaxConditionState(string, bool, vector<FourYuanItem>&);     // ＜条件语句＞
string syntaxCondition(string, bool, vector<FourYuanItem>&);          // <条件>
void syntaxLoopState(string, bool, vector<FourYuanItem>&);          // <循环语句>
string syntaxStep();                     // ＜步长＞
bool syntaxCallReFunc(string, bool, vector<FourYuanItem>&);         // <有返回值函数调用语句>
bool syntaxCallNoFunc(string, bool, vector<FourYuanItem>&);         // ＜无返回值函数调用语句＞
vector<VALUE_TYPE> syntaxValueParam(string, bool, vector<FourYuanItem>&);   // ＜值参数表＞
bool syntaxAssignState(string, bool, vector<FourYuanItem>&);        // <赋值语句>
bool syntaxReadState(string, bool, vector<FourYuanItem>&);          // ＜读语句＞
bool syntaxWriteState(string, bool, vector<FourYuanItem>&);         // ＜写语句＞
bool syntaxString();                   // <字符串>
bool syntaxReturnState(string, bool, vector<FourYuanItem>&);        // ＜返回语句＞
Exp_ret syntaxExpre(string, bool, vector<FourYuanItem>&);           // <表达式>
int syntaxTerm(string, vector<PostfixItem>&, bool, vector<FourYuanItem>&);                // <项>
int syntaxFactor(string, vector<PostfixItem>&, bool, vector<FourYuanItem>&);              // <因子>
void syntaxMain();                     // <主函数>
