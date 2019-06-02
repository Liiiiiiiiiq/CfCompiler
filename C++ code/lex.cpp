#include<iostream>
#include<string>
#include<fstream>
#include<vector>
#include<set>
#include<iomanip>

using namespace std;


//tool.cpp
extern void emit_error(int, string , string );
extern int ter_num;
extern string termi[];

//字符结构
struct token {
	string type;//类型
	string content;//内容
	int line;//单词行号
	token(string t, string c,int l=0) {
		type = t;
		content = c;
		line = l;
	}
};

const string  path_code = "E:\\Cf_compiler\\code\\cf_text.cf";//源文件

bool lex_ok = true;//是否没有错误
int line = 1;//当前行号
set<string> ter_set;//终结符set
ifstream fin;//输入文件
char s;//当前处理的char
string tem_string;//用于生成ID或者STRING或者NUM
bool is_back = false;//是否回退
vector<token> token_stream;//token流 用于下一步的语法分析


//判断是不是数字
bool digit() {
	return s >= '0'&&s <= '9' ? true : false;
}

//判断是不是字母
bool letter() {
	return s >= 'A'&&s <= 'Z' || s >= 'a'&&s <= 'z' ? true: false;
}

//返回类型 0表示数字 1表示字母 其他如;{}()等直接返回
char get_type() {
	if (digit())return 0;
	if (letter())return 1;
	return s;
}

//获取下一个非空字符
void get() {
	if (is_back)
		is_back = 0;
	else
		fin.get(s);
	while (s == ' ' || s == '\t' || s == '\n') {
		if (s == '\n')
			line++;
		fin.get(s);
	}
}

//判断是不是关键字
bool is_reserve() {
	return ter_set.find(tem_string) != ter_set.end();
}

//处理一个字符
bool process() {
	get();
	tem_string.clear();
	if (fin.eof())return false;
	switch (get_type()) {
	case 0:
		do {
			tem_string += s;
			fin.get(s);
		} while (digit());
		is_back = true;
		token_stream.push_back(token("n", tem_string,line));
		return true;
	case 1:
		do {
			tem_string += s;
			fin.get(s);
		} while (letter() || digit());
		is_back = true;
		token_stream.push_back(token(is_reserve() ? tem_string : "i", tem_string,line));
		return true;
	case ',':
	case ';':
	case '(':
	case ')':
	case '[':
	case ']':
	case '{':
	case '}':
	case '<':
	case '>':
		tem_string += s;
		token_stream.push_back(token(tem_string, tem_string, line));
		return true;
	case '=':
		tem_string += s;
		fin.get(s);
		if (s == '=')
			tem_string += s;
		else
			is_back = true;
		token_stream.push_back(token(tem_string, tem_string, line));
		return true;
	case '!':
		tem_string += s;
		fin.get(s);
		if (s == '=') {
			tem_string += s;
			token_stream.push_back(token(tem_string, tem_string, line));
		}
		else {
			is_back = 1;
			lex_ok = false;
			emit_error(line, tem_string, "词法错误");
		}
		return true;
	case '+':
	case '-':
	case '*':
	case '/':
		tem_string += s;
		token_stream.push_back(token(tem_string, tem_string, line));
		return true;
	case '"':
		do {
			tem_string += s;
			fin.get(s);
			if (fin.eof()) {
				lex_ok = false;
				emit_error(line, tem_string, "字符串缺少右\"");
				return false;
			}
		} while (s!='"');
		tem_string += s;
		token_stream.push_back(token("s", tem_string, line));
		return true;
	default:
		lex_ok = false;
		emit_error(line, tem_string, "未识别的字符");
		return true;
	}

}


//词法分析总过程
bool lex() {
	fin.open(path_code);
	if (!fin.is_open()) {
		cout << "文法文件打开失败!";
		return lex_ok;
	}
	for (int i = 0; i < ter_num-1; i++)
		ter_set.insert(termi[i]);
	while (process());
	return lex_ok;
}
