#pragma once
#include<vector>
#include<iostream>
#include<string>
#include<string.h>
#include<fstream>
#include<Windows.h>
#define DIR 1
#define FILE 2
#define NONE 0

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#define VERSION "23.0601.1527_pm"
using namespace std;

const int BlockSize = 256;
struct MemoryBlock
{
	string fname;		//文件名称
	int type;			//文件类型，目录还是文件
	int selfPos;		//自己的块号
	int fatherPos;		//上一级目录的块号
	string permissions; //文件权限 r只读 w只写 rw读写
	int line;			//文件内容的行数,目录包含的子目录
	int dir_childPos[BlockSize];		//目录所包含的其他块号
	string file_context[BlockSize];	//存储内容

	void init_Mb()
	{
		fname = " ";
		line = 0;
		type = 0;
		fatherPos = 0;
		selfPos = 0;
		permissions = "-1";
		for (int i = 0; i < BlockSize; i++)
		{
			dir_childPos[i] = 0;
			file_context[i] = " ";
		}
	}
};



int editfile = -1;//全局变量，存储当前打开的文件块号

const char* VirtualFileSystem = "ghy.bin";//虚拟文件存储位置
string currentPath = "ghy";

MemoryBlock DISKBLOCK[BlockSize];


//获取文件行数
int CountLine(string name)
{
	char ch;
	int lines = 0;
	fstream f(name, ios::in);
	while (f.get(ch))
	{
		if (ch == '\n')
			lines++;
	}
	return lines;

}

//分隔函数，将一个字符串按照某个字符分隔成字符串数组
vector<string> split(string str, char ch) {
	int start = 0, end = 0, size = str.size();
	vector<string> result;
	for (end = 0; end < size; end++)
	{
		if (str[end] == ch) {
			if (end > start) {
				string newStr = str.substr(start, end - start);
				result.push_back(newStr);
			}
			start = end + 1;
		}
	}
	if (end > start) {
		string newStr = str.substr(start, end - start);
		result.push_back(newStr);
	}
	return result;
}

string change(string name)
{
	int i = 0;
	for (; i < BlockSize; i++)
	{
		if (DISKBLOCK[i].fname == currentPath)
		{
			break;
		}
	}//找到了当前路径

	vector<string>vec = split(name, '/');
	if (vec.size() == 1)
	{
		if (vec[0] != "ghy")
			name = currentPath + "/" + name;
	}
	return name;
}

void writeBack();
//格式化DISKBLOCK
void clear();
void head();
void format(int num)
{
	writeBack();
	if (DISKBLOCK[0].permissions == "format" && num == 1)
	{
		return;
	}
	cout << "Formatting..." << endl;
	Sleep(1500);
	clear();
	head();
	currentPath = "ghy";
	DISKBLOCK[0].fname = "ghy";
	DISKBLOCK[0].type = DIR;
	DISKBLOCK[0].line = 0;
	DISKBLOCK[0].fatherPos = -1;
	DISKBLOCK[0].permissions = "format";
	//cout << DISKBLOCK[0].permissions << endl;
	DISKBLOCK[0].selfPos = 0;
	for (int i = 0; i < BlockSize; i++)
	{
		DISKBLOCK[0].file_context[i] = "";
		DISKBLOCK[0].dir_childPos[i] = 0;
	}
	for (int i = 1; i < BlockSize; i++)
	{
		DISKBLOCK[i].fname = " ";
		DISKBLOCK[i].type = NONE;
		DISKBLOCK[i].line = 0;
		DISKBLOCK[i].fatherPos = 0;
		DISKBLOCK[i].permissions = "-1";
		DISKBLOCK[i].selfPos = i;
		for (int j = 0; j < BlockSize; j++)
		{
			DISKBLOCK[i].file_context[j] = "";
			DISKBLOCK[i].dir_childPos[j] = 0;
		}
	}
	writeBack();
}

//读取并加载虚拟磁盘
void read()
{
	fstream f(VirtualFileSystem);
	for (int i = 0; i < BlockSize; i++)
	{
		string str1;
		getline(f, str1);
		DISKBLOCK[i].fname = str1;

		string str;
		getline(f, str);
		if (str == "DIR")
			DISKBLOCK[i].type = 1;
		else if (str == "FILE")
			DISKBLOCK[i].type = 2;
		else
			DISKBLOCK[i].type = 0;

		string str2;
		getline(f, str2);
		DISKBLOCK[i].line = atoi(str2.c_str());

		string str3;
		getline(f, str3);
		DISKBLOCK[i].selfPos = atoi(str3.c_str());

		string str4;
		getline(f, str4);
		DISKBLOCK[i].fatherPos = atoi(str4.c_str());

		string str_;
		getline(f, str_);
		DISKBLOCK[i].permissions = str_.c_str();

		string str5;
		if (DISKBLOCK[i].line == 0)
			getline(f, str5);
		else {
			if (DISKBLOCK[i].type == FILE) {
				for (int j = 0; j < DISKBLOCK[i].line; j++)
				{
					getline(f, str5);
					DISKBLOCK[i].file_context[j] = str5;
				}
				getline(f, str5);
			}
			else if (DISKBLOCK[i].type == DIR)
			{
				getline(f, str5);
				vector<string>vec = split(str5, '\t');
				for (int j = 0; j < DISKBLOCK[i].line; j++)
					DISKBLOCK[i].dir_childPos[j] = atoi(vec[j].c_str());
			}
		}
	}
}

/*
* 创建一个 ofstream 类型的对象 fout，用于向磁盘文件写入数据。ios::trunc 表示清空文件内容。
* 遍历虚拟文件系统中的每一个块（block）
* 根据块的类型（空块、目录块或文件块），将相应的信息写入到磁盘文件中。
* 每个块的信息包括文件名、类型、行数、占用的内存块数、父目录的编号（如果有的话）以及具体内容（只有文件块才有）。
* 在写入完每个块的信息后，再输出一个空行。
*/
//写回磁盘
void writeBack() {
	ofstream fout(VirtualFileSystem, ios::trunc); // 创建一个 ofstream 对象 fout，用于向磁盘文件写入数据。ios::trunc表示清空文件内容。
	for (int i = 0; i < BlockSize; i++) { // 遍历虚拟文件系统中的每一个块
		if (DISKBLOCK[i].type == 1) { // 如果是目录块，则将其信息写入磁盘文件中
			fout << DISKBLOCK[i].fname << endl << "DIR" << endl << DISKBLOCK[i].line << endl << DISKBLOCK[i].selfPos << endl << DISKBLOCK[i].fatherPos << endl<<DISKBLOCK[i].permissions<<endl;
			for (int j = 0; j < DISKBLOCK[i].line; j++) {
				fout << DISKBLOCK[i].dir_childPos[j] << "\t"; // 输出子目录的编号
			}
		}
		else if (DISKBLOCK[i].type == 2) { // 如果是文件块，则将其信息写入磁盘文件中
			fout << DISKBLOCK[i].fname << endl << "FILE" << endl << DISKBLOCK[i].line << endl << DISKBLOCK[i].selfPos << endl << DISKBLOCK[i].fatherPos << endl << DISKBLOCK[i].permissions << endl;
			for (int j = 0; j < DISKBLOCK[i].line; j++) {
				fout << DISKBLOCK[i].file_context[j] << endl; // 输出文件内容
			}
		}
		else if (DISKBLOCK[i].type == 0) { // 如果是空块，则将其信息写入磁盘文件中
			fout << DISKBLOCK[i].fname << endl << "NONE" << endl << DISKBLOCK[i].line << endl << DISKBLOCK[i].selfPos << endl << DISKBLOCK[i].fatherPos << endl << DISKBLOCK[i].permissions << endl;
		}
		fout << endl; // 输出一个空行
	}
}


//查看帮助信息
void help()
{
	cout << "    format\t\t\t\t:格式化磁盘" << endl;
	cout << "    mkdir [filename]\t\t\t:创建目录" << endl;
	cout << "    rmdir [filename]\t\t\t:删除目录" << endl;
	cout << "    renew \t\t\t\t:刷新缓存" << endl;
	cout << "    cd [filename]\t\t\t:切换当前目录（可用相对路径）" << endl;
	cout << "    dir [*.--]	\t\t\t:查找所有目录下后缀为.--的文件" << endl;
	cout << "    dir [/option]\t\t\t:查看当前目录下的文件及目录" << endl;
	cout << "       /d\t\t\t\t  -显示所有目录" << endl;
	cout << "       /f [-option]\t\t\t  -显示文件" << endl;
	cout << "          -a\t\t\t\t   -所有" << endl;
	cout << "          -r\t\t\t\t   -显示只读文件" << endl;
	cout << "          -w\t\t\t\t   -显示只写文件" << endl;
	cout << "          -rw\t\t\t\t   -显示读写文件" << endl;
	cout << "       /s\t\t\t\t   -显示所有文件和目录" << endl;
	cout << "    create [filename] [-option]\t\t:创建文件，并赋予权限" << endl;
	cout << "       -r\t\t\t\t   -只读" << endl;
	cout << "       -w\t\t\t\t   -只写" << endl;
	cout << "       -rw\t\t\t\t   -读写" << endl;
	cout << "    read [filename] [-option]\t\t:查看文件中的信息" << endl;
	cout << "       -a\t\t\t\t   -查看所有内容" << endl;
	cout << "       -s [offset]\t\t\t   -调用lseek查看offset位置之后的内容" << endl;
	cout << "    write [filename] [-option]\t\t:修改文件" << endl;
	cout << "       -i [offset] [data]\t\t   -调用lseek在offset位置之后插入数据" << endl;
	cout << "       -n [data]\t\t\t   -新增行增加数据" << endl;
	cout << "    rename [filename/dirname] [newname]\t:文件或目录重命名"<<endl;
	cout << "    rm [filename]\t\t\t:删除指定文件" << endl;
	cout << "    import [filepath] [-option]\t\t:从指定的路径中导入文件到磁盘中" << endl;
	cout << "       -r\t\t\t\t   -导入文件为只读" << endl;
	cout << "       -w\t\t\t\t   -导入文件为只写" << endl;
	cout << "       -rw\t\t\t\t   -导入文件为读写" << endl;
	cout << "    export [filename] [filepath]\t:将当前目录下的文件导出到指定的路径下" << endl;
	cout << "    find [filename/dirname]\t\t:按名字查找文件或目录,列出基本信息" << endl;
	cout << "    exit\t\t\t\t:退出并保存当前文件系统" << endl;
	cout << "    clear\t\t\t\t:清除当前屏幕中的内容" << endl;
	cout << "    time\t\t\t\t:输出当前系统时间" << endl;
	cout << "    help\t\t\t\t:查看命令的使用说明" << endl;
}

//清屏
void clear()
{
	system("cls");
}

/*
* 遍历虚拟文件系统中的每一个块（block），找到一个空的块，用于存储新创建的目录。
* 如果没有空的块可用，则输出 "空间不足" 并返回。
* 找到当前目录所在的位置。
* 遍历当前目录的子目录，判断是否有与输入的目录名称 dir 相同的目录，如果有，则输出 "目录重名！" 并返回。
* 根据输入的目录路径找到其父目录。
* 如果找不到父目录，则输出 "路径输入有误！" 并返回。
* 在父目录中添加子目录信息。
* 将新创建的目录信息写入到找到的空块中。
* 将新创建的目录与其父目录建立关联。
*/
//创建目录
void mkdir(string dir) {
	int i = 1;
	for (; i < BlockSize; i++) {
		if (DISKBLOCK[i].type == NONE) // 查找空块
			break;
	}
	// 如果空间不足
	if (i == BlockSize - 1) {
		cout << "Not enough space" << endl;
		return;
	}

	int tmp; // 当前目录的位置
	for (tmp = 0; tmp < BlockSize; tmp++) {
		if (DISKBLOCK[tmp].fname == currentPath)
			break;
	}
	//查找当前目录

	for (int p = 0; p < DISKBLOCK[tmp].line; p++) {
		if (DISKBLOCK[DISKBLOCK[tmp].dir_childPos[p]].fname == dir) {
			cout << "Duplicate directory name!" << endl;
			return;
		}
	}
	// 检查当前目录下是否有与输入目录名同名的目录

	// 查找输入目录的父目录
	vector<string>vec = split(dir, '/');
	string str;
	for (int j = 0; j < vec.size() - 1; j++) {
		if (j != vec.size() - 2) {
			str += vec[j] + "/";
		}
		else {
			str = str + vec[j];
		}
	}

	int q = 0;
	for (; q < BlockSize; q++) {
		if (DISKBLOCK[q].fname == str)
			break;
	}
	// 找到父目录

	//if (q == BlockSize - 1) {
	//	cout << "Invalid path!" << endl;
	//	return;
	//}
	//// 如果找不到父目录

	for (int j = 0; j < BlockSize; j++) {
		if (DISKBLOCK[q].dir_childPos[j] == 0) {
			DISKBLOCK[q].dir_childPos[j] = i;
			DISKBLOCK[q].line++;
			break;
		}
	}
	// 将子目录的信息添加到父目录中

	DISKBLOCK[i].fname = dir; // 将新目录的信息写入空块
	DISKBLOCK[i].selfPos = i;
	DISKBLOCK[i].fatherPos = q;
	DISKBLOCK[i].line = 0;
	DISKBLOCK[i].type = DIR;
	DISKBLOCK[i].permissions = "-1";
	for (int p = 0; p < BlockSize; p++) {
		DISKBLOCK[i].file_context[p] = "";
		DISKBLOCK[i].dir_childPos[p] = 0;
	}
	// 将子目录写入空块
	cout << "Success" << endl;
}

/*
* 遍历所有的 DISKBLOCK 块
* 如果当前 DISKBLOCK 块的fname与目标路径相同，则将当前路径设为目标路径，并返回
* 如果没有找到目标路径，则输出错误信息
*/
//移动当前路径
void cd(string path)
{
	// 遍历所有的DISKBLOCK块
	for (int j = 0; j < BlockSize; j++)
	{
		// 如果当前DISKBLOCK块的文件名与目标路径相同
		if (DISKBLOCK[j].fname == path)
		{
			// 将当前路径设为目标路径
			currentPath = path;
			return;
		}
	}
	// 如果没有找到目标路径，则输出错误信息
	cout << "Invalid path" << endl;
}

/*
* 找到当前路径所在的 DISKBLOCK 块
* 如果当前 DISKBLOCK 块已满，则输出 "Current directory is full!" 错误信息
* 找到一个空的 DISKBLOCK 块
* 如果内存已满，则输出 "Memory is full! Please delete some files first!" 错误信息
* 打开要引入的文件，并逐行读取文件内容，并将其写入到空的 DISKBLOCK 块中
* 更新空的 DISKBLOCK 块的相关信息，包括文件名、行数、类型等
* 更新当前路径所在的 DISKBLOCK 块的相关信息，包括文件名、行数、占用位置等
*/
// 在当前目录下从磁盘引入文件
void _import(string filepath, string opt) // 要在文件路径的末尾加上空格
{
	//cout <<"."<< opt <<"." << endl;
	if (opt == "-rw")
	{
	}else if(opt == "-r")
	{ 
	}else if(opt == "-w")
	{
	}
	else
	{
		cout << "Invalid Input" << endl;
		return;
	}
	int i = 0;
	for (; i < BlockSize; i++) // 找到当前路径
	{
		if (DISKBLOCK[i].fname == currentPath)
		{
			break;
		}
	}
	if (DISKBLOCK[i].line == BlockSize)
	{
		cout << "Current directory is full!" << endl; // 当前目录已满
		return;
	}
	int temp = 0;
	for (; temp < BlockSize; temp++) // 找到空的内存块
	{
		if (DISKBLOCK[temp].type == NONE)
			break;
	}
	if (temp == BlockSize)
	{
		cout << "Memory is full! Please delete some files first!" << endl; // 内存已满！请先删除！
		return;
	}
	int line = CountLine(filepath); // 打开要引入的文件
	fstream f(filepath);
	for (int p = 0; p < line + 1; p++) // 写入
	{
		getline(f, DISKBLOCK[temp].file_context[p]);
	}
	if (opt == "-rw")
	{
		DISKBLOCK[temp].permissions = "rw";
	}
	else if (opt == "-r")
	{
		DISKBLOCK[temp].permissions = "r";
	}
	else if (opt == "-w")
	{
		DISKBLOCK[temp].permissions = "w";
	}
	DISKBLOCK[temp].selfPos = temp;
	DISKBLOCK[temp].fatherPos = i;
	vector<string> vec = split(filepath, '\\');
	string f_name = currentPath + "/" + vec[vec.size() - 1];
	int times = 0;
	for (int k = 0; k < BlockSize; k++)
	{
		if (f_name == DISKBLOCK[k].fname)
		{
			times++;
		}
	}
	if (times != 0)
	{
		cout << "Duplicate filename and [" << vec[vec.size() - 1] << "] will be renameed [" << vec[vec.size() - 1] << "(" << times << ")]" << endl;
		DISKBLOCK[temp].fname = f_name + "(" + to_string(times) + ")";
	}
	else
	{
		DISKBLOCK[temp].fname = f_name;
	}
	DISKBLOCK[temp].line = line;
	DISKBLOCK[temp].type = FILE;
	for (int p = 0; p < BlockSize; p++)
	{
		if (DISKBLOCK[i].dir_childPos[p] == 0)
		{
			DISKBLOCK[i].dir_childPos[p] = temp;
			DISKBLOCK[i].line++;
			break;
		}
	}
	cout << "Write completed." << endl; // 写入完成
}


//显示目录下包含的子目录和文件
void dir(string input)
{

	bool isExist = false;//用来表示目录中是否存在东西
	int pos = 0;

	for (; pos < BlockSize; pos++)//找到了当前路径
	{
		if (DISKBLOCK[pos].fname == currentPath)
		{
			break;
		}
	}
	char ch = input[0];
	if (ch == '*')
	{
		string _suffix = input.substr(1);
		//cout << "***" << _suffix << "***" << endl;
		string file_suffix;
		bool isOutput = false;
		int strPos = -1;
		for (int k = 0; k < BlockSize; k++)
		{
			int j = 0;
			for (; j < DISKBLOCK[k].fname.length(); j++)
			{
				//cout << DISKBLOCK[i].fname[j] << " ";
				char curCh = DISKBLOCK[k].fname[j];
				if (curCh == '.')
				{
					strPos = j;
					break;
				}
			}
			int l = DISKBLOCK[k].fname.length() - 1;
			for (; l > 0; l--)
			{
				//cout << DISKBLOCK[i].fname[j] << " ";
				char curCh = DISKBLOCK[k].fname[l];
				if (curCh == '/')
				{
					strPos = l;
					break;
				}
			}

			string file_preffix = DISKBLOCK[k].fname.substr(0, l);
			//cout << endl;
			string file_suffix = DISKBLOCK[k].fname.substr(j);
			//cout << "---"<<file_suffix << endl;
			//当前路径下的文件
			if (file_suffix == _suffix&&file_preffix == currentPath)
			{
				isOutput = true;
				cout << "  " << DISKBLOCK[k].fname << endl;
			}
		}

		if (isOutput == false)
		{
			cout << "Not Found" << endl;
		}
		return;
	}
	if (input == "/s")		
	{
		for (int j = 0; j < DISKBLOCK[pos].line; j++)
		{
			vector<string> vec = split(DISKBLOCK[DISKBLOCK[pos].dir_childPos[j]].fname, '/');
			cout << vec[vec.size() - 1] << "\t";
			isExist = true;
		}
		cout << endl;
	}
	else if (input == "/d")		//显示当前目录
	{
		for (int j = 0; j < DISKBLOCK[pos].line; j++)
		{
			if (DISKBLOCK[DISKBLOCK[pos].dir_childPos[j]].type == DIR)
			{
				vector<string> vec = split(DISKBLOCK[DISKBLOCK[pos].dir_childPos[j]].fname, '/');
				cout << vec[vec.size() - 1] << "\t";
				isExist = true;
			}
		}
		cout << endl;
	}
	else if (input == "/f")//显示所有文件
	{
		string opt; cin >> opt;
		for (int j = 0; j < DISKBLOCK[pos].line; j++)
		{
			if (DISKBLOCK[DISKBLOCK[pos].dir_childPos[j]].type == FILE)
			{
				if (opt == "-r" && DISKBLOCK[DISKBLOCK[pos].dir_childPos[j]].permissions == "r")
				{
					vector<string> vec = split(DISKBLOCK[DISKBLOCK[pos].dir_childPos[j]].fname, '/');
					cout << vec[vec.size() - 1] << "\t";
					isExist = true;
				}
				else if (opt == "-w" && DISKBLOCK[DISKBLOCK[pos].dir_childPos[j]].permissions == "w")
				{
					vector<string> vec = split(DISKBLOCK[DISKBLOCK[pos].dir_childPos[j]].fname, '/');
					cout << vec[vec.size() - 1] << "\t";
					isExist = true;
				}
				else if (opt == "-rw" && DISKBLOCK[DISKBLOCK[pos].dir_childPos[j]].permissions == "rw")
				{
					vector<string> vec = split(DISKBLOCK[DISKBLOCK[pos].dir_childPos[j]].fname, '/');
					cout << vec[vec.size() - 1] << "\t";
					isExist = true;
				}
				else if (opt == "-a")
				{
					vector<string> vec = split(DISKBLOCK[DISKBLOCK[pos].dir_childPos[j]].fname, '/');
					cout << vec[vec.size() - 1] << "\t";
					isExist = true;
				}
			}
		}
		cout << endl;
	}

	if (isExist == false)
	{
		cout << "Null" << endl<<endl;
	}
}

//导出文件
void _export(string filename, string path)
{
	for (int j = 0; j < BlockSize; j++)
	{
		if (DISKBLOCK[j].fname == filename)
		{
			vector<string>vec = split(filename, '/');
			ofstream fout(path + vec[vec.size() - 1]);
			if (!fout.is_open())
			{
				cout << "Can't open"<<endl;
				return;
			}
			for (int p = 0; p < DISKBLOCK[j].line; p++)
			{
				fout << DISKBLOCK[j].file_context[p] << endl;
			}
			cout << "Export complete" << endl;
			fout.close();
		}
	}
}

//******递归删除目录包括子目录及文件
void rmdir(string name)
{
	for (int j = 0; j < BlockSize; j++)
	{
		/*if (DISKBLOCK[j].type == FILE)
		{
			cout << "It is FILE, you want [rm]?" << endl;
			return;
		}*/
		if (DISKBLOCK[j].fname == name)
		{
			int len = DISKBLOCK[j].line;//内容长度
			int i = DISKBLOCK[j].fatherPos;
			if (len != 0)
			{
				int* a = new int[len];
				for (int q = 0; q < len; q++)
				{
					a[q] = DISKBLOCK[j].dir_childPos[q];
				}
				for (int q = 0; q < len; q++)
				{
					rmdir(DISKBLOCK[a[q]].fname);
				}
			}
			DISKBLOCK[j].fname = "";
			DISKBLOCK[j].type = NONE;
			DISKBLOCK[j].fatherPos = 0;
			DISKBLOCK[j].line = 0;
			for (int p = 0; p < DISKBLOCK[j].line; p++)
			{
				DISKBLOCK[j].file_context[p] = "";
				DISKBLOCK[j].dir_childPos[p] = 0;
			}

			//把父目录的line--，pos更改
			DISKBLOCK[i].line--;
			int p = 0;
			while (p < BlockSize - 1)
			{
				if (DISKBLOCK[i].dir_childPos[p] == j)
				{
					while (p < BlockSize - 1)
					{
						DISKBLOCK[i].dir_childPos[p] = DISKBLOCK[i].dir_childPos[p + 1];
						p++;
					}
					break;
				}
				else
				{
					p++;
				}
			}
			cout << "Success" << endl;
			return;
		}
	}

}

//删除文件
void rm(string name)
{

	for (int j = 0; j < BlockSize; j++)
	{
		string file_name = currentPath + "/" + name;
		if (file_name == DISKBLOCK[j].fname && DISKBLOCK[j].type == DIR)
		{
			cout << "It is DIR,you want [rmdir]?" << endl;
			return;
		}
		if (DISKBLOCK[j].fname == name && DISKBLOCK[j].type == FILE)
		{
			int i = DISKBLOCK[j].fatherPos;
			int len = DISKBLOCK[j].line;
			DISKBLOCK[j].fname = "";
			DISKBLOCK[j].type = NONE;
			DISKBLOCK[j].fatherPos = 0;
			DISKBLOCK[j].permissions = "-1";
			DISKBLOCK[j].line = 0;
			for (int p = 0; p < BlockSize; p++)
			{
				DISKBLOCK[j].file_context[p] = "";
				DISKBLOCK[j].dir_childPos[p] = 0;
			}
			//把父目录的line--，pos更改
			DISKBLOCK[i].line--;
			int p = 0;
			while (p < BlockSize - 1)
			{
				if (DISKBLOCK[i].dir_childPos[p] == j)
				{
					while (p < BlockSize - 1)
					{
						DISKBLOCK[i].dir_childPos[p] = DISKBLOCK[i].dir_childPos[p + 1];
						p++;
					}
					break;
				}
				else
				{
					p++;
				}
			}
			cout << "Success" << endl;
			return;
		}

	}
}

/*
* 创建一个 vector 类型的变量 vec 用于存储文件路径的各个部分。
* 遍历虚拟文件系统中的每一个块（block），块的数量由 BlockSize 定义。
* 将文件名按照斜杠（'/'）进行分割，存储到 vec 中。
* 如果最后一个元素与传入的名称 name 相同，则进行下一步操作。
* 输出文件名（包括路径），并提示用户输入 'Y' 或 'y' 以查看该文件或目录的详细信息。
* 如果用户输入了 'Y' 或 'y'，则输出文件或目录的详细信息，包括路径、类型（文件或目录）、行数、占用的内存块数等。
* 如果是目录，则输出其子目录的编号，如果是文件，则输出文件内容及其权限。
* 如果用户输入了 'Y' 或 'y'，则函数返回，否则继续遍历下一个块。
* 如果遍历完所有块都没有找到匹配的文件或目录->不存在
*/
void find(string name) {//不同路径但是重名的文件
	vector<string> vec; // 存储文件路径的各个部分
	for (int i = 0; i < BlockSize; i++) { // 遍历虚拟文件系统中的每一个块
		vec = split(DISKBLOCK[i].fname, '/'); // 将文件名按照斜杠（'/'）进行分割，存储到 vec 中
		if (vec[vec.size() - 1] == name) { // 如果最后一个元素与传入的名称 name 相同，则进行下一步操作
			cout << DISKBLOCK[i].fname<<"?[y]" << endl; // 输出文件名（包括路径）
			char ch;
			cin >> ch;
			if (ch == 'Y' || ch == 'y') { // 如果用户输入了 'Y' 或 'y'，则输出文件或目录的详细信息
				cout << "path：" << DISKBLOCK[i].fname << endl; // 输出路径
				if (DISKBLOCK[i].type == NONE)
					cout << "NULL" << endl;
				else { // 如果不是空块，则输出详细信息
					if (DISKBLOCK[i].type == DIR) { // 如果是目录，则输出其子目录的编号
						cout << "It is a DIRECTORY" << endl;
						cout << DISKBLOCK[i].line << " sub dir/file" << endl;
					}
					else { // 如果是文件，则输出文件内容及其权限
						cout << "It is a FILE" << endl;
						cout << DISKBLOCK[i].line << " lines" << endl;
					}
				}
				cout << "Memory block：" << DISKBLOCK[i].selfPos << endl; // 输出占用的内存块数
				if (DISKBLOCK[i].fatherPos == -1)
					cout << "Root: ghy" << endl;
				//else
				//	cout << "parentdir：" << DISKBLOCK[DISKBLOCK[i].fatherNum].fname << endl; // 输出父目录的路径
				if (DISKBLOCK[i].type == DIR) { // 如果是目录，则输出其子目录的编号
					cout << "sub dir/file memory position：";
					if (DISKBLOCK[i].line != 0) { // 如果有子目录，则逐个输出它们的编号
						for (int p = 0; p < DISKBLOCK[i].line; p++) {
							cout << DISKBLOCK[i].dir_childPos[p] << "\t";
						}
						cout << endl;
					}
					else { // 如果没有子目录，则输出 "Null"
						cout << "Null" << endl;
					}
				}
				else if (DISKBLOCK[i].type == FILE) { // 如果是文件，则输出文件内容及其权限
					cout << "File content：";
					if (DISKBLOCK[i].line != 0) { // 如果文件有内容，则逐行输出
						for (int p = 0; p < DISKBLOCK[i].line; p++) {
							if (DISKBLOCK[i].permissions == "w") { // 如果文件只有写权限
								cout << "File is Writable ONLY" << endl;
								break;
							}
							cout << DISKBLOCK[i].file_context[p] << endl;
						}
					}
					else { // 如果文件没有内容，则输出 "Null"
						if (DISKBLOCK[i].permissions == "w") { // 如果文件只有写权限
							cout << "File is Writable ONLY" << endl;
							break;
						}
						cout << "Null" << endl;
					}
					cout << "File permissions:" << DISKBLOCK[i].permissions << endl; // 输出文件的权限
				}
				return; // 函数返回
			}
		}
	}
	cout << "No Such File" << endl; // 如果遍历完所有块都没有找到匹配的文件
}


//创建文件
void create(string fname,string permissions)
{
	cin >> permissions; //cout << permissions << endl;
	if (permissions == "-r")
	{
		permissions = "r";
	}
	else if (permissions == "-w")
	{
		permissions = "w";
	}
	else if (permissions == "-rw")
	{
		permissions = "rw";
	}
	else
	{
		cout << "Invalid Input" << endl;
		return;
	}
	int i = 0;
	for (; i < BlockSize; i++)
	{
		if (DISKBLOCK[i].fname == currentPath)
		{
			break;
		}
	}
	if (i == BlockSize)
	{
		cout << "Invalid Input" << endl;
		return;
	}
	for (int j = 0; j < BlockSize; j++)
	{
		if (DISKBLOCK[j].fname == currentPath + "/" + fname)
		{
			cout << "Invalid Input" << endl;
			return;
		}
	}
	if (DISKBLOCK[i].line == BlockSize)
	{
		cout << "Not enough space" << endl;
		return;
	}
	int tmp = 0;
	for (; tmp < BlockSize; tmp++)
	{
		if (DISKBLOCK[tmp].type == NONE)
			break;
	}
	if (tmp == BlockSize)
	{
		cout << "Not enough space" << endl;
		return;
	}

	DISKBLOCK[tmp].fname = currentPath + "/" + fname;
	DISKBLOCK[tmp].selfPos = tmp;
	DISKBLOCK[tmp].fatherPos = i;
	DISKBLOCK[tmp].line = 0;
	DISKBLOCK[tmp].type = FILE;
	//DISKBLOCK[tmp].curPos = 0;
	DISKBLOCK[tmp].permissions = permissions;
	for (int p = 0; p < BlockSize; p++)
	{
		DISKBLOCK[tmp].file_context[p] = "";
	}
	DISKBLOCK[i].dir_childPos[DISKBLOCK[i].line] = tmp;
	DISKBLOCK[i].line++;
	cout << "Success" << endl;
}



void time()
{
	time_t t;
	time(&t);
	cout << ctime(&t) << endl;
}

//重命名
void rename(string oldname, string newname)
{
	int i = 0;
	for (; i < BlockSize; i++)
	{
		if (DISKBLOCK[i].fname == oldname)
		{
			//重名检测
			for (int j = 0; j < BlockSize; j++)
			{
				if (j == i)
				{
					continue;
				}
				else
				{
					if (DISKBLOCK[j].fname == newname)
					{
						cout << "Duplicate directory name!" << endl;
						return;
					}
				}
			}
			DISKBLOCK[i].fname = newname;
			cout << "Success" << endl;
			break;
		}
	}
	if (i == BlockSize - 1)
	{
		cout << "No such file" << endl;
	}
}

void showVer()
{
	cout << "VIRTUAL FILE SYSTEM [版本: " << VERSION << "]" << endl;
}

bool lseek(int blockNum, int& line, int& pos, int offset)
{
	int fileLength = 0;
	for (int i = 0; i < DISKBLOCK[blockNum].line; i++) {
		fileLength += DISKBLOCK[blockNum].file_context[i].length();
		//cout << "*** " << fileLength << endl;
	}
	if (offset > fileLength || offset < 0) {
		cout << "Invalid Input" << endl;
		return false;
	}
	/*
		* 1 累加前j行中文件总字符数量
		* 2 当字符总数>=你需要的当前位置时,该行就是你修改的所需的行
		* 3 于是找到前一行,并减去当前行
		* 4 用你需要的位置offset减去前面的总数,就是你在这一行的位置
		* 5 传递参数,将当前行作为修改行,将当前位置作为需要插入字符串的位置
	*/
	int sumCh = 0;
	int curLi, curPos;
	for (int i = 0; i < DISKBLOCK[blockNum].line; i++)
	{
		sumCh += DISKBLOCK[blockNum].file_context[i].length();
		if (sumCh > offset)
		{
			sumCh -= DISKBLOCK[blockNum].file_context[i].length();
			curLi = i;
			curPos = offset - sumCh;
			line = curLi;
			pos = curPos;
			//cout << "*** curLi " << curLi << " curPos " << curPos << endl;
			break;
		}
	}
	return true;
}
//读取并显示文件内容 
void readfile(string fname)
{

	string opt;
	cin >> opt;
	int i = 0;
	for (; i < BlockSize; i++)
	{
		if (DISKBLOCK[i].fname == currentPath + "/" + fname)
		{
			break;
		}
	}
	if (i == BlockSize)
	{
		cout << "No such file" << endl;
		return;
	}
	if (i != editfile)
	{
		cout << "file not opened" << endl;
		return;
	}
	if (DISKBLOCK[i].type == DIR)
	{
		cout << "It is DIR" << endl;
		return;
	}
	if (DISKBLOCK[i].permissions == "w")
	{
		cout << "File is Writable ONLY";
		return;
	}
	if (DISKBLOCK[i].file_context->empty())
	{
		cout << endl << "Null" << endl << endl;;
	}
	else
	{
		if (opt == "-a")//all
		{
			//read [filename] [opt]
			//cout << "文件内容:" << endl;
			for (int j = 0; j < DISKBLOCK[editfile].line; j++)
			{
				cout << DISKBLOCK[editfile].file_context[j] << endl;
			}
		}
		else if (opt == "-s")//separate
		{
			//read [filename] [opt] [offset]
			int offset;
			cin >> offset;
			int _line, _pos;
			//使用lseek
			if (lseek(i, _line, _pos, offset) == false)
			{
				return;
			}
			else
			{
				for (int j = _pos; j < DISKBLOCK[i].file_context[_line].length(); j++)
				{
					//先输出lseek定位到的行
					//string[]的构造
					cout << DISKBLOCK[i].file_context[_line][j];
				}
				cout << endl;
				//输出后面的行
				for (int j = _line + 1; j < DISKBLOCK[i].line; j++)
				{
					//事实上lseek定位到的行是最后一行就不输出了，否则 context 会数组越界
					if (j > DISKBLOCK[i].line)
					{
						return;
					}
					else
					{
						cout << DISKBLOCK[editfile].file_context[j] << endl;
					}
				}
			}
		}
	}
	
}


//编辑并保存文件内容
void writefile(string fname)
{
	string opt;
	cin >> opt;
	int i = 0;
	for (; i < BlockSize; i++)
	{
		if (DISKBLOCK[i].fname == currentPath + "/" + fname)
		{
			break;
		}
	}
	if (i == BlockSize)
	{
		cout << "No such file" << endl;
		return;
	}
	if (i != editfile)
	{
		cout << "File not opened" << endl;
		return;
	}
	if (DISKBLOCK[i].type == DIR)
	{
		cout << "It is DIR" << endl;
		return;
	}

	if (DISKBLOCK[i].permissions == "r")
	{
		cout << "File is Read ONLY" << endl;
		return;
	}
	if (opt == "-n")
	{
		//write [filename] [opt] [data]
		getchar();//
		string data;
		getline(cin, data);
		DISKBLOCK[i].file_context[DISKBLOCK[i].line++] = data;
		cout << "Success" << endl;
	}
	else if (opt == "-i")
	{
		//write [filename] [opt] [position] [data]
		int offset;
		cin >> offset;
		getchar();
		string data;
		getline(cin, data);
		int _line, _pos;
		if (lseek(i, _line, _pos, offset) == false)
		{
			return;
		}
		else
		{
			DISKBLOCK[i].file_context[_line].insert(_pos, data);
			cout << "Success" << endl;
		}
	}
	else
	{
		cout << "Invalid Input" << endl;
	}
}

//打开文件
void open(string fname)
{
	int i = 0;
	for (; i < BlockSize; i++)
	{
		if (DISKBLOCK[i].fname == currentPath + "/" + fname)
		{
			break;
		}
	}
	if (i == BlockSize)
	{
		cout << "No such file" << endl;
		return;
	}
	if (DISKBLOCK[i].type == DIR)
	{
		cout << "It is DIR" << endl;
		return;
	}
	if (editfile != -1)
	{
		//cout << DISKBLOCK[editfile].fname << "文件已打开!" << endl;
		cout << "Already opened file  " << DISKBLOCK[editfile].fname << " ,please close this file first" << endl;
		return;
	}
	editfile = i;  //记录打开文件的块号
	cout << "Success" << endl;
}


//关闭文件
void close()
{
	if (editfile == -1)
	{
		cout << "No files open" << endl;
		return;
	}
	editfile = -1;   //重置编辑文件块号
	cout << "Success" << endl;
}
//
void head()
{
	cout << "VIRTUAL FILE SYSTEM [版本: " << VERSION << "]" << endl;
	cout << "(BJFU) 201002113 GHY。 虚拟文件系统。" << endl << endl;
}

int main() {
	read();
	string opt;
	string cmd_1, cmd_2;
	head();
	format(1);
	//cout << "首次使用请运行format指令" << endl;
	while (1)
	{
		bool isClear = false;
		cout << currentPath + ">";

		
		cin >> opt;
		if (opt == "mkdir")
		{
			read();
			cin >> cmd_1;
			if (cmd_1[0] == '/')
			{
				cout << "Invalid Input" << endl;
			}
			else
			{
				cmd_1 = change(cmd_1);
				mkdir(cmd_1);
			}
			writeBack();
		}
		else if (opt == "clear")
		{
			clear();
			head();
		}
		else if (opt == "help")
		{
			help();
		}
		else if (opt == "create")
		{
			read();
			cin >> cmd_1;
			//cin >> cmd3;

			create(cmd_1, cmd_2);
			writeBack();
		}
		else if (opt == "cd")
		{
			read();
			cin >> cmd_1;
			cmd_1 = change(cmd_1);
			cd(cmd_1);
		}
		else if (opt == "dir")
		{
			read();
			cin >> cmd_1;
			dir(cmd_1);
		}
		else if (opt == "exit")
		{
			read();
			writeBack();
			break;
		}
		else if (opt == "import")
		{
			read();
			cin >> cmd_1;
			cin >> cmd_2;
			_import(cmd_1,cmd_2);
			writeBack();
		}
		else if (opt == "export")
		{
			read();
			cin >> cmd_1;
			cmd_1 = change(cmd_1);
			cin >> cmd_2;
			_export(cmd_1, cmd_2);
			writeBack();
		}
		else if (opt == "rmdir")
		{
			read();
			cin >> cmd_1;
			cmd_1 = currentPath + "/" + cmd_1;
			rmdir(cmd_1);
			writeBack();
		}
		else if(opt == "open")
		{
			read();
			cin >> cmd_1;
			//cmd_1 = change(cmd_1);
			open(cmd_1);
		}
		else if (opt == "close")
		{
			close();
		}
		else if (opt == "format")
		{
			format(2);
		}
		else if (opt == "rm")
		{
			read();
			cin >> cmd_1;
			cmd_1 = change(cmd_1);
			rm(cmd_1);
			writeBack();
		}
		else if (opt == "find")
		{

			read();
			cin >> cmd_1;
			find(cmd_1);
		}
		else if (opt == "time")
		{
			time();
		}
		else if (opt == "rename")
		{
			read();
			cin >> cmd_1;
			cmd_1 = currentPath + "/" + cmd_1;
			cin >> cmd_2;
			cmd_2 = currentPath + "/" + cmd_2;
			rename(cmd_1, cmd_2);
			writeBack();
		}
		else if (opt == "ver")
		{
			showVer();
		}
		else if (opt == "read")
		{
			read();
			cin >> cmd_1;
			readfile(cmd_1);
		}
		else if (opt == "write")
		{
			read();
			cin >> cmd_1;
			
			writefile(cmd_1);
			writeBack();
		}
		else if (opt == "renew")
		{
			read();
		}
		else {

			read();
			cout << "Invalid Input,input help to learn more." << endl;
		}
		
	}

}
