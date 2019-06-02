#include<stack>
#include<string>
#include<vector>
#include<map>
#include<iostream>
#include<fstream>

using namespace std;



#define NUM 0                   //纯数字
#define VAR_INT 1               //变量数字
#define VAR_INT_ARR 2           //变量数字数组
#define VAR_STRING 3            //变量字符串
#define VAR_STR_ARR 4           //变量字符串数组
#define STRING 5                //纯字符串

#define NIL 6                   //空
#define LABEL 7                 //跳转标签
#define FUNC 8                  //函数


//ir指令类型
#define JMP 9                    //跳转指令
#define ASSIGH 10                //赋值指令
#define ADD 11                   //加指令
#define SUB 12                   //减指令
#define MUL 13                   //乘指令
#define DIV 14                   //除指令
#define CALL 15                  //函数调用指令
#define NEQ 16                   //不等指令
#define LA 17                    //大于指令
#define SM 18                    //小于指令
#define EQ 19                    //等于指令



//lex.cpp
struct token {
	string type;//类型
	string content;//内容
	int line;//单词行号
	token(string t, string c, int l = 0) {
		type = t;
		content = c;
		line = l;
	}
};
extern vector<token> token_stream;//词法分析产生的字符流
//syntax.cpp
extern int cur_pos;//当前分析位置
//tool.cpp
extern void emit_error(int, string, string);//打印错误




//操作数信息
struct op_info {
	int type;//操作数类型 （变量 数字 字符串 数组一项)
	string value;
	string position;//数字操作数的位置 有可能是变量而不是常量
	op_info(int t,string v="",string po="") {//字符串 变量  数组一项
		type = t;
		value = v;
		position = po;
	}
	op_info() {};
};


//IR中间代码 四元式
struct ir_code {
	int type;//操作类型  (JMP 等类汇编)
	op_info op1, op2, result;
	ir_code(int t, op_info p1, op_info p2, op_info re) {
		type = t;
		op1 = p1;
		op2 = p2;
		result = re;
	}
};

//变量信息
struct var_info {
	int type;//变量类型  (int string 数组)
	int length;//如果是数组记录数组长度
	var_info(int t, int l = -1) {
		type = t;
		length = l;
	}
};

//if while 语句跳转地址回填辅助结构
struct if_while_back{
	bool is_if;
	int back_index;//需要回填的IR地址
	string label_out;//if while 的出口
	string label_in;//while 的入口
	if_while_back(bool i, int b, string out,string in) {
		is_if = i;
		back_index = b;
		label_out = out;
		label_in = in;
	}
};

//函数过程信息
struct func_info {
	//vector<var_info> paras;//函数调用参数 为printf内部函数使用 用户定义函数不能有参数
	vector<ir_code> codes;//函数翻译成的四元式序列
};


const string  path_asm = "E:\\Cf_compiler\\asm\\test.asm";//生成的汇编代码
const string  path_asm_bat = "E:\\Cf_compiler\\asm.bat";//用于编译汇编代码的批处理
const string printf_asm="printf PROC\n\tCMP _is_num, 0\n\tJZ _T3\n\tMOV AL, _show_num\n\tMOV CX, 3\n_T1: \n\tMOV AH, 0\n\tDIV _C10\n\tPUSH AX\n\
\tLOOP _T1\n\tMOV CX, 3\n_T2 : \n\tPOP DX\n\tXCHG DH, DL\n\tOR DL, 30H\n\tMOV AH, 2\n\tINT 21H\n\tLOOP _T2\n\tJMP _T4\n_T3:\n\tMOV DX, _show_string\n\
\tMOV AH, 9\n\tINT 21H\n_T4 : \n\tCMP _is_enter, 0\n\tJZ _T5\n\tMOV DL, 10\n\tMOV AH, 2\n\tINT 21H\n_T5 : \n\tret\nprintf ENDP\n";


ofstream fout;//输出汇编文件
unsigned int label_seed = 0;//用于系统产生临时标号
unsigned int var_seed = 0;//用于系统产生临时变量
unsigned int str_seed = 0;//用于系统产生字符串常量名字
map<string, func_info>::iterator cur_fun_it;//当前正在处理的函数 用于插入IR中间代码


//变量信息表
map<string, var_info> var_map;

//函数过程信息表
map<string, func_info> fun_map;

//字符串常量信息表 左边是字符串内容 右边是名字
map<string, string> str_map;


//表达式操作符号类型栈
stack<string> ex_t_stack;

//表达式操作数栈
stack<op_info> ex_o_stack;

//if_while出口回填辅助栈
stack<if_while_back> i_w_stack;



//---------------------------------------------------------工具函数

//产生IR
void emit_ir(int type, op_info p1, op_info p2, op_info res) {
	cur_fun_it->second.codes.push_back(ir_code(type, p1, p2, res));
}


//产生临时标号(用于跳转)
string get_tem_label() {
	return "__LABEL" + to_string(label_seed++);
}

//产生临时变量(用于算术计算)
op_info get_tem_var(int type) {
	string name= "__VAR" + to_string(var_seed++);
	var_map.insert(make_pair(name, var_info(type)));
	return op_info(type,name);
}

//通过字符串常量获取其自动生成的名字 主要为了便于汇编数据空间分配
string get_str_id(string s) {
	map<string, string>::iterator it = str_map.find(s);
	if (it == str_map.end()) {
		string name = "__STR" + to_string(str_seed++);
		str_map.insert(make_pair(s, name));
		return name;
	}
	return it->second;
}

//把op_info 里面的名字生成汇编名字 主要用于处理数组操作数 如 a[5]  ->  [a+5]
string get_asm_name(op_info op,bool isVar=false) {
	switch(op.type)
	{
	case VAR_INT:
	case VAR_STRING:
	case NUM:
	case STRING:
		return op.value;
	case VAR_INT_ARR:
		if(isVar)
			return op.value + "[SI]";
		else
			return "[" + op.value + "+" + op.position + "]";
	case VAR_STR_ARR:
		if (isVar)
			return op.value + "[SI]";
		int t = atoi(op.position.c_str())*2;
		return "[" + op.value + "+" + to_string(t) + "]";
	}
}

//生成asm汇编文件
void to_asm() {
	fout.open(path_asm);
	if (!fout.is_open()) {
		cout << "文法文件打开失败!";
		return;
	}
	//数据段生成
	fout<<"DATAS SEGMENT\n";
	//字符串常量空间分配
	for (pair<string, string> p : str_map)
		fout << "\t" << p.second << " DB "  << p.first << ",'$'\n";
	//变量空间分配
	for (pair<string, var_info> p : var_map) {
		switch(p.second.type)
		{
		case VAR_INT:
			fout << "\t" << p.first << " DB " << "0\n";//变量只有8位
			break;
		case VAR_INT_ARR:
			fout << "\t" << p.first << " DB " << p.second.length <<" DUP(0)\n";
			break;
		case VAR_STRING:
			fout << "\t" << p.first << " DW " << "0\n";//只是保存字符串常量的地址
			break;
		case VAR_STR_ARR:
			fout << "\t" << p.first << " DW " << p.second.length << " DUP(0)\n";
			break;
		}
		
	}
	//printf专用变量
	fout << "\t_is_num DB 0\n";
	fout << "\t_show_string DW 0\n";
	fout << "\t_show_num DB 0\n";
	fout << "\t_C10 DB 10\n";
	fout << "\t_is_enter DB 0\n";
	fout << "DATAS ENDS\n";
	//代码段生成
	fout <<"CODES SEGMENT\n";
	fout << "\tASSUME CS:CODES,DS:DATAS\n";
	string op1, op2,re;
	bool is_var1, is_var2,is_re;
	for (pair<string, func_info> p : fun_map) {
		string s = p.first == "cfmain" ? "START:\n\tMOV AX,DATAS\n\tMOV DS, AX\n" : p.first + " PROC\n";
		fout << s;
		for (ir_code ir:p.second.codes) {
			is_var1 = is_var2 = is_re = false;
			switch(ir.type)
			{
			case ASSIGH:
				if ((ir.op2.type == VAR_INT_ARR|| ir.op2.type == VAR_STR_ARR) && (ir.op2.position[0] > '9' || ir.op2.position[0] < '0')) {
					fout << "\tMOV BX,0\n";
					fout << "\tMOV BL," << ir.op2.position << "\n";
					fout << "\tMOV SI,BX\n";
					if(ir.op1.type==VAR_STR_ARR)
						fout << "\tADD SI,SI\n";
					is_var2 = true;
				}
				op2 = get_asm_name(ir.op2, is_var2);
				if(ir.op2.type==VAR_INT_ARR||ir.op2.type==VAR_INT||ir.op2.type==NUM)
					fout << "\tMOV AL," << op2 << "\n";
				else if(ir.op2.type == VAR_STR_ARR || ir.op2.type == VAR_STRING)
					fout << "\tMOV AX," << op2 << "\n";
				else if(ir.op2.type==STRING)
					fout << "\tLEA AX," << op2 << "\n";
				if ((ir.op1.type == VAR_INT_ARR||ir.op1.type==VAR_STR_ARR) && (ir.op1.position[0] > '9' || ir.op1.position[0] < '0')) {
					fout << "\tMOV BX,0\n";
					fout << "\tMOV BL," << ir.op1.position << "\n";
					fout << "\tMOV SI,BX\n";
					if (ir.op1.type == VAR_STR_ARR)
						fout << "\tADD SI,SI\n";
					is_var1 = true;
				}
				op1 = get_asm_name(ir.op1, is_var1);
				if (ir.op2.type == VAR_INT_ARR || ir.op2.type == VAR_INT||ir.op2.type==NUM)
					fout << "\tMOV " << op1 << ",AL\n";
				else
					fout << "\tMOV " << op1 << ",AX\n";
				break;
			case ADD:
			case SUB:
			case MUL:
			case DIV:
				if (ir.op1.type == VAR_INT_ARR && (ir.op1.position[0] > '9' || ir.op1.position[0] < '0')) {
					fout << "\tMOV BX,0\n";
					fout << "\tMOV BL," << ir.op1.position << "\n";
					fout << "\tMOV SI,BX\n";
					is_var1 = true;
				}
				op1 = get_asm_name(ir.op1, is_var1);
				fout << "\tMOV AX,0\n";
				fout << "\tMOV AL," << op1 << "\n";
				if (ir.op2.type == VAR_INT_ARR && (ir.op2.position[0] > '9' || ir.op2.position[0] < '0')) {
					fout << "\tMOV BX,0\n";
					fout << "\tMOV BL," << ir.op2.position << "\n";
					fout << "\tMOV SI,BX\n";
					is_var2 = true;
				}
				op2 = get_asm_name(ir.op2, is_var2);
				if (ir.type == ADD)
					fout << "\tADD AL," << op2 <<"\n";
				else if(ir.type==SUB)
					fout << "\tSUB AL," << op2 <<"\n";
				else if (ir.type == MUL) {
					fout << "\tMOV BL," << op2 << "\n";
					fout << "\tMUL BL\n";
				}
				else if (ir.type == DIV) {
					fout << "\tMOV BL," << op2 << "\n";
					fout << "\tDIV BL\n";
				}
					
				if (ir.result.type == VAR_INT_ARR && (ir.result.position[0] > '9' || ir.result.position[0] < '0')) {
					fout << "\tMOV BX,0\n";
					fout << "\tMOV BL," << ir.result.position << "\n";
					fout << "\tMOV SI,BX";
					is_re = true;
				}
				re = get_asm_name(ir.result, is_re);
				fout << "\tMOV " << re << ",AL\n";
				break;
			case JMP:
				fout << "\tJMP " << ir.result.value << "\n";
				break;
			case SM:
			case LA:
			case EQ:
			case NEQ:
				if (ir.op1.type == VAR_INT_ARR && (ir.op1.position[0] > '9' || ir.op1.position[0] < '0')) {
					fout << "\tMOV BX,0\n";
					fout << "\tMOV BL," << ir.op1.position << "\n";
					fout << "\tMOV SI,BX\n";
					is_var1 = true;
				}
				op1 = get_asm_name(ir.op1, is_var1);
				fout << "\tMOV AL," << op1 << "\n";
				if (ir.op2.type == VAR_INT_ARR && (ir.op2.position[0] > '9' || ir.op2.position[0] < '0')) {
					fout << "\tMOV BX,0\n";
					fout << "\tMOV BL," << ir.op2.position << "\n";
					fout << "\tMOV SI,BX\n";
					is_var2 = true;
				}
				op2 = get_asm_name(ir.op2, is_var2);
				fout << "\tCMP AL," << op2 << "\n";
				if (ir.type == SM)
					fout << "\tJNB " << ir.result.value << "\n";
				else if (ir.type == LA) 
					fout << "\tJNA " << ir.result.value << "\n";
				else if (ir.type == NEQ)
					fout << "\tJZ " << ir.result.value << "\n";
				else if (ir.type == EQ)
					fout << "\tJNZ " << ir.result.value << "\n";
				break;
			case CALL:
				if (ir.result.value == "printf") {
					if (ir.op2.value == "0")
						fout << "\tMOV _is_enter,0\n";
					else
						fout << "\tMOV _is_enter,1\n";
					if (ir.op1.type == VAR_INT) {
						fout << "\tMOV _is_num,1\n";
						fout << "\tMOV AX,0\n";
						fout << "\tMOV AL," << ir.op1.value << "\n";
						fout << "\tMOV _show_num,AL\n";
					}
					else if (ir.op1.type == NUM) {
						fout << "\tMOV _is_num,1\n";
						fout << "\tMOV _show_num,"<<ir.op1.value<<"\n";
					}
					else if(ir.op1.type == VAR_STRING){
						fout << "\tMOV _is_num,0\n";
						fout << "\tMOV AX," << ir.op1.value << "\n";
						fout << "\tMOV _show_string,AX\n";
					}
					else if (ir.op1.type == STRING) {
						fout << "\tMOV _is_num,0\n";
						fout << "\tLEA AX," << ir.op1.value << "\n";
						fout << "\tMOV _show_string,AX\n";
					}
					fout << "\tCALL printf\n";
				}
				else 
					fout << "\tCALL " << ir.result.value << "\n";
				break;
			case LABEL:
				fout << ir.result.value<<":\n";
				break;
			}
		}
		s = p.first == "cfmain" ? "\tMOV AH,4CH\n\tINT 21H\n" : "\tRET\n"+p.first + " ENDP\n";
		fout << s;
	}
	fout << printf_asm << "\n";
	fout << "CODES ENDS\n";
	fout << "\tEND START";
	fout.close();
	system(path_asm_bat.c_str());
}

//测试展示中间代码
void show_ir() {
	string m[25];
	m[JMP] = "JMP";
	m[ASSIGH] = "ASSIGH";
	m[ADD] = "ADD";
	m[SUB] = "SUB";
	m[MUL] = "MUL";
	m[DIV] = "DIV";
	m[CALL] = "CALL";
	m[NEQ] = "NEQ";
	m[LA] = "LA";
	m[SM] = "SM";
	m[EQ] = "EQ";
	m[LABEL] = "LABEL";
	for (pair<string,func_info> p:fun_map) {
		cout << "函数:" << p.first<<"的IR中间代码"<<endl;
		for (ir_code ir : p.second.codes) {
			string p1, p2, re;
			p1 = ir.op1.value;
			if (ir.op1.position != "")
				p1 = p1 + "[" + ir.op1.position + "]";
			p2 = ir.op2.value;
			if (ir.op2.position != "")
				p2 = p2 + "[" + ir.op2.position + "]";
			re = ir.result.value;
			if (ir.result.position != "")
				re = re + "[" + ir.result.position + "]";
			cout << "(" << m[ir.type] << "," << p1 << "," << p2 << "," << re << ")" << endl;
		}
	}
}



//----------------------------------------------------------语义程序

//检查是否存在main函数
bool semantic_00() {
	if (fun_map.find("cfmain") == fun_map.end()) {
		emit_error(token_stream[cur_pos].line, "语法错误，", "程序缺少cfmain函数。");
		return false;
	}
	return true;
};


//申明处理
bool semantic_01() {
	token t = token_stream[cur_pos-1];
	if (t.type== ";") {//变量申明
		t = token_stream[cur_pos - 2];
		if (t.type == "i") {//普通变量
			if (var_map.find(t.content) != var_map.end()) {
				emit_error(t.line, t.content, "变量名已经存在");
				return false;
			}
			if(token_stream[cur_pos - 3].type == "int")
				var_map.insert(make_pair(t.content, var_info(VAR_INT)));
			else
				var_map.insert(make_pair(t.content, var_info(VAR_STRING)));
		}
		else {//数组变量
			t = token_stream[cur_pos - 5];
			if (var_map.find(t.content) != var_map.end()) {
				emit_error(t.line, t.content, "变量名已经存在");
				return false;
			}
			int length = atoi(token_stream[cur_pos - 3].content.c_str());
			if(token_stream[cur_pos - 6].type == "int")
				var_map.insert(make_pair(t.content, var_info(VAR_INT_ARR,length)));
			else
				var_map.insert(make_pair(t.content, var_info(VAR_STR_ARR, length)));
		}
	}
	else{//函数申明
		func_info fun;
		if (fun_map.find(t.content) != fun_map.end()) {
			emit_error(t.line, t.content, "变量名已经存在");
			return false;
		}
		cur_fun_it = fun_map.insert(make_pair(t.content, fun)).first;//加入函数表
	}
	return true;
};



//判断当前语言式if还是while 预处理
bool semantic_02() {
	token t = token_stream[cur_pos - 2];
	if (t.type == "if") {
		i_w_stack.push(if_while_back(true, cur_fun_it->second.codes.size(), get_tem_label(),""));
	}
	else {
		i_w_stack.push(if_while_back(false, cur_fun_it->second.codes.size(), get_tem_label(),get_tem_label()));
		emit_ir(LABEL,op_info(NIL),op_info(NIL), op_info(LABEL, i_w_stack.top().label_in));//设个while入口标记 用于while回跳
	}
	
	return true;
};

//if while 尾部出口回填
bool semantic_03() {
	if_while_back t = i_w_stack.top();
	i_w_stack.pop();
	//while 还要一个回跳指令
	if (!t.is_if)
		emit_ir(JMP, op_info(NIL), op_info(NIL), op_info(LABEL, t.label_in));//while { S }尾 JMP 到入口判断是否继续
	emit_ir(LABEL, op_info(NIL), op_info(NIL), op_info(LABEL, t.label_out));//设个 if while 出口标记
	return true;
};

//算术操作数入栈
bool semantic_04() {
	token t = token_stream[cur_pos - 1];
	if (t.type == "i") {//VAR_INT VAR_STRING
		map<string, var_info>::iterator it = var_map.find(t.content);
		if (it == var_map.end()) {
			emit_error(t.line, t.content, "变量不存在");
			return false;
		}
		var_info v= it->second;
		if (v.type == VAR_INT_ARR || v.type == VAR_STR_ARR) {
			emit_error(t.line, t.content, "数组名不能参加运算");
			return false;
		}
		ex_o_stack.push(op_info(v.type, t.content));
	}
	else if (t.type == "s") {//STRING
		ex_o_stack.push(op_info(STRING, get_str_id(t.content)));
	}
	else if (t.type == "n") {//NUM
		ex_o_stack.push(op_info(NUM, t.content));
	}
	else if (t.type == "]") {//VAR_INT_ARR VAR_STR_ARR 的一项
		t = token_stream[cur_pos - 4];
		map<string, var_info>::iterator it = var_map.find(t.content);
		if (it == var_map.end()) {
			emit_error(t.line, t.content, "变量不存在");
			return false;
		}
		var_info v = var_map.find(t.content)->second;
		if (token_stream[cur_pos - 2].type == "i") {
			token t1 = token_stream[cur_pos - 2];
			map<string, var_info>::iterator it = var_map.find(t1.content);
			if (it == var_map.end()) {
				emit_error(t1.line, t1.content, "变量不存在");
				return false;
			}
			if (it->second.type != VAR_INT) {
				emit_error(t1.line, t1.content, "必须是整数");
				return false;
			}
			ex_o_stack.push(op_info(v.type, t.content, it->first));
		}
		else {
			int po = atoi(token_stream[cur_pos - 2].content.c_str());
			//判断数组是否超界
			if (po >= v.length) {
				emit_error(t.line, t.content, "数组下标超界");
				return false;
			}
			ex_o_stack.push(op_info(v.type, t.content, token_stream[cur_pos - 2].content));
		}	
	}
	return true;
};

//算术操作符入栈
bool semantic_05() {
	token t = token_stream[cur_pos - 1];
	ex_t_stack.push(t.type);
	return true;
};

//根据2个算术辅助栈进行结果的计算
bool semantic_06() {
	string type = ex_t_stack.top();
	ex_t_stack.pop();
	op_info op2 = ex_o_stack.top();
	ex_o_stack.pop();
	op_info op1 = ex_o_stack.top();
	ex_o_stack.pop();
	if (type == "=") {
		if (!(op1.type >= VAR_INT&&op1.type <= VAR_STR_ARR)) {
			emit_error(token_stream[cur_pos].line-1, token_stream[cur_pos].content, "赋值左边必须是变量");
			return false;
		}
		if ((op1.type == VAR_INT || op1.type == VAR_INT_ARR) && (op2.type == VAR_STRING || op2.type == VAR_STR_ARR|| op2.type == STRING) ||
			(op1.type == VAR_STRING || op1.type == VAR_STR_ARR) && (op2.type == VAR_INT || op2.type == VAR_INT_ARR|| op2.type == NUM)) {
			emit_error(token_stream[cur_pos].line-1, token_stream[cur_pos].content, "左右类型不匹配");
			return false;
		}
		emit_ir(ASSIGH, op1, op2, op1);
	}
	else {
		if (op1.type == STRING || op1.type == VAR_STRING || op1.type == VAR_STR_ARR ||
			op2.type == STRING || op2.type == VAR_STRING || op2.type == VAR_STR_ARR) {
			emit_error(token_stream[cur_pos].line, token_stream[cur_pos].content, "算术计算不能出现STRING");
			return false;
		}
		op_info tem = get_tem_var(VAR_INT);
		switch (type[0])
		{
		case '+':
			emit_ir(ADD, op1, op2, tem);
			break;
		case '-':
			emit_ir(SUB, op1, op2, tem);
			break;
		case '*':
			emit_ir(MUL, op1, op2, tem);
			break;
		case '/':
			emit_ir(DIV, op1, op2, tem);
			break;
			//确定if和while跳转
		case '<':
			emit_ir(SM, op1, op2, op_info(LABEL, i_w_stack.top().label_out));
			return true;
		case '>':
			emit_ir(LA, op1, op2, op_info(LABEL, i_w_stack.top().label_out));
			return true;
		case '!':
			emit_ir(NEQ, op1, op2, op_info(LABEL, i_w_stack.top().label_out));
			return true;
		case '=':
			emit_ir(EQ, op1, op2, op_info(LABEL, i_w_stack.top().label_out));
			return true;
		}
		ex_o_stack.push(tem);//结果压入数据栈
	}
	return true;
};

//函数调用
bool semantic_07() {
	token t = token_stream[cur_pos - 3];
	if (t.type == "(") {//普通函数调用
		t = token_stream[cur_pos - 4];
		map<string, func_info>::iterator it =fun_map.find(t.content);
		if (it == fun_map.end()) {
			emit_error(t.line, t.content, "函数名不存在");
			return false;
		}
		emit_ir(CALL, op_info(NIL), op_info(NIL), op_info(FUNC, t.content));
	}
	else {//printf函数调用
		t = token_stream[cur_pos - 5];
		string n = token_stream[cur_pos - 3].content;//0表示不打回车 非0表示打回车
		if (t.type == "i") {
			map<string, var_info>::iterator it = var_map.find(t.content);
			if (it == var_map.end()) {
				emit_error(t.line, t.content, "变量名不存在");
				return false;
			}
			var_info v =it->second;
			emit_ir(CALL, op_info(v.type, t.content), op_info(NUM, n), op_info(FUNC, "printf"));
		}
		else if (t.type == "s")
			emit_ir(CALL, op_info(STRING, get_str_id(t.content)), op_info(NUM, n), op_info(FUNC, "printf"));
		else
			emit_ir(CALL, op_info(NUM, t.content), op_info(NUM, n), op_info(FUNC, "printf"));
	}
	return true;
};
