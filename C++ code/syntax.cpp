#include<iostream>
#include<string>
#include<vector>
#include<set>
#include<map>
#include<stack>
#include<iomanip>

using namespace std;


//lex.cpp
struct token {
	string type;//����
	string content;//����
	int line;//�����к�
	token(string t, string c, int l = 0) {
		type = t;
		content = c;
		line = l;
	}
};
extern bool lex();//�ʷ�����
extern vector<token> token_stream;//�ʷ������������ַ���
//tool.cpp
extern bool is_ter(string);//�ж��ǲ����ս��
extern void emit_error(int, string, string);
extern void init_parse_table();//��ʼ��LL1������
extern vector<string> string2right(string);
extern vector<string>** parse_table;//�ʷ�������
extern map<string, int> nonter2index;//���ս��תindex
extern map<string, int> ter2index;//�ս��תindex
//semantic.cpp
//�������
extern bool semantic_00();
extern bool semantic_01();
extern bool semantic_02();
extern bool semantic_03();
extern bool semantic_04();
extern bool semantic_05();
extern bool semantic_06();
extern bool semantic_07();
extern void to_asm();
extern void show_ir();


int cur_pos = 0;//��ǰ����λ��
stack<string> parst_stack;//����ջ


bool call_semantic(char c) {
	switch (c){
	case '0':
		return semantic_00();
	case '1':
		return semantic_01();
	case '2':
		return semantic_02();
	case '3':
		return semantic_03();
	case '4':
		return semantic_04();
	case '5':
		return semantic_05();
	case '6':
		return semantic_06();
	case '7':
		return semantic_07();
	}
}

//�﷨����
bool syntax() {
	token_stream.push_back(token("#", "#"));
	parst_stack.push("#");
	parst_stack.push("P");
	do {
		string s_top = parst_stack.top();
		if (s_top[0] >= '0'&&s_top[0] <= '7') {//�����������
			if (call_semantic(s_top[0])) {
				parst_stack.pop();
				continue;
			}
			else
				return false;
		}
		string s_cur = token_stream[cur_pos].type;
		string sss = token_stream[cur_pos].content;//----
		if (token_stream[cur_pos].line == 10)
			int pp = 0;
		if (is_ter(s_top)) {
			if ( s_top==s_cur) {
				if (s_cur != "#") {
					cur_pos++;
					parst_stack.pop();
				}
			}
			else {
				emit_error(token_stream[cur_pos].line, token_stream[cur_pos].content, "�﷨����");
				return false;
			}
		}
		else {
			vector<string> rights = parse_table[nonter2index[s_top]][ter2index[s_cur]];
			if (rights.size()>0) {
				parst_stack.pop();
				if (rights[0] != "e") {
					vector<string> nonters = string2right(rights[0]);
					for (int i = nonters.size() - 1; i >= 0; i--)
						parst_stack.push(nonters[i]);
				}
			}
			else {
				emit_error(token_stream[cur_pos].line, token_stream[cur_pos].content, "�﷨����");
				return false;
			}
		}
	} while (parst_stack.top() != "#");
	return true;
}

void main() {
	init_parse_table();//��ʼ��������
	if (lex()) {//�ʷ�������ȷ
		string result = syntax() ? "OK" : "FAILE";//�﷨�������
		if (result == "OK")
			to_asm();//����ɻ�ಢִ��
		cout << result << endl;
	}
}