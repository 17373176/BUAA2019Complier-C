/* SyntaxAnalysis.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
 * 本次project包括中间代码四元式生成，MIPS代码生成，识别目标：C语言
 * 增加预处理;_CRT_SECURE_NO_WARNINGS
 * 把测试数据设计文档也打包一起提交
 */

#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include <cstring>
#include <cctype>
#include <map>
#include <vector>
#include <algorithm>
#include "STManager.h"
#include "SyntaxAnalysis.h"
#include "CodeGenerator.h"
#include "op.h"

 /* 头文件的包含关系：STM包含SymbolTableItem.h，SymbolTableItem.h包含ConstEnumDefine.h
  * CodeGenerator.h包含STM
  */

using namespace std;

/*------常量定义------*/
const string syntaxWord[SYNTAX_ELE_NUM] = { // 语法成分数组
	"字符串", "程序", "常量说明", "常量定义", "变量说明", "变量定义", "声明头部",
	"无符号整数", "整数", "有返回值函数定义", "无返回值函数定义", "复合语句",
	"参数表", "主函数", "表达式", "项", "因子", "语句", "赋值语句", "条件语句",
	"条件", "循环语句", "步长", "有返回值函数调用语句", "无返回值函数调用语句",
	"值参数表", "语句列", "读语句", "写语句", "返回语句"
};

const string syntaxError[SYNTAX_ERROR_NUM] = { // 语法错误标识码
	"非法字符串", "非法程序", "非法常量说明", "非法常量定义", "非法变量说明", "非法变量定义", "非法声明头部",
	"非法无符号整数", "非法整数", "非法有返回值函数定义", "非法无返回值函数定义", "非法复合语句",
	"非法参数表", "非法主函数", "非法表达式", "非法项", "非法因子", "非法语句", "非法赋值语句", "非法条件语句",
	"非法条件", "非法循环语句", "非法步长", "非法有返回值函数调用语句", "非法无返回值函数调用语句",
	"非法值参数表", "非法语句列", "非法读语句", "非法写语句", "非法返回语句"
};

const string errorSym_to_o[SYNTAX_ERROR_NUM] = { // 错误类型详细信息输出
	"非法符号或不符合词法", "名字重定义", "未定义的名字", "函数参数个数不匹配", "函数参数类型不匹配", "条件判断中出现不合法的类型", "无返回值的函数存在不匹配的return语句",
	"有返回值的函数缺少return语句或存在不匹配的return语句", "数组元素的下标只能是整型表达式", "不能改变常量的值", "应为分号", "应为右小括号)",
	"应为右中括号]", "do-while应为语句中缺少while", "常量定义中=后面只能是整型或字符型常量", "自己定义错误"
};

/*------全局变量------*/
string token_g;                          // 单词字符串
char textCodes[TEXT_LEN];                // 被处理的文本字符数组
int word_lines = 1;						 // 被处理文本单词所在行数
int inDex;                               // 处理文本的下标
int printWordIndex = 0;                  // 打印单词时使用的下标
int syntaxWordNo = 0;                    // 分析的单词个数
SYM nextSym;                             // 下一个单词的类别码
Token_Sym token_sym[SYNTAX_WORD_NUM];    // 存储识别完成的词法
STManager symbolTableManager;            // 符号表管理器
string reDeclareFuncName;			     // 所在域，声明的函数名称
bool hasReFunc = false;					 // 有返回值函数是否有return语句
int exp_int = 0;						 // 因子的值
char exp_ch = '\0';
bool hasReturn = false;					 // main函数是否有return语句
SYM relationShip = INEXIST;				 // 条件关系

// 代码生成部分
static int labelCount = 0; // 全局标签计数器
int tempVarCount = 0; // 全局临时变量计数器
static string funcName = "GLOBAL"; // 临时变量的所属域,当域发生变化,整个计数器clear
static int globalStrCount = 0; // 全局字符串计数器
vector<FourYuanItem> nullCache; // 空cache

vector<FourYuanItem> globalItmCodeArr; // 中间代码生成数组
vector<string> constStringArr; // 程序需要打印的常量字符串数组,放在data段
map<string, string> strAndLabel; // 字符串与伪指令标签<string, 对应的标签或者字符串>
map<string, unsigned> maxTempOrderMap; // 函数中最大的临时变量编号(数量)<函数名, 编号>
map<string, string> varToRegisterMap; // 变量到寄存器的映射

static unsigned strMemSize = 0; //字符串占据的大小
unsigned int paramSpBegin; //临时参数栈空间指针
unsigned int currentParamSp; //当前参数栈指针的地址
unsigned int returnValueSpace; //函数返回值存放点
unsigned int funcBeginAddr; //整体函数栈的起始地址(即全局数据区域起始地址)

// 优化处理部分
vector<FourYuanItem> op_ItmCodeArr; // 中间代码生成数组
map<string, unsigned> op_maxTempOrderMap;



/*------main函数------*/
int main() {
	FILE* stream;
	if ((stream = freopen("testfile.txt", "r", stdin)) == NULL) return -1;
	//if ((stream = freopen("ItmCode.txt", "w", stdout)) == NULL) return -1;
	//if ((stream = freopen("mips.txt", "w", stdout)) == NULL) return -1;
	char ch;
	while ((ch = getchar()) != EOF)
		textCodes[inDex++] = ch;
	inDex = 0; // 复位
	while (getNextCh() != '\0') {
		backIndex();
		SYM symbol = getSymbol(); // 当前识别的单词类型
		if (symbol >= IDENFR && symbol <= INEXIST) { // 在类型码范围内的单词，包括不符合词法的，之后预读再处理
			token_sym[syntaxWordNo].symbol = symbol;
			token_sym[syntaxWordNo].word = token_g;
			token_sym[syntaxWordNo++].lines = word_lines;
		}
	}
	inDex = 0; // 复位
	/*for (int i = 0; i < syntaxWordNo; i++) {
		cout << token_sym[i].word << ' ' << token_sym[i].lines << endl;
	}*/
	syntaxProcedure(); // 调用语法分析程序
	return 0;
}

/*------函数定义------*/
char getNextCh() { // 获取下一个字符
	return textCodes[inDex++];
}

void backIndex() { // 退回到上一个字符
	inDex--;
}

bool isAlnum(char ch) { // 判断是否符合文法里的字母数字，包括下划线
	if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_')
		return true;
	return false;
}

bool isChar(char ch) { // 判断是否符合文法里的字符，包括下划线
	if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
		|| ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '_')
		return true;
	return false;
}

bool isNum(char ch) {
	return ch >= '0' && ch <= '9';
}

bool isString(char ch) { // 判断是否符合文法里字符串中的字符
	if (ch == 32 || ch == 33 || (ch >= 35 && ch <= 126))
		return true;
	return false;
}

int isReserve() { // 判断token中的字符串是保留字还是标识符
	for (int i = 0; i < KEY_WORD_NUM; i++) {
		if (token_g == wordNameArr[i]) return i + 4; // 返回保留字字符串数组对应到相应的类别码
	}
	return -1;
}

int getNum(string str) { // 字符串转数字
	return stoi(str);
}

void initial() { // 对字符数组单词符都清空
	token_g.clear();
}

void analysisError(int code, int line) { // 显示地输出代码分析出现的错误
	cout << "第 " << line << " 行 " << token_sym[printWordIndex].word << " 单词语法分析出错:" << syntaxError[code] << endl;
}

/*-------词法分析完成--------*/

SYM getSymbol() { // 获取相应的类型
	initial(); //初始化
	char now_char_g = getNextCh(); // 当前读入字符
	while (isspace(now_char_g)) { // 跳过空格、回车，不会影响分词和语句的识别
		if (now_char_g == '\n') word_lines++;
		now_char_g = getNextCh();
	}
	// 标识符 := <字母>{<字母>|<数字>}, 字母 := _|a...z|A...Z
	if (isalpha(now_char_g) || now_char_g == '_') { // 标识符首字母可以是下划线
		while (isAlnum(now_char_g) != 0) { // 符合文法的标识符字母数字下划线
			token_g += now_char_g;
			now_char_g = getNextCh();
		}
		backIndex(); // 不是字母数字时，结束并回退
		int cur = isReserve();
		if (cur != -1) { // 是保留字
			return SYM(cur);
		}
		else  return IDENFR; // 是标识符
	}
	else if (isNum(now_char_g)) {
		while (isNum(now_char_g)) {
			token_g += now_char_g;
			now_char_g = getNextCh();
		}
		backIndex(); // 不是数字时，结束并回退
		if ((token_g[0] != '0' && token_g.size() > 1) || token_g.size() == 1)
			return INTCON;
	}
	else if (now_char_g == '\'') { // 字符常量，左单引号
		now_char_g = getNextCh();
		if (isChar(now_char_g)) {
			token_g += now_char_g;
			now_char_g = getNextCh();
			if (now_char_g == '\'') // 右单引号
				return CHARCON;
			else
				while (now_char_g != '\'') // 把不符合单引号字符的跳过
					now_char_g = getNextCh();
		}
		else
			while (now_char_g != '\'') // 把不符合单引号字符的跳过
				now_char_g = getNextCh();
	}
	else if (now_char_g == '\"') { // 字符串常量，左双引号
		now_char_g = getNextCh();
		while (isString(now_char_g)) {
			token_g += now_char_g;
			now_char_g = getNextCh();
		}
		if (now_char_g == '\"') // 右双引号
			return STRCON;
		else // 遇到字符串里有非法的字符
			while (now_char_g != '\"')
				now_char_g = getNextCh();
	}
	else {
		token_g += now_char_g;
		if (now_char_g == '+') return PLUS;
		else if (now_char_g == '-') return MINU;
		else if (now_char_g == '*') return MULT;
		else if (now_char_g == '/') return DIV;
		else if (now_char_g == '<') { // 逻辑小于，小于等于
			now_char_g = getNextCh();
			if (now_char_g == '=') {
				token_g += now_char_g;
				return LEQ; // 逻辑小于等于
			}
			backIndex(); // 回退
			return LSS; // 逻辑小于
		}
		else if (now_char_g == '>') { // 逻辑大于，大于等于
			now_char_g = getNextCh();
			if (now_char_g == '=') {
				token_g += now_char_g;
				return GEQ; // 逻辑大于等于
			}
			backIndex(); // 回退
			return GRE; // 逻辑大于
		}
		else if (now_char_g == '=') { // 赋值，逻辑等于
			now_char_g = getNextCh();
			if (now_char_g == '=') {
				token_g += now_char_g;
				return EQL; // 逻辑等于
			}
			backIndex(); // 回退
			return ASSIGN; // 赋值
		}
		else if (now_char_g == '!') { // 逻辑不等于
			now_char_g = getNextCh();
			if (now_char_g == '=') {
				token_g += now_char_g;
				return NEQ; // 逻辑不等于
			}
		}
		else if (now_char_g == ';') return SEMICN;
		else if (now_char_g == ',') return COMMA;
		else if (now_char_g == '(') return LPARENT;
		else if (now_char_g == ')') return RPARENT;
		else if (now_char_g == '[') return LBRACK;
		else if (now_char_g == ']') return RBRACK;
		else if (now_char_g == '{') return LBRACE;
		else if (now_char_g == '}') return RBRACE;
	}
	return INEXIST;
}

SYM getPreSymbol() { // 获取预读类型
	if (inDex < syntaxWordNo) {
		if (token_sym[inDex].symbol < INEXIST)
			return token_sym[inDex++].symbol;
		else {
			errorHandling(token_sym[inDex].lines, a_ErrorS); // 不符合词法或非法字符，预读有可能输出多次错误
			inDex++; // 往后跳一个单词
			return JUMP; // 往后读取下一个单词
		}
	}
	return INEXIST;
}

string getTokenSymWord() { // 获得当前打印单词
	return token_sym[printWordIndex].word;
}

string getWord() {
	return token_sym[inDex - 1].word;
}

string getWord(int i) {
	return token_sym[i].word;
}

int getNowSymLines() { // 获得当前单词所在行数，这里由于已经预读了，Index已经+1,所以要减掉1
	return token_sym[inDex - 2].lines;
}

void backPreSymbol() { // 回退预读类型
	inDex--;
}

void printSymbol(SYM symbol) { // 打印识别单词的类别码
	/* cout << SYM_to_str[token_sym[printWordIndex].symbol] << " " << token_sym[printWordIndex].word << endl;
	printWordIndex++; */
}

void printSyntax(int index) { // 打印识别单词的语法成分
	/* cout << '<' << syntaxWord[index] << '>' << endl; */
}

void errorHandling(int textLine, ERRORSYM sym) { // 错误处理<错误在文本中行数，错误类别码>
	cout << textLine << " " << errorSym_to_str[sym] << endl;
	//cout << textLine << " " << errorSym_to_str[sym] << errorSym_to_o[sym] << endl;
	//cout << "第 " << line << " 行 " << token_sym[printWordIndex].word << " 单词语法分析出错:" << syntaxError[code] << endl;
}

/*-------语法分析，符号表、错误处理完成--------*/

// ＜程序＞::=[＜常量说明＞][＜变量说明＞]{＜有返回值函数定义＞|＜无返回值函数定义＞}＜主函数＞
void syntaxProcedure() {
	nextSym = getPreSymbol(); // 获取第一个单词类别码
	syntaxcConstDeclare(GLOBAL_FUNCNAME);// [常量说明]
	syntaxVarDeclare(GLOBAL_FUNCNAME); // [变量说明]
	syntaxFunc();
	// 写入中间代码
	writeItmCodeToFile("ItmCode.txt", globalItmCodeArr, maxTempOrderMap);
	// 写入MIPS汇编代码
	generateMipsCode("oldMips.txt", false);
	// 优化后
	generateOpItmArr(op_ItmCodeArr); // 得到新的中间代码后缀数组
	writeItmCodeToFile("op_ItmCode.txt", op_ItmCodeArr, maxTempOrderMap);
	generateMipsCode("mips.txt", true);
}

// <函数定义>
void syntaxFunc() {
	while (syntaxDeclareHead()) {
		if (nextSym == LPARENT) { // 左括号( 并且不是在复合语句里，复合语句里变量定义没有函数
			printSymbol(nextSym); // 打印左括号
			nextSym = getPreSymbol();
			syntaxReFuncDefine(reDeclareFuncName); // [有返回值函数定义]
		}
	}
	while (nextSym == VOIDTK) syntaxNoReFuncDefine();	// [无返回值函数定义]（非main函数）
	while (nextSym == INTTK || nextSym == CHARTK || nextSym == VOIDTK) syntaxFunc();
}

// ＜常量说明＞::=const＜常量定义＞;{const＜常量定义＞;}
void syntaxcConstDeclare(string funcName) {
	bool conDeclareFlag = false;
	while (nextSym == CONSTTK) { // const
		conDeclareFlag = true; // 程序有常量说明
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		syntaxConstDefine(funcName);
		if (nextSym == SEMICN) {
			printSymbol(SEMICN); // 分号;为每一行结束
			nextSym = getPreSymbol();
		}
		else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
		else errorHandling(getNowSymLines(), k_ErrorS); // 应该为分号
	}
	if (conDeclareFlag) printSyntax(2); // 打印常量说明
}

// ＜常量定义＞::=int＜标识符＞＝＜整数＞{,＜标识符＞＝＜整数＞}|char＜标识符＞＝＜字符＞{,＜标识符＞＝＜字符＞}
void syntaxConstDefine(string funcName) {
	if (nextSym == INTTK) { // int
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		constIntDefine(funcName);
		while (nextSym == COMMA) { // 逗号,
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			constIntDefine(funcName);
		} // 识别完成，还剩下分号
	}
	else if (nextSym == CHARTK) { // char
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		constCharDefine(funcName);
		while (nextSym == COMMA) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			constCharDefine(funcName);
		}
	}
	printSyntax(3); // 打印<常量定义>
}

// 扩展迭代-INT常量定义
void constIntDefine(string funcName) {
	if (nextSym == IDENFR) { // 标识符
		string id = getWord();
		int num = 0; // 后面不一定有正确定义的整型
		bool isMinu = false; // 是否有负号
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == ASSIGN) { // 赋值
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (nextSym == PLUS || nextSym == MINU) { // 整数前可以有符号
				isMinu = (nextSym == MINU); // 有负号
				printSymbol(nextSym);
				nextSym = getPreSymbol();
			}
			if (nextSym == INTCON) { // 整数
				string s = "-" + getWord();
				if (isMinu) num = getNum(s); // 获得带负号整数
				else num = getNum(getWord());
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				printSyntax(7); // <无符号整数>
				printSyntax(8); // <整数>
			}
			else {
				errorHandling(getNowSymLines(), o_ErrorS); // 非法整型常量
				while (nextSym != SEMICN && nextSym != COMMA)
					nextSym = getPreSymbol(); // 跳过非法类型
			}
			if (!symbolTableManager.pushItem(id, funcName, num)) // 把整型常量放入符号表
				errorHandling(getNowSymLines(), b_ErrorS); // 重定义
		}
	}
}

// 扩展迭代-CHAR常量定义
void constCharDefine(string funcName) {
	if (nextSym == IDENFR) { // 标识符
		string id = getWord();
		char ch = '\0'; // 后面不一定有正确定义的字符
		//printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == ASSIGN) { // 赋值
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (nextSym == CHARCON) {
				ch = getWord()[0];
				printSymbol(nextSym); // 字符
				nextSym = getPreSymbol();
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else {
				errorHandling(getNowSymLines(), o_ErrorS); // 字符常量定义应该为字符
				while (nextSym != SEMICN && nextSym != COMMA)
					nextSym = getPreSymbol(); // 跳过非法类型
			}
			if (!symbolTableManager.pushItem(id, funcName, ch)) // 把字符常量放入符号表
				errorHandling(getNowSymLines(), b_ErrorS); // 重定义
		}
	}
}

// 检查是否与有返回值函数定义冲突
void syntaxVarDeclare(string funcName) { // ＜变量说明＞::=＜变量定义＞;{＜变量定义＞;}
	bool varDeclareFlag = false;
	if (funcName != GLOBAL_FUNCNAME) { // 函数里变量定义不会与函数定义冲突
		while (nextSym == INTTK || nextSym == CHARTK) { // 类型标识符
			//printSymbol(nextSym);
			SYM sym = nextSym; // 确认是整型还是字符
			nextSym = getPreSymbol();
			if (syntaxVarDefine(sym, funcName)) { // 变量定义
				varDeclareFlag = true;
				if (nextSym == SEMICN) {
					printSymbol(SEMICN); // 分号;为每一行结束
					nextSym = getPreSymbol(); // 获取下一个单词类别
				}
				else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
				else errorHandling(getNowSymLines(), k_ErrorS); // 应该为分号
			}
		}
	}
	else {
		while (nextSym == INTTK || nextSym == CHARTK) { // 类型标识符
			//printSymbol(nextSym);
			SYM sym = nextSym; // 确认是整型还是字符
			nextSym = getPreSymbol();
			if (syntaxVarDefine(sym, funcName)) { // 变量定义
				varDeclareFlag = true;
				if (nextSym == SEMICN) {
					printSymbol(SEMICN); // 分号;为每一行结束
					nextSym = getPreSymbol(); // 获取下一个单词类别
				}
				else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
				else errorHandling(getNowSymLines(), k_ErrorS); // 应该为分号
			}
			else { // 有返回值函数定义
				backPreSymbol();
				backPreSymbol();
				backPreSymbol();
				nextSym = getPreSymbol();
				break;
			}
		}
	}
	if (varDeclareFlag) printSyntax(4); // 打印变量说明
}

// ＜变量定义＞::=＜类型标识符＞(＜标识符＞|＜标识符＞'['＜无符号整数＞']'){,(＜标识符＞|＜标识符＞'['＜无符号整数＞']')}
bool syntaxVarDefine(SYM sym, string funcName) {
	bool varDefineF = false;
	bool flag = false; // 是否预读输出标记
	VALUE_TYPE value_T = sym == INTTK ? Int_Type : Char_Type;
	if (nextSym == IDENFR) { // 标识符
		printSymbol(nextSym);
		string id = getWord();
		int len = 0;
		nextSym = getPreSymbol();
		if (nextSym == LBRACK) { // 左方括号[
			len = 1; // 数组长度,也可以作为是变量还是数组定义的标志
			varDefineF = true;
			flag = true;
			printSymbol(nextSym);
			printSymbol(nextSym);
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (nextSym == INTCON) { // 无符号整数
				len = getNum(getWord());
				printSymbol(nextSym);
				printSyntax(7);
				nextSym = getPreSymbol();
				if (nextSym == RBRACK) { // 右方括号]
					printSymbol(nextSym);
					nextSym = getPreSymbol();
				}
				else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
				else errorHandling(getNowSymLines(), m_ErrorS); // 非法右方括号
			}
			else errorHandling(getNowSymLines(), i_ErrorS); // 数组下标只能是整数
			if (!symbolTableManager.pushItem(id, funcName, value_T, len)) // 数组定义插入符号表
				errorHandling(getNowSymLines(), b_ErrorS); // 重定义
		}
		if (nextSym != LPARENT) { // 不是左括号'(',如果不是左括号就是变量定义

			if (!varDefineF && !symbolTableManager.pushItem(id, funcName, Var_Kind, value_T)) // 变量定义插入符号表
				errorHandling(getNowSymLines(), b_ErrorS); // 重定义
			varDefineF = true;
			if (nextSym == SEMICN) {
				flag = true;
				printSymbol(nextSym);
				printSymbol(nextSym);
			}
			else if (nextSym == COMMA) {
				if (!flag) {
					printSymbol(nextSym);
					printSymbol(nextSym);
				}
				while (nextSym == COMMA) { // 迭代
					printSymbol(nextSym);
					nextSym = getPreSymbol();
					if (nextSym == IDENFR) { // 标识符
						id = getWord();
						len = 0;
						printSymbol(nextSym);
						nextSym = getPreSymbol();
						if (nextSym == LBRACK) { // 左方括号[
							len = 1; //数组长度
							varDefineF = true;
							printSymbol(nextSym);
							nextSym = getPreSymbol();
							if (nextSym == INTCON) { // 无符号整数
								len = getNum(getWord());
								printSymbol(nextSym);
								printSyntax(7);
							}
							else errorHandling(getNowSymLines(), i_ErrorS); // 数组下标只能是整数
							nextSym = getPreSymbol();// 跳过
							if (nextSym == RBRACK) { // 右方括号]
								printSymbol(nextSym);
								nextSym = getPreSymbol();
							}
							else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
							else errorHandling(getNowSymLines(), m_ErrorS); // 非法右方括号
							if (!symbolTableManager.pushItem(id, funcName, value_T, len)) // 数组定义插入符号表
								errorHandling(getNowSymLines(), b_ErrorS); // 重定义
						} // 可以只有标识符
						if (!len) { // 不是数组,是变量
							if (!symbolTableManager.pushItem(id, funcName, Var_Kind, value_T)) // 变量定义插入符号表
								errorHandling(getNowSymLines(), b_ErrorS); // 重定义
						}
					}
				}
			}
		}
	}
	if (varDefineF) printSyntax(5); // 打印变量定义
	return varDefineF;
}

// 声明头部和左括号(已经被变量说明识别了一次
// ＜有返回值函数定义＞::=＜声明头部＞'('＜参数表＞')' '{'＜复合语句＞'}'
void syntaxReFuncDefine(string funcName) {
	bool funcFlag = false;
	syntaxParamTable(funcName); // <参数表>
	if (nextSym == RPARENT) { // 右括号)
		printSymbol(nextSym);
		nextSym = getPreSymbol();
	}
	else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
	else {
		errorHandling(getNowSymLines(), l_ErrorS);
		while (nextSym != LBRACE)
			nextSym = getPreSymbol(); // 跳过非法符号
	}
	if (nextSym == LBRACE) { // 左大括号{
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		syntaxCompStatement(funcName); // <复合语句>
		if (nextSym == RBRACE) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			funcFlag = true;
		}
		else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
		//else errorHandling(getNowSymLines(), my_ErrorS); // 不等于右大括号}则非法有返回值函数定义
	}
	//else errorHandling(getNowSymLines(), my_ErrorS); // 非法有返回值函数定义
	if (funcFlag) printSyntax(9); // 打印<有返回值函数定义>
}

// ＜声明头部＞::=int＜标识符＞|char＜标识符＞
bool syntaxDeclareHead() {
	if (nextSym == INTTK || nextSym == CHARTK) {
		FUNC_TYPE funcType = nextSym == INTTK ? ReturnInt_Type : ReturnChar_Type;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == IDENFR) { // 标识符
			reDeclareFuncName = getWord();
			if (!symbolTableManager.pushItem(reDeclareFuncName, GLOBAL_FUNCNAME, funcType)) // 把函数定义插入符号表
				errorHandling(getNowSymLines(), b_ErrorS); // 重定义
			FourYuanItem item; // 四元式项
			item.type = FunctionDef;
			item.funcType = funcType;
			item.target = reDeclareFuncName; // 定义的函数名
			globalItmCodeArr.push_back(item); // 插入中间代码数组
			printSymbol(nextSym);
			printSyntax(6); // 打印<声明头部>
			nextSym = getPreSymbol();
			if (nextSym == LPARENT) { // 左括号(
				return true;
			}
		}
	}
	return false;
}

// 区分主函数
// ＜无返回值函数定义＞::=void＜标识符＞'('＜参数表＞')''{'＜复合语句＞'}'
void syntaxNoReFuncDefine() {
	bool noReFuncFlag = false;
	printSymbol(nextSym);
	nextSym = getPreSymbol();
	if (nextSym == MAINTK) syntaxMain(); // 无返回类型的主函数
	else if (nextSym == IDENFR) { // 用户自定义
		string funcName = getWord(); // 获得当前识别的单词
		noReFuncFlag = true;
		symbolTableManager.pushItem(funcName, GLOBAL_FUNCNAME, Void_Type);
		FourYuanItem item; // 四元式项
		item.type = FunctionDef;
		item.funcType = Void_Type;
		item.target = funcName; // 定义的函数名
		globalItmCodeArr.push_back(item); // 插入中间代码数组
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LPARENT) { // 左括号(
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			syntaxParamTable(funcName); // <参数表>
			if (nextSym == RPARENT) { // 右括号)
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				if (nextSym == LBRACE) { // 左大括号{
					printSymbol(nextSym);
					nextSym = getPreSymbol();
					syntaxCompStatement(funcName); // <复合语句>
					if (nextSym == RBRACE) printSymbol(nextSym);
					FourYuanItem item;
					item.type = ReturnEmpty; // 函数末尾自动添加空return
					globalItmCodeArr.push_back(item);
				}
				else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else errorHandling(getNowSymLines(), l_ErrorS);
		}
	}
	if (noReFuncFlag) printSyntax(10); // 打印无返回值函数定义
	nextSym = getPreSymbol();
}

// ＜参数表＞::=＜类型标识符＞＜标识符＞{,＜类型标识符＞＜标识符＞}|＜空＞
void syntaxParamTable(string funcName) {
	while (nextSym == INTTK || nextSym == CHARTK) {
		VALUE_TYPE value_T = nextSym == INTTK ? Int_Type : Char_Type;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == IDENFR) {
			string id = getWord();
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (!symbolTableManager.pushItem(id, funcName, Para_Kind, value_T)) // 把函数参数插入符号表
				errorHandling(getNowSymLines(), b_ErrorS); // 重定义
			FourYuanItem item; // 四元式项
			item.type = ParamDef; // 参数定义
			item.valueType = value_T;
			item.target = id; // 标识符名
			globalItmCodeArr.push_back(item); // 插入中间代码数组
			if (nextSym == COMMA) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
			}
		}
	}
	printSyntax(12); // 打印<参数表>
}

// 复合语句里不会出现函数，变量说明因此也去除函数的考虑
// ＜复合语句＞::=[＜常量说明＞][＜变量说明＞]＜语句列＞
void syntaxCompStatement(string funcName) {
	hasReFunc = false;
	syntaxcConstDeclare(funcName);
	syntaxVarDeclare(funcName); // 此处是函数里面的变量定义，不会再有函数定义出现
	syntaxStatementLine(funcName, false, nullCache);
	printSyntax(11); // 打印<复合语句>
	FUNC_TYPE funcType = symbolTableManager.idCheckInState(funcName);
	if (funcType != Void_Type && hasReFunc == false)
		errorHandling(getNowSymLines(), h_ErrorS); // 有返回值函数缺少return语句
}

// ＜语句列＞::={＜语句＞}
void syntaxStatementLine(string funcName, bool isCache, vector<FourYuanItem>& cache) {
	syntaxStatement(funcName, isCache, cache);
	while (nextSym == IFTK || nextSym == FORTK || nextSym == DOTK || nextSym == WHILETK
		|| nextSym == SCANFTK || nextSym == PRINTFTK || nextSym == IDENFR || nextSym == SEMICN
		|| nextSym == RETURNTK || nextSym == LBRACE) {
		syntaxStatement(funcName, isCache, cache);
	}
	printSyntax(26); // 打印<语句列>
}

// 注意后面的分号，花括号，以及空语句
// <语句>::=<条件语句>|<循环语句>|'{'<语句列>'}'|<有返回值函数调用语句>;|<无返回值函数调用语句>;|<赋值语句>;|＜读语句＞;|＜写语句＞;|＜空＞;|＜返回语句＞;
void syntaxStatement(string funcName, bool isCache, vector<FourYuanItem>& cache) {
	if (nextSym == SEMICN) { // 空语句只有分号
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		printSyntax(17); // 打印<语句>
	}
	else if (nextSym == IFTK) syntaxConditionState(funcName, isCache, cache); // 在函数内部打印<语句>
	else if (nextSym == FORTK || nextSym == DOTK || nextSym == WHILETK) syntaxLoopState(funcName, isCache, cache);
	else if (nextSym == LBRACE) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == IFTK || nextSym == FORTK || nextSym == DOTK || nextSym == WHILETK || nextSym == SCANFTK || nextSym == PRINTFTK || nextSym == IDENFR || nextSym == SEMICN || nextSym == RETURNTK) {
			syntaxStatementLine(funcName, isCache, cache);
		}
		if (nextSym == RBRACE) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			printSyntax(17); // 打印<语句>
		}
		else errorHandling(getNowSymLines(), my_ErrorS);
	}
	else if (nextSym == IDENFR) {
		string id = getWord(); // 获得标识符名称（也许是函数名
		reDeclareFuncName = funcName; // 将全局函数名标识符作为之后需要计算的
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (syntaxCallReFunc(id, isCache, cache)) { // 有返回值函数调用
			if (nextSym == SEMICN) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				printSyntax(17); // 打印<语句>
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else errorHandling(getNowSymLines(), k_ErrorS);
		}
		else if (syntaxCallNoFunc(id, isCache, cache)) { // 无返回值函数调用
			if (nextSym == SEMICN) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				printSyntax(17); // 打印<语句>
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else errorHandling(getNowSymLines(), k_ErrorS);
		}
		else if (syntaxAssignState(id, isCache, cache)) {
			if (nextSym == SEMICN) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				printSyntax(17); // 打印<语句>
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else errorHandling(getNowSymLines(), k_ErrorS);
		}
	}
	else if (syntaxReadState(funcName, isCache, cache)) {
		if (nextSym == SEMICN) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			printSyntax(17); // 打印<语句>
		}
		else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
		else errorHandling(getNowSymLines(), k_ErrorS);
	}
	else if (syntaxWriteState(funcName, isCache, cache)) {
		if (nextSym == SEMICN) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			printSyntax(17); // 打印<语句>
		}
		else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
		else errorHandling(getNowSymLines(), k_ErrorS);
	}
	else if (syntaxReturnState(funcName, isCache, cache)) {
		hasReFunc = true; // 有return 语句
		if (nextSym == SEMICN) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			printSyntax(17); // 打印<语句>
		}
		else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
		else errorHandling(getNowSymLines(), k_ErrorS);
	}
	else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
	else errorHandling(getNowSymLines(), k_ErrorS); // 语句不能啥也没有，应该为分号
}

// ＜条件语句＞::=if'('＜条件＞')'＜语句＞[else＜语句＞]
void syntaxConditionState(string funcName, bool isCache, vector<FourYuanItem>& cache) {
	if (nextSym == IFTK) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		FourYuanItem item;
		string label1 = generateLabel(); // 满足条件顺序执行语句最后跳转到label1，为条件语句全部结尾处
		string label2 = generateLabel(); // 不满足条件跳转到label2，为else语句开始
		string jump; // 判断条件
		if (nextSym == LPARENT) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			jump = syntaxCondition(funcName, isCache, cache);
			if (relationShip == LSS) {
				item.type = BGEZ;
				item.target = label2;
				item.left = jump;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			else if (relationShip == LEQ) {
				item.type = BGZ;
				item.target = label2;
				item.left = jump;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			else if (relationShip == EQL) {
				item.type = BNZ;
				item.target = label2;
				item.left = jump;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			else if (relationShip == NEQ) {
				item.type = BEZ;
				item.target = label2;
				item.left = jump;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			else if (relationShip == GEQ) {
				item.type = BLZ;
				item.target = label2;
				item.left = jump;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			else if (relationShip == GRE) {
				item.type = BLEZ;
				item.target = label2;
				item.left = jump;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else errorHandling(getNowSymLines(), l_ErrorS);
			tempVarCount = 0; // 临时变量先清空
			syntaxStatement(funcName, isCache, cache);
			// 设好无条件跳转
			item.type = Jump;
			item.target = label1; // 执行第一个if语句跳转到条件语句结尾
			if (isCache) cache.push_back(item);
			else globalItmCodeArr.push_back(item);
			item.type = Label; // 设置第一个标签label2在else前
			item.target = label2;
			if (isCache) cache.push_back(item);
			else globalItmCodeArr.push_back(item);
			if (nextSym == ELSETK) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				syntaxStatement(funcName, isCache, cache);
			}
			tempVarCount = 0; // 临时变量清空
			item.type = Label; // 设置第二个跳转标签label1在条件语句结尾
			item.target = label1;
			if (isCache) cache.push_back(item);
			else globalItmCodeArr.push_back(item);
			printSyntax(19); // 打印<条件语句>
			printSyntax(17); // 打印<语句>
		}
		else errorHandling(getNowSymLines(), my_ErrorS);
	}
}

// ＜条件＞::=＜表达式＞＜关系运算符＞＜表达式＞|＜表达式＞ 表达式为0条件为假，否则为真
// ＜关系运算符＞::= <｜<=｜>｜>=｜!=｜==
string syntaxCondition(string funcName, bool isCache, vector<FourYuanItem>& cache) {
	FourYuanItem item;
	string left, right;

	bool conditionFlag = false;
	Exp_ret exp1 = syntaxExpre(funcName, isCache, cache), exp2;
	if (exp1.surable) {
		item.type = AssignState;
		item.isRightArr = item.isTargetArr = false;
		item.target = generateVar();
		item.left = exp1.type == Int_Type ? to_string(exp1.num) : to_string((int)exp1.ch);
		item.op = '+';
		item.right = "0";
		if (isCache) cache.push_back(item);
		else globalItmCodeArr.push_back(item);
		left = item.target;
	}
	else {
		if (exp1.name.size() > 0 && exp1.name.at(0) == 'T') { // 是不确定的值，就用临时变量
			FourYuanItem four;
			four.type = AssignState;
			four.target = generateVar();
			four.isTargetArr = four.isRightArr = false;
			four.left = exp1.name;
			four.op = '+';
			four.right = "0";
			if (isCache) cache.push_back(four);
			else globalItmCodeArr.push_back(four);
			left = four.target;
			item.target = left;
		}
		else {
			left = exp1.name;
			item.target = exp1.name;
		}
	}
	if (!exp1.isEmpty) conditionFlag = true;
	if (nextSym == LSS || nextSym == LEQ || nextSym == GRE || nextSym == GEQ || nextSym == NEQ || nextSym == EQL) {
		relationShip = nextSym;
		conditionFlag = true;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		exp2 = syntaxExpre(funcName, isCache, cache);
		if (exp2.surable) {
			item.type = AssignState;
			item.isRightArr = item.isTargetArr = false;
			item.target = generateVar();
			item.left = exp2.type == Int_Type ? to_string(exp2.num) : to_string((int)exp2.ch);
			item.op = '+';
			item.right = "0";
			if (isCache) cache.push_back(item);
			else globalItmCodeArr.push_back(item);
			right = item.target;
		}
		else {
			if (exp2.name.size() > 0 && exp2.name.at(0) == 'T') { // 是不确定的值，就用临时变量
				FourYuanItem four;
				four.type = AssignState;
				four.target = generateVar();
				four.isTargetArr = four.isRightArr = false;
				four.left = exp2.name;
				four.op = '+';
				four.right = "0";
				if (isCache) cache.push_back(four);
				else globalItmCodeArr.push_back(four);
				right = four.target;
			}
			else right = exp2.name;
		}
		item.target = generateVar();
		item.type = AssignState;
		item.isRightArr = item.isTargetArr = false;
		item.left = left;
		item.right = right;
		item.op = '-';
		if (isCache) cache.push_back(item);
		else globalItmCodeArr.push_back(item);
		tempVarCount = 0;
		return item.target;
	}
	// 判断表达式是否为1或0,当添加只存在一个<表达式>的时候才需要判断
	if (!exp2.isEmpty && exp1.type != Int_Type)
		errorHandling(getNowSymLines(), f_ErrorS); // 条件类型错误
	else if (!exp2.isEmpty)
		if (!(exp1.type == Int_Type && exp2.type == Int_Type)) // 需要判断两个表达式是否都是整型
			errorHandling(getNowSymLines(), f_ErrorS); // 条件类型错误
	if (conditionFlag) printSyntax(20); // 打印<条件>
	relationShip = NEQ; // 只有一个表达式的时候
	tempVarCount = 0;
	return item.target;
}

// <循环语句>::=while'('＜条件＞')'＜语句＞|do＜语句＞while'('＜条件＞')'|for'('＜标识符＞＝＜表达式＞;＜条件＞;＜标识符＞＝＜标识符＞(+|-)＜步长＞')'＜语句＞
void syntaxLoopState(string funcName, bool isCache, vector<FourYuanItem>& cache) {
	bool loopFlag = false;
	FourYuanItem item;
	string label1 = generateLabel(), label2 = generateLabel(),
		label3 = generateLabel(), labelEnd = generateLabel();
	// label1为条件判断前，label2为循环体，label3为条件后，步长前，labelEnd为循环结尾

	if (nextSym == FORTK) {
		loopFlag = true;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LPARENT) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (nextSym == IDENFR) {
				string id = getWord();
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				if (nextSym == ASSIGN) {
					item.type = AssignState;
					item.isTargetArr = item.isRightArr = false; // 默认不是数组
					item.op = '+';
					item.right = "0";
					int checkre = symbolTableManager.checkAssignId(id, funcName);
					if (checkre == -1) errorHandling(getNowSymLines(), c_ErrorS); // 等号左边未定义
					else if (checkre == -3) errorHandling(getNowSymLines(), j_ErrorS); // 常量不能被赋值
					item.target = (checkre >= 0) ? ("G" + to_string(checkre) + id) : id; // G代表标识符变量
					printSymbol(nextSym);
					nextSym = getPreSymbol();
					Exp_ret exp = syntaxExpre(funcName, isCache, cache);
					item.target = (checkre >= 0) ? ("G" + to_string(checkre) + id) : id;
					if (exp.surable) item.left = (exp.type == Int_Type) ? to_string(exp.num) : to_string((int)exp.ch); // 是确定的值
					else {  // 不确定值的表达式
						if (exp.name.size() > 0 && exp.name[0] == 'T') { // 临时变量
							FourYuanItem it; // 单独等号右边表达式的四元式项
							it.type = AssignState;
							it.target = generateVar(); // 生成临时变量器T
							it.isTargetArr = it.isRightArr = false;
							it.left = exp.name;
							it.right = "0";
							it.op = '+';
							if (isCache) cache.push_back(it);
							else globalItmCodeArr.push_back(it);
							item.left = it.target; // 赋值语句
						}
						else item.left = exp.name; // 标识符
					}
					if (isCache) cache.push_back(item);
					else globalItmCodeArr.push_back(item);

					// 设置条件判断前的标签label1
					item.type = Label;
					item.target = label1;
					if (isCache) cache.push_back(item);
					else globalItmCodeArr.push_back(item);
					if (nextSym == SEMICN) {
						printSymbol(nextSym);
						nextSym = getPreSymbol();
						string jump = syntaxCondition(funcName, isCache, cache); // 条件
						if (relationShip == LSS) { // 如果不满足就跳转至标签end，结束循环体
							item.type = BGEZ;
							item.target = labelEnd;
							item.left = jump;
							if (isCache) cache.push_back(item);
							else globalItmCodeArr.push_back(item);
						}
						else if (relationShip == LEQ) {
							item.type = BGZ;
							item.target = labelEnd;
							item.left = jump;
							if (isCache) cache.push_back(item);
							else globalItmCodeArr.push_back(item);
						}
						else if (relationShip == EQL) {
							item.type = BNZ;
							item.target = labelEnd;
							item.left = jump;
							if (isCache) cache.push_back(item);
							else globalItmCodeArr.push_back(item);
						}
						else if (relationShip == NEQ) {
							item.type = BEZ;
							item.target = labelEnd;
							item.left = jump;
							if (isCache) cache.push_back(item);
							else globalItmCodeArr.push_back(item);
						}
						else if (relationShip == GEQ) {
							item.type = BLZ;
							item.target = labelEnd;
							item.left = jump;
							if (isCache) cache.push_back(item);
							else globalItmCodeArr.push_back(item);
						}
						else if (relationShip == GRE) {
							item.type = BLEZ;
							item.target = labelEnd;
							item.left = jump;
							if (isCache) cache.push_back(item);
							else globalItmCodeArr.push_back(item);
						}
						if (nextSym == SEMICN) { // 分析步长，赋值语句
							FourYuanItem stepItem;
							printSymbol(nextSym);
							nextSym = getPreSymbol();
							if (nextSym == IDENFR) {
								id = getWord();
								printSymbol(nextSym);
								nextSym = getPreSymbol();
								if (nextSym == ASSIGN) {
									stepItem.type = AssignState;
									stepItem.isTargetArr = stepItem.isRightArr = false; // 默认不是数组
									checkre = symbolTableManager.checkAssignId(id, funcName);
									if (checkre == -1) errorHandling(getNowSymLines(), c_ErrorS); // 等号左边未定义
									else if (checkre == -3) errorHandling(getNowSymLines(), j_ErrorS); // 常量不能被赋值
									stepItem.target = (checkre >= 0) ? ("G" + to_string(checkre) + id) : id; // G代表标识符变量
									printSymbol(nextSym);
									nextSym = getPreSymbol();
									if (nextSym == IDENFR) {
										id = getWord();
										checkre = symbolTableManager.checkAssignId(id, funcName);
										if (checkre == -1) errorHandling(getNowSymLines(), c_ErrorS); // 等号左边未定义
										else if (checkre == -3) errorHandling(getNowSymLines(), j_ErrorS); // 常量不能被赋值
										stepItem.left = (checkre >= 0) ? ("G" + to_string(checkre) + id) : id; // G代表标识符变量
										printSymbol(nextSym);
										nextSym = getPreSymbol();
										if (nextSym == PLUS || nextSym == MINU) {
											stepItem.op = nextSym == PLUS ? '+' : '-';
											printSymbol(nextSym);
											nextSym = getPreSymbol();
											string integer = syntaxStep();
											stepItem.right = integer; // 步长
											/* 此时不加入中间代码，把步长赋值语句放到循环语句后面
											if (isCache) cache.push_back(stepItem);
											else globalItmCodeArr.push_back(stepItem);
											*/
											if (nextSym == RPARENT) {
												printSymbol(nextSym);
												nextSym = getPreSymbol();
												syntaxStatement(funcName, isCache, cache);
												// 把步长加入中间代码
												if (isCache) cache.push_back(stepItem);
												else globalItmCodeArr.push_back(stepItem);

												// 跳到循环开头
												item.type = Jump;
												item.target = label1; // 跳转到循环开头条件判断
												item.left = jump;
												if (isCache) cache.push_back(item);
												else globalItmCodeArr.push_back(item);
												
												// 产生label end loop，在循环体结束后的标签
												item.type = Label; // 用作结束循环
												item.target = labelEnd;
												if (isCache) cache.push_back(item);
												else globalItmCodeArr.push_back(item);
												if (loopFlag) {
													printSyntax(21); // 打印<循环语句>
													printSyntax(17); // 打印<语句>
												}
											}
											else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
											else errorHandling(getNowSymLines(), l_ErrorS);
										}
									}
								}
							}
						}
						else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
						else errorHandling(getNowSymLines(), k_ErrorS); // 应该为分号
					}
					else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
					else errorHandling(getNowSymLines(), k_ErrorS); // 应该为分号
				}
			}
		}
	}
	else if (nextSym == WHILETK) {
		item.type = Label;
		item.target = label1;
		if (isCache) cache.push_back(item);
		else globalItmCodeArr.push_back(item);
		loopFlag = true;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LPARENT) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			string jump = syntaxCondition(funcName, isCache, cache); // 条件跳转
			if (relationShip == LSS) {
				item.type = BGEZ;
				item.target = label2;  // 标签2为结束循环
				item.left = jump;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			else if (relationShip == LEQ) {
				item.type = BGZ;
				item.target = label2;
				item.left = jump;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			else if (relationShip == EQL) {
				item.type = BNZ;
				item.target = label2;
				item.left = jump;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			else if (relationShip == NEQ) {
				item.type = BEZ;
				item.target = label2;
				item.left = jump;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			else if (relationShip == GEQ) {
				item.type = BLZ;
				item.target = label2;
				item.left = jump;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			else if (relationShip == GRE) {
				item.type = BLEZ;
				item.target = label2;
				item.left = jump;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				syntaxStatement(funcName, isCache, cache);
				// 执行完循环体跳回判断条件
				item.type = Jump;
				item.target = label1; // 标签1条件判断
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
				if (loopFlag) {
					printSyntax(21); // 打印<循环语句>
					printSyntax(17); // 打印<语句>
				}
				// 产生label2作为循环体结尾
				item.type = Label;
				item.target = label2; // 标签2
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else errorHandling(getNowSymLines(), l_ErrorS);
		}
	}
	else if (nextSym == DOTK) {
		item.type = Label; // 循环开头标签
		item.target = label1;
		if (isCache) cache.push_back(item);
		else globalItmCodeArr.push_back(item);
		loopFlag = true;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		syntaxStatement(funcName, isCache, cache); // 告诉它这是do循环的while
		if (nextSym == WHILETK) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
		}
		else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
		else {
			errorHandling(getNowSymLines(), n_ErrorS); // do_while 少了while
			while (nextSym != LPARENT) nextSym = getPreSymbol();
		}
		if (nextSym == LPARENT) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			string jump = syntaxCondition(funcName, isCache, cache);
			if (relationShip == LSS) {
				item.type = BLZ;
				item.target = label1; // 这里是标签1，且满足条件跳转，否则结束
				item.left = jump;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			else if (relationShip == LEQ) {
				item.type = BLEZ;
				item.target = label2;
				item.left = jump;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			else if (relationShip == EQL) {
				item.type = BEZ;
				item.target = label2;
				item.left = jump;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			else if (relationShip == NEQ) {
				item.type = BNZ;
				item.target = label2;
				item.left = jump;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			else if (relationShip == GEQ) {
				item.type = BGEZ;
				item.target = label2;
				item.left = jump;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			else if (relationShip == GRE) {
				item.type = BGZ;
				item.target = label2;
				item.left = jump;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				if (loopFlag) {
					printSyntax(21); // 打印<循环语句>
					printSyntax(17); // 打印<语句>
				}
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else errorHandling(getNowSymLines(), l_ErrorS);
		}
	} // 其他语句情况
}

// ＜步长＞::=＜无符号整数＞
string syntaxStep() {
	if (nextSym == INTCON) {
		string integer = getWord();
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		printSyntax(7); // <无符号整数>
		printSyntax(22); // 打印<步长>
		return integer;
	}
	//else errorHandling(getNowSymLines(), my_ErrorS);
}

// ＜有返回值函数调用语句＞::=＜标识符＞'('＜值参数表＞')'  
bool syntaxCallReFunc(string funcName, bool isCache, vector<FourYuanItem>& cache) {
	if (nextSym == LPARENT) {
		FUNC_TYPE type = symbolTableManager.idCheckInState(funcName);
		if (type != Void_Type) { // 有返回值的函数调用FourYuanItem item;
			FourYuanItem item;
			item.type = FunctionCall;
			item.target = funcName; // 本句调用的函数名
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			vector<VALUE_TYPE> retParamTable = syntaxValueParam(reDeclareFuncName, isCache, cache);
			int reCheck = symbolTableManager.funcCallCheck(funcName, true, retParamTable);
			if (reCheck == 1)
				errorHandling(getNowSymLines(), d_ErrorS); // 参数个数不匹配
			else if (reCheck == 2)
				errorHandling(getNowSymLines(), e_ErrorS); // 参数类型不匹配
			if (isCache) cache.push_back(item);
			else globalItmCodeArr.push_back(item);
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				printSyntax(23); // 打印＜有返回值函数调用语句＞
				return true;
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else {
				errorHandling(getNowSymLines(), l_ErrorS);
				while (nextSym != SEMICN) nextSym = getPreSymbol();
			}
			if (type == NotDefine_Func)
				errorHandling(getNowSymLines(), c_ErrorS); // 未定义
		} // 也有可能是无返回值调用
	}
	return false;
}

// ＜无返回值函数调用语句＞::=＜标识符＞'('＜值参数表＞')'
bool syntaxCallNoFunc(string funcName, bool isCache, vector<FourYuanItem>& cache) {
	if (nextSym == LPARENT) {
		FUNC_TYPE type = symbolTableManager.idCheckInState(funcName);
		if (type != NotDefine_Func) { // 函数调用
			FourYuanItem item;
			item.type = FunctionCall;
			item.target = funcName; // 调用的函数名
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			vector<VALUE_TYPE> retParamTable = syntaxValueParam(reDeclareFuncName, isCache, cache);
			int reCheck = symbolTableManager.funcCallCheck(funcName, false, retParamTable);
			if (reCheck == 1)
				errorHandling(getNowSymLines(), d_ErrorS); // 参数个数不匹配
			else if (reCheck == 2)
				errorHandling(getNowSymLines(), e_ErrorS); // 参数类型不匹配
			if (isCache) cache.push_back(item);
			else globalItmCodeArr.push_back(item);
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				printSyntax(24); // 打印＜无返回值函数调用语句＞
				return true;
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else {
				errorHandling(getNowSymLines(), l_ErrorS);
				while (nextSym != SEMICN) nextSym = getPreSymbol();
			}
			if (type == NotDefine_Func)
				errorHandling(getNowSymLines(), c_ErrorS); // 未定义
		}
	}
	return false;
}

// ＜值参数表＞::=＜表达式＞{ ,＜表达式＞ }｜＜空＞
vector<VALUE_TYPE> syntaxValueParam(string funcName, bool isCache, vector<FourYuanItem>& cache) {
	vector<string> paramTable; // 值参临时变量名
	vector<VALUE_TYPE> retParamTable; // 值参类型
	Exp_ret retPara = syntaxExpre(funcName, isCache, cache); // 得到当前值参数类型
	FourYuanItem item;
	if (!retPara.isEmpty) { // 值参表可以为空
		retParamTable.push_back(retPara.type);
		if (retPara.surable) {
			item.type = AssignState;
			item.target = generateVar(); // 生成临时变量器T
			item.isTargetArr = item.isRightArr = false;
			item.left = (retPara.type == Int_Type) ? to_string(retPara.num) : to_string((int)retPara.ch); // 是确定的值
			item.right = "0";
			item.op = '+';
			if (isCache) cache.push_back(item);
			else globalItmCodeArr.push_back(item);
			paramTable.push_back(item.target);
		}
		else {
			if (retPara.name.size() > 0 && retPara.name.at(0) == 'T') { // 临时变量
				FourYuanItem it;
				it.type = AssignState;
				it.target = generateVar(); // 生成临时变量器T
				it.isTargetArr = it.isRightArr = false;
				it.left = retPara.name;
				it.op = '+';
				it.right = "0";
				if (isCache) cache.push_back(it);
				else globalItmCodeArr.push_back(it);
				paramTable.push_back(it.target);
			}
			else paramTable.push_back(retPara.name); // 标识符名
		}
		while (nextSym == COMMA) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			retPara = syntaxExpre(funcName, isCache, cache);
			retParamTable.push_back(retPara.type);
			if (retPara.surable) {
				item.type = AssignState;
				item.target = generateVar(); // 生成临时变量器T
				item.isTargetArr = item.isRightArr = false;
				item.left = (retPara.type == Int_Type) ? to_string(retPara.num) : to_string((int)retPara.ch); // 是确定的值
				item.right = "0";
				item.op = '+';
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
				paramTable.push_back(item.target);
			}
			else {
				if (retPara.name.size() > 0 && retPara.name.at(0) == 'T') { // 临时变量
					FourYuanItem it;
					it.type = AssignState;
					it.target = generateVar(); // 生成临时变量器T
					it.isTargetArr = it.isRightArr = false;
					it.left = retPara.name;
					it.op = '+';
					it.right = "0";
					if (isCache) cache.push_back(it);
					else globalItmCodeArr.push_back(it);
					paramTable.push_back(it.target);
				}
				else paramTable.push_back(retPara.name); // 标识符名
			}
		}
	}
	for (unsigned int i = 0; i < paramTable.size(); i++) {
		item.type = ValueParamDeliver;
		item.target = paramTable.at(i);
		if (isCache) cache.push_back(item);
		else globalItmCodeArr.push_back(item);
	}
	printSyntax(25); // 打印<值参数表>
	return retParamTable;
}

// <赋值语句>::=(＜标识符＞＝＜表达式＞)|(＜标识符＞'['＜表达式＞']'=＜表达式＞)
bool syntaxAssignState(string funcName, bool isCache, vector<FourYuanItem>& cache) {
	string id = funcName; // 这是标识符的名称
	FourYuanItem item; // 赋值语句的四元式项
	item.type = AssignState;
	item.isTargetArr = item.isRightArr = false; // 默认不是数组
	item.op = '+';
	item.right = "0";
	if (nextSym == ASSIGN) {
		int checkre = symbolTableManager.checkAssignId(id, reDeclareFuncName);
		if (checkre == -1) errorHandling(getNowSymLines(), c_ErrorS); // 等号左边未定义
		else if (checkre == -3) errorHandling(getNowSymLines(), j_ErrorS); // 常量不能被赋值
		item.target = (checkre >= 0) ? ("G" + to_string(checkre) + id) : id; // G代表标识符变量
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		Exp_ret exp = syntaxExpre(reDeclareFuncName, isCache, cache);
		item.target = (checkre >= 0) ? ("G" + to_string(checkre) + id) : id;
		//if (checkre >= 0 && symbolTableManager.getItemValueType(checkre) == exp.type) { // 赋值类型匹配

		if (exp.surable) item.left = (exp.type == Int_Type) ? to_string(exp.num) : to_string((int)exp.ch); // 是确定的值
		else {  // 不确定值的表达式
			if (exp.name.size() > 0 && exp.name[0] == 'T') { // 临时变量
				FourYuanItem it; // 单独等号右边表达式的四元式项
				it.type = AssignState;
				it.target = generateVar(); // 生成临时变量器T
				it.isTargetArr = it.isRightArr = false;
				it.left = exp.name;
				it.right = "0";
				it.op = '+';
				if (isCache) cache.push_back(it);
				else globalItmCodeArr.push_back(it);
				item.left = it.target; // 赋值语句
			}
			else item.left = exp.name; // 标识符
		}
		if (isCache) cache.push_back(item);
		else globalItmCodeArr.push_back(item);
		printSyntax(18); // 打印<赋值语句>
		return true;
	}
	else if (nextSym == LBRACK) { // 数组
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		Exp_ret exp = syntaxExpre(reDeclareFuncName, isCache, cache); // 下标表达式
		int order;
		item.isTargetArr = true; // 左边是数组
		if (exp.surable) {
			if (exp.type == Char_Type) {
				order = symbolTableManager.idArrExpCheck(id, reDeclareFuncName, true, exp.ch);
				item.index1 = "" + to_string(int(exp.ch));
			}
			else {
				order = symbolTableManager.idArrExpCheck(id, reDeclareFuncName, true, exp.num);
				item.index1 = "" + to_string(exp.num);
			}
		}
		else {
			order = symbolTableManager.idArrExpCheck(id, reDeclareFuncName, false);
			if (exp.name.size() > 0 && exp.name[0] == 'T') { // 临时变量
				FourYuanItem it; // 单独等号右边表达式的四元式项
				it.type = AssignState;
				it.target = generateVar(); // 生成临时变量器T
				it.isTargetArr = it.isRightArr = false;
				it.left = exp.name;
				it.right = "0";
				it.op = '+';
				if (isCache) cache.push_back(it);
				else globalItmCodeArr.push_back(it);
				item.index1 = it.target; // 下标更换
			}
			else item.index1 = exp.name; //下标 标识符
		}
		if (order >= 0) item.target = "G" + to_string(order) + id; // G代表标识符变量
		if (exp.type != Int_Type) errorHandling(getNowSymLines(), i_ErrorS); // 下标表达式要为整型
		if (order == -1) errorHandling(getNowSymLines(), c_ErrorS); // 数组未定义
		if (nextSym == RBRACK) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (nextSym == ASSIGN) { // =
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				Exp_ret exp2 = syntaxExpre(reDeclareFuncName, isCache, cache); // 赋值右边表达式
				if (exp2.surable) {
					item.left = (exp2.type == Int_Type) ? to_string(exp2.num) : to_string((int)exp2.ch); // 是确定的值
				}
				else {
					order = symbolTableManager.idArrExpCheck(id, reDeclareFuncName, false);
					if (exp2.name.size() > 0 && exp2.name[0] == 'T') { // 临时变量
						FourYuanItem it; // 单独等号右边表达式的四元式项
						it.type = AssignState;
						it.target = generateVar(); // 生成临时变量器T
						it.isTargetArr = it.isRightArr = false;
						it.left = exp2.name;
						it.right = "0";
						it.op = '+';
						if (isCache) cache.push_back(it);
						else globalItmCodeArr.push_back(it);
						item.left = it.target; // 下标更换
					}
					else item.left = exp2.name; //下标 标识符
				}
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
				printSyntax(18); // 打印<赋值语句>
				return true;
			}
		}
		else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
		else errorHandling(getNowSymLines(), m_ErrorS); // 应为右方括号
	}
	return false;
}

// ＜读语句＞::=scanf'('＜标识符＞{ ,＜标识符＞ }')'
bool syntaxReadState(string funcName, bool isCache, vector<FourYuanItem>& cache) {
	if (nextSym == SCANFTK) { // scanf
		FourYuanItem item;
		int order = -1; // 标识符在符号表中的下标
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LPARENT) { // (
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (nextSym == IDENFR) { // 标识符
				string id = getWord();
				order = symbolTableManager.checkAssignId(id, funcName);
				if (order >= 0) {
					item.type = (symbolTableManager.getItemValueType(order) == Int_Type) ? ReadInt : ReadChar;
					item.target = "G" + to_string(order) + id; // 符号表下标用于区分
					if (isCache) cache.push_back(item);
					else globalItmCodeArr.push_back(item);
				}
				else errorHandling(getNowSymLines(), c_ErrorS); // 未定义
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				while (nextSym == COMMA) { // , 循环继续
					printSymbol(nextSym);
					nextSym = getPreSymbol();
					if (nextSym == IDENFR) { // 标识符
						string id = getWord();
						order = symbolTableManager.checkAssignId(id, funcName);
						if (order >= 0) {
							item.type = (symbolTableManager.getItemValueType(order) == Int_Type) ? ReadInt : ReadChar;
							item.target = "G" + to_string(order) + id;
							if (isCache) cache.push_back(item);
							else globalItmCodeArr.push_back(item);
						}
						printSymbol(nextSym);
						nextSym = getPreSymbol();
					}
				}
				if (nextSym == RPARENT) { // 结束，最后是右括号)
					printSymbol(nextSym);
					nextSym = getPreSymbol();
					printSyntax(27); // 打印<读语句>
					return true;
				}
				else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
				else {
					errorHandling(getNowSymLines(), l_ErrorS);
					while (nextSym != SEMICN) nextSym = getPreSymbol();
				}
			}
		}
	}
	return false;
}

// <写语句>::=printf '('<字符串>,<表达式> ')'|printf'('<字符串>')'|printf'('<表达式>')'
bool syntaxWriteState(string funcName, bool isCache, vector<FourYuanItem>& cache) {
	if (nextSym == PRINTFTK) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		FourYuanItem item;
		int tempCount = tempVarCount; // 临时变量计数器，需要保存，表达式执行结束后恢复
		if (nextSym == LPARENT) { // (
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			Exp_ret exp = syntaxExpre(funcName, isCache, cache);
			if (!exp.isEmpty) { // 只识别表达式即可
				if (exp.surable) { // 确定的值,但表达式值不可能是str
					item.type = (exp.type == Int_Type) ? PrintInt : PrintChar;
					string str = "";
					str += exp.ch;
					item.target = (exp.type == Int_Type) ? to_string(exp.num) : str;
				}
				else {
					item.type = PrintId;
					item.target = exp.name;
					item.isPrintCharId = (exp.type == Char_Type) ? true : false; // 打印的是不确定值的标识符，且为char型id
				}
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
			}
			else if (syntaxString()) {
				string str = getWord(inDex - 2); // 由于预读往后延了两个单词
				item.type = PrintStr; // 打印字符串
				item.target = str;
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
				if (nextSym == COMMA) { // 打印有表达式
					printSymbol(nextSym);
					nextSym = getPreSymbol();
					exp = syntaxExpre(funcName, isCache, cache);
					if (exp.surable) { // 确定的值,但表达式值不可能是str
						item.type = (exp.type == Int_Type) ? PrintInt : PrintChar;
						string str = "";
						str += exp.ch;
						item.target = (exp.type == Int_Type) ? to_string(exp.num) : str;
					}
					else {
						item.type = PrintId;
						item.target = exp.name;
						item.isPrintCharId = (exp.type == Char_Type) ? true : false; // 打印的是不确定值的标识符，且为char型id
					}
					if (isCache) cache.push_back(item);
					else globalItmCodeArr.push_back(item);
				}
			}
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				printSyntax(28); // 打印<写语句>
				item.type = PrintChar;
				item.target = "\n"; // 每一个printf都打印换行
				if (isCache) cache.push_back(item);
				else globalItmCodeArr.push_back(item);
				tempVarCount = tempCount; // 恢复
				return true;
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else {
				errorHandling(getNowSymLines(), l_ErrorS);
				while (nextSym != SEMICN) nextSym = getPreSymbol();
			}
		}
	}
	return false;
}

bool syntaxString() { // <字符串>
	if (nextSym == STRCON) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		return true;
	}
	return false;
}

// ＜返回语句＞::=return['('＜表达式＞')']   
bool syntaxReturnState(string funcName, bool isCache, vector<FourYuanItem>& cache) {
	if (nextSym == RETURNTK) {
		hasReFunc = true; // 有return语句
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		FourYuanItem item;
		int tempCount = tempVarCount; // 临时变量计数器，需要保存，表达式执行结束后恢复
		bool has_ret = false; // 是否有返回值
		if (nextSym == LPARENT) {
			has_ret = true;
			VALUE_TYPE value_T = Int_Type; // 默认
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			Exp_ret exp = syntaxExpre(funcName, isCache, cache);
			value_T = exp.type; // 表达式值类型
			if (exp.surable) {
				if (exp.type == Int_Type) {
					item.type = ReturnInt;
					item.target = to_string(exp.num); // 将整型数值转化为string作为返回结果
				}
				else {
					item.type = ReturnChar;
					item.target.insert(0, 1, exp.ch);
				}
			}
			else {
				item.type = ReturnId;
				item.target = exp.name; // 表达式返回的标识符名
			}
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else {
				errorHandling(getNowSymLines(), l_ErrorS);
				while (nextSym != SEMICN) nextSym = getPreSymbol();
			}
			if (!symbolTableManager.checkReturn(funcName, value_T))
				errorHandling(getNowSymLines(), h_ErrorS); // 有返回值函数存在不匹配return语句
			if (symbolTableManager.idCheckInState(funcName) == Void_Type) // 如果是void函数，且return语句有表达式
				errorHandling(getNowSymLines(), g_ErrorS); // 无返回值函数存在不匹配return语句

		} // 可以没有括号表达式
		if (!has_ret) {// 如果没有括号表达式则无返回值
			if (funcName == "main") { // 主函数遇到return语句直接结束
				item.type = EndProcedure;
				hasReturn = true;
			}
			else item.type = ReturnEmpty;
		}
		printSyntax(29); // 打印<返回语句>
		if (isCache) cache.push_back(item);
		else globalItmCodeArr.push_back(item);
		tempVarCount = tempCount; // 恢复
		return true;
	}
	return false;
}

// <表达式>::=[+-]＜项＞{＜加法运算符＞＜项＞} [+|-]只作用于第一个<项>
// ＜加法运算符＞::= +｜-
Exp_ret syntaxExpre(string funcName, bool isCache, vector<FourYuanItem>& cache) {
	int returnFlag = -1; // 1代表整型2代表字符
	// 确定表达式值类型
	Exp_ret exp;
	bool sure = false; // 是否是确定值
	int expResult = 0; // 表达式值
	VALUE_TYPE type; // 值类型
	vector<PostfixItem> tar, obj; // 将表达式存入tar，转变为后缀obj，再计算
	if (nextSym == PLUS || nextSym == MINU) { // 可以有也可以没有
		PostfixItem item;
		item.type = Char_Type;
		item.number = (nextSym == PLUS) ? '+' : '-'; // 正负号
		item.isOperator = true;
		tar.push_back(item);
		printSymbol(nextSym);
		nextSym = getPreSymbol();
	}
	returnFlag = syntaxTerm(funcName, tar, isCache, cache);
	while (nextSym == PLUS || nextSym == MINU) {
		PostfixItem item;
		item.type = Char_Type;
		item.number = (nextSym == PLUS) ? '+' : '-'; // 正负号
		item.isOperator = true;
		tar.push_back(item);
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		returnFlag = syntaxTerm(funcName, tar, isCache, cache);
	}
	if (returnFlag != -1) printSyntax(14); // 打印<表达式>
	if (tar.size() == 0) exp.isEmpty = true;
	else exp.isEmpty = false;
	turnToPostfixExp(tar, obj); // 将表达式转化为后缀表达式obj进行计算
	exp.name = calculateExp(obj, sure, type, expResult, isCache, cache, funcName);
	exp.surable = sure;
	exp.type = type;
	if (sure) {
		if (type == Int_Type) exp.num = expResult;
		else exp.ch = expResult;
	}

	return exp;
}

// <项>::=＜因子＞{＜乘法运算符＞＜因子＞} 返回1代表整数，2代表字符,-1代表没有
// <乘法运算符>::= *｜/ 
int syntaxTerm(string funcName, vector<PostfixItem>& obj, bool isCache, vector<FourYuanItem>& cache) {
	int termFlag = syntaxFactor(funcName, obj, isCache, cache);
	if (termFlag >= 0) {
		while (nextSym == MULT || nextSym == DIV) {
			PostfixItem item;
			item.type = Char_Type;
			item.number = (nextSym == MULT) ? '*' : '/';
			item.isOperator = true;
			obj.push_back(item);
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			termFlag = syntaxFactor(funcName, obj, isCache, cache);
		}
		printSyntax(15); // 打印<项>
	}
	return termFlag;
}

// <因子>::=<标识符>|<标识符>'['<表达式>']'|'('<表达式>')'|<整数>|<字符>|<有返回值函数调用语句> 
// <标识符>和<有返回值函数调用>存在冲突,返回1代表整数，2代表字符
int syntaxFactor(string funcName, vector<PostfixItem>& obj, bool isCache, vector<FourYuanItem>& cache) {
	vector<SymbolTableItem> globalSymbolTable = symbolTableManager.getTable();
	int factorFlag = -1;
	PostfixItem item1;
	FourYuanItem item2;
	if (nextSym == IDENFR) {
		string id = getWord();
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LBRACK) { // 是数组
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			Exp_ret exp = syntaxExpre(funcName, isCache, cache);
			int index1 = exp.num;
			if (exp.type != Int_Type) errorHandling(getNowSymLines(), i_ErrorS); // 下标表达式要为整型
			factorFlag = symbolTableManager.idArrExpCheck(id, funcName, true, index1); // 临时获得符号表项号
			if (factorFlag == -1) errorHandling(getNowSymLines(), c_ErrorS); // 未定义
			//else if (factorFlag == -2) errorHandling(getNowSymLines(), my_ErrorS); // 越界
			//else factorFlag = symbolTableManager.getItemValueType(factorFlag); // 得到数组值类型
			int orderx;
			string index2;
			if (exp.surable) {
				item2.type = AssignState;
				item2.target = generateVar();
				item2.isRightArr = false;
				item2.isTargetArr = false;
				if (exp.type == Char_Type) {
					orderx = symbolTableManager.idArrExpCheck(id, funcName, true, exp.ch);
					item2.left.insert(0, 1, exp.ch);
					item2.op = '+';
					item2.right = "0";
				}
				else {
					orderx = symbolTableManager.idArrExpCheck(id, funcName, true, exp.num);
					item2.left = "" + to_string(exp.num);
					item2.op = '+';
					item2.right = "0";
				}
				if (isCache) cache.push_back(item2);
				else globalItmCodeArr.push_back(item2);

				index2 = item2.target;
			}
			else {
				orderx = symbolTableManager.idArrExpCheck(id, funcName, false);
				if (exp.name.size() > 0 && exp.name[0] == 'T') {
					item2.type = AssignState;
					item2.target = generateVar();
					item2.isTargetArr = item2.isRightArr = false;
					item2.left = exp.name;
					item2.op = '+';
					item2.right = "0";
					if (isCache) cache.push_back(item2);
					else globalItmCodeArr.push_back(item2);

					index2 = item2.target;
				}
				else {
					index2 = exp.name;
				}
			}
			if (orderx >= 0) {
				item2.type = AssignState;
				item2.target = generateVar();
				item2.isRightArr = true;
				item2.isTargetArr = false;
				item2.left = "G" + to_string(orderx) + id;
				item2.index2 = index2;
				if (isCache) cache.push_back(item2);
				else globalItmCodeArr.push_back(item2);

				item1.type = String_Type;
				SymbolTableItem t = globalSymbolTable.at(orderx);
				item1.isCharVar = false;
				if (t.getValueType() == Char_Type)
					item1.isCharVar = true;
				item1.str = item2.target;
				obj.push_back(item1);
			}
			else {
				item1.type = String_Type;
				item1.str = id;
			}

			if (nextSym == RBRACK) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else errorHandling(getNowSymLines(), m_ErrorS); // 应为右方括号
		}
		else if (nextSym == LPARENT) { // 有返回值函数调用
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			vector<VALUE_TYPE> retParamTable = syntaxValueParam(funcName, isCache, cache);
			factorFlag = 1;
			int reCheck = symbolTableManager.funcCallCheck(funcName, true, retParamTable);
			if (reCheck == 1)
				errorHandling(getNowSymLines(), d_ErrorS); // 参数个数不匹配
			else if (reCheck == 2)
				errorHandling(getNowSymLines(), e_ErrorS); // 参数类型不匹配
			item2.type = FunctionCall;
			item2.target = id;
			if (isCache) cache.push_back(item2);
			else globalItmCodeArr.push_back(item2);
			item2.type = AssignState;
			item2.target = generateVar();
			item2.isTargetArr = item2.isRightArr = false;
			item2.left = "Ret";
			item2.op = '+';
			item2.right = "0";
			if (isCache) cache.push_back(item2);
			else globalItmCodeArr.push_back(item2);
			// 压入中间结果栈
			item1.type = String_Type;
			item1.isCharVar = false;
			if (symbolTableManager.idCheckInState(id) == ReturnChar_Type) // 这里的返回类型需要判断是否是char
				item1.isCharVar = true;
			item1.str = item2.target;
			obj.push_back(item1);
			if (nextSym == RPARENT) { // 右大括号
				printSymbol(nextSym);
				nextSym = getPreSymbol();
			}
		}
		if (factorFlag == -1) { // 不是数组也不是有返回值带参数函数调用,就是简单变量或常量或者函数参数
			factorFlag = symbolTableManager.idCheckInFactor(id, funcName);
			if (factorFlag == -1) errorHandling(getNowSymLines(), c_ErrorS); // 未定义
			if (factorFlag >= 0) {
				SymbolTableItem y = globalSymbolTable.at(factorFlag);
				if (y.getItemKind() == Con_Kind) { // 常量
					item1.type = y.getValueType();
					item1.number = (item1.type == Int_Type) ? (y.getConInt()) : (y.getConChar());
					if (item1.type == Char_Type)
						item1.isOperator = false; // 常量不可能是运算符
				}
				else {
					item1.isCharVar = false;
					if ((y.getItemKind() == Var_Kind || y.getItemKind() == Para_Kind) && y.getValueType() == Char_Type)
						item1.isCharVar = true;
					item1.type = String_Type;
					item1.str = "G" + to_string(factorFlag) + id;; // G12类似
				}
			}
			else {
				item1.type = String_Type;
				item1.isCharVar = false;
				item1.str = id;
				if (isCache) cache.push_back(item2);
				else globalItmCodeArr.push_back(item2);
			}
			obj.push_back(item1);
		}
	}
	else if (nextSym == LPARENT) { // 表达式
		factorFlag = 0;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		Exp_ret exp = syntaxExpre(funcName, isCache, cache);
		if (exp.isEmpty);
		else {
			if (exp.surable) {
				item1.type = Int_Type;
				item1.number = (exp.type == Int_Type) ? exp.num : exp.ch;
			}
			else {
				item1.type = String_Type;
				if (exp.name.size() > 0 && exp.name.at(0) == 'T') {
					item2.type = AssignState;
					item2.target = generateVar();
					item2.isTargetArr = item2.isRightArr = false;
					item2.left = exp.name;
					item2.op = '+';
					item2.right = "0";
					if (isCache) cache.push_back(item2);
					else globalItmCodeArr.push_back(item2);
					item1.str = item2.target;
				}
				else item1.str = exp.name;
				item1.isCharVar = (exp.type == Char_Type) ? true : false;
			}
			obj.push_back(item1);
		}
		if (nextSym == RPARENT) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
		}
		else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
		else errorHandling(getNowSymLines(), l_ErrorS);
	}
	else if (nextSym == PLUS || nextSym == MINU || nextSym == INTCON) {
		bool isMinu = false;
		if (nextSym == PLUS || nextSym == MINU) {
			isMinu = (nextSym == MINU);
			printSymbol(nextSym);
			nextSym = getPreSymbol();
		}
		if (nextSym == INTCON) {
			factorFlag = 1;
			item1.type = Int_Type;
			if (isMinu) item1.number = stoi("-" + getWord());
			else item1.number = stoi(getWord());
			obj.push_back(item1);
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			printSyntax(7); // 打印无符号整数
			printSyntax(8); // 打印整数
		}
	}
	else if (nextSym == CHARCON) {
		factorFlag = 2;
		item1.type = Char_Type;
		item1.number = getWord()[0];
		item1.isOperator = false;
		obj.push_back(item1);
		printSymbol(nextSym);
		nextSym = getPreSymbol();
	}
	if (factorFlag) printSyntax(16); // 打印<因子>
	return factorFlag;
}

// <主函数>::= void main'('')''{'＜复合语句＞'}'
void syntaxMain() {
	if (nextSym == MAINTK) {
		string funcName = "main";
		if (!symbolTableManager.pushItem("main", GLOBAL_FUNCNAME, Void_Type)) // 把主函数插入符号表
			errorHandling(getNowSymLines(), b_ErrorS);
		FourYuanItem mainItem;
		mainItem.type = FunctionDef;
		mainItem.funcType = Void_Type;
		mainItem.target = "main";
		globalItmCodeArr.push_back(mainItem);
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LPARENT) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else {
				errorHandling(getNowSymLines(), l_ErrorS);
				while (nextSym != LBRACE) nextSym = getPreSymbol();
			}
			if (nextSym == LBRACE) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				syntaxCompStatement("main"); // 主函数作用域
				if (!hasReturn) {
					mainItem.type = EndProcedure;
					mainItem.target = "main";
					globalItmCodeArr.push_back(mainItem);
				}
				if (nextSym == RBRACE) {
					printSymbol(nextSym);
					printSyntax(13); // 打印<主函数>
					printSyntax(1); // 打印<程序>
					nextSym = getPreSymbol();
					if (nextSym != INEXIST) errorHandling(0, my_ErrorS); // 主函数后有多余代码
				}
				else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
				//else errorHandling(getNowSymLines(), my_ErrorS); // 右大括号
			}
		}
	}
}

/*-------中间代码、MIPS汇编生成部分-------*/

void updataItem(FourYuanItem& item) {

}

void itemPush(Exp_ret exp, FourYuanItem& item2, int& orderx, string id, bool isCache, vector<FourYuanItem>& cache, string& index2) {
	if (exp.surable) {
		item2.type = AssignState;
		item2.target = generateVar();
		item2.isRightArr = false;
		item2.isTargetArr = false;
		if (exp.type == Char_Type) {
			orderx = symbolTableManager.idArrExpCheck(id, funcName, true, exp.ch);
			item2.left.insert(0, 1, exp.ch);;
			item2.op = '+';
			item2.right = "0";
		}
		else {
			orderx = symbolTableManager.idArrExpCheck(id, funcName, true, exp.num);
			item2.left = "" + to_string(exp.num);
			item2.op = '+';
			item2.right = "0";
		}
		if (isCache) cache.push_back(item2);
		else globalItmCodeArr.push_back(item2);

		index2 = item2.target;
	}
	else {
		orderx = symbolTableManager.idArrExpCheck(id, funcName, false);
		if (exp.name.size() > 0 && exp.name[0] == 'T') {
			item2.type = AssignState;
			item2.target = generateVar();
			item2.isTargetArr = item2.isRightArr = false;
			item2.left = exp.name;
			item2.op = '+';
			item2.right = "0";
			if (isCache) cache.push_back(item2);
			else globalItmCodeArr.push_back(item2);
			index2 = item2.target;
		}
		else index2 = exp.name;
	}
}

// 生成标签并返回
string generateLabel() {
	labelCount++; // 标签计数
	return ("Label" + to_string(labelCount)); // 标签号
}

// 生成临时变量并返回
string generateVar() {
	tempVarCount++;
	return ("T" + to_string(tempVarCount));
}

string generateStrLabel() {
	globalStrCount++;
	return ("String" + to_string(globalStrCount));
}

// 判断字符串是否是数字串
bool isStringDigit(string str) {
	for (unsigned int i = 0; i < str.size(); i++)
		if (str[i] > '9' || str[i] < '0')
			return false;
	return true;
}

// 字符串转为整数
int strToInt(string str) {
	return stoi(str);
}

// 将中间代码写入到文件中
void writeItmCodeToFile(string file, vector<FourYuanItem> ItmCode, map<string, unsigned>& maxTemp) {
	maxTemp.clear(); // 清空向量数组
	constStringArr.clear();
	strAndLabel.clear();
	ofstream out(file, ios::out); // 输出流到文件 
	string funcName = "GLOBAL";
	for (unsigned int i = 0; i < ItmCode.size(); i++) {
		FourYuanItem item = ItmCode.at(i);
		// 枚举四元式类型
		if (item.type == ValueParamDeliver) // 值参入栈语句
			out << "Push " << item.target << endl;
		else if (item.type == FunctionCall) // 函数调用语句
			out << "Call " << item.target << endl;
		else if (item.type == AssignState) { // 赋值语句
			if (item.isTargetArr) { // 结果(被赋值的)是数组 id[index] = 
				out << item.target << '[' << item.index1 << "] = ";
				if (item.isRightArr) // 从数组中取值到数组中,在中间代码等号右边 id[index]
					out << item.left << '[' << item.index2 << ']' << endl;
				else out << item.left << " " << item.op << " " << item.right << endl; // 操作数值或者Id
			}
			else { // 判断是否为临时变量
				if (item.target.size() > 0 && item.target.at(0) == 'T') { // T为临时变量，则插入到函数maxTempOrderMap中
					map<string, unsigned>::iterator iter = maxTemp.find(funcName); // 获取函数的最大临时变量个数
					unsigned order = strToInt(item.target.substr(1)); // 截取字符串，将T1的数字1转为整型，不超过9
					if (iter == maxTemp.end()) // 如果当前函数最大临时变量
						maxTemp.insert(map<string, unsigned>::value_type(funcName, order)); // value_type是map值类型
					else
						if (iter->second < order) iter->second = order; // 更改Map中最大编号的值
				} // 不是临时变量直接取出目标结果标识符Id
				out << item.target << " = ";
				if (item.isRightArr) // 如果是取数组值
					out << item.left << "[" << item.index2 << "]" << endl;
				else out << item.left << " " << item.op << " " << item.right << endl;
			}
		}
		else if (item.type == Label) // 标签语句
			out << item.target << ":" << endl;
		else if (item.type == FunctionDef) { // 函数声明语句
			funcName = item.target;
			if (item.funcType == Void_Type) out << "Void " + funcName << "()" << endl;
			else if (item.funcType == ReturnInt_Type) out << "Int " + funcName << "()" << endl;
			else out << "Char " + funcName << "()" << endl;
		}
		else if (item.type == ParamDef) { // 参数定义语句
			if (item.valueType == Int_Type) out << "Para int " << item.target << endl;
			else out << "Para char " << item.target << endl;
		}
		else if (item.type == Jump) // 直接跳转到目标标签语句
			out << "Jump " << item.target << endl;
		else if (item.type == BEZ) // left等于0跳转语句
			out << "BEZ " << item.left << " " << item.target << endl;
		else if (item.type == BNZ) // 不等于0跳转语句
			out << "BNZ " << item.left << " " << item.target << endl;
		else if (item.type == BLZ) // 小于0跳转语句
			out << "BLZ " << item.left << " " << item.target << endl;
		else if (item.type == BLEZ) // 小于等于0跳转语句
			out << "BLEZ " << item.left << " " << item.target << endl;
		else if (item.type == BGZ) // 大于0跳转语句
			out << "BGZ " << item.left << " " << item.target << endl;
		else if (item.type == BGEZ) // 大于等于0跳转语句
			out << "BGEZ " << item.left << " " << item.target << endl;
		else if (item.type == ReadChar) // 读字符语句
			out << "Read Char " << item.target << endl;
		else if (item.type == ReadInt) // 读整数语句
			out << "Read Int " << item.target << endl;
		else if (item.type == PrintStr) { // 写字符串语句
			out << "Print string " << '\"' << item.target << '\"' << endl;
			cancelEscapeChar(item.target); // 消除转义字符
			constStringArr.push_back(item.target);
		}
		else if (item.type == PrintChar) {// 写表达式中的字符语句
			if (item.target == "\n")
				out << "New Line." << endl;
			else
				out << "Print char " << '\'' << item.target.at(0) << '\'' << endl;
		}
		else if (item.type == PrintInt) // 写表达式中的整数语句
			out << "Print int " << strToInt(item.target) << endl;
		else if (item.type == PrintId) // 写表达式中的标识符语句
			out << "Print id " << item.target << endl;
		else if (item.type == ReturnInt) // 返回整型语句
			out << "Ret int " << strToInt(item.target) << endl;
		else if (item.type == ReturnChar) // 返回字符型语句
			out << "Ret char " << '\'' << item.target.at(0) << '\'' << endl;
		else if (item.type == ReturnId) // 返回表达式中的标识符语句
			out << "Ret id " << item.target << endl;
		else if (item.type == ReturnEmpty) // void型返回语句
			out << "Ret" << endl;
	}
	out.close();
}

// 转化为后缀表达式
void turnToPostfixExp(vector<PostfixItem> ori, vector<PostfixItem>& obj) {
	if (ori.size() == 1) { // 当只有一个表达式时，直接插入到后缀表达式数组
		obj.push_back(ori.at(0)); return;
	}
	else if (ori.size() > 0) { // 大于0的情况,先处理开头有正负号的情况
		PostfixItem it = ori.at(0);
		if (it.type == Char_Type && (it.number == '+' || it.number == '-') && it.isOperator == true) {
			if (it.number == '-') {
				it.type = Int_Type; // 修改作为新的一项
				it.number = 0;
				ori.insert(ori.begin(), it); // 在开头插入一个项:{整型,0}
			}
			else ori.erase(ori.begin()); // 删除开头项:'+'
		}
	}
	vector<PostfixItem> temp; // 临时数组，相当于运算符栈
	for (unsigned int i = 0; i < ori.size(); i++) {
		PostfixItem it = ori.at(i);
		if (it.type == Char_Type) {
			if (it.number == '+' || it.number == '-') {
				if (!it.isOperator) // 不是运算符则是char常量
					obj.push_back(it);
				else { // 是运算符
					while (temp.size() != 0) {
						obj.push_back(temp.at(temp.size() - 1));
						temp.pop_back();
					}
					temp.push_back(it);
				}
			}
			else if (it.number == '*' || it.number == '/') {
				if (!it.isOperator) // 不是运算符则是char常量
					obj.push_back(it);
				else { // 是运算符
					while (temp.size() != 0) {
						if (temp.at(temp.size() - 1).number == '*' ||
							temp.at(temp.size() - 1).number == '/') { // 前面的运算符是*或/,优先级大于+或-
							obj.push_back(temp.at(temp.size() - 1));
							temp.pop_back();
						}
						else break;
					}
					temp.push_back(it);
				}
			}
			else obj.push_back(it); // char常量
		}
		else obj.push_back(it); // int数值
	}
	while (temp.size() != 0) { // 将栈中剩余的符号插入
		obj.push_back(temp.at(temp.size() - 1)); // 从后往前插入数组
		temp.pop_back();
	}

}

// 后缀表达式obj的计算,返回后缀表达式标识符或者临时变量名
string calculateExp(vector<PostfixItem>& obj, bool& sure, VALUE_TYPE& type, int& expResult, bool isCache, vector<FourYuanItem>& cache, string funcName) {
	PostfixItem pItem1, pItem2;
	FourYuanItem item;
	string tempVar; // 变量名
	sure = false;
	vector<PostfixItem> temp; // 符号栈
	if (obj.size() == 1) {
		pItem1 = obj.at(0);
		if (pItem1.type == Int_Type || pItem1.type == Char_Type) {
			type = pItem1.type;
			expResult = pItem1.number; // 整型数值或者字符型ascii码
			sure = true;
			return "";
		}
		else {
			if (pItem1.isCharVar) type = Char_Type; // 是字符型变量
			else type = Int_Type;
			return pItem1.str;
		}
	}
	else {
		type = Int_Type;
		for (unsigned int i = 0; i < obj.size(); i++) {
			pItem1 = obj.at(i);
			if (pItem1.type == Char_Type) { // char类型字符或者运算符
				item.type = AssignState;
				item.isTargetArr = item.isRightArr = false;
				if (pItem1.number == '+' || pItem1.number == '-') {
					if (!pItem1.isOperator) // 不是运算符
						temp.push_back(pItem1);
					else if (temp.size() > 1) {
						bool isAbleDirect = true;
						int leftDigit, rightDigit;
						objExpLeftR(temp, isAbleDirect, item.right, rightDigit); // 表达式右边
						objExpLeftR(temp, isAbleDirect, item.left, leftDigit); // 表达式左边
						if (isAbleDirect) {
							pItem1.type = Int_Type;
							pItem1.number = (pItem1.number == '+') ? (leftDigit + rightDigit) : (leftDigit - rightDigit);
							temp.push_back(pItem1);
						}
						else {
							tempVar = generateVar();
							item.target = tempVar;
							item.op = pItem1.number;
							if (isCache) cache.push_back(item);
							else globalItmCodeArr.push_back(item);

							pItem1.type = String_Type;
							pItem1.str = tempVar;
							temp.push_back(pItem1);
						}
					}
				}
				else if (pItem1.number == '*' || pItem1.number == '/') {
					if (!pItem1.isOperator) // 不是运算符
						temp.push_back(pItem1);
					else if (temp.size() > 1) {
						bool isAbleDirect = true;
						int leftDigit, rightDigit;
						objExpLeftR(temp, isAbleDirect, item.right, rightDigit); // 表达式右边
						objExpLeftR(temp, isAbleDirect, item.left, leftDigit); // 表达式左边
						if (isAbleDirect) {
							pItem1.type = Int_Type;
							pItem1.number = (pItem1.number == '*') ? (leftDigit * rightDigit) : (leftDigit / rightDigit);
							temp.push_back(pItem1);
						}
						else {
							tempVar = generateVar();
							item.target = tempVar;
							item.op = pItem1.number;
							if (isCache) cache.push_back(item);
							else globalItmCodeArr.push_back(item);

							pItem1.type = String_Type;
							pItem1.str = tempVar;
							temp.push_back(pItem1);
						}
					}
				}
				else temp.push_back(pItem1); // 是整型
			}
			else temp.push_back(pItem1);
		}
		if (temp.size() >= 1) { // 最后计算结果
			if (temp.at(0).type == Int_Type) {
				sure = true;
				type = Int_Type;
				expResult = temp.at(0).number;
				return "";
			}
			return temp.at(0).str;
		}
	}
	return "";
}

// 计算后缀表达式左右两项
void objExpLeftR(vector<PostfixItem>& temp, bool& isAbleDirect, string& dir, int& digit) {
	if (temp.at(temp.size() - 1).type == String_Type) { // 表达式右边
		dir = temp.at(temp.size() - 1).str;
		temp.pop_back();
		isAbleDirect = false;
	}
	else {
		dir = "" + to_string(temp.at(temp.size() - 1).number);
		digit = temp.at(temp.size() - 1).number;
		temp.pop_back();
	}
}

// 将中间代码翻译成MIPS汇编语言代码并写入文件
void generateMipsCode(string file, bool isOp) {
	ofstream out(file, ios::out);
	out << ".data" << " # generate data segment" << endl; // 先生成.data伪指令
	generateData(out); // 生成data段代码

	//修约strMemSize成为4的倍数
	strMemSize = strMemSize + 4 - (strMemSize % 4);
	//临时参数栈基址设置为strMemSize+4
	paramSpBegin = strMemSize + 4 + dataBaseAddr;
	currentParamSp = paramSpBegin;
	returnValueSpace = strMemSize + dataBaseAddr;
	funcBeginAddr = paramSpBegin + tempStackMax;

	out << ".text" << " # generate text segment" << endl; // 生成.text伪指令
	//generateText(out);
	if (!isOp) generateText(out, globalItmCodeArr); // 生成text段代码
	else generateText(out, op_ItmCodeArr);
	out << "# generate mips code successfully." << endl;
	out.close();
}

// 生成data段
void generateData(ofstream& out) {
	strMemSize = 0;
	for (unsigned int i = 0; i < constStringArr.size(); i++) {
		string item = constStringArr.at(i);
		// 检查是否是重复的公共字符串
		map<string, string>::iterator iter = strAndLabel.find(item);
		if (iter != strAndLabel.end()) continue;
		strMemSize += item.size() + 1;//'\0'占一个字节
		string label = generateStrLabel();
		out << "\t" << label << ":.asciiz \"" << item << "\"" << endl;
		strAndLabel.insert(map<string, string>::value_type(item, label));
	}
}

// 生成text段
void generateText(ofstream& out, vector<FourYuanItem> itm) {
	// 最初先生成跳转到main函数的代码执行
	out << "j main" << endl;
	for (unsigned int i = 0; i < itm.size(); i++) {
		FourYuanItem item = itm.at(i);
		if (item.type == ValueParamDeliver) {
			// push对应的全部都是变量,取地址
			// 首先看push的符号表里面的还是临时生成的变量
			if (item.target.at(0) == 'G') { // 符号表内
				int order = strToInt(item.target.substr(1));
				SymbolTableItem item = symbolTableManager.getTable().at(order);
				if (item.getFuncName() == "GLOBAL") {
					getAddrOfGlobal(item.getIdName(), "$a2", out);
				}
				else {
					getAddrOfLocal(item.getFuncName(), item.getIdName(), "$a2", out);
				}
				out << "lw $a2 0($a2)" << endl;
			}
			else { // 临时变量
				int g = strToInt(item.target.substr(1));
				if (g > TEMP_REGISTER) {
					getAddrOfTemp(g, "$a2", out);
					out << "lw $a2 0($a2)" << endl;
				}
				else {
					out << "move $a2 " << "$t" << (g + 3) << endl;
				}
			}
			out << "sw $a2 " << currentParamSp << "($0)" << endl;
			currentParamSp += 4;
		}
		else if (item.type == FunctionCall) {
			currentParamSp = paramSpBegin;//回到起点
			// 函数调用,跳转,生成所跳转的目标函数运行栈空间的首地址
			out << "jal " << item.target << endl; // 此时mars运行会将返回地址写入到$ra寄存器(函数执行时需要保存它)
		}
		else if (item.type == AssignState)
			helpAssignStatement(item, out);
		else if (item.type == Label)
			out << item.target << ":" << endl;
		else if (item.type == FunctionDef) {
			out << item.target << ":" << endl; // 生成跳转到函数的标签
			helpFunctionDef(item.target, out);
		}
		else if (item.type == ParamDef) { ; } // 忽略处理,因为已经通过符号表在函数定义处处理了变量、参数
		else if (item.type == Jump)
			out << "j " << item.target << endl;
		else if (item.type == BEZ || item.type == BNZ || item.type == BLZ || item.type == BLEZ || item.type == BGZ || item.type == BGEZ)
			helpBJump(item, out);
		else if (item.type == ReadChar || item.type == ReadInt) {
			if (item.type == ReadChar) out << "li $v0 12" << endl;
			else out << "li $v0 5" << endl;
			out << "syscall" << endl;
			int order = strToInt(item.target.substr(1));
			SymbolTableItem item = globalSymbolTable.at(order);
			if (item.getFuncName() == "GLOBAL") {
				getAddrOfGlobal(item.getIdName(), "$a3", out);
			}
			else {
				getAddrOfLocal(item.getFuncName(), item.getIdName(), "$a3", out);
			}
			out << "sw $v0 0($a3)" << endl;
		}
		else if (item.type == PrintStr) {
			cancelEscapeChar(item.target); // 消除转义字符
			map<string, string>::iterator iter = strAndLabel.find(item.target);
			if (iter != strAndLabel.end())
				out << "la $a0 " << iter->second << endl;
			out << "li $v0 4" << endl;
			out << "syscall" << endl;

		}
		else if (item.type == PrintChar) {
			out << "li $a0 " << (int)item.target.at(0) << endl;
			out << "li $v0 11" << endl;
			out << "syscall" << endl;
		}
		else if (item.type == PrintInt) {
			out << "li $a0 " << strToInt(item.target) << endl;
			out << "li $v0 1" << endl;
			out << "syscall" << endl;
		}
		else if (item.type == PrintId) {
			// 判断是打印int还是char
			if (item.target.at(0) == 'G') {
				int order = strToInt(item.target.substr(1));
				SymbolTableItem item = globalSymbolTable.at(order);
				if (item.getFuncName() == "GLOBAL") {
					getAddrOfGlobal(item.getIdName(), "$a0", out);
				}
				else {
					getAddrOfLocal(item.getFuncName(), item.getIdName(), "$a0", out);
				}
				out << "lw $a0 0($a0)" << endl;
			}
			else if (item.target.at(0) == 'T') {
				int g = strToInt(item.target.substr(1));
				if (g > TEMP_REGISTER) {
					getAddrOfTemp(g, "$a0", out);
					out << "lw $a0 0($a0)" << endl;
				}
				else {
					out << "move $a0 " << "$t" << (g + 3) << endl;
				}
			}

			if (item.isPrintCharId) out << "li $v0 11" << endl;
			else out << "li $v0 1" << endl;
			out << "syscall" << endl;
		}
		else if (item.type == ReturnInt || item.type == ReturnChar) {
			out << "li $v0 " << strToInt(item.target) << endl;
			out << "sw $v0 " << returnValueSpace << "($0)" << endl;
			// 返回地址
			helpReturn(out);
		}
		else if (item.type == ReturnId) {
			if (item.target.at(0) == 'G') {
				int order = strToInt(item.target.substr(1));
				SymbolTableItem item = globalSymbolTable.at(order);
				if (item.getFuncName() == "GLOBAL") {
					getAddrOfGlobal(item.getIdName(), "$v0", out);
				}
				else {
					getAddrOfLocal(item.getFuncName(), item.getIdName(), "$v0", out);
				}
				out << "lw $v0 0($v0)" << endl;
			}
			else if (item.target.at(0) == 'T') {
				int g = strToInt(item.target.substr(1));
				if (g > TEMP_REGISTER) {
					getAddrOfTemp(g, "$v0", out);
					out << "lw $v0 0($v0)" << endl;
				}
				else {
					out << "move $v0 " << "$t" << (g + 3) << endl;
				}
			}
			out << "sw $v0 " << returnValueSpace << "($0)" << endl;
			helpReturn(out); // 返回地址
		}
		else if (item.type == ReturnEmpty)
			helpReturn(out); // 返回地址
		else if (item.type == EndProcedure) {
			out << "li $v0 10" << endl;
			out << "syscall" << endl;
		}
	}
}

// 全局变量
void getAddrOfGlobal(string name, string targetReg, ofstream& out) {
	vector<SymbolTableItem> globalSymbolTable = symbolTableManager.getTable();
	int number = 0;
	unsigned int i;
	for (i = 0; i < globalSymbolTable.size(); i++) {
		SymbolTableItem item = globalSymbolTable.at(i);
		if (item.getItemKind() != Con_Kind) break;
	}
	for (; i < globalSymbolTable.size(); i++) {
		SymbolTableItem item = globalSymbolTable.at(i);
		if (item.getIdName() == name) break;
		if (item.getArrLen() == 0) number += 4;
		else number += item.getArrLen() * 4;
	}
	out << "li " << targetReg << " " << funcBeginAddr + number << endl;
}
// 局部的变量
void getAddrOfLocal(string funcName, string eleName, string targetReg, ofstream& out) {
	vector<SymbolTableItem> globalSymbolTable = symbolTableManager.getTable();
	int number = 0;
	unsigned int i;
	for (i = 0; i < globalSymbolTable.size(); i++) {
		SymbolTableItem item = globalSymbolTable.at(i);
		if (item.getItemKind() == Func_Kind && item.getIdName() == funcName) break;
	}
	for (i = i + 1; i < globalSymbolTable.size(); i++) {
		SymbolTableItem item = globalSymbolTable.at(i);
		if (item.getItemKind() == Con_Kind) continue;
		if (item.getIdName() == eleName) break;
		if (item.getArrLen() == 0) number += 4;
		else number += item.getArrLen() * 4;
	}
	out << "addiu " << targetReg << " $fp " << number + 8 << endl;
}

// 获取临时变量的地址
void getAddrOfTemp(int order, string targetReg, ofstream& out) {
	// @need: order > TEMP_REGISTER
	out << "move " << targetReg << " $k0" << endl;
	out << "addiu " << targetReg << " " << targetReg << " " << (order - 1 - TEMP_REGISTER) * 4 << endl;
}

//生成Text段代码的辅助函数
//赋值语句处理(优化了右操作数为0的运算)
void helpAssignStatement(FourYuanItem item, ofstream& out) {
	vector<SymbolTableItem> globalSymbolTable = symbolTableManager.getTable();
	// 使用$t0~$t7
	if (item.isTargetArr) { // a[i] = temp1
		// 首先最终赋值元素的地址放在$t0寄存器中,运算结果放在$t1寄存器
		// 数组型变量是以G+order+id的形式出现
		int order = strToInt(item.target.substr(1));
		SymbolTableItem arrayItem = globalSymbolTable.at(order);
		// 数组首地址放在$t0中
		if (arrayItem.getFuncName() == "GLOBAL") { // 是全局的
			getAddrOfGlobal(arrayItem.getIdName(), "$t0", out);
		}
		else getAddrOfLocal(arrayItem.getFuncName(), arrayItem.getIdName(), "$t0", out);
		// 观察index1即数组下标
		string index1 = item.index1;
		if (index1.at(0) == 'G') { // 在符号表
			order = strToInt(index1.substr(1));
			SymbolTableItem index1Item = globalSymbolTable.at(order);
			// 地址放在$t1
			if (index1Item.getFuncName() == "GLOBAL") {
				getAddrOfGlobal(index1Item.getIdName(), "$t1", out);
			}
			else getAddrOfLocal(index1Item.getFuncName(), index1Item.getIdName(), "$t1", out);
			out << "lw $t1 0($t1)" << endl; // 下标值存入$t1
		}
		else if (index1.at(0) == 'T') {//在临时变量表
			//放在$t1
			int g = strToInt(index1.substr(1));
			if (g > TEMP_REGISTER) {
				getAddrOfTemp(g, "$t1", out);
				out << "lw $t1 0($t1)" << endl;
			}
			else out << "move $t1 " << "$t" << (g + 3) << endl;
		}
		else { // 纯数字
			int number = strToInt(index1);
			out << "li $t1 " << number << endl;
		}
		out << "li $t2 4" << endl;
		out << "mult $t1 $t2" << endl;
		out << "mflo $t1" << endl;
		out << "addu $t0 $t0 $t1" << endl;//最终赋值元素地址存入了$t0
		//结构决定了下面是left + right结构 --->运算结果存在$t1 left值放在$t1 right值放在$t2
		string left = item.left;
		string right = item.right;
		char op = item.op;
		if (left.at(0) == 'G') { // 符号表
			order = strToInt(left.substr(1));
			SymbolTableItem leftItem = globalSymbolTable.at(order);
			if (leftItem.getFuncName() == "GLOBAL") {
				getAddrOfGlobal(leftItem.getIdName(), "$t1", out);
			}
			else getAddrOfLocal(leftItem.getFuncName(), leftItem.getIdName(), "$t1", out);
			out << "lw $t1 0($t1)" << endl;
		}
		else if (left.at(0) == 'T') { // 临时变量表
			int g = strToInt(left.substr(1));
			if (g > TEMP_REGISTER) {
				getAddrOfTemp(g, "$t1", out);
				out << "lw $t1 0($t1)" << endl;
			}
			else out << "move $t1 " << "$t" << (g + 3) << endl;
		}
		else if (left == "Ret") { // 函数调用的返回值
			out << "lw $t1 " << returnValueSpace << "($0)" << endl;
		}
		else out << "li $t1 " << strToInt(left) << endl; // 数字
		// 右操作数为"0",不需要生成代码
		if (right != "0") {
			if (right.at(0) == 'G') { // 符号表
				order = strToInt(right.substr(1));
				SymbolTableItem rightItem = globalSymbolTable.at(order);
				if (rightItem.getFuncName() == "GLOBAL") {
					getAddrOfGlobal(rightItem.getIdName(), "$t2", out);
				}
				else {
					getAddrOfLocal(rightItem.getFuncName(), rightItem.getIdName(), "$t2", out);
				}
				out << "lw $t2 0($t2)" << endl;
			}
			else if (right.at(0) == 'T') {//临时变量表
				int g = strToInt(right.substr(1));
				if (g > TEMP_REGISTER) {
					getAddrOfTemp(g, "$t2", out);
					out << "lw $t2 0($t1)" << endl;
				}
				else {
					out << "move $t2 " << "$t" << (g + 3) << endl;
				}
			}
			else {//数字
				out << "li $t2 " << strToInt(right) << endl;
			}
			if (op == '+') {
				out << "addu $t1 $t1 $t2" << endl;
			}
			else if (op == '-') {
				out << "subu $t1 $t1 $t2" << endl;
			}
			else if (op == '*') {
				out << "mult $t1 $t2" << endl;
				out << "mflo $t1" << endl;
			}
			else if (op == '/') {
				out << "div $t1 $t2" << endl;
				out << "mflo $t1" << endl;
			}
		}
		else {
			if (item.op == '*')
				out << "move $t1 $0" << endl;
		}
	}
	else {
		//$t0存放target地址
		bool isTemp = false;
		int g, g1;
		if (item.target.at(0) == 'G') {//符号表内的变量
			int order = strToInt(item.target.substr(1));
			SymbolTableItem item = globalSymbolTable.at(order);
			if (item.getFuncName() == "GLOBAL") {
				getAddrOfGlobal(item.getIdName(), "$t0", out);
			}
			else getAddrOfLocal(item.getFuncName(), item.getIdName(), "$t0", out);
		}
		else if (item.target.at(0) == 'T') {//临时变量
			g1 = g = strToInt(item.target.substr(1));
			if (g > TEMP_REGISTER) {
				getAddrOfTemp(strToInt(item.target.substr(1)), "$t0", out);
			}
			else isTemp = true;
		}
		//分析等号右边数组
		if (item.isRightArr) {
			//单纯的数组取值
			string left = item.left;
			int order = strToInt(left.substr(1));
			SymbolTableItem x = globalSymbolTable.at(order);
			//分析索引下标,将数组地址取出放在$t1
			if (x.getFuncName() == "GLOBAL") {
				getAddrOfGlobal(x.getIdName(), "$t1", out);
			}
			else getAddrOfLocal(x.getFuncName(), x.getIdName(), "$t1", out);
			//下标地址取出放在$t2
			string index2 = item.index2;
			if (index2.at(0) == 'G') {//在符号表
				order = strToInt(index2.substr(1));
				SymbolTableItem index2Item = globalSymbolTable.at(order);
				if (index2Item.getFuncName() == "GLOBAL") {
					getAddrOfGlobal(index2Item.getIdName(), "$t2", out);
				}
				else {
					getAddrOfLocal(index2Item.getFuncName(), index2Item.getIdName(), "$t2", out);
				}
				out << "lw $t2 0($t2)" << endl;
			}
			else if (index2.at(0) == 'T') {//在临时变量表
				g = strToInt(index2.substr(1));
				if (g > TEMP_REGISTER) {
					getAddrOfTemp(g, "$t2", out);
					out << "lw $t2 0($t2)" << endl;
				}
				else {
					out << "move $t2 " << "$t" << (g + 3) << endl;
				}
			}
			else {//纯数字
				out << "li $t2 " << strToInt(index2) << endl;
			}
			out << "li $t3 4" << endl;
			out << "mult $t2 $t3" << endl;
			out << "mflo $t2" << endl;
			out << "addu $t1 $t1 $t2" << endl;
			//取出数据,放在$t1
			out << "lw $t1 0($t1)" << endl;
		}
		else {
			//左右操作数
			int order;
			string left = item.left;
			string right = item.right;
			char op = item.op;
			if (left.at(0) == 'G') {//符号表
				order = strToInt(left.substr(1));
				SymbolTableItem leftItem = globalSymbolTable.at(order);
				if (leftItem.getFuncName() == "GLOBAL") {
					getAddrOfGlobal(leftItem.getIdName(), "$t1", out);
				}
				else getAddrOfLocal(leftItem.getFuncName(), leftItem.getIdName(), "$t1", out);
				out << "lw $t1 0($t1)" << endl;
			}
			else if (left.at(0) == 'T') {//临时变量表
				g = strToInt(left.substr(1));
				if (g > TEMP_REGISTER) {
					getAddrOfTemp(g, "$t1", out);
					out << "lw $t1 0($t1)" << endl;
				}
				else out << "move $t1 " << "$t" << (g + 3) << endl;
			}
			else if (left == "Ret") {//函数调用的返回值
				out << "lw $t1 " << returnValueSpace << "($0)" << endl;
			}
			else { // 数字
				if (right.at(0) != 'G' && right.at(0) != 'T' && right.size() > 0) { // 右边也是数字
					// 直接把两个数字计算后再
					int a = strToInt(left), b = strToInt(right), value = 0;
					if (op == '+') value = a + b;
					else if (op == '-') value = a - b;
					else if (op == '*') value = a * b;
					else if (op == '/') value = a / b;
					out << "li $t1 " << value << endl;
					//out << "li $t1 " << strToInt(left) << endl;
					if (isTemp)
						out << "move $t" << (g1 + 3) << " $t1" << endl;
					else out << "sw $t1 0($t0)" << endl;
					return;
				}
				out << "li $t1 " << strToInt(left) << endl;
			}
			if (right != "0") { // 等于零就直接不管
				if (right.at(0) == 'G') { // 符号表
					order = strToInt(right.substr(1));
					SymbolTableItem rightItem = globalSymbolTable.at(order);
					if (rightItem.getFuncName() == "GLOBAL") {
						getAddrOfGlobal(rightItem.getIdName(), "$t2", out);
					}
					else {
						getAddrOfLocal(rightItem.getFuncName(), rightItem.getIdName(), "$t2", out);
					}
					out << "lw $t2 0($t2)" << endl;
				}
				else if (right.at(0) == 'T') {//临时变量表
					g = strToInt(right.substr(1));
					if (g > TEMP_REGISTER) {
						getAddrOfTemp(g, "$t2", out);
						out << "lw $t2 0($t2)" << endl;
					}
					else {
						out << "move $t2 " << "$t" << (g + 3) << endl;
					}
				}
				else if (right == "Ret") {
					out << "lw $t2 " << returnValueSpace << "($0)" << endl;
				}
				else {//数字
					out << "li $t2 " << strToInt(right) << endl;
				}
				if (op == '+') {
					out << "addu $t1 $t1 $t2" << endl;
				}
				else if (op == '-') {
					out << "subu $t1 $t1 $t2" << endl;
				}
				else if (op == '*') {
					out << "mult $t1 $t2" << endl;
					out << "mflo $t1" << endl;
				}
				else if (op == '/') {
					out << "div $t1 $t2" << endl;
					out << "mflo $t1" << endl;
				}
			}
			else {
				if (item.op == '*')
					out << "move $t1 $0" << endl;
			}
		}
		if (isTemp) {
			out << "move $t" << (g1 + 3) << " $t1" << endl;
			return;
		}
	}
	//从$t1将值存入内存
	out << "sw $t1 0($t0)" << endl;
}

// 函数定义处理(空间分配,数据保存等一系列操作)
int getGlobalVarSumSpace() {
	vector<SymbolTableItem> globalSymbolTable = symbolTableManager.getTable();
	int number = 0;
	for (unsigned int i = 0; i < globalSymbolTable.size(); i++) {
		SymbolTableItem item = globalSymbolTable.at(i);
		if (item.getItemKind() == Func_Kind)
			break;
		if (item.getItemKind() == Con_Kind)
			continue;
		if (item.getArrLen() == 0)
			number += 4;
		else
			number += item.getArrLen() * 4;
	}
	return number;
}

void initializeStack(string funcName, ofstream& out) {
	vector<SymbolTableItem> globalSymbolTable = symbolTableManager.getTable();
	int paramC = 0; //参 数计数器
	int number = 0; // 变量不进行初始化
	for (unsigned int i = 0; i < globalSymbolTable.size(); i++) {
		SymbolTableItem item = globalSymbolTable.at(i);
		if (item.getItemKind() == Func_Kind && item.getIdName() == funcName) {
			for (unsigned int j = i + 1; j < globalSymbolTable.size(); j++) {
				item = globalSymbolTable.at(j);
				if (item.getFuncName() != funcName)
					break;
				if (item.getItemKind() == Con_Kind)
					continue;
				if (item.getItemKind() == Para_Kind) { // 是参数
					// 借用$v1寄存器作为中转
					out << "lw $v1 " << paramSpBegin + paramC * 4 << "($0)" << endl;
					string lala = "G" + to_string(item.getOrder()) + item.getIdName();
					map<string, string>::iterator itr = varToRegisterMap.find(lala);
					if (itr != varToRegisterMap.end()) {
						out << "move " << itr->second << " $v1" << endl;
					}
					else out << "sw $v1 0($sp)" << endl;
					paramC++;
					out << "addiu $sp $sp 4" << endl;
				}
				else if (item.getItemKind() == Var_Kind) {
					if (item.getArrLen() == 0) {
						number += 4;
					}
					else number += 4 * item.getArrLen();
				}
			}
			break;
		}
	}
	out << "addiu $sp $sp " << number << endl;
	out << "move $k0 $sp" << endl; // 临时变量区入口地址确定,存放入$k0
	out << "move $k1 $0 " << endl;
	// 分配所需最大的临时空间
	map<string, unsigned>::iterator iter = maxTempOrderMap.find(funcName);
	if (iter != maxTempOrderMap.end()) {
		if (iter->second > TEMP_REGISTER)
			out << "addiu $sp $sp " << (iter->second - TEMP_REGISTER) * 4 << endl;
		out << "addiu $k1 $0 " << iter->second << endl;
	}
}

void helpFunctionDef(string funcName, ofstream& out) {
	if (funcName == "main") { // main函数只需要做全局变量的数据分配
		int size = getGlobalVarSumSpace();
		out << "li $fp " << funcBeginAddr + size << endl;
		out << "addiu $sp $fp 8" << endl;
		initializeStack(funcName, out);
	}
	else { // 将$k0,$k1寄存器的值存入栈
		out << "sw $k0 0($sp)" << endl;
		out << "sw $k1 4($sp)" << endl;
		out << "sw $t4 8($sp)" << endl;
		out << "sw $t5 12($sp)" << endl;
		out << "sw $t6 16($sp)" << endl;
		out << "sw $t7 20($sp)" << endl;
		out << "sw $t8 24($sp)" << endl;
		out << "sw $t9 28($sp)" << endl;

		out << "addiu $sp $sp 32" << endl;
		out << "sw $fp 4($sp)" << endl; // 保存上一级函数的基地址
		out << "move $fp $sp" << endl; // 设置fp
		out << "sw $ra 0($fp)" << endl; // 设置返回地址
		out << "addiu $sp $fp 8" << endl; // 初始化栈空间
		initializeStack(funcName, out);
	}
}

// 跳转语句B类统一处理
void helpBJump(FourYuanItem item, ofstream& out) {
	vector<SymbolTableItem> globalSymbolTable = symbolTableManager.getTable();
	//把判断的变量值存入$a1寄存器中,再调用转移函数
	string obj = item.left;
	if (obj.at(0) == 'G') {
		int order = strToInt(obj.substr(1));
		SymbolTableItem item = globalSymbolTable.at(order);
		if (item.getFuncName() == "GLOBAL")
			getAddrOfGlobal(item.getIdName(), "$a1", out);
		else getAddrOfLocal(item.getFuncName(), item.getIdName(), "$a1", out);
		out << "lw $a1 0($a1)" << endl;
	}
	else if (obj.at(0) == 'T') {
		int g = strToInt(obj.substr(1));
		if (g > TEMP_REGISTER) {
			getAddrOfTemp(g, "$a1", out);
			out << "lw $a1 0($a1)" << endl;
		}
		else out << "move $a1 " << "$t" << (g + 3) << endl;
	}
	if (item.type == BEZ)
		out << "beq $a1 $0 " << item.target << endl;
	else if (item.type == BNZ)
		out << "bne $a1 $0 " << item.target << endl;
	else if (item.type == BLZ)
		out << "bltz $a1 " << item.target << endl;
	else if (item.type == BLEZ)
		out << "blez $a1 " << item.target << endl;
	else if (item.type == BGZ)
		out << "bgtz $a1 " << item.target << endl;
	else if (item.type == BGEZ)
		out << "bgez $a1 " << item.target << endl;
}

// 函数返回处理
void helpReturn(ofstream& out) {
	out << "move $sp $fp" << endl; // 栈指针恢复到$fp
	//$k0 $k1寄存器值恢复
	out << "lw $t9 -4($sp)" << endl;
	out << "lw $t8 -8($sp)" << endl;
	out << "lw $t7 -12($sp)" << endl;
	out << "lw $t6 -16($sp)" << endl;
	out << "lw $t5 -20($sp)" << endl;
	out << "lw $t4 -24($sp)" << endl;
	out << "lw $k1 -28($sp)" << endl;
	out << "lw $k0 -32($sp)" << endl;
	out << "addiu $sp $sp -32" << endl;

	out << "lw $ra 0($fp)" << endl; // 返回地址存入$ra

	out << "lw $fp 4($fp)" << endl; // 函数栈区起始地址恢复--->上一级函数基地址$fp恢复

	out << "jr $ra" << endl; // 执行jr
}

// 取消字符串中的转义字符，所有的\都加上一个\即可
void cancelEscapeChar(string& target) {
	vector< unsigned int> indexSets;
	for (unsigned int i = 0; i < target.size(); i++) {
		if (target.at(i) == '\\')
			indexSets.push_back(i);
	}
	for (unsigned int i = 0; i < indexSets.size(); i++)
		target.insert(indexSets.at(i) + i, "\\");
}

/*------------优化处理部分-----------*/
void generateOpItmArr(vector<FourYuanItem>& itmArr) {
	ofstream out("Itm.txt", ios::out);
	for (unsigned int i = 0; i < globalItmCodeArr.size(); i++) {
		FourYuanItem item = globalItmCodeArr.at(i);
		//out << item.type << " tar: " << item.target << " op: " << item.op << " left: " << item.left << " right: " << item.right << endl;
		bool is = false;
		/*
		if (item.type == AssignState && (item.left == "0" || item.right == "0")) {
			i++;
			FourYuanItem nextItem;
			if (i < globalItmCodeArr.size()) {
				nextItem = globalItmCodeArr.at(i);
				if (nextItem.type == AssignState) {
					if (item.target == nextItem.left || item.target == nextItem.right) {
						if (item.left == "0") {
							if (item.target == nextItem.left) {
								nextItem.left.clear();
								if (item.op == '-') nextItem.left = "-";
								nextItem.left += item.right;
							}
							else {
								nextItem.right.clear();
								if (item.op == '-') nextItem.right = "-";
								nextItem.right += item.right;
							}
							itmArr.push_back(nextItem);
							is = true;
						}
						else if (item.right == "0") {
							if (item.target == nextItem.left)
								nextItem.left = item.left;
							else nextItem.right = item.left;
							itmArr.push_back(nextItem);
							is = true;
						}
					}
				}
			}
			if (!is) {
				itmArr.push_back(item);
				if (i < globalItmCodeArr.size())itmArr.push_back(nextItem);
			}
		}
		else itmArr.push_back(item);
		*/
		itmArr.push_back(item);
	}
	out.close();
}


/*
void op_initializeStack(string funcName, ofstream& out) {
	//参数计数器
	int paramC = 0;
	int number = 0;//变量不进行初始化
	for (unsigned int i = 0; i < globalSymbolTable.size(); i++) {
		SymbolTableItem item = globalSymbolTable.at(i);
		if (item.getItemKind() == Func_Kind && item.getIdName() == funcName) {
			for (unsigned int j = i + 1; j < globalSymbolTable.size(); j++) {
				item = globalSymbolTable.at(j);
				if (item.getFuncName() != funcName)
					break;
				if (item.getItemKind() == Con_Kind)
					continue;
				if (item.getItemKind() == Para_Kind) {//是参数
					out << "lw $v1 " << paramSpBegin + paramC * 4 << "($0)" << endl;
					out << "sw $v1 0($sp)" << endl;
					paramC++;
					out << "addiu $sp $sp 4" << endl;
				}
				else if (item.getItemKind() == Var_Kind) {
					if (item.getArrLen() == 0) {
						number += 4;
					}
					else number += 4 * item.getArrLen();
				}
			}
			break;
		}
	}
	out << "addiu $sp $sp " << number << endl;
	out << "move $k0 $sp" << endl;
	out << "move $k1 $0 " << endl;
	map<string, unsigned>::iterator iter = op_maxTempOrderMap.find(funcName);
	if (iter != op_maxTempOrderMap.end()) {
		if (iter->second > TEMP_REGISTER)
			out << "addiu $sp $sp " << (iter->second - TEMP_REGISTER) * 4 << endl;
		out << "addiu $k1 $0 " << iter->second << endl;
	}
}

void op_helpAssignStatement(FourYuanItem item, ofstream& out) {
	//使用$t0~$t7
	if (item.isTargetArr) {//a[i] = temp1
						   //首先最终赋值元素的地址放在$t0寄存器中,运算结果放在$t1寄存器
						   //数组型变量是以G+order+id的形式出现
		int order = strToInt(item.target.substr(1));
		SymbolTableItem arrayItem = globalSymbolTable.at(order);
		//数组首地址放在$t0中
		if (arrayItem.getFuncName() == "GLOBAL") {//是全局的
			getAddrOfGlobal(arrayItem.getIdName(), "$t0", out);
		}
		else getAddrOfLocal(arrayItem.getFuncName(), arrayItem.getIdName(), "$t0", out);
		string index1 = item.index1;
		if (index1.at(0) == 'G') {//在符号表
			map<string, string>::iterator myItr = varToRegisterMap.find(index1);
			if (myItr != varToRegisterMap.end())
				out << "move $t1 " << myItr->second << endl;
			else {
				order = strToInt(index1.substr(1));
				SymbolTableItem index1Item = globalSymbolTable.at(order);
				//地址放在$t1
				if (index1Item.getFuncName() == "GLOBAL") {
					getAddrOfGlobal(index1Item.getIdName(), "$t1", out);
				}
				else getAddrOfLocal(index1Item.getFuncName(), index1Item.getIdName(), "$t1", out);
				//放在$t1
				out << "lw $t1 0($t1)" << endl;//下标值存入$t1
			}
		}
		else if (index1.at(0) == 'T') {//在临时变量表
									   //放在$t1
			int g = strToInt(index1.substr(1));
			if (g > TEMP_REGISTER) {
				getAddrOfTemp(g, "$t1", out);
				out << "lw $t1 0($t1)" << endl;
			}
			else out << "move $t1 " << "$t" << (g + 3) << endl;
		}
		else {//纯数字
			int number = strToInt(index1);
			out << "li $t1 " << number << endl;
		}
		out << "li $t2 4" << endl;
		out << "mult $t1 $t2" << endl;
		out << "mflo $t1" << endl;
		out << "addu $t0 $t0 $t1" << endl;//最终赋值元素地址存入了$t0
										  //结构决定了下面是left + right结构 --->运算结果存在$t1 left值放在$t1 right值放在$t2
		string left = item.left;
		string right = item.right;
		char op = item.op;
		if (left.at(0) == 'G') {//符号表
			map<string, string>::iterator myItr = varToRegisterMap.find(left);
			if (myItr != varToRegisterMap.end()) {
				out << "move $t1 " << myItr->second << endl;
			}
			else {
				order = strToInt(left.substr(1));
				SymbolTableItem leftItem = globalSymbolTable.at(order);
				if (leftItem.getFuncName() == "GLOBAL") {
					getAddrOfGlobal(leftItem.getIdName(), "$t1", out);
				}
				else getAddrOfLocal(leftItem.getFuncName(), leftItem.getIdName(), "$t1", out);
				out << "lw $t1 0($t1)" << endl;
			}
		}
		else if (left.at(0) == 'T') {//临时变量表
			int g = strToInt(left.substr(1));
			if (g > TEMP_REGISTER) {
				getAddrOfTemp(g, "$t1", out);
				out << "lw $t1 0($t1)" << endl;
			}
			else out << "move $t1 " << "$t" << (g + 3) << endl;
		}
		else if (left == "Ret") {//函数调用的返回值
			out << "lw $t1 " << returnValueSpace << "($0)" << endl;
		}
		else out << "li $t1 " << strToInt(left) << endl;
		//右操作数为"0",不需要生成代码
		if (right != "0") {
			if (right.at(0) == 'G') {//符号表
				map<string, string>::iterator myItr = varToRegisterMap.find(right);
				if (myItr != varToRegisterMap.end()) {
					out << "move $t2 " << myItr->second << endl;
				}
				else {
					order = strToInt(right.substr(1));
					SymbolTableItem rightItem = globalSymbolTable.at(order);
					if (rightItem.getFuncName() == "GLOBAL") {
						getAddrOfGlobal(rightItem.getIdName(), "$t2", out);
					}
					else getAddrOfLocal(rightItem.getFuncName(), rightItem.getIdName(), "$t2", out);
					out << "lw $t2 0($t2)" << endl;
				}
			}
			else if (right.at(0) == 'T') {//临时变量表
				int g = strToInt(right.substr(1));
				if (g > TEMP_REGISTER) {
					getAddrOfTemp(g, "$t2", out);
					out << "lw $t2 0($t1)" << endl;
				}
				else {
					out << "move $t2 " << "$t" << (g + 3) << endl;
				}
			}
			else {//数字
				out << "li $t2 " << strToInt(right) << endl;
			}
			if (op == '+') {
				out << "addu $t1 $t1 $t2" << endl;
			}
			else if (op == '-') {
				out << "subu $t1 $t1 $t2" << endl;
			}
			else if (op == '*') {
				out << "mult $t1 $t2" << endl;
				out << "mflo $t1" << endl;
			}
			else if (op == '/') {
				out << "div $t1 $t2" << endl;
				out << "mflo $t1" << endl;
			}
		}
		else {
			if (item.op == '*')
				out << "move $t1 $0" << endl;
		}
	}
	else {
		//$t0存放target地址
		bool isTemp = false;
		bool isReg = false;
		int g, g1;
		string targetReg;
		if (item.target.at(0) == 'G') {//符号表内的变量
			map<string, string>::iterator myItr = varToRegisterMap.find(item.target);
			if (myItr != varToRegisterMap.end()) {
				targetReg = myItr->second;
				isReg = true;
			}
			else {
				int order = strToInt(item.target.substr(1));
				SymbolTableItem item = globalSymbolTable.at(order);
				if (item.getFuncName() == "GLOBAL") {
					getAddrOfGlobal(item.getIdName(), "$t0", out);
				}
				else getAddrOfLocal(item.getFuncName(), item.getIdName(), "$t0", out);
			}
		}
		else if (item.target.at(0) == 'T') {//临时变量
			g1 = g = strToInt(item.target.substr(1));
			if (g > TEMP_REGISTER) 
				getAddrOfTemp(strToInt(item.target.substr(1)), "$t0", out);
			else isTemp = true;
		}
		//分析left
		if (item.isRightArr) {
			//单纯的数组取值
			string left = item.left;
			int order = strToInt(left.substr(1));
			SymbolTableItem x = globalSymbolTable.at(order);
			//分析索引下标,将数组地址取出放在$t1
			if (x.getFuncName() == "GLOBAL")
				getAddrOfGlobal(x.getIdName(), "$t1", out);
			else getAddrOfLocal(x.getFuncName(), x.getIdName(), "$t1", out);
			//下标地址取出放在$t2
			string index2 = item.index2;
			if (index2.at(0) == 'G') {//在符号表
				map<string, string>::iterator myItr = varToRegisterMap.find(index2);
				if (myItr != varToRegisterMap.end()) {
					out << "move $t2 " << myItr->second << endl;
				}
				else {
					order = strToInt(index2.substr(1));
					SymbolTableItem index2Item = globalSymbolTable.at(order);
					if (index2Item.getFuncName() == "GLOBAL") {
						getAddrOfGlobal(index2Item.getIdName(), "$t2", out);
					}
					else getAddrOfLocal(index2Item.getFuncName(), index2Item.getIdName(), "$t2", out);
					out << "lw $t2 0($t2)" << endl;
				}
			}
			else if (index2.at(0) == 'T') {//在临时变量表
				g = strToInt(index2.substr(1));
				if (g > TEMP_REGISTER) {
					getAddrOfTemp(g, "$t2", out);
					out << "lw $t2 0($t2)" << endl;
				}
				else {
					out << "move $t2 " << "$t" << (g + 3) << endl;
				}
			}
			else {//纯数字
				out << "li $t2 " << strToInt(index2) << endl;
			}
			out << "li $t3 4" << endl;
			out << "mult $t2 $t3" << endl;
			out << "mflo $t2" << endl;
			out << "addu $t1 $t1 $t2" << endl;
			//取出数据,放在$t1
			out << "lw $t1 0($t1)" << endl;
		}
		else {
			//左右操作数
			int order;
			string left = item.left;
			string right = item.right;
			char op = item.op;
			if (left.at(0) == 'G') {//符号表
				map<string, string>::iterator myItr = varToRegisterMap.find(left);
				if (myItr != varToRegisterMap.end()) {
					out << "move $t1 " << myItr->second << endl;
				}
				else {
					order = strToInt(left.substr(1));
					SymbolTableItem leftItem = globalSymbolTable.at(order);
					if (leftItem.getFuncName() == "GLOBAL") {
						getAddrOfGlobal(leftItem.getIdName(), "$t1", out);
					}
					else {
						getAddrOfLocal(leftItem.getFuncName(), leftItem.getIdName(), "$t1", out);
					}
					out << "lw $t1 0($t1)" << endl;
				}
			}
			else if (left.at(0) == 'T') {//临时变量表
				g = strToInt(left.substr(1));
				if (g > TEMP_REGISTER) {
					getAddrOfTemp(g, "$t1", out);
					out << "lw $t1 0($t1)" << endl;
				}
				else {
					out << "move $t1 " << "$t" << (g + 3) << endl;
				}
			}
			else if (left == "Ret") {//函数调用的返回值
				out << "lw $t1 " << returnValueSpace << "($0)" << endl;
			}
			else {//数字
				out << "li $t1 " << strToInt(left) << endl;
			}
			if (right != "0") {
				if (right.at(0) == 'G') {//符号表
					map<string, string>::iterator myItr = varToRegisterMap.find(right);
					if (myItr != varToRegisterMap.end()) {
						out << "move $t2 " << myItr->second << endl;
					}
					else {
						order = strToInt(right.substr(1));
						SymbolTableItem rightItem = globalSymbolTable.at(order);
						if (rightItem.getFuncName() == "GLOBAL") {
							getAddrOfGlobal(rightItem.getIdName(), "$t2", out);
						}
						else {
							getAddrOfLocal(rightItem.getFuncName(), rightItem.getIdName(), "$t2", out);
						}
						out << "lw $t2 0($t2)" << endl;
					}
				}
				else if (right.at(0) == 'T') {//临时变量表
					g = strToInt(right.substr(1));
					if (g > TEMP_REGISTER) {
						getAddrOfTemp(g, "$t2", out);
						out << "lw $t2 0($t2)" << endl;
					}
					else {
						out << "move $t2 " << "$t" << (g + 3) << endl;
					}
				}
				else {//数字
					out << "li $t2 " << strToInt(right) << endl;
				}
				if (op == '+') {
					out << "addu $t1 $t1 $t2" << endl;
				}
				else if (op == '-') {
					out << "subu $t1 $t1 $t2" << endl;
				}
				else if (op == '*') {
					out << "mult $t1 $t2" << endl;
					out << "mflo $t1" << endl;
				}
				else if (op == '/') {
					out << "div $t1 $t2" << endl;
					out << "mflo $t1" << endl;
				}
			}
			else {
				if (item.op == '*')
					out << "move $t1 $0" << endl;
			}
		}
		if (isTemp) {
			out << "move $t" << (g1 + 3) << " $t1" << endl;
			return;
		}
		if (isReg) {
			out << "move " << targetReg << " $t1" << endl;
			return;
		}
	}
	//从$t1将值存入内存
	out << "sw $t1 0($t0)" << endl;
}

void op_helpFunctionDef(string funcName, ofstream& out) {
	if (funcName == "main") {
		//main函数只需要做全局变量的数据分配
		int size = getGlobalVarSumSpace();
		out << "li $fp " << funcBeginAddr + size << endl;
		out << "addiu $sp $fp 8" << endl;
		op_initializeStack(funcName, out);
	}
	else {
		//将$k0,$k1寄存器的值保存入栈
		out << "sw $k0 0($sp)" << endl;
		out << "sw $k1 4($sp)" << endl;
		out << "sw $t4 8($sp)" << endl;
		out << "sw $t5 12($sp)" << endl;
		out << "sw $t6 16($sp)" << endl;
		out << "sw $t7 20($sp)" << endl;
		out << "sw $t8 24($sp)" << endl;
		out << "sw $t9 28($sp)" << endl;
		out << "sw $s0 32($sp)" << endl;
		out << "sw $s1 36($sp)" << endl;
		out << "sw $s2 40($sp)" << endl;
		out << "sw $s3 44($sp)" << endl;
		out << "sw $s4 48($sp)" << endl;
		out << "sw $s5 52($sp)" << endl;
		out << "sw $s6 56($sp)" << endl;
		out << "sw $s7 60($sp)" << endl;
		out << "addiu $sp $sp 64" << endl;
		//设置上一级函数的基地址
		out << "sw $fp 4($sp)" << endl;
		//设置fp
		out << "move $fp $sp" << endl;
		//设置返回地址
		out << "sw $ra 0($fp)" << endl;
		//初始化栈空间
		out << "addiu $sp $fp 8" << endl;
		initializeStack(funcName, out);
	}
}

void op_helpBJump(FourYuanItem item, ofstream& out) {
	//把判断的变量值存入$a1寄存器中,再调用转移函数
	string obj = item.left;
	map<string, string>::iterator myItr = varToRegisterMap.find(obj);
	if (myItr != varToRegisterMap.end()) {
		out << "addu $a1 " << myItr->second << " $0" << endl;
	}
	else {
		if (obj.at(0) == 'G') {
			int order = strToInt(obj.substr(1));
			SymbolTableItem item = globalSymbolTable.at(order);
			if (item.getFuncName() == "GLOBAL") {
				getAddrOfGlobal(item.getIdName(), "$a1", out);
			}
			else {
				getAddrOfLocal(item.getFuncName(), item.getIdName(), "$a1", out);
			}
			out << "lw $a1 0($a1)" << endl;
		}
		else if (obj.at(0) == 'T') {
			int g = strToInt(obj.substr(1));
			if (g > TEMP_REGISTER) {
				getAddrOfTemp(g, "$a1", out);
				out << "lw $a1 0($a1)" << endl;
			}
			else {
				out << "move $a1 " << "$t" << (g + 3) << endl;
			}
		}
	}
	switch (item.type)
	{
	case BEZ:
		out << "beq $a1 $0 " << item.target << endl;
		break;
	case BNZ:
		out << "bne $a1 $0 " << item.target << endl;
		break;
	case BLZ:
		out << "bltz $a1 " << item.target << endl;
		break;
	case BLEZ:
		out << "blez $a1 " << item.target << endl;
		break;
	case BGZ:
		out << "bgtz $a1 " << item.target << endl;
		break;
	case BGEZ:
		out << "bgez $a1 " << item.target << endl;
		break;
	default:
		break;
	}
}

void op_helpReturn(ofstream& out, string funcName) {
	//栈指针恢复到$fp
	out << "move $sp $fp" << endl;
	//$k0 $k1 t4~t9 s0~s7寄存器值恢复,根据函数情况而定
	//顶层放s0~s7
	out << "lw $s7 -4($sp)" << endl;
	out << "lw $s6 -8($sp)" << endl;
	out << "lw $s5 -12($sp)" << endl;
	out << "lw $s4 -16($sp)" << endl;
	out << "lw $s3 -20($sp)" << endl;
	out << "lw $s2 -24($sp)" << endl;
	out << "lw $s1 -28($sp)" << endl;
	out << "lw $s0 -32($sp)" << endl;
	out << "lw $t9 -36($sp)" << endl;
	out << "lw $t8 -40($sp)" << endl;
	out << "lw $t7 -44($sp)" << endl;
	out << "lw $t6 -48($sp)" << endl;
	out << "lw $t5 -52($sp)" << endl;
	out << "lw $t4 -56($sp)" << endl;
	out << "lw $k1 -60($sp)" << endl;
	out << "lw $k0 -64($sp)" << endl;
	out << "addiu $sp $sp -64" << endl;
	//返回地址存入$ra
	out << "lw $ra 0($fp)" << endl;
	//函数栈区起始地址恢复--->上一级函数基地址$fp恢复
	out << "lw $fp 4($fp)" << endl;
	//执行jr
	out << "jr $ra" << endl;
}

void op_generateText(ofstream& out) {
	string funcName = "GLOBAL";
	out << "j main" << endl;
	for (unsigned int i = 0; i < globalItmCodeArr.size(); i++) {
		FourYuanItem item = globalItmCodeArr.at(i);
		map<string, string>::iterator myItr;
		if (item.type == ValueParamDeliver) {
			myItr = varToRegisterMap.find(item.target);
			if (myItr != varToRegisterMap.end()) {
				out << "sw " << myItr->second << " " << currentParamSp << "($0)" << endl;
				currentParamSp += 4;
			}
			else {
				if (item.target.at(0) == 'G') {
					int order = strToInt(item.target.substr(1));
					SymbolTableItem item = globalSymbolTable.at(order);
					if (item.getFuncName() == "GLOBAL") {
						getAddrOfGlobal(item.getIdName(), "$a2", out);
					}
					else {
						getAddrOfLocal(item.getFuncName(), item.getIdName(), "$a2", out);
					}
					out << "lw $a2 0($a2)" << endl;
				}
				else {//临时变量
					int g = strToInt(item.target.substr(1));
					if (g > TEMP_REGISTER) {
						getAddrOfTemp(g, "$a2", out);
						out << "lw $a2 0($a2)" << endl;
					}
					else {
						out << "move $a2 " << "$t" << (g + 3) << endl;
					}
				}
				out << "sw $a2 " << currentParamSp << "($0)" << endl;
				currentParamSp += 4;
			}
		}
		else if (item.type == FunctionCall) {
			currentParamSp = paramSpBegin;//回到起点
										  //函数调用,跳转,生成所跳转的目标函数运行栈空间的首地址
			out << "jal " << item.target << endl;//此时mars运行会将返回地址写入到$ra寄存器(函数执行时需要保存它)
		}
		else if (item.type == AssignState)
			op_helpAssignStatement(item, out);
		else if (item.type == Label)
			out << item.target << ":" << endl;
		else if (item.type == FunctionDef) {
			//函数定义要处理的大问题
			out << item.target << ":" << endl;//生成跳转标签
			funcName = item.target;
			op_helpFunctionDef(item.target, out);
		}
		else if (item.type == ParamDef) { ; }
		else if (item.type == Jump)
			out << "j " << item.target << endl;
		else if (item.type == BEZ || item.type == BNZ || item.type == BLZ || item.type == BLEZ || item.type == BGZ || item.type == BGEZ)
			helpBJump(item, out);
		else if (item.type == ReadChar || item.type == ReadInt) {
			if (item.type == ReadChar) out << "li $v0 12" << endl;
			else out << "li $v0 5" << endl;
			out << "syscall" << endl;
			myItr = varToRegisterMap.find(item.target);
			if (myItr != varToRegisterMap.end())
				out << "addu " << myItr->second << " $v0 $0" << endl;
			else {
				int order = strToInt(item.target.substr(1));
				SymbolTableItem item = globalSymbolTable.at(order);
				if (item.getFuncName() == "GLOBAL") {
					getAddrOfGlobal(item.getIdName(), "$a3", out);
				}
				else {
					getAddrOfLocal(item.getFuncName(), item.getIdName(), "$a3", out);
				}
				out << "sw $v0 0($a3)" << endl;
			}
		}
		else if (item.type == PrintStr) {
			cancelEscapeChar(item.target);
			map<string, string>::iterator iter = strAndLabel.find(item.target);
			if (iter != strAndLabel.end())
				out << "la $a0 " << iter->second << endl;
			out << "li $v0 4" << endl;
			out << "syscall" << endl;
		}
		else if (item.type == PrintChar) {
			out << "li $a0 " << (int)item.target.at(0) << endl;
			out << "li $v0 11" << endl;
			out << "syscall" << endl;
		}
		else if (item.type == PrintInt) {
			out << "li $a0 " << strToInt(item.target) << endl;
			out << "li $v0 1" << endl;
			out << "syscall" << endl;
		}
		else if (item.type == PrintId) {
			//判断是打印int还是char
			myItr = varToRegisterMap.find(item.target);
			if (myItr != varToRegisterMap.end()) {
				out << "addu $a0 " << myItr->second << " $0" << endl;
				if (item.isPrintCharId) out << "li $v0 11" << endl;
				else out << "li $v0 1" << endl;
				out << "syscall" << endl;
			}
			else {
				if (item.target.at(0) == 'G') {
					int order = strToInt(item.target.substr(1));
					SymbolTableItem item = globalSymbolTable.at(order);
					if (item.getFuncName() == "GLOBAL") {
						getAddrOfGlobal(item.getIdName(), "$a0", out);
					}
					else {
						getAddrOfLocal(item.getFuncName(), item.getIdName(), "$a0", out);
					}
					out << "lw $a0 0($a0)" << endl;
				}
				else if (item.target.at(0) == 'T') {
					int g = strToInt(item.target.substr(1));
					if (g > TEMP_REGISTER) {
						getAddrOfTemp(g, "$a0", out);
						out << "lw $a0 0($a0)" << endl;
					}
					else {
						out << "move $a0 " << "$t" << (g + 3) << endl;
					}
				}

				if (item.isPrintCharId) out << "li $v0 11" << endl;
				else out << "li $v0 1" << endl;
				out << "syscall" << endl;
			}
		}
		else if (item.type == ReturnInt || item.type == ReturnChar) {
			out << "li $v0 " << strToInt(item.target) << endl;
			out << "sw $v0 " << returnValueSpace << "($0)" << endl;
			//返回地址
			op_helpReturn(out, funcName);
		}
		else if (item.type == ReturnId) {
			myItr = varToRegisterMap.find(item.target);
			if (myItr != varToRegisterMap.end()) {
				out << "sw " << myItr->second << " " << returnValueSpace << "($0)" << endl;
				op_helpReturn(out, funcName);
			}
			else {
				if (item.target.at(0) == 'G') {
					int order = strToInt(item.target.substr(1));
					SymbolTableItem item = globalSymbolTable.at(order);
					if (item.getFuncName() == "GLOBAL") {
						getAddrOfGlobal(item.getIdName(), "$v0", out);
					}
					else {
						getAddrOfLocal(item.getFuncName(), item.getIdName(), "$v0", out);
					}
					out << "lw $v0 0($v0)" << endl;
				}
				else if (item.target.at(0) == 'T') {
					int g = strToInt(item.target.substr(1));
					if (g > TEMP_REGISTER) {
						getAddrOfTemp(g, "$v0", out);
						out << "lw $v0 0($v0)" << endl;
					}
					else {
						out << "move $v0 " << "$t" << (g + 3) << endl;
					}
				}
				out << "sw $v0 " << returnValueSpace << "($0)" << endl;
				//返回地址
				op_helpReturn(out, funcName);
			}
		}
		else if (item.type == ReturnEmpty)
			op_helpReturn(out, funcName);
		else if (item.type == EndProcedure) {
			out << "li $v0 10" << endl;
			out << "syscall" << endl;
		}
	}
}
*/
