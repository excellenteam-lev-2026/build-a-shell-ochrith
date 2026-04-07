//Exercise Build a Shell
//Ochrith PEREZ 209727361


#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <stdlib.h>
#include <fstream>


using namespace std;

struct Job{
    string command;
    int PID;
    string state;

};
vector<Job> jobsList;
 void checkJobs()
 {
    int status;
    for (auto& job :jobsList)
    {
        
        pid_t result = waitpid(job.PID, &status, WNOHANG);    // check if process finished but not waiting for him 
        if (result > 0){
             job.state="DONE";
        }
           

    }
 }


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
		
    const char* home= getenv("HOME");    //for history file - i choose to put the file here to open from every directory 
    string pathForHistory=(string)home+"/history.txt";
    ofstream history(pathForHistory,ios::app);
    bool background=false;
    int status;
	string cmd="";
	
	do {
        background=false;   // for background processes
        checkJobs();
		cout << ">>";
        getline(cin,cmd);   //read until tape ENTER ,better than cin>>cmd 
   
         // force to write on the dosk -not wait to write at the end of the shell 
        if (cmd=="") continue;
        else {
             
            history << cmd<< endl;     // adding command to history file even if the command is not valid ( in the exercise it's not specified so i add all the command except empty command)
             history.flush();
        
        }
       
		vector<string> splitted_command = split(cmd);
    


		bool interne_cmd = in_or_out_command(splitted_command[0]);

        //---------------INTERNE COMAND FIRST-----------------------------------------
        //----------------------------------------------------------------------------
        if (interne_cmd)

        {
            
            if (splitted_command[0] == "cd") {
                if (splitted_command.size()<2 or (splitted_command.size() >1 && splitted_command[1]=="~"))
                    {  
                        chdir(home);   //default value
                        continue;
                    }

                else if (chdir(splitted_command[1].c_str())!=0) 
                    cout<<" Failed to change directory, check if "<<splitted_command[1]<<" exists !"<<endl;
            }
            else if (splitted_command[0] == "exit") {
                exit(0);
            }
            else if (splitted_command[0] == "echo") {
                if (splitted_command.size() >1){
                    //const char** msg=(char* const*)splitted_command[1];
                    if (splitted_command[1][0]=='$')                 //bonus
                        cout<<getenv(splitted_command[1].substr(1).c_str())<<endl;
                    else {
                            for (size_t t=1;t<splitted_command.size();++t) cout<<splitted_command[t]<<" ";
                            cout<<endl;
                    }
                }
                
                
            }
          
            else if (splitted_command[0] == "myjobs")
            {
                for (auto& job : jobsList)
                    cout<<"["<<job.PID<<"]  "<<job.state<<"   "<<job.command<<endl;

            }
            else if (splitted_command[0] == "myhistory")
            {   cout<<"------------------- History ------------------"<<endl;
                ifstream r(pathForHistory);
                string line;
                while (getline(r,line)) cout<<line <<endl;
            }
            else cout << "no such interne comand found"<<endl;
            

            continue;                            // go back to while after execute intene command
        }



		//first off all --> have to find the comamnd in the PATH repertories
		const char* path = getenv("PATH");
        vector<string> dirs=get_dirs(path);  // get all the path for search a command 
        string my_command_entire_path="";
        if (is_complete_path(splitted_command[0]))
            my_command_entire_path=splitted_command[0];    //  if the user entered a command with '/' char consider its the full path command
        else
        {
            my_command_entire_path=search_command_in_PATH(dirs,splitted_command[0]);  // else go search the path over all the directories
            if (my_command_entire_path=="")
            {    cout<<"No such command"<<endl;
                 continue;
            }
            else  {

            }
        }
            

			

		//------------------ Externe command--------------------------------
        //------------------------------------------------------------------
        if (splitted_command[splitted_command.size()-1]=="&") {
                        background=true;
                        splitted_command.pop_back();    //remove the char "&" from the arg list
                    }
        pid_t child=fork();
        if (child==0)   // i am the child
                {

                     const char** args=fromVectorToArray(splitted_command);
                     
                     execv(my_command_entire_path.c_str(),(char* const*)args);  //even if exev failed the child process will continue so write code afetr the line is useless
                     cout<<"Command failed or not exists!"<<endl;
                     delete[] args;
                     exit(EXIT_FAILURE);    // if the command failed/not exists end the child xith exit to let the parend continue and not wait  uselessly
                }
               
        else if (child >0) { // i am the parent so i have to wait to my son
                if (background)
                {   
                    //-------------------------- Run Commmand In Background----------------
                      // -----  remove the arg "&"
                    Job job;
                    job.command=cmd;
                    job.PID=child;
                    job.state="RUNNING";
                    jobsList.push_back(job);
                    
                }
                    
                else
                    waitpid(child,&status,0);
                
                
            }
            
        else
                cout<<" fork failed"<<endl;
        
            
        
	}while (true);

    history.close();

}


