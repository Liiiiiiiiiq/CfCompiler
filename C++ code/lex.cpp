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

//�ַ��ṹ
struct token {
	string type;//����
	string content;//����
	int line;//�����к�
	token(string t, string c,int l=0) {
		type = t;
		content = c;
		line = l;
	}
};

const string  path_code = "E:\\Cf_compiler\\code\\cf_text.cf";//Դ�ļ�

bool lex_ok = true;//�Ƿ�û�д���
int line = 1;//��ǰ�к�
set<string> ter_set;//�ս��set
ifstream fin;//�����ļ�
char s;//��ǰ�����char
string tem_string;//��������ID����STRING����NUM
bool is_back = false;//�Ƿ����
vector<token> token_stream;//token�� ������һ�����﷨����


//�ж��ǲ�������
bool digit() {
	return s >= '0'&&s <= '9' ? true : false;
}

//�ж��ǲ�����ĸ
bool letter() {
	return s >= 'A'&&s <= 'Z' || s >= 'a'&&s <= 'z' ? true: false;
}

//�������� 0��ʾ���� 1��ʾ��ĸ ������;{}()��ֱ�ӷ���
char get_type() {
	if (digit())return 0;
	if (letter())return 1;
	return s;
}

//��ȡ��һ���ǿ��ַ�
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

//�ж��ǲ��ǹؼ���
bool is_reserve() {
	return ter_set.find(tem_string) != ter_set.end();
}

//����һ���ַ�
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
			emit_error(line, tem_string, "�ʷ�����");
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
				emit_error(line, tem_string, "�ַ���ȱ����\"");
				return false;
			}
		} while (s!='"');
		tem_string += s;
		token_stream.push_back(token("s", tem_string, line));
		return true;
	default:
		lex_ok = false;
		emit_error(line, tem_string, "δʶ����ַ�");
		return true;
	}

}


//�ʷ������ܹ���
bool lex() {
	fin.open(path_code);
	if (!fin.is_open()) {
		cout << "�ķ��ļ���ʧ��!";
		return lex_ok;
	}
	for (int i = 0; i < ter_num-1; i++)
		ter_set.insert(termi[i]);
	while (process());
	return lex_ok;
}
