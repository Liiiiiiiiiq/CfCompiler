#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<set>
#include<map>
#include<iomanip>

using namespace std;

/*
注意：右部的表示有两种
	1:一个右部为vector<string> 那么说明string是非终结符
	2:一个右部为string 那么说明把每一个非终结符合并在一起为string 便于保存
*/


//产生式
struct production {
	string name;//名字
	vector<vector<string>> rights;//右部 每一个右部里面的非终结符都是string 所以vector<string> 表示一个右部 再加vector表示右部链
	bool is_null = false;//是否为空
};

const string  path_grammar = "E:\\Cf_compiler\\grammar.txt";//文法规定文件
//const string  path_parse_table = "E:\\Cf_compiler\\parse_table_semantic.txt";//分析表文件
const string  path_parse_table = "E:\\Cf_compiler\\parse_table.txt";//分析表文件

int ter_num = 27;//终结符个数
string termi[] = { "i","n","s",",",";","(",")","[","]","{","}","<",">","=","!=","==","+","-","*","/","int","string","def","if","while","printf","#" };
vector<string>** parse_table;//二维分析表 里面的一项表示：可选的产生式链，string是一个产生式右部
vector<production> productions;//产生式集和
vector<set<string>> firsts;//FIRST集和
vector<set<string>> follows;//FOLLOW集和
set<string> prod_names;//产生式名字set
map<string, int> nonter2index;//从产生式名字到下标的映射
map<string, int> ter2index;//从终结符名字到下标的映射


//判断是不是终结符
bool is_ter(string s) {
	return prod_names.find(s) == prod_names.end();
}

//判断是否可以为空
bool is_null(string s) {
	if (is_ter(s))
		return false;
	return productions[nonter2index[s]].is_null;
}

//产生式右边的vector<string> 合并成一个string 便于输出到文件
string right2string(vector<string> right) {
	string s;
	for (string t : right)
		s =s+t+"|";
	return s;
}

//合并的右部string转换成分开的vector<string> 即便于分析每一个非终结符
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

//从文件初始化文法
void init_grammar() {
	ifstream fin(path_grammar);
	if (!fin.is_open()) {
		cout << "文法文件打开失败!";
		exit(0);
	}
	string tem;
	int num = 0;
	//从文件获取文法
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

//生成FIRST和FOLLOW集和
void generate_first_follow() {
	bool is_update = true;//是否更新了集和
	int b_size, f_size;//集和前大小和集和后大小
	firsts.resize(productions.size());
	follows.resize(productions.size());
	//产生FIRST集和
	while (is_update) {
		is_update = false;
		for (int i = 0; i < productions.size(); i++) {
			int b_size = firsts[i].size();
			bool b_null = productions[i].is_null;
			//对产生式的每一个右部进行处理
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
	//生成FOLLOW集和
	is_update = true;
	follows[0].insert("#");
	while (is_update) {
		b_size = f_size = 0;
		for (set<string> set : follows)
			b_size += set.size();
		for (int i = 0; i < productions.size(); i++) {
			//对产生式的每一个右部进行处理
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

//生成LL1分析表
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

//展示first和follow集合
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

//展示分析表
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

//输出分析表到文件
void out2file(){
	ofstream fout(path_parse_table);
	if (!fout.is_open()) {
		cout << "文法文件打开失败!";
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

//从分析表文件产生parse_table
void init_parse_table() {
	init_grammar();
	ifstream fin(path_parse_table);
	if (!fin.is_open()) {
		cout << "文法文件打开失败!";
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

//显示错误
void emit_error(int line,string content,string s) {
	cout << line << "行:  " << content << "  " << s << endl;
}


//产生分析表
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