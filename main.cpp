#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <stdlib.h>

using namespace std;



//list<string> internes_command = { "cd","exit","echo","myjobs","myhistory" };
//enum INTERNAL_COMMANDS {cd,exit,echo,myjobs,myhistory};
string internes_command[5] = { "cd","exit","echo","myjobs","myhistory" };



vector<string> split(const string& command) {
	vector<string> result;
	istringstream iss(command);    // read word per word from a string , not from a strem like cin>>
	string word;

	while (iss >> word) {
		result.push_back(word);
	}

	return result;
}

bool in_or_out_command(string cmd) {

	for (int i = 0;i < 5;i++)
		if (cmd == internes_command[i])
			return true;
	return false;
 }

vector<string> get_dirs(string env_var) {   // i have done a second fucntion for split PATH because iss>> is not enough here-have to use getline

	istringstream iss(env_var);
	vector<string> result;
	string single_path = "";
	while (getline(iss, single_path, ':'))
	{
		result.push_back(single_path);
	}
	return result;
}


bool is_complete_path(string& path)
{
    return path.find('/')!=string::npos;
}
string command_exists_in_single_path(string path, string command)
{
	DIR* dp = opendir(path.c_str());
	if (dp == nullptr)
	{
		
        return "-1"; // or exit() to end all the programme 
	}
	struct dirent* entry;
	
    string file_command="";
	while ((entry = readdir(dp)) != nullptr) {
		if (command==entry->d_name) {   // conversion implicite command=string in left so convert also d_name to string
             
            file_command=path+"/"+entry->d_name;
             break;
            }
	}

	closedir(dp);
	return file_command;
}

string search_command_in_PATH(vector<string>& dirs, string command)
{
    string search_result="";
	for (vector<string>::iterator it=dirs.begin();it!= dirs.end();++it)
    {
        search_result=command_exists_in_single_path(*it,command);
        
        if   (search_result !="" && search_result!="-1")
               return search_result;
    }
        
    return search_result;
 }

const char** fromVectorToArray(const vector<string>& vec)
{
    int size = vec.size();
    const char** args = new const char*[size + 1];

    for (int i = 0; i < size; i++)
        args[i] = vec[i].c_str();  // c_str() valide tant que vec existe

    args[size] = nullptr;  // fin du tableau
    return args;
}

 
int main() {
    int status;
	cout << "------------the shell is running---------------------" << endl;
	string cmd="";
	
	do {
		cout << ">>";
        getline(cin,cmd);   //read until tape ENTER ,better than cin>>cmd 
        if (cmd=="") continue;
		vector<string> splitted_command = split(cmd);
		cout << "result[0]= " << splitted_command[0]<<endl;


		bool interne_cmd = in_or_out_command(splitted_command[0]);

        //---------------INTERNE COMAND FIRST
        if (interne_cmd)

        {
            cout << "--------------Interne Command-------------" << endl;
            if (splitted_command[0] == "cd") {
                chdir(splitted_command[1].c_str());
            }
            else if (splitted_command[0] == "exit") {
                exit(0);
            }
            else if (splitted_command[0] == "echo") cout<<splitted_command[1]<<endl;
            else cout << "no such interne comand found"<<endl;
            

            continue;
            


        }



		//first off all i have to find the comamnd in the PATH repertories
		const char* path = getenv("PATH");
		//// searhc if var exist ( if command without path)
        vector<string> dirs=get_dirs(path);
        string my_command_entire_path="";
        if (is_complete_path(splitted_command[0]))
            my_command_entire_path=splitted_command[0];
        else
        {
            my_command_entire_path=search_command_in_PATH(dirs,splitted_command[0]);
            if (my_command_entire_path=="")
            {    perror("no such command");
                continue;
            }
            else  {}
        }
            

			

		//------------------ continue to externe command
        {
            cout << "-----------------External Command---------------" << endl;
            pid_t child=fork();
            if (child==0)   // i am the child
                {
                     cout<<"child running"<<endl;
                     const char** args=fromVectorToArray(splitted_command);
                     execv(my_command_entire_path.c_str(),(char* const*)args);  //even if exev failed the child process will continue so write code afetr the line is useless
                     
                
                }
               
            else if (child >0) { // i am the parent so i have to wait to my son
                waitpid(child,&status,0);
            }
            
            else
                perror(" fork failed");
        }
	}while (true);
		




}


//arrete ici 
//>>echo "jhv"
//result[0]= echo
//--------------Interne Command-------------
//"jhv"