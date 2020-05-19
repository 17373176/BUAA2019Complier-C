/* SyntaxAnalysis.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
 * Syntax 错误处理程序，输出错误后，直接跳过该错误分析
 * 包括词法分析、语法分析部分，主要是增加词法、语法、语义错误处理，识别目标：C语言
 * 增加预处理;_CRT_SECURE_NO_WARNINGS
 * 把测试数据设计文档也打包一起提交
 */

#include <iostream>
#include <cstdio>
#include <string>
#include <cctype>
#include <map>
#include <vector>
#include <algorithm>
#include "STManager.h"

 /* 头文件的包含关系：STM包含SymbolTableItem.h，SymbolTableItem.h包含ConstEnumDefine.h*/

  /*-------宏定义------*/
#define TEXT_LEN 512 * 512 * 4   // 输入文本代码长度
#define SYNTAX_WORD_NUM 1024 * 4 // 识别单词个数
#define KEY_WORD_NUM 40          // 关键字保留字个数
#define WORD_LEN 100             // 单词长度 
#define SYMBOL_NUM 40            // 单词类别码个数
#define SYNTAX_ELE_NUM 40        // 语法成分个数
#define SYNTAX_ERROR_NUM 40      // 错误类别个数

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

struct Token_Sym { // 类别码和相应单词
	SYM symbol; // 单词类别
	string word; // 单词
	int lines; // 单词所在行数
	Token_Sym() {
		this->symbol = INEXIST;
		this->lines = 0;
	}
};

struct Exp_ret { // 表达式类型
	string name; // 表达式返回的名字--->统一规定,表达式都需要有一个返回变量名(临时变量)
	bool surable; // 是否确定值
	VALUE_TYPE type; // 值类型
	int num;
	char ch;
	bool isEmpty; // 是否是空的
	Exp_ret() {
		this->surable = false;  this->type = Empty_Type; this->isEmpty = true; this->num = -1; this->ch = '\0';
	}
};

/*------全局变量------*/
string token_g;                          // 单词字符串
char textCodes[TEXT_LEN];                // 被处理的文本字符数组
int word_lines = 1;						 // 被处理文本单词所在行数
int index;                               // 处理文本的下标
int printWordIndex = 0;                  // 打印单词时使用的下标
int syntaxWordNo = 0;                    // 分析的单词个数
SYM nextSym;                             // 下一个单词的类别码
Token_Sym token_sym[SYNTAX_WORD_NUM];    // 存储识别完成的词法
STManager symbolTableManager;            // 符号表管理器
string reDeclareFuncName;			     // 所在域，声明的函数名称
bool hasReFunc = false;					 // 有返回值函数是否有return语句
int exp_int = 0;						 // 因子的值
char exp_ch = '\0';

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
int getNowSymLines();                   // 获得当前单词所在行数
void backPreSymbol();                   // 回退预读类型
void printSymbol(SYM);                  // 打印识别单词的类别码
void printSyntax(int);                  // 打印识别单词的语法成分
void analysisError(int, int);           // 代码分析错误显示地输出，参数<错误分析单词或语法，代码错误所在行>
void errorHandling(int, ERRORSYM);      // 错误处理，输出<错误所在行，错误类别>

/*------语法分析部分------*/
void syntaxProcedure();                 // ＜程序＞
void syntaxFunc();                      // <函数>
void syntaxcConstDeclare(string);            // ＜常量说明＞
void syntaxConstDefine(string);              // ＜常量定义＞
void constIntDefine(string);                 // 扩展迭代-INT常量定义
void constCharDefine(string);                // 扩展迭代-CHAR常量定义
void syntaxVarDeclare(string);           // ＜变量说明＞
bool syntaxVarDefine(SYM, string);                // ＜变量定义＞
void syntaxReFuncDefine(string);             // ＜有返回值函数定义＞
void syntaxNoReFuncDefine();           // ＜无返回值函数定义＞
bool syntaxDeclareHead();              // ＜声明头部＞
void syntaxParamTable(string);               // ＜参数表＞
void syntaxCompStatement(string);            // ＜复合语句＞
void syntaxStatementLine(string);            // ＜语句列＞
void syntaxStatement(string);                // <语句>
void syntaxConditionState(string);           // ＜条件语句＞
void syntaxCondition(string);                // <条件>
void syntaxLoopState(string);                // <循环语句>
void syntaxStep();                     // ＜步长＞
bool syntaxCallReFunc(string);         // <有返回值函数调用语句>
bool syntaxCallNoFunc(string);         // ＜无返回值函数调用语句＞
void syntaxValueParam(string, bool);               // ＜值参数表＞
bool syntaxAssignState(string);              // <赋值语句>
bool syntaxReadState(string);                // ＜读语句＞
bool syntaxWriteState(string);               // ＜写语句＞
bool syntaxString();                   // <字符串>
bool syntaxReturnState(string);              // ＜返回语句＞
Exp_ret syntaxExpre(string);                    // <表达式>
int syntaxTerm(string);                     // <项>
int syntaxFactor(string);                   // <因子>
void syntaxMain();                     // <主函数>

/*------main函数------*/
int main() {
	FILE* stream;
	if ((stream = freopen("testfile.txt", "r", stdin)) == NULL) return -1;
	//if ((stream = freopen("error.txt", "w", stdout)) == NULL) return -1;
	char ch;
	while ((ch = getchar()) != EOF)
		textCodes[index++] = ch;
	index = 0; // 复位
	while (getNextCh() != '\0') {
		backIndex();
		SYM symbol = getSymbol(); // 当前识别的单词类型
		if (symbol >= IDENFR && symbol <= INEXIST) { // 在类型码范围内的单词，包括不符合词法的，之后预读再处理
			token_sym[syntaxWordNo].symbol = symbol;
			token_sym[syntaxWordNo].word = token_g;
			token_sym[syntaxWordNo++].lines = word_lines;
		}
	}
	index = 0; // 复位
	/*for (int i = 0; i < syntaxWordNo; i++) {
		cout << token_sym[i].word << ' ' << token_sym[i].lines << endl;
	}*/
	syntaxProcedure(); // 调用语法分析程序
	return 0;
}

/*------函数定义------*/
char getNextCh() { // 获取下一个字符
	return textCodes[index++];
}

void backIndex() { // 退回到上一个字符
	index--;
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
	int a = 0;
	for (unsigned int i = 0; i < str.size(); i++) {
		a *= 10;
		a += str[i] - '0';
	}
	return a;
}

void initial() { // 对字符数组单词符都清空
	token_g.clear();
}

void analysisError(int code, int line) { // 显示地输出代码分析出现的错误
	cout << "第 " << line << " 行 " << token_sym[printWordIndex].word << " 单词语法分析出错:" << syntaxError[code] << endl;
}

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
		if (now_char_g == '+') {
			return PLUS;
		}
		else if (now_char_g == '-') {
			return MINU;
		}
		else if (now_char_g == '*') {
			return MULT;
		}
		else if (now_char_g == '/') {
			return DIV;
		}
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
		else if (now_char_g == ';') {
			return SEMICN;
		}
		else if (now_char_g == ',') {
			return COMMA;
		}
		else if (now_char_g == '(') {
			return LPARENT;
		}
		else if (now_char_g == ')') {
			return RPARENT;
		}
		else if (now_char_g == '[') {
			return LBRACK;
		}
		else if (now_char_g == ']') {
			return RBRACK;
		}
		else if (now_char_g == '{') {
			return LBRACE;
		}
		else if (now_char_g == '}') {
			return RBRACE;
		}
	}
	return INEXIST;
}

SYM getPreSymbol() { // 获取预读类型
	if (index < syntaxWordNo) {
		if (token_sym[index].symbol < INEXIST)
			return token_sym[index++].symbol;
		else {
			errorHandling(token_sym[index].lines, a_ErrorS); // 不符合词法或非法字符，预读有可能输出多次错误
			index++; // 往后跳一个单词
			return JUMP; // 往后读取下一个单词
		}
	}
	else exit(0);
}

string getTokenSymWord() { // 获得当前打印单词
	return token_sym[printWordIndex].word;
}

string getWord() {
	return token_sym[index - 1].word;
}

int getNowSymLines() { // 获得当前单词所在行数，这里由于已经预读了，Index已经+1,所以要减掉1
	return token_sym[index - 2].lines;
}

void backPreSymbol() { // 回退预读类型
	index--;
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

// ＜程序＞::=[＜常量说明＞][＜变量说明＞]{＜有返回值函数定义＞|＜无返回值函数定义＞}＜主函数＞
void syntaxProcedure() {
	nextSym = getPreSymbol(); // 获取第一个单词类别码
	syntaxcConstDeclare(GLOBAL_FUNCNAME);// [常量说明]
	syntaxVarDeclare(GLOBAL_FUNCNAME); // [变量说明]
	syntaxFunc();
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
	//else errorHandling(getNowSymLines(), my_ErrorS); // 非法常量定义
	printSyntax(3); // 打印<常量定义>
}

// 扩展迭代-INT常量定义
void constIntDefine(string funcName) {
	if (nextSym == IDENFR) { // 标识符
		string id = getWord();
		int num = 0; // 后面不一定有正确定义的整型
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == ASSIGN) { // 赋值
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (nextSym == PLUS || nextSym == MINU) { // 整数前可以有符号
				printSymbol(nextSym);
				nextSym = getPreSymbol();
			}
			if (nextSym == INTCON) { // 整数
				num = getNum(getWord()); // 获得整数
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
		//else errorHandling(getNowSymLines(), my_ErrorS); // 非法赋值
	}
	//else errorHandling(getNowSymLines(), my_ErrorS); // 非法标识符
}

// 扩展迭代-CHAR常量定义
void constCharDefine(string funcName) {
	if (nextSym == IDENFR) { // 标识符
		string id = getWord();
		char ch = '\0'; // 后面不一定有正确定义的字符
		printSymbol(nextSym);
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
		//else errorHandling(getNowSymLines(), my_ErrorS); // 非法赋值
	}
	//else errorHandling(getNowSymLines(), my_ErrorS); // 非法标识符
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
		//printSymbol(nextSym);
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
								nextSym = getPreSymbol();
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
					//else errorHandling(getNowSymLines(), my_ErrorS);
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
			printSymbol(nextSym);
			printSyntax(6); // 打印<声明头部>
			nextSym = getPreSymbol();
			if (nextSym == LPARENT) { // 左括号(
				return true;
			}
			//else errorHandling(getNowSymLines(), my_ErrorS); // 非法声明头部
		}
		//else errorHandling(getNowSymLines(), my_ErrorS); // 非法声明头部
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
		//symbolTable[{ getTokenSymWord(), tableDepth }] = { FUNC_K, VOID_T, 0 }; // 插入符号表
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
					if (nextSym == RBRACE) {
						printSymbol(nextSym);
					}
					//else errorHandling(getNowSymLines(), my_ErrorS); // 非法无返回值函数定义
				}
				else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
				//else errorHandling(getNowSymLines(), my_ErrorS); // 非法无返回值函数定义
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else errorHandling(getNowSymLines(), l_ErrorS);
		}
		//else errorHandling(getNowSymLines(), my_ErrorS); // 非法无返回值函数定义
	}
	//else errorHandling(getNowSymLines(), my_ErrorS); // 非法无返回值函数定义
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
	syntaxStatementLine(funcName);
	printSyntax(11); // 打印<复合语句>
	FUNC_TYPE funcType = symbolTableManager.idCheckInState(funcName);
	if (funcType != Void_Type && hasReFunc == false)
		errorHandling(getNowSymLines(), h_ErrorS); // 有返回值函数缺少return语句
}

// ＜语句列＞::={＜语句＞}
void syntaxStatementLine(string funcName) {
	syntaxStatement(funcName);
	while (nextSym == IFTK || nextSym == FORTK || nextSym == DOTK || nextSym == WHILETK
		|| nextSym == SCANFTK || nextSym == PRINTFTK || nextSym == IDENFR || nextSym == SEMICN
		|| nextSym == RETURNTK || nextSym == LBRACE) {
		syntaxStatement(funcName);
	}
	printSyntax(26); // 打印<语句列>
}

// 注意后面的分号，花括号，以及空语句
// <语句>::=<条件语句>|<循环语句>|'{'<语句列>'}'|<有返回值函数调用语句>;|<无返回值函数调用语句>;|<赋值语句>;|＜读语句＞;|＜写语句＞;|＜空＞;|＜返回语句＞;
void syntaxStatement(string funcName) {
	if (nextSym == SEMICN) { // 空语句只有分号
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		printSyntax(17); // 打印<语句>
	}
	else if (nextSym == IFTK) syntaxConditionState(funcName); // 在函数内部打印<语句>
	else if (nextSym == FORTK || nextSym == DOTK || nextSym == WHILETK) syntaxLoopState(funcName);
	else if (nextSym == LBRACE) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == IFTK || nextSym == FORTK || nextSym == DOTK || nextSym == WHILETK || nextSym == SCANFTK || nextSym == PRINTFTK || nextSym == IDENFR || nextSym == SEMICN || nextSym == RETURNTK) {
			syntaxStatementLine(funcName);
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
		reDeclareFuncName = id; // 将标识符作为之后需要计算的
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (syntaxCallReFunc(id)) { // 有返回值函数调用
			if (nextSym == SEMICN) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				printSyntax(17); // 打印<语句>
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else errorHandling(getNowSymLines(), k_ErrorS);
		}
		else if (syntaxCallNoFunc(id)) { // 无返回值函数调用
			if (nextSym == SEMICN) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				printSyntax(17); // 打印<语句>
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else errorHandling(getNowSymLines(), k_ErrorS);
		}
		else if (syntaxAssignState(funcName)) {
			if (nextSym == SEMICN) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				printSyntax(17); // 打印<语句>
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else errorHandling(getNowSymLines(), k_ErrorS);
		}
	}
	else if (syntaxReadState(funcName)) {
		if (nextSym == SEMICN) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			printSyntax(17); // 打印<语句>
		}
		else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
		else errorHandling(getNowSymLines(), k_ErrorS);
	}
	else if (syntaxWriteState(funcName)) {
		if (nextSym == SEMICN) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			printSyntax(17); // 打印<语句>
		}
		else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
		else errorHandling(getNowSymLines(), k_ErrorS);
	}
	else if (syntaxReturnState(funcName)) {
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
void syntaxConditionState(string funcName) {
	if (nextSym == IFTK) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LPARENT) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			syntaxCondition(funcName);
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else errorHandling(getNowSymLines(), l_ErrorS);
			syntaxStatement(funcName);
			if (nextSym == ELSETK) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				syntaxStatement(funcName);
			}
			printSyntax(19); // 打印<条件语句>
			printSyntax(17); // 打印<语句>
		}
		else errorHandling(getNowSymLines(), my_ErrorS);
	}
}

// ＜条件＞::=＜表达式＞＜关系运算符＞＜表达式＞|＜表达式＞ 表达式为0条件为假，否则为真
// ＜关系运算符＞::= <｜<=｜>｜>=｜!=｜==
void syntaxCondition(string funcName) {
	bool conditionFlag = false;
	Exp_ret exp = syntaxExpre(funcName), exp2;
	if (!exp.isEmpty) conditionFlag = true;
	if (nextSym == LSS || nextSym == LEQ || nextSym == GRE || nextSym == GEQ || nextSym == NEQ || nextSym == EQL) {
		conditionFlag = true;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		exp2 = syntaxExpre(funcName);
	}
	// 判断表达式是否为1或0,当添加只存在一个<表达式>的时候才需要判断
	if (!exp2.isEmpty && exp.type != Int_Type)
		errorHandling(getNowSymLines(), f_ErrorS); // 条件类型错误
	else if (!(exp.type == Int_Type && exp2.type == Int_Type)) // 需要判断两个表达式是否都是整型
		errorHandling(getNowSymLines(), f_ErrorS); // 条件类型错误
	if (conditionFlag) printSyntax(20); // 打印<条件>
}

// <循环语句>::=while'('＜条件＞')'＜语句＞|do＜语句＞while'('＜条件＞')'|for'('＜标识符＞＝＜表达式＞;＜条件＞;＜标识符＞＝＜标识符＞(+|-)＜步长＞')'＜语句＞
void syntaxLoopState(string funcName) {
	bool loopFlag = false;
	if (nextSym == FORTK) {
		loopFlag = true;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LPARENT) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (nextSym == IDENFR) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				if (nextSym == ASSIGN) {
					printSymbol(nextSym);
					nextSym = getPreSymbol();
					Exp_ret exp = syntaxExpre(funcName);
					if (nextSym == SEMICN) {
						printSymbol(nextSym);
						nextSym = getPreSymbol();
						syntaxCondition(funcName);
						if (nextSym == SEMICN) {
							printSymbol(nextSym);
							nextSym = getPreSymbol();
							if (nextSym == IDENFR) {
								printSymbol(nextSym);
								nextSym = getPreSymbol();
								if (nextSym == ASSIGN) {
									printSymbol(nextSym);
									nextSym = getPreSymbol();
									if (nextSym == IDENFR) {
										printSymbol(nextSym);
										nextSym = getPreSymbol();
										if (nextSym == PLUS || nextSym == MINU) {
											printSymbol(nextSym);
											nextSym = getPreSymbol();
											syntaxStep();
											if (nextSym == RPARENT) {
												printSymbol(nextSym);
												nextSym = getPreSymbol();
												syntaxStatement(funcName);
												if (loopFlag) {
													printSyntax(21); // 打印<循环语句>
													printSyntax(17); // 打印<语句>
												}
											}
											else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
											else errorHandling(getNowSymLines(), l_ErrorS);
										}
										//else errorHandling(getNowSymLines(), my_ErrorS);
									}
									//else errorHandling(getNowSymLines(), my_ErrorS);
								}
								//else errorHandling(getNowSymLines(), my_ErrorS);
							}
							//else errorHandling(getNowSymLines(), my_ErrorS);
						}
						else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
						else errorHandling(getNowSymLines(), k_ErrorS); // 应该为分号
					}
					else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
					else errorHandling(getNowSymLines(), k_ErrorS); // 应该为分号
				}
				//else errorHandling(getNowSymLines(), my_ErrorS);
			}
			//else errorHandling(getNowSymLines(), my_ErrorS);
		}
		//else errorHandling(getNowSymLines(), my_ErrorS);
	}
	else if (nextSym == WHILETK) {
		loopFlag = true;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LPARENT) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			syntaxCondition(funcName);
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				syntaxStatement(funcName);
				if (loopFlag) {
					printSyntax(21); // 打印<循环语句>
					printSyntax(17); // 打印<语句>
				}
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else errorHandling(getNowSymLines(), l_ErrorS);
		}
		//else errorHandling(getNowSymLines(), my_ErrorS);
	}
	else if (nextSym == DOTK) {
		loopFlag = true;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		syntaxStatement(funcName); // 告诉它这是do循环的while
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
			syntaxCondition(funcName);
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
		//else errorHandling(getNowSymLines(), my_ErrorS);
	} // 其他语句情况
}

// ＜步长＞::=＜无符号整数＞
void syntaxStep() {
	if (nextSym == INTCON) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		printSyntax(7); // <无符号整数>
		printSyntax(22); // 打印<步长>
	}
	//else errorHandling(getNowSymLines(), my_ErrorS);
}

// ＜有返回值函数调用语句＞::=＜标识符＞'('＜值参数表＞')'  
bool syntaxCallReFunc(string funcName) {
	if (nextSym == LPARENT) {
		FUNC_TYPE type = symbolTableManager.idCheckInState(funcName);
		if (type != Void_Type) { // 有返回值的函数调用
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			syntaxValueParam(funcName, true);
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
bool syntaxCallNoFunc(string funcName) {
	if (nextSym == LPARENT) {
		FUNC_TYPE type = symbolTableManager.idCheckInState(funcName);
		if (type != NotDefine_Func) { // 函数调用
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			syntaxValueParam(funcName, false);
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
		//else errorHandling(getNowSymLines(), my_ErrorS);
	}
	return false;
}

// ＜值参数表＞::=＜表达式＞{ ,＜表达式＞ }｜＜空＞
void syntaxValueParam(string funcName, bool isCallRe) {
	vector<string> paramTable; // 参数名称
	vector<VALUE_TYPE> retParamTable; // 参数值类型
	Exp_ret retPara = syntaxExpre(funcName); // 得到当前值参数类型
	paramTable.push_back(retPara.name);
	retParamTable.push_back(retPara.type);
	while (nextSym == COMMA) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		retPara = syntaxExpre(funcName);
		paramTable.push_back(retPara.name);
		retParamTable.push_back(retPara.type);
	}
	int reCheck = symbolTableManager.funcCallCheck(funcName, isCallRe, retParamTable);
	if (reCheck == 1)
		errorHandling(getNowSymLines(), d_ErrorS); // 参数个数不匹配
	else if (reCheck == 2)
		errorHandling(getNowSymLines(), e_ErrorS); // 参数类型不匹配
	printSyntax(25); // 打印<值参数表>
}

// <赋值语句>::=(＜标识符＞＝＜表达式＞)|(＜标识符＞'['＜表达式＞']'=＜表达式＞)
bool syntaxAssignState(string funcName) {
	string id = reDeclareFuncName;
	if (nextSym == ASSIGN) {
		int checkre = symbolTableManager.checkAssignId(id, funcName);
		if (checkre == -1) errorHandling(getNowSymLines(), c_ErrorS); // 未定义
		else if (checkre == -3) errorHandling(getNowSymLines(), j_ErrorS); // 常量不能被赋值
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		Exp_ret exp = syntaxExpre(funcName);
		if (!exp.isEmpty) {
			if (checkre >= 0 && symbolTableManager.getItemValueType(checkre) != exp.type) // 赋值类型不匹配
				//errorHandling(getNowSymLines(), my_ErrorS);
				printSyntax(18); // 打印<赋值语句>
			return true;
		}
		//else errorHandling(getNowSymLines(), my_ErrorS);
	}
	else if (nextSym == LBRACK) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		Exp_ret exp = syntaxExpre(funcName); // 下标表达式
		int index = 1;
		VALUE_TYPE type = Empty_Type;
		//if (exp.isEmpty) errorHandling(getNowSymLines(), my_ErrorS); // 下标不能为空
		if (exp.type != Int_Type) errorHandling(getNowSymLines(), i_ErrorS); // 下标表达式要为整型
		int order = symbolTableManager.idArrExpCheck(id, funcName, true, index); // 临时获得符号表项号
		if (order == -1) errorHandling(getNowSymLines(), c_ErrorS); // 未定义
		//else if (order == -2) errorHandling(getNowSymLines(), my_ErrorS); // 越界
		else type = symbolTableManager.getItemValueType(order); // 得到数组值类型
		if (nextSym == RBRACK) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (nextSym == ASSIGN) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				Exp_ret exp2 = syntaxExpre(funcName); // 赋值右边表达式
				if (!exp2.isEmpty) {
					//if (exp2.type != type) errorHandling(getNowSymLines(), my_ErrorS); // 赋值类型不匹配
					printSyntax(18); // 打印<赋值语句>
					return true;
				}
				//else errorHandling(getNowSymLines(), my_ErrorS);
			}
			//else errorHandling(getNowSymLines(), my_ErrorS);
		}
		else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
		else errorHandling(getNowSymLines(), m_ErrorS); // 应为右方括号
	}
	return false;
}

// ＜读语句＞::=scanf'('＜标识符＞{ ,＜标识符＞ }')'
bool syntaxReadState(string funcName) {
	if (nextSym == SCANFTK) { // scanf
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LPARENT) { // (
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (nextSym == IDENFR) { // 标识符
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				while (nextSym == COMMA) { // , 循环继续
					printSymbol(nextSym);
					nextSym = getPreSymbol();
					if (nextSym == IDENFR) { // 标识符
						printSymbol(nextSym);
						nextSym = getPreSymbol();
					}
					//else errorHandling(getNowSymLines(), my_ErrorS);
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
			//else errorHandling(getNowSymLines(), my_ErrorS);
		}
		//else errorHandling(getNowSymLines(), my_ErrorS);
	} // 非读语句，其他情况
	return false;
}

// <写语句>::=printf '('<字符串>,<表达式> ')'|printf'('<字符串>')'|printf'('<表达式>')'
bool syntaxWriteState(string funcName) {
	if (nextSym == PRINTFTK) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LPARENT) { // (
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			Exp_ret exp = syntaxExpre(funcName);
			if (!exp.isEmpty) { ; } // 只识别表达式即可
			else if (syntaxString()) {
				if (nextSym == COMMA) {
					printSymbol(nextSym);
					nextSym = getPreSymbol();
					exp = syntaxExpre(funcName);
				}
			}
			//else errorHandling(getNowSymLines(), my_ErrorS);
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				printSyntax(28); // 打印<写语句>
				return true;
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else {
				errorHandling(getNowSymLines(), l_ErrorS);
				while (nextSym != SEMICN) nextSym = getPreSymbol();
			}
		}
		//else errorHandling(getNowSymLines(), my_ErrorS);
	}
	return false;
}

bool syntaxString() { // <字符串>
	if (nextSym == STRCON) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		printSyntax(0); // 打印<字符串>
		return true;
	}
	return false;
}

// ＜返回语句＞::=return['('＜表达式＞')']   
bool syntaxReturnState(string funcName) {
	if (nextSym == RETURNTK) {
		hasReFunc = true; // 有return语句
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		bool has_ret = false; // 是否有返回值
		if (nextSym == LPARENT) {
			has_ret = true;
			VALUE_TYPE value_T = Int_Type; // 默认
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			Exp_ret exp = syntaxExpre(funcName);
			//if (exp.isEmpty) errorHandling(getNowSymLines(), my_ErrorS);
			value_T = exp.type; // 表达式值类型
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

		} // 可以没有括号表达式
		if (has_ret) // 如果没有括号表达式则无返回值
			if (symbolTableManager.idCheckInState(funcName) == Void_Type) // 如果是void函数，且return语句有表达式
				errorHandling(getNowSymLines(), g_ErrorS); // 无返回值函数存在不匹配return语句
		printSyntax(29); // 打印<返回语句>
		return true;
	}
	return false;
}

// <表达式>::=[+-]＜项＞{＜加法运算符＞＜项＞} [+|-]只作用于第一个<项>
// ＜加法运算符＞::= +｜-
Exp_ret syntaxExpre(string funcName) {
	int returnFlag = -1; // 1代表整型2代表字符
	// 确定表达式值类型
	Exp_ret exp;
	if (nextSym == PLUS || nextSym == MINU) { // 可以有也可以没有
		returnFlag = 1;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
	}
	returnFlag = syntaxTerm(funcName);
	while (nextSym == PLUS || nextSym == MINU) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		returnFlag = syntaxTerm(funcName);
	}
	if (returnFlag != -1) {
		exp.isEmpty = false;
		exp.type = returnFlag == 1 ? Int_Type : Char_Type;
		printSyntax(14); // 打印<表达式>
	}
	return exp;
}

// <项>::=＜因子＞{＜乘法运算符＞＜因子＞} 返回1代表整数，2代表字符,-1代表没有
// <乘法运算符>::= *｜/ 
int syntaxTerm(string funcName) {
	int termFlag = syntaxFactor(funcName);
	if (termFlag) {
		while (nextSym == MULT || nextSym == DIV) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			termFlag = syntaxFactor(funcName);
		}
		printSyntax(15); // 打印<项>
	}
	return termFlag;
}

// <因子>::=<标识符>|<标识符>'['<表达式>']'|'('<表达式>')'|<整数>|<字符>|<有返回值函数调用语句> 
// <标识符>和<有返回值函数调用>冲突,返回1代表整数，2代表字符
int syntaxFactor(string funcName) {
	int factorFlag = -1;
	if (nextSym == IDENFR) {
		string id = getWord();
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LBRACK) { // 有可能是数组
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			Exp_ret exp = syntaxExpre(funcName);
			int index = 1;
			//if (exp.isEmpty) errorHandling(getNowSymLines(), my_ErrorS); // 下标不能为空
			if (exp.type != Int_Type) errorHandling(getNowSymLines(), i_ErrorS); // 下标表达式要为整型
			factorFlag = symbolTableManager.idArrExpCheck(id, funcName, true, index); // 临时获得符号表项号
			if (factorFlag == -1) errorHandling(getNowSymLines(), c_ErrorS); // 未定义
			//else if (factorFlag == -2) errorHandling(getNowSymLines(), my_ErrorS); // 越界
			else factorFlag = symbolTableManager.getItemValueType(factorFlag); // 得到数组值类型
			if (nextSym == RBRACK) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else errorHandling(getNowSymLines(), m_ErrorS); // 应为右方括号
		}
		else if (syntaxCallReFunc(id)) // 可能是有返回值函数调用,这里函数调用名是id，funcname是改调用语句所属函数
			factorFlag = symbolTableManager.idCheckInState(id) == ReturnInt_Type ? 1 : 2; // 得到函数类型
		if (factorFlag == -1) { // 不是数组也不是有返回值函数调用,就是简单变量或参数
			factorFlag = symbolTableManager.idCheckInFactor(id, funcName);
			if (factorFlag == -1) errorHandling(getNowSymLines(), c_ErrorS); // 未定义
			else factorFlag = symbolTableManager.getItemValueType(factorFlag) == Int_Type ? 1 : 2;
		}
	}
	else if (nextSym == LPARENT) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		Exp_ret exp = syntaxExpre(funcName);
		if (exp.isEmpty);//errorHandling(getNowSymLines(), my_ErrorS);
		else {
			factorFlag = exp.type == Int_Type ? 1 : 2;
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
			}
			else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
			else errorHandling(getNowSymLines(), l_ErrorS);
		}
	}
	else if (nextSym == INTCON) {
		factorFlag = 1;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		printSyntax(7); // 打印无符号整数
		printSyntax(8); // 打印整数
	}
	else if (nextSym == CHARCON) {
		factorFlag = 2;
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
				if (nextSym == RBRACE) {
					printSymbol(nextSym);
					printSyntax(13); // 打印<主函数>
					printSyntax(1); // 打印<程序>
					nextSym = getPreSymbol();
					errorHandling(0, my_ErrorS); // 主函数后有多余代码
				}
				else if (nextSym == JUMP) nextSym = getPreSymbol();// 跳过
				//else errorHandling(getNowSymLines(), my_ErrorS); // 右大括号
			}
			//else errorHandling(getNowSymLines(), my_ErrorS); // 缺左大括号
		}
		//else errorHandling(getNowSymLines(), my_ErrorS);
	}
}

