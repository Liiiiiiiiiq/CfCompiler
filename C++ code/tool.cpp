#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<set>
#include<map>
#include<iomanip>

using namespace std;

/*
ע�⣺�Ҳ��ı�ʾ������
	1:һ���Ҳ�Ϊvector<string> ��ô˵��string�Ƿ��ս��
	2:һ���Ҳ�Ϊstring ��ô˵����ÿһ�����ս���ϲ���һ��Ϊstring ���ڱ���
*/


//����ʽ
struct production {
	string name;//����
	vector<vector<string>> rights;//�Ҳ� ÿһ���Ҳ�����ķ��ս������string ����vector<string> ��ʾһ���Ҳ� �ټ�vector��ʾ�Ҳ���
	bool is_null = false;//�Ƿ�Ϊ��
};

const string  path_grammar = "E:\\Cf_compiler\\grammar.txt";//�ķ��涨�ļ�
//const string  path_parse_table = "E:\\Cf_compiler\\parse_table_semantic.txt";//�������ļ�
const string  path_parse_table = "E:\\Cf_compiler\\parse_table.txt";//�������ļ�

int ter_num = 27;//�ս������
string termi[] = { "i","n","s",",",";","(",")","[","]","{","}","<",">","=","!=","==","+","-","*","/","int","string","def","if","while","printf","#" };
vector<string>** parse_table;//��ά������ �����һ���ʾ����ѡ�Ĳ���ʽ����string��һ������ʽ�Ҳ�
vector<production> productions;//����ʽ����
vector<set<string>> firsts;//FIRST����
vector<set<string>> follows;//FOLLOW����
set<string> prod_names;//����ʽ����set
map<string, int> nonter2index;//�Ӳ���ʽ���ֵ��±��ӳ��
map<string, int> ter2index;//���ս�����ֵ��±��ӳ��


//�ж��ǲ����ս��
bool is_ter(string s) {
	return prod_names.find(s) == prod_names.end();
}

//�ж��Ƿ����Ϊ��
bool is_null(string s) {
	if (is_ter(s))
		return false;
	return productions[nonter2index[s]].is_null;
}

//����ʽ�ұߵ�vector<string> �ϲ���һ��string ����������ļ�
string right2string(vector<string> right) {
	string s;
	for (string t : right)
		s =s+t+"|";
	return s;
}

//�ϲ����Ҳ�stringת���ɷֿ���vector<string> �����ڷ���ÿһ�����ս��
vector<string> string2right(string st) {
	vector<string> v;
	if (st == "e")
		v.push_back(st);
	else {
		int s = 0;
		for (int k = 0; k < st.size(); k++) {
			if (st[k] == '|') {
				v.push_back(st.substr(s, k - s));
				s = k + 1;
			}
		}
	}
	return v;
}

//���ļ���ʼ���ķ�
void init_grammar() {
	ifstream fin(path_grammar);
	if (!fin.is_open()) {
		cout << "�ķ��ļ���ʧ��!";
		exit(0);
	}
	string tem;
	int num = 0;
	//���ļ���ȡ�ķ�
	while (getline(fin, tem)) {
		production p;
		vector<string> ri;
		int start = 0;
		bool is_produ = true;
		for (int i = 0; i < tem.length(); i++) {
			if (tem[i] == ' ') {
				if (is_produ) {
					p.name = tem.substr(start, i - start);
					prod_names.insert(p.name);
					nonter2index.insert(make_pair(p.name, num++));
					is_produ = false;
				}
				else
					ri.push_back(tem.substr(start, i - start));
				start = i + 1;
			}
			if (tem[i] == '|') {
				ri.push_back(tem.substr(start, i - start));
				p.rights.push_back(ri);
				ri.clear();
				start = i + 1;
			}
		}
		productions.push_back(p);
	}
	fin.close();
	for (int i = 0; i < ter_num; i++)
		ter2index.insert(make_pair(termi[i], i));
}

//����FIRST��FOLLOW����
void generate_first_follow() {
	bool is_update = true;//�Ƿ�����˼���
	int b_size, f_size;//����ǰ��С�ͼ��ͺ��С
	firsts.resize(productions.size());
	follows.resize(productions.size());
	//����FIRST����
	while (is_update) {
		is_update = false;
		for (int i = 0; i < productions.size(); i++) {
			int b_size = firsts[i].size();
			bool b_null = productions[i].is_null;
			//�Բ���ʽ��ÿһ���Ҳ����д���
			for (vector<string> right : productions[i].rights) {
				if (is_ter(right[0])) {
					if (right[0] == "e")
						productions[i].is_null = true;
					else
						firsts[i].insert(right[0]);
				}
				else {
					for (string s : firsts[nonter2index[right[0]]])
						firsts[i].insert(s);
					int j = 0;
					while (j<right.size() - 1 && is_null(right[j])) {
						for (string s : firsts[nonter2index[right[j + 1]]])
							firsts[i].insert(s);
						j++;
					}
					if (j == right.size() - 1 && is_null(right[j]))
						productions[i].is_null = true;
				}
			}
			if (b_size != firsts[i].size() || b_null != productions[i].is_null)
				is_update = true;
		}
	}
	//����FOLLOW����
	is_update = true;
	follows[0].insert("#");
	while (is_update) {
		b_size = f_size = 0;
		for (set<string> set : follows)
			b_size += set.size();
		for (int i = 0; i < productions.size(); i++) {
			//�Բ���ʽ��ÿһ���Ҳ����д���
			for (vector<string> right : productions[i].rights) {
				int j;
				for (j = 0; j < right.size() - 1; j++) {
					if (!is_ter(right[j])) {
						if (is_ter(right[j + 1]))
							follows[nonter2index[right[j]]].insert(right[j + 1]);
						else {
							for (string s : firsts[nonter2index[right[j + 1]]])
								follows[nonter2index[right[j]]].insert(s);
						}

					}
				}
				if (!is_ter(right[j])) {
					for (string s : follows[i])
						follows[nonter2index[right[j]]].insert(s);
				}
				j = right.size() - 1;
				while (j>0 && is_null(right[j]) && !is_ter(right[j - 1])) {
					for (string s : follows[i])
						follows[nonter2index[right[j - 1]]].insert(s);
					j--;
				}
			}
		}
		for (set<string> set : follows)
			f_size += set.size();
		is_update = b_size == f_size ? false : true;
	}
}

//����LL1������
void generate_parse_table() {
	parse_table = new vector<string>*[productions.size()];
	for (int i = 0; i < productions.size(); i++)
		parse_table[i] = new vector<string>[ter_num];
	for (production p : productions) {
		for (vector<string> right : p.rights) {
			if (right[0] == "e")continue;
			string tem = right2string(right);
			if (prod_names.find(right[0]) == prod_names.end())
				parse_table[nonter2index[p.name]][ter2index[right[0]]].push_back(tem);
			else {
				for (string s : firsts[nonter2index[right[0]]])
					parse_table[nonter2index[p.name]][ter2index[s]].push_back(tem);
			}
		}
		if (p.is_null) {
			for (string s : follows[nonter2index[p.name]])
				parse_table[nonter2index[p.name]][ter2index[s]].push_back("e");
		}
	}
}

//չʾfirst��follow����
void show_first_follow() {
	cout << "FIRST:" << endl;
	for (int i = 0; i < productions.size(); i++) {
		cout << productions[i].name << ":";
		for (string s : firsts[i])
			cout << s << " ";
		cout << endl;
	}
	cout << "FOLLOW:" << endl;
	for (int i = 0; i < productions.size(); i++) {
		cout << productions[i].name << ":";
		for (string s : follows[i])
			cout << s << " ";
		cout << endl;
	}
}

//չʾ������
void show_parse_table() {
	cout << "   ";
	for (int i = 0; i < ter_num; i++)
		cout << setw(10) << termi[i];
	cout << endl;
	for (int i = 0; i < productions.size(); i++) {
		cout << setw(3) << productions[i].name;
		for (int j = 0; j < ter_num; j++) {
			string tem;
			for (string s : parse_table[i][j])
				tem =tem+s+"or";
			tem = tem.substr(0, tem.size() - 2);
			cout << setw(10) << tem;
		}
		cout << endl;
	}
}

//����������ļ�
void out2file(){
	ofstream fout(path_parse_table);
	if (!fout.is_open()) {
		cout << "�ķ��ļ���ʧ��!";
		return;
	}
	for (int i = 0; i < productions.size(); i++) {
		string ou;
		for (int j = 0; j < ter_num; j++) {
			bool is_null = true;
			for (string s : parse_table[i][j]) {
				ou = ou + s + "or";
				is_null = false;
			}
			if (is_null)
				ou += "-";
			else
				ou = ou.substr(0, ou.size() - 2);
			ou += " ";
		}
		fout << ou << endl;
	}
}

//�ӷ������ļ�����parse_table
void init_parse_table() {
	init_grammar();
	ifstream fin(path_parse_table);
	if (!fin.is_open()) {
		cout << "�ķ��ļ���ʧ��!";
		exit(0);
	}
	parse_table = new vector<string>*[productions.size()];
	for (int i = 0; i < productions.size(); i++)
		parse_table[i] = new vector<string>[ter_num];
	string tem;
	int num = 0,i,j;
	while(fin>>tem){
		if (tem != "-") {
			i = num / ter_num;
			j = num%ter_num;
			parse_table[i][j].push_back(tem);
		}
		num++;
	}
}

//��ʾ����
void emit_error(int line,string content,string s) {
	cout << line << "��:  " << content << "  " << s << endl;
}


//����������
void mainx() {
	init_grammar();
	generate_first_follow();
	generate_parse_table();
	show_first_follow();
	show_parse_table();
	out2file();
	//init_parse_table();
	system("pause");
}