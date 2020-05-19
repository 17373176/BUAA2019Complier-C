/* ConstEnumDefine.h : 此头文件定义全局常量、枚举类型、各种类别
 *
 * SYM类别码、ITEM_KIND符号表项类型、VALUE_TYPE值类型、FUNC_TYPE函数返回类型、ERRORSYM错误类别码、
 * SYM_to_str[SYMBOL_NUM]类别码映射字符串数组、wordNameArr[KEY_WORD_NUM]关键字保留字单词数组、
 * syntaxWord[SYNTAX_ELE_NUM]语法成分数组、syntaxError[SYNTAX_ERROR_NUM]语法错误标识码、
 * errorSym_to_o[SYNTAX_ERROR_NUM] 错误类型详细信息输出、errorSym_to_str[SYNTAX_ERROR_NUM]错误类别输出
 *
 */

#pragma once

#include <string>

#define TEXT_LEN 512 * 512 * 4   // 输入文本代码长度
#define SYNTAX_WORD_NUM 1024 * 4 // 识别单词个数
#define KEY_WORD_NUM 40          // 关键字保留字个数
#define WORD_LEN 100             // 单词长度 
#define SYMBOL_NUM 40            // 单词类别码个数
#define SYNTAX_ELE_NUM 40        // 语法成分个数
#define SYNTAX_ERROR_NUM 40      // 错误类别个数

#define GLOBAL_FUNCNAME "GLOBAL" // 全局域，函数域由函数名决定

using namespace std;

/*------常量定义------*/
enum SYM { // 类别码
	IDENFR, INTCON, CHARCON, STRCON, CONSTTK, INTTK, CHARTK, VOIDTK, MAINTK,
	IFTK, ELSETK, DOTK, WHILETK, FORTK, SCANFTK, PRINTFTK, RETURNTK, PLUS,
	MINU, MULT, DIV, LSS, LEQ, GRE, GEQ, EQL, NEQ, ASSIGN, SEMICN, COMMA,
	LPARENT, RPARENT, LBRACK, RBRACK, LBRACE, RBRACE, INEXIST, JUMP // 错误处理需要跳过的
};

enum ITEM_KIND { // 符号表项类型，变量(数组由length决定)，常量，参数，函数等种类
	Con_Kind, Var_Kind, Para_Kind, Func_Kind
};

enum VALUE_TYPE { // 值类型
	Int_Type, Char_Type, String_Type, Empty_Type
};

enum FUNC_TYPE { // 函数返回类型
	Void_Type, ReturnInt_Type, ReturnChar_Type, NotDefine_Func
};

enum ERRORSYM { // 错误类别码
	a_ErrorS, b_ErrorS, c_ErrorS, d_ErrorS, e_ErrorS, f_ErrorS, g_ErrorS, h_ErrorS,
	i_ErrorS, j_ErrorS, k_ErrorS, l_ErrorS, m_ErrorS, n_ErrorS, o_ErrorS, my_ErrorS
};

const string SYM_to_str[SYMBOL_NUM] = { // 类别码映射字符串数组
	"IDENFR", "INTCON", "CHARCON", "STRCON", "CONSTTK", "INTTK", "CHARTK", "VOIDTK", "MAINTK",
	"IFTK", "ELSETK", "DOTK", "WHILETK", "FORTK", "SCANFTK", "PRINTFTK", "RETURNTK", "PLUS",
	"MINU", "MULT", "DIV", "LSS", "LEQ", "GRE", "GEQ", "EQL", "NEQ", "ASSIGN", "SEMICN",
	"COMMA", "LPARENT", "RPARENT", "LBRACK", "RBRACK", "LBRACE", "RBRACE", "INEXIST"
};

const string wordNameArr[KEY_WORD_NUM] = { // 关键字保留字单词数组
	"const", "int", "char", "void", "main", "if", "else",
	"do", "while", "for", "scanf", "printf", "return"
};

const string errorSym_to_str[SYNTAX_ERROR_NUM] = { // 错误类别输出
	"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "my"
};
