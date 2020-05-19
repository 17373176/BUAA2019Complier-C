//LexicalAnalysis.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//词法分析程序，采用有限状态机，按分析顺序依次输出每个词语的类别码

#include <iostream>
#include <cstdio>
#include <cctype>
#include <string>
#include <algorithm>

#define TEXT_LEN 512 * 512 * 4
#define KEY_WORD_ARR 40
#define WORD_NAME_LEN 100
#define SYMBOL_NUM 40

using namespace std;

enum SYM { // 类别码
	IDENFR, INTCON, CHARCON, STRCON, CONSTTK, INTTK, CHARTK, VOIDTK, MAINTK,
	IFTK, ELSETK, DOTK, WHILETK, FORTK, SCANFTK, PRINTFTK, RETURNTK, PLUS,
	MINU, MULT, DIV, LSS, LEQ, GRE, GEQ, EQL, NEQ, ASSIGN, SEMICN, COMMA,
	LPARENT, RPARENT, LBRACK, RBRACK, LBRACE, RBRACE, INEXIST
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

string token_g; // 单词字符串
char textCodes[TEXT_LEN]; // 被处理的文本字符数组
int index; // 处理文本的下标

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
	while (isspace(now_char_g)) { // 如果是空格，则跳过连续的空格，不会影响分词和语句的识别
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

int main() {
	FILE* stream;
	if ((stream = freopen("testfile.txt", "r", stdin)) == NULL) return -1;
	if ((stream = freopen("output.txt", "w", stdout)) == NULL) return -1;
	char ch;
	while ((ch = getchar()) != EOF)
		textCodes[index++] = ch;
	index = 0; // 复位
	while (getNextCh() != '\0') {
		backIndex();
		SYM symbol = getSymbol(); // 当前识别的单词类型
		if (symbol >= IDENFR && symbol < INEXIST) // 在类型码范围内的单词
			cout << SYM_to_str[symbol] << " " << token_g << endl;
		else analysisError(); // 分析失败
	}
	return 0;
}
