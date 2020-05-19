// Parsing.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
// parsing 语法分析程序，包括词法分析部分，识别目标：C语言
// 增加预处理;_CRT_SECURE_NO_WARNINGS
// 把测试数据设计文档也打包一起提交

#include <iostream>
#include <cstdio>
#include <string>
#include <cctype>
#include <map>
#include <algorithm>

/*-------宏定义------*/
#define TEXT_LEN 512 * 512 * 4
#define SYNTAX_WORD 1024 * 4
#define KEY_WORD_ARR 40
#define WORD_NAME_LEN 100
#define SYMBOL_NUM 40
#define SYNTAX_ELE_NUM 40
#define SYNTAX_ERROR_NUM 40

using namespace std;

/*------常量定义------*/
enum SYM { // 类别码
	IDENFR, INTCON, CHARCON, STRCON, CONSTTK, INTTK, CHARTK, VOIDTK, MAINTK,
	IFTK, ELSETK, DOTK, WHILETK, FORTK, SCANFTK, PRINTFTK, RETURNTK, PLUS,
	MINU, MULT, DIV, LSS, LEQ, GRE, GEQ, EQL, NEQ, ASSIGN, SEMICN, COMMA,
	LPARENT, RPARENT, LBRACK, RBRACK, LBRACE, RBRACE, INEXIST
};

enum KIND {
	CON_K, VAR_K, PARA_K, FUNC_K
};

enum TYPE {
	INT_T, CHAR_T, STRING_T, INTARR_T, CHARARR_T, VOID_T, RE_T
};

const string SYM_to_str[SYMBOL_NUM] = { // 类别码映射字符串数组
	"IDENFR", "INTCON", "CHARCON", "STRCON", "CONSTTK", "INTTK", "CHARTK", "VOIDTK", "MAINTK",
	"IFTK", "ELSETK", "DOTK", "WHILETK", "FORTK", "SCANFTK", "PRINTFTK", "RETURNTK", "PLUS",
	"MINU", "MULT", "DIV", "LSS", "LEQ", "GRE", "GEQ", "EQL", "NEQ", "ASSIGN", "SEMICN",
	"COMMA", "LPARENT", "RPARENT", "LBRACK", "RBRACK", "LBRACE", "RBRACE", "INEXIST"
};

const string wordNameArr[KEY_WORD_ARR] = { // 保留字单词
	"const", "int", "char", "void", "main", "if", "else",
	"do", "while", "for", "scanf", "printf", "return"
};

const string syntaxWord[SYNTAX_ELE_NUM] = { // 语法成分
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

struct SYMBOLTABLE { // 符号表
	//string name;
	//KIND kind;
	TYPE type;
	bool operator < (const SYMBOLTABLE& a) const {
		return type < a.type;
	}
};

struct KEY { // hash_map<key, value>
	string name;
	//TYPE type;
	bool operator < (const KEY& a) const {
		return name < a.name;
	}
};

struct Token_Sym { // 类别码和相应单词
	SYM symbol;
	string word;
	Token_Sym() {
		this->symbol = INEXIST;
		this->word = "";
	}
};

/*------全局变量------*/
string token_g; // 单词字符串
char textCodes[TEXT_LEN]; // 被处理的文本字符数组
int index; // 处理文本的下标
int printWordIndex = 0; // 打印单词时使用的下标
int syntaxWordNo = 0; // 分析的单词个数
SYM nextSym; // 下一个单词的类别码
Token_Sym token_sym[SYNTAX_WORD]; // 存储识别完成的词法
map <KEY, SYMBOLTABLE> symbolTable; // 定义符号表

/*------函数声明------*/
char getNextCh(); // 获取下一个字符
void backIndex(); // 退回到上一个字符
bool isAlnum(char ch); // 判断是否符合文法里的字母数字，包括下划线
bool isChar(char ch); // 判断是否符合文法里的字符，包括下划线
bool isString(char ch); // 判断是否符合文法里字符串中的字符
int isReserve(); // 判断token中的字符串是保留字还是标识符
void initial(); // 对字符数组单词符都清空
void analysisError(); // 语法分析错误退出
SYM getSymbol(); // 获取相应的类型
SYM getPreSymbol(); // 获取预读类型
string getTokenSymWord(); // 获得当前打印单词
void backPreSymbol(); // 回退预读类型
void printSymbol(SYM); // 打印识别单词的类别码
void printSyntax(int); // 打印识别单词的语法成分
void errorHandling(int, int); // 错误处理

/*------语法分析部分------*/
void syntaxProcedure(); // ＜程序＞
void syntaxFunc(); // <函数>
void syntaxcConstDeclare(); // ＜常量说明＞
void syntaxConstDefine(); // ＜常量定义＞
void constIntDefine(); // 扩展迭代-INT常量定义
void constCharDefine(); // 扩展迭代-CHAR常量定义
void syntaxVarDeclare(bool); // ＜变量说明＞
bool syntaxVarDefine(); // ＜变量定义＞
void syntaxReFuncDefine(); // ＜有返回值函数定义＞
void syntaxNoReFuncDefine(); // ＜无返回值函数定义＞
bool syntaxDeclareHead(); // ＜声明头部＞
void syntaxParamTable(); // ＜参数表＞
void syntaxCompStatement(); // ＜复合语句＞
void syntaxStatementLine(); // ＜语句列＞
void syntaxStatement(); // <语句>
void syntaxConditionState(); // ＜条件语句＞
void syntaxCondition(); // <条件>
void syntaxLoopState(); // <循环语句>
void syntaxStep(); // ＜步长＞
bool syntaxCallReFunc(string); // <有返回值函数调用语句>
bool syntaxCallNoFunc(string); // ＜无返回值函数调用语句＞
void syntaxValueParam(); // ＜值参数表＞
bool syntaxAssignState(); // <赋值语句>
bool syntaxReadState(); // ＜读语句＞
bool syntaxWriteState(); // ＜写语句＞
bool syntaxString(); // <字符串>
bool syntaxReturnState(); // ＜返回语句＞
bool syntaxExpre(); // <表达式>
bool syntaxTerm(); // <项>
bool syntaxFactor(); // <因子>
void syntaxMain(); // <主函数>

/*------main函数------*/
int main() {
	FILE* stream;
	if ((stream = freopen("testfile.txt", "r", stdin)) == NULL) return -1;
	//if ((stream = freopen("output.txt", "w", stdout)) == NULL) return -1;
	char ch;
	while ((ch = getchar()) != EOF)
		textCodes[index++] = ch;
	index = 0; // 复位
	while (getNextCh() != '\0') {
		backIndex();
		SYM symbol = getSymbol(); // 当前识别的单词类型
		if (symbol >= IDENFR && symbol < INEXIST) { // 在类型码范围内的单词
			token_sym[syntaxWordNo].symbol = symbol;
			token_sym[syntaxWordNo++].word = token_g;
		}
		else analysisError(); // 分析失败
	}
	index = 0; // 复位
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

bool isString(char ch) { // 判断是否符合文法里字符串中的字符
	if (ch == 32 || ch == 33 || (ch >= 35 && ch <= 126))
		return true;
	return false;
}

int isReserve() { // 判断token中的字符串是保留字还是标识符
	for (int i = 0; i < KEY_WORD_ARR; i++) {
		if (token_g == wordNameArr[i]) return i + 4; // 返回保留字字符串数组对应到相应的类别码
	}
	return -1;
}

void initial() { // 对字符数组单词符都清空
	token_g.clear();
}

void analysisError() {
	//printf("词法分析存在不合法的单词\n");
	//exit(1);
}

SYM getSymbol() { // 获取相应的类型
	initial(); //初始化
	char now_char_g = getNextCh(); // 当前读入字符
	while (isspace(now_char_g)) { // 跳过空格、回车，不会影响分词和语句的识别
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
		else { // 是标识符
			return IDENFR;
		}
	}
	else if (isdigit(now_char_g)) {
		while (isdigit(now_char_g)) {
			token_g += now_char_g;
			now_char_g = getNextCh();
		}
		backIndex(); // 不是数字时，结束并回退
		if ((token_g[0] != '0' && token_g.size() > 1) || token_g.size() == 1)
			return INTCON;
		else analysisError();
	}
	else if (now_char_g == '\'') { // 字符常量，左单引号
		now_char_g = getNextCh();
		if (isChar(now_char_g)) {
			token_g += now_char_g;
			now_char_g = getNextCh();
			if (now_char_g == '\'') // 右单引号
				return CHARCON;
		}
	}
	else if (now_char_g == '\"') { // 字符串常量，左双引号
		now_char_g = getNextCh();
		while (isString(now_char_g)) {
			token_g += now_char_g;
			now_char_g = getNextCh();
		}
		if (now_char_g == '\"') // 右双引号
			return STRCON;
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
		return token_sym[index++].symbol;
	}
	else exit(0);
}

string getTokenSymWord() { // 获得当前打印单词
	return token_sym[printWordIndex].word;
}

void backPreSymbol() { // 回退预读类型
	index--;
}

void printSymbol(SYM symbol) { // 打印识别单词的类别码
	cout << SYM_to_str[token_sym[printWordIndex].symbol] << " " << token_sym[printWordIndex].word << endl;
	printWordIndex++;
}

void printSyntax(int index) { // 打印识别单词的语法成分
	cout << '<' << syntaxWord[index] << '>' << endl;
}

void errorHandling(int code, int line) { // 错误处理函数
	cout << "第 " << line << " 行 " << token_sym[printWordIndex].word << " 单词语法分析出错:" << syntaxError[code] << endl;
}

// ＜程序＞::=[＜常量说明＞][＜变量说明＞]{＜有返回值函数定义＞|＜无返回值函数定义＞}＜主函数＞
void syntaxProcedure() { 
	nextSym = getPreSymbol(); // 获取第一个单词类别码
	syntaxcConstDeclare();// [常量说明]
	syntaxVarDeclare(false); // [变量说明]
	syntaxFunc();
}

// <函数定义>
void syntaxFunc() {
	while (syntaxDeclareHead()) {
		if (nextSym == LPARENT) { // 左括号( 并且不是在复合语句里，复合语句里变量定义没有函数
			printSymbol(nextSym); // 打印左括号
			nextSym = getPreSymbol();
			syntaxReFuncDefine(); // [有返回值函数定义]
		}
	}
	while (nextSym == VOIDTK) syntaxNoReFuncDefine();	// [无返回值函数定义]（非main函数）
	while (nextSym == INTTK || nextSym == CHARTK || nextSym == VOIDTK) syntaxFunc();
}

// ＜常量说明＞::=const＜常量定义＞;{const＜常量定义＞;}
void syntaxcConstDeclare() { 
	bool conDeclareFlag = false;
	while (nextSym == CONSTTK) { // const
		conDeclareFlag = true; // 程序有常量说明
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		syntaxConstDefine();
		if (nextSym == SEMICN) printSymbol(SEMICN); // 分号;为每一行结束
		else errorHandling(4, __LINE__); // 非法分号
		nextSym = getPreSymbol(); // 获取下一个单词类别
	}
	if (conDeclareFlag) printSyntax(2); // 打印常量说明
}

// ＜常量定义＞::=int＜标识符＞＝＜整数＞{,＜标识符＞＝＜整数＞}|char＜标识符＞＝＜字符＞{,＜标识符＞＝＜字符＞}
void syntaxConstDefine() { 
	if (nextSym == INTTK) { // int
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		constIntDefine();
		while (nextSym == COMMA) { // 逗号,
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			constIntDefine();
		} // 识别完成，还剩下分号
	}
	else if (nextSym == CHARTK) { // char
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		constCharDefine();
		while (nextSym == COMMA) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			constCharDefine();
		}
	}
	else errorHandling(3, __LINE__); // 非法常量定义
	printSyntax(3); // 打印<常量定义>
}

// 扩展迭代-INT常量定义
void constIntDefine() { 
	if (nextSym == IDENFR) { // 标识符
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
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				printSyntax(7); // <无符号整数>
				printSyntax(8); // <整数>
			}
			else errorHandling(8, __LINE__); // 非法整数
		}
		else errorHandling(3, __LINE__); // 非法赋值
	}
	else errorHandling(3, __LINE__); // 非法标识符
}

// 扩展迭代-CHAR常量定义
void constCharDefine() {
	if (nextSym == IDENFR) { // 标识符
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == ASSIGN) { // 赋值
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (nextSym == CHARCON) {
				printSymbol(nextSym); // 字符
				nextSym = getPreSymbol();
			}
			else errorHandling(3, __LINE__); // 非法
		}
		else errorHandling(3, __LINE__); // 非法赋值
	}
	else errorHandling(3, __LINE__); // 非法标识符
}

// 检查是否与有返回值函数定义冲突
void syntaxVarDeclare(bool funcFlag) { // ＜变量说明＞::=＜变量定义＞;{＜变量定义＞;}
	bool varDeclareFlag = false;
	if (funcFlag) { // 函数里变量定义不会与函数定义冲突
		while (nextSym == INTTK || nextSym == CHARTK) { // 类型标识符
			//printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (syntaxVarDefine()) { // 变量定义
				varDeclareFlag = true;
				if (nextSym == SEMICN) {
					printSymbol(SEMICN); // 分号;为每一行结束
					nextSym = getPreSymbol(); // 获取下一个单词类别
				}
			}
		}
	}
	else {
		while (nextSym == INTTK || nextSym == CHARTK) { // 类型标识符
			//printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (syntaxVarDefine()) { // 变量定义
				varDeclareFlag = true;
				if (nextSym == SEMICN) {
					printSymbol(SEMICN); // 分号;为每一行结束
					nextSym = getPreSymbol(); // 获取下一个单词类别
				}
			}
			else {
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
bool syntaxVarDefine() { 
	bool varDefineF = false;
	bool flag = false; // 是否预读输出标记
	if (nextSym == IDENFR) { // 标识符
		//printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LBRACK) { // 左方括号[
			varDefineF = true;
			flag = true;
			printSymbol(nextSym);
			printSymbol(nextSym);
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (nextSym == INTCON) { // 无符号整数
				printSymbol(nextSym);
				printSyntax(7);
				nextSym = getPreSymbol();
				if (nextSym == RBRACK) { // 右方括号]
					printSymbol(nextSym);
					nextSym = getPreSymbol();
				}
				else errorHandling(5, __LINE__); // 非法右方括号
			}
		}
		else if (nextSym == SEMICN) {
			flag = true;
			printSymbol(nextSym);
			printSymbol(nextSym);
			varDefineF = true;
		}
		if (nextSym == COMMA) {
			varDefineF = true;
			if (!flag) {
				printSymbol(nextSym);
				printSymbol(nextSym);
			}
			while (nextSym == COMMA) { // 迭代
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				if (nextSym == IDENFR) { // 标识符
					printSymbol(nextSym);
					nextSym = getPreSymbol();
					if (nextSym == LBRACK) { // 左方括号[
						varDefineF = true;
						printSymbol(nextSym);
						nextSym = getPreSymbol();
						if (nextSym == INTCON) { // 无符号整数
							printSymbol(nextSym);
							printSyntax(7);
							nextSym = getPreSymbol();
							if (nextSym == RBRACK) { // 右方括号]
								printSymbol(nextSym);
								nextSym = getPreSymbol();
							}
							else errorHandling(5, __LINE__); // 非法右方括号
						}
						else errorHandling(5, __LINE__);
					} // 可以只有标识符
				}
				else errorHandling(5, __LINE__);
			}
		}
	}
	if (varDefineF) printSyntax(5); // 打印变量定义
	return varDefineF;
}

// 声明头部和左括号(已经被变量说明识别了一次
// ＜有返回值函数定义＞::=＜声明头部＞'('＜参数表＞')' '{'＜复合语句＞'}'
void syntaxReFuncDefine() { 
	bool funcFlag = false;
	syntaxParamTable(); // <参数表>
	if (nextSym == RPARENT) { // 右括号)
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LBRACE) { // 左大括号{
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			syntaxCompStatement(); // <复合语句>
			if (nextSym == RBRACE) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				funcFlag = true;
			}
			else errorHandling(9, __LINE__); // 不等于右大括号}则非法有返回值函数定义
		}
		else errorHandling(9, __LINE__); // 非法有返回值函数定义
	}
	else errorHandling(9, __LINE__); // 非法有返回值函数定义
	if (funcFlag) printSyntax(9); // 打印<有返回值函数定义>
}

// ＜声明头部＞::=int＜标识符＞|char＜标识符＞
bool syntaxDeclareHead() {
	if (nextSym == INTTK || nextSym == CHARTK) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == IDENFR) { // 标识符
			symbolTable[{getTokenSymWord()}] = { RE_T }; // 插入符号表
			printSymbol(nextSym);
			printSyntax(6); // 打印<声明头部>
			nextSym = getPreSymbol();
			if (nextSym == LPARENT) { // 左括号(
				return true;
			}
			else errorHandling(6, __LINE__); // 非法声明头部
		}
		else errorHandling(6, __LINE__); // 非法声明头部
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
		noReFuncFlag = true;
		symbolTable[{ getTokenSymWord() }] = { VOID_T }; // 插入符号表
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LPARENT) { // 左括号(
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			syntaxParamTable(); // <参数表>
			if (nextSym == RPARENT) { // 右括号)
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				if (nextSym == LBRACE) { // 左大括号{
					printSymbol(nextSym);
					nextSym = getPreSymbol();
					syntaxCompStatement(); // <复合语句>
					if (nextSym == RBRACE) {
						printSymbol(nextSym);
					}
					else errorHandling(24, __LINE__); // 非法无返回值函数定义
				}
				else errorHandling(10, __LINE__); // 非法无返回值函数定义
			}
			else errorHandling(10, __LINE__); // 非法无返回值函数定义
		}
		else errorHandling(10, __LINE__); // 非法无返回值函数定义
	}
	else errorHandling(10, __LINE__); // 非法无返回值函数定义
	if (noReFuncFlag) printSyntax(10); // 打印无返回值函数定义
	nextSym = getPreSymbol();
}

// ＜参数表＞::=＜类型标识符＞＜标识符＞{,＜类型标识符＞＜标识符＞}|＜空＞
void syntaxParamTable() { 
	while (nextSym == INTTK || nextSym == CHARTK) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == IDENFR) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
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
void syntaxCompStatement() { 
	syntaxcConstDeclare();
	syntaxVarDeclare(true); // 不能有函数定义出现
	syntaxStatementLine();
	printSyntax(11); // 打印<复合语句>
}

// ＜语句列＞::={＜语句＞}
void syntaxStatementLine() { 
	syntaxStatement();
	while (nextSym == IFTK || nextSym == FORTK || nextSym == DOTK || nextSym == WHILETK 
		|| nextSym == SCANFTK || nextSym == PRINTFTK || nextSym == IDENFR || nextSym == SEMICN 
		|| nextSym == RETURNTK || nextSym == LBRACE) {
		syntaxStatement();
	}
	printSyntax(26); // 打印<语句列>
}

// 注意后面的分号，花括号，以及空语句
// <语句>::=<条件语句>|<循环语句>|'{'<语句列>'}'|<有返回值函数调用语句>;|<无返回值函数调用语句>;|<赋值语句>;|＜读语句＞;|＜写语句＞;|＜空＞;|＜返回语句＞;
void syntaxStatement() { 
	if (nextSym == SEMICN) { // 空语句只有分号
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		printSyntax(17); // 打印<语句>
	}
	else if (nextSym == IFTK) syntaxConditionState(); // 在函数内部打印<语句>
	else if (nextSym == FORTK || nextSym == DOTK || nextSym == WHILETK) syntaxLoopState();
	else if (nextSym == LBRACE) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == IFTK || nextSym == FORTK || nextSym == DOTK || nextSym == WHILETK || nextSym == SCANFTK || nextSym == PRINTFTK || nextSym == IDENFR || nextSym == SEMICN || nextSym == RETURNTK) {
			syntaxStatementLine();
		}
		if (nextSym == RBRACE) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			printSyntax(17); // 打印<语句>
		}
		else errorHandling(17, __LINE__);
	}
	else if (nextSym == IDENFR) {
		string funcName = getTokenSymWord(); // 获得标识符名称（也许是函数名）
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (syntaxCallReFunc(funcName)) {
			if (nextSym == SEMICN) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				printSyntax(17); // 打印<语句>
			}
			else errorHandling(17, __LINE__);
		}
		else if (syntaxCallNoFunc(funcName)) {
			if (nextSym == SEMICN) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				printSyntax(17); // 打印<语句>
			}
			else errorHandling(17, __LINE__);
		}
		else if (syntaxAssignState()) {
			if (nextSym == SEMICN) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				printSyntax(17); // 打印<语句>
			}
			else errorHandling(17, __LINE__);
		}
	}
	else if (syntaxReadState()) {
		if (nextSym == SEMICN) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			printSyntax(17); // 打印<语句>
		}
		else errorHandling(17, __LINE__);
	}
	else if (syntaxWriteState()) {
		if (nextSym == SEMICN) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			printSyntax(17); // 打印<语句>
		}
		else errorHandling(17, __LINE__);
	}
	else if (syntaxReturnState()) {
		if (nextSym == SEMICN) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			printSyntax(17); // 打印<语句>
		}
		else errorHandling(17, __LINE__);
	}
}

// ＜条件语句＞::=if'('＜条件＞')'＜语句＞[else＜语句＞]
void syntaxConditionState() {
	if (nextSym == IFTK) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LPARENT) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			syntaxCondition();
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				syntaxStatement();
				if (nextSym == ELSETK) {
					printSymbol(nextSym);
					nextSym = getPreSymbol();
					syntaxStatement();
				}
				printSyntax(19); // 打印<条件语句>
				printSyntax(17); // 打印<语句>
			}
			else errorHandling(19, __LINE__);
		}
		else errorHandling(19, __LINE__);
	}
}

// ＜条件＞::=＜表达式＞＜关系运算符＞＜表达式＞|＜表达式＞ 表达式为0条件为假，否则为真
// ＜关系运算符＞::= <｜<=｜>｜>=｜!=｜==
void syntaxCondition() {
	bool conditionFlag = false;
	conditionFlag = syntaxExpre();
	if (nextSym == LSS || nextSym == LEQ || nextSym == GRE || nextSym == GEQ || nextSym == NEQ || nextSym == EQL) {
		conditionFlag = true;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		syntaxExpre();
	}
	if (conditionFlag) printSyntax(20); // 打印<条件>
}

// <循环语句>::=while'('＜条件＞')'＜语句＞|do＜语句＞while'('＜条件＞')'|for'('＜标识符＞＝＜表达式＞;＜条件＞;＜标识符＞＝＜标识符＞(+|-)＜步长＞')'＜语句＞
void syntaxLoopState() { 
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
					syntaxExpre();
					if (nextSym == SEMICN) {
						printSymbol(nextSym);
						nextSym = getPreSymbol();
						syntaxCondition();
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
												syntaxStatement();
												if (loopFlag) {
													printSyntax(21); // 打印<循环语句>
													printSyntax(17); // 打印<语句>
												}
											}
											else errorHandling(21, __LINE__);
										}
										else errorHandling(21, __LINE__);
									}
									else errorHandling(21, __LINE__);
								}
								else errorHandling(21, __LINE__);
							}
							else errorHandling(21, __LINE__);
						}
						else errorHandling(21, __LINE__);
					}
					else errorHandling(21, __LINE__);
				}
				else errorHandling(21, __LINE__);
			}
			else errorHandling(21, __LINE__);
		}
		else errorHandling(21, __LINE__);
	}
	else if (nextSym == WHILETK) {
		loopFlag = true;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LPARENT) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			syntaxCondition();
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				syntaxStatement();
				if (loopFlag) {
					printSyntax(21); // 打印<循环语句>
					printSyntax(17); // 打印<语句>
				}
			}
			else errorHandling(21, __LINE__);
		}
		else errorHandling(21, __LINE__);
	}
	else if (nextSym == DOTK) {
		loopFlag = true;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		syntaxStatement(); // 告诉它这是do循环的while
		if (nextSym == WHILETK) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (nextSym == LPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				syntaxCondition();
				if (nextSym == RPARENT) {
					printSymbol(nextSym);
					nextSym = getPreSymbol();
					if (loopFlag) {
						printSyntax(21); // 打印<循环语句>
						printSyntax(17); // 打印<语句>
					}
				}
				else errorHandling(21, __LINE__);
			}
			else errorHandling(21, __LINE__);
		}
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
	else errorHandling(22, __LINE__);
}

// ＜有返回值函数调用语句＞::=＜标识符＞'('＜值参数表＞')'
bool syntaxCallReFunc(string funcName) {
	if (nextSym == LPARENT) {
		if (symbolTable.count({ funcName }) && symbolTable[{ funcName }].type == RE_T) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			syntaxValueParam();
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				printSyntax(23); // 打印＜有返回值函数调用语句＞
				return true;
			}
			else errorHandling(23, __LINE__);
		} // 也有可能是无返回值调用
	}
	else if (syntaxAssignState()) {// 可能是赋值语句
		if (nextSym == SEMICN) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
		}
		else errorHandling(17, __LINE__);
		printSyntax(17); // 打印<语句>
	}
	return false;
}

// ＜无返回值函数调用语句＞::=＜标识符＞'('＜值参数表＞')'
bool syntaxCallNoFunc(string funcName) {
	if (nextSym == LPARENT) {
		if (symbolTable.count({ funcName }) && symbolTable[{ funcName }].type == VOID_T) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			syntaxValueParam();
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				printSyntax(24); // 打印＜无返回值函数调用语句＞
				return true;
			}
			else errorHandling(24, __LINE__);
		}
		else errorHandling(24, __LINE__);
	}
	return false;
}

// ＜值参数表＞::=＜表达式＞{ ,＜表达式＞ }｜＜空＞
void syntaxValueParam() { 
	syntaxExpre();
	while (nextSym == COMMA) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		syntaxExpre();
	}
	printSyntax(25); // 打印<值参数表>
}

// <赋值语句>::=(＜标识符＞＝＜表达式＞)|(＜标识符＞'['＜表达式＞']'=＜表达式＞)
bool syntaxAssignState() { 
	if (nextSym == ASSIGN) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (syntaxExpre()) {
			printSyntax(18); // 打印<赋值语句>
			return true;
		}
		else errorHandling(18, __LINE__);
	}
	else if (nextSym == LBRACK) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		syntaxExpre();
		if (nextSym == RBRACK) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (nextSym == ASSIGN) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				if (syntaxExpre()) {
					printSyntax(18); // 打印<赋值语句>
					return true;
				}
				else errorHandling(18, __LINE__);
			}
			else errorHandling(18, __LINE__);
		}
		else errorHandling(18, __LINE__);
	}
	return false;
}

// ＜读语句＞::=scanf'('＜标识符＞{ ,＜标识符＞ }')'
bool syntaxReadState() { 
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
					else errorHandling(27, __LINE__);
				}
				if (nextSym == RPARENT) { // 结束，最后是右括号)
					printSymbol(nextSym);
					nextSym = getPreSymbol();
					printSyntax(27); // 打印<读语句>
					return true;
				}
				else errorHandling(27, __LINE__);
			}
			else errorHandling(27, __LINE__);
		}
		else errorHandling(27, __LINE__);
	} // 非读语句，其他情况
	return false;
}

// <写语句>::=printf '('<字符串>,<表达式> ')'|printf'('<字符串>')'|printf'('<表达式>')'
bool syntaxWriteState() { 
	if (nextSym == PRINTFTK) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LPARENT) { // (
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (syntaxExpre()) { } // 只识别表达式即可
			else if (syntaxString()) {
				if (nextSym == COMMA) {
					printSymbol(nextSym);
					nextSym = getPreSymbol();
					syntaxExpre();
				}
			}
			else errorHandling(28, __LINE__);
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				printSyntax(28); // 打印<写语句>
				return true;
			}
			else errorHandling(28, __LINE__);
		}
		else errorHandling(28, __LINE__);
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
bool syntaxReturnState() { 
	if (nextSym == RETURNTK) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LPARENT) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (!syntaxExpre()) errorHandling(29, __LINE__);
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
			}
			else errorHandling(29, __LINE__);
		} // 可以没有括号表达式
		printSyntax(29); // 打印<返回语句>
		return true;
	}
	return false;
}

// <表达式>::=[+-]＜项＞{＜加法运算符＞＜项＞} [+|-]只作用于第一个<项>
// ＜加法运算符＞::= +｜-
bool syntaxExpre() { 
	bool returnFlag = false;;
	if (nextSym == PLUS || nextSym == MINU) { // 可以有也可以没有
		returnFlag = true;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
	}
	if (syntaxTerm()) returnFlag = true;
	while (nextSym == PLUS || nextSym == MINU) {
		returnFlag = true;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		syntaxTerm();
	}
	if (returnFlag) printSyntax(14); // 打印<表达式>
	return returnFlag;
}

// <项>::=＜因子＞{＜乘法运算符＞＜因子＞}
// <乘法运算符>::= *｜/
bool syntaxTerm() { 
	bool termFlag = syntaxFactor();
	if (termFlag) {
		while (nextSym == MULT || nextSym == DIV) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			syntaxFactor();
		}
		printSyntax(15); // 打印<项>
	}
	return termFlag;
}

// <因子>::=<标识符>|<标识符>'['<表达式>']'|'('<表达式>')'|<整数>|<字符>|<有返回值函数调用语句> 
// <标识符>和<有返回值函数调用冲突>
bool syntaxFactor() { 
	bool factorFlag = false;
	if (nextSym == IDENFR) {
		string funcName = getTokenSymWord();
		factorFlag = true;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LBRACK) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (!syntaxExpre()) errorHandling(16, __LINE__);
			if (nextSym == RBRACK) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
			}
			else errorHandling(16, __LINE__);
		}
		else if (syntaxCallReFunc(funcName)) factorFlag = true; // 可能是有返回值函数调用
	}
	else if (nextSym == LPARENT) {
		factorFlag = true;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (!syntaxExpre()) errorHandling(16, __LINE__);
		else {
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
			} 
			else errorHandling(16, __LINE__);
		}
	}
	else if (nextSym == INTCON) {
		factorFlag = true;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		printSyntax(7); // 打印无符号整数
		printSyntax(8); // 打印整数
	}
	else if (nextSym == CHARCON) {
		factorFlag = true;
		printSymbol(nextSym);
		nextSym = getPreSymbol();
	}
	if (factorFlag) printSyntax(16); // 打印<因子>
	return factorFlag;
}

// <主函数>::= void main'('')''{'＜复合语句＞'}'
void syntaxMain() { 
	if (nextSym == MAINTK) {
		printSymbol(nextSym);
		nextSym = getPreSymbol();
		if (nextSym == LPARENT) {
			printSymbol(nextSym);
			nextSym = getPreSymbol();
			if (nextSym == RPARENT) {
				printSymbol(nextSym);
				nextSym = getPreSymbol();
				if (nextSym == LBRACE) {
					printSymbol(nextSym);
					nextSym = getPreSymbol();
					syntaxCompStatement();
					if (nextSym == RBRACE) {
						printSymbol(nextSym);
						printSyntax(13); // 打印<主函数>
						printSyntax(1); // 打印<程序>
						nextSym = getPreSymbol();
					}
				}
			}
		}
		errorHandling(13, __LINE__);
	}
}

