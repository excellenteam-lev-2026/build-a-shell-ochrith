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
#include <fcntl.h>
#include <array>

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

    for (auto& job : jobsList)
    {
        pid_t result = waitpid(job.PID, &status, WNOHANG);

        if (result > 0)
        {
            job.state = "DONE";
        }
    }
}

string internes_command[5] = {
    "cd",
    "exit",
    "echo",
    "myjobs",
    "myhistory"
};

vector<string> split(const string& command)
{
    vector<string> result;

    istringstream iss(command);
    string word;

    while (iss >> word)
    {
        result.push_back(word);
    }

    return result;
}

bool in_or_out_command(string cmd)
{
    for (int i = 0; i < 5; i++)
    {
        if (cmd == internes_command[i])
            return true;
    }

    return false;
}

vector<string> get_dirs(string env_var)
{
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
    return path.find('/') != string::npos;
}

string command_exists_in_single_path(string path, string command)
{
    DIR* dp = opendir(path.c_str());

    if (dp == nullptr)
    {
        return "-1";
    }

    struct dirent* entry;

    string file_command = "";

    while ((entry = readdir(dp)) != nullptr)
    {
        if (command == entry->d_name)
        {
            file_command = path + "/" + entry->d_name;
            break;
        }
    }

    closedir(dp);

    return file_command;
}

string search_command_in_PATH(vector<string>& dirs, string command)
{
    string search_result = "";

    for (vector<string>::iterator it = dirs.begin();
         it != dirs.end();
         ++it)
    {
        search_result = command_exists_in_single_path(*it, command);

        if (search_result != "" && search_result != "-1")
            return search_result;
    }

    return search_result;
}

const char** fromVectorToArray(const vector<string>& vec)
{
    int size = vec.size();

    const char** args = new const char*[size + 1];

    for (int i = 0; i < size; i++)
    {
        args[i] = vec[i].c_str();
    }

    args[size] = nullptr;

    return args;
}

void clean_command_forredirection(
    vector<string>& splitted_command,
    string& input_redirection,
    string& output_redirection,
    bool& trunc)
{
    for (size_t i = 0; i < splitted_command.size(); ++i)
    {
        if (splitted_command[i] == "<")
        {
            if (i + 1 >= splitted_command.size())
                continue;

            input_redirection = splitted_command[i + 1];

            splitted_command.erase(
                splitted_command.begin() + i,
                splitted_command.begin() + i + 2
            );

            --i;
        }

        else if (splitted_command[i] == ">" ||
                 splitted_command[i] == ">>")
        {
            if (i + 1 >= splitted_command.size())
                continue;

            string operator_redirection = splitted_command[i];

            output_redirection = splitted_command[i + 1];

            trunc = (operator_redirection == ">");

            splitted_command.erase(
                splitted_command.begin() + i,
                splitted_command.begin() + i + 2
            );

            --i;
        }
    }
}

vector<vector<string>> commands_separated_pipes(string cmd)
{
    vector<vector<string>> commands;

    istringstream iss(cmd);

    string command;

    while (getline(iss, command, '|'))
    {
        vector<string> splitted_command = split(command);

        commands.push_back(splitted_command);
    }

    return commands;
}

int main()
{
    const char* home = getenv("HOME");

    string pathForHistory = (string)home + "/history.txt";

    ofstream history(pathForHistory, ios::app);

    bool background = false;

    int status;

    string cmd = "";

    do
    {
        background = false;

        checkJobs();

        cout << ">>";

        getline(cin, cmd);

        if (cmd == "")
            continue;

        history << cmd << endl;
        history.flush();

        // ---------------------------------------------------------
        // Vérifier si la commande est en background
        // ---------------------------------------------------------

        vector<string> splitted_command = split(cmd);

        if (splitted_command.empty())
            continue;

        if (splitted_command[splitted_command.size() - 1] == "&")
        {
            background = true;

            // On enlève & de la commande
            splitted_command.pop_back();

            // Reconstruire cmd sans &
            cmd = "";

            for (size_t i = 0; i < splitted_command.size(); i++)
            {
                cmd += splitted_command[i];

                if (i + 1 < splitted_command.size())
                    cmd += " ";
            }
        }

        // ---------------------------------------------------------
        // Séparer les commandes par |
        // ---------------------------------------------------------

        vector<vector<string>> pipeline =
            commands_separated_pipes(cmd);

        if (pipeline.empty())
            continue;

        int number_of_processes = pipeline.size();

        // ---------------------------------------------------------
        // Cas commande interne simple
        // ---------------------------------------------------------

        if (number_of_processes == 1 &&
            !pipeline[0].empty() &&
            in_or_out_command(pipeline[0][0]))
        {
            string command = pipeline[0][0];

            if (command == "cd")
            {
                if (pipeline[0].size() < 2 ||
                    pipeline[0][1] == "~")
                {
                    chdir(home);
                }
                else if (chdir(pipeline[0][1].c_str()) != 0)
                {
                    cout << "Failed to change directory, check if "
                         << pipeline[0][1]
                         << " exists !"
                         << endl;
                }
            }

            else if (command == "exit")
            {
                exit(0);
            }

            else if (command == "echo")
            {
                if (pipeline[0].size() > 1)
                {
                    if (pipeline[0][1][0] == '$')
                    {
                        const char* value =
                            getenv(pipeline[0][1].substr(1).c_str());

                        if (value != nullptr)
                            cout << value << endl;
                        else
                            cout << endl;
                    }
                    else
                    {
                        for (size_t t = 1;
                             t < pipeline[0].size();
                             ++t)
                        {
                            cout << pipeline[0][t];

                            if (t + 1 < pipeline[0].size())
                                cout << " ";
                        }

                        cout << endl;
                    }
                }
                else
                {
                    cout << endl;
                }
            }

            else if (command == "myjobs")
            {
                for (auto& job : jobsList)
                {
                    cout << "["
                         << job.PID
                         << "]  "
                         << job.state
                         << "   "
                         << job.command
                         << endl;
                }
            }

            else if (command == "myhistory")
            {
                cout << "------------------- History ------------------"
                     << endl;

                ifstream r(pathForHistory);

                string line;

                while (getline(r, line))
                    cout << line << endl;
            }

            continue;
        }

        // ---------------------------------------------------------
        // PATH
        // ---------------------------------------------------------

        const char* path = getenv("PATH");

        vector<string> dirs = get_dirs(path);

        // ---------------------------------------------------------
        // CREATE PIPES
        // ---------------------------------------------------------

        vector<array<int, 2>> pipes;

        if (number_of_processes > 1)
        {
            pipes.resize(number_of_processes - 1);

            for (int i = 0;
                 i < number_of_processes - 1;
                 i++)
            {
                if (pipe(pipes[i].data()) == -1)
                {
                    perror("pipe");
                    return 1;
                }
            }
        }

        // ---------------------------------------------------------
        // CREATE PROCESSES
        // ---------------------------------------------------------

        vector<pid_t> children;

        for (int i = 0;
             i < number_of_processes;
             i++)
        {
            // -----------------------------------------------------
            // Redirections de CETTE commande
            // -----------------------------------------------------

            string input_redirection = "";

            string output_redirection = "";

            bool trunc = false;

            clean_command_forredirection(
                pipeline[i],
                input_redirection,
                output_redirection,
                trunc
            );

            if (pipeline[i].empty())
            {
                continue;
            }

            // -----------------------------------------------------
            // Trouver le chemin de CETTE commande
            // -----------------------------------------------------

            string command = pipeline[i][0];

            string command_path = "";

            if (is_complete_path(command))
            {
                command_path = command;
            }
            else
            {
                command_path =
                    search_command_in_PATH(dirs, command);

                if (command_path == "")
                {
                    cerr << "No such command: "
                         << command
                         << endl;

                    continue;
                }
            }

            // -----------------------------------------------------
            // FORK
            // -----------------------------------------------------

            pid_t pid = fork();

            if (pid == 0)
            {
                // =================================================
                // CHILD
                // =================================================

                // -------------------------------------------------
                // PIPE INPUT
                // -------------------------------------------------

                if (i > 0)
                {
                    if (dup2(
                        pipes[i - 1][0],
                        STDIN_FILENO
                    ) == -1)
                    {
                        perror("dup2");

                        exit(EXIT_FAILURE);
                    }
                }

                // -------------------------------------------------
                // PIPE OUTPUT
                // -------------------------------------------------

                if (i < number_of_processes - 1)
                {
                    if (dup2(
                        pipes[i][1],
                        STDOUT_FILENO
                    ) == -1)
                    {
                        perror("dup2");

                        exit(EXIT_FAILURE);
                    }
                }

                // -------------------------------------------------
                // INPUT REDIRECTION
                // -------------------------------------------------

                if (!input_redirection.empty())
                {
                    int fd =
                        open(
                            input_redirection.c_str(),
                            O_RDONLY
                        );

                    if (fd == -1)
                    {
                        cerr << "Failed to open input file: "
                             << input_redirection
                             << endl;

                        exit(EXIT_FAILURE);
                    }

                    if (dup2(
                        fd,
                        STDIN_FILENO
                    ) == -1)
                    {
                        perror("dup2");

                        close(fd);

                        exit(EXIT_FAILURE);
                    }

                    close(fd);
                }

                // -------------------------------------------------
                // OUTPUT REDIRECTION
                // -------------------------------------------------

                if (!output_redirection.empty())
                {
                    int flags =
                        O_WRONLY | O_CREAT;

                    if (trunc)
                        flags |= O_TRUNC;
                    else
                        flags |= O_APPEND;

                    int fd =
                        open(
                            output_redirection.c_str(),
                            flags,
                            0644
                        );

                    if (fd == -1)
                    {
                        cerr << "Failed to open output file: "
                             << output_redirection
                             << endl;

                        exit(EXIT_FAILURE);
                    }

                    if (dup2(
                        fd,
                        STDOUT_FILENO
                    ) == -1)
                    {
                        perror("dup2");

                        close(fd);

                        exit(EXIT_FAILURE);
                    }

                    close(fd);
                }

                // -------------------------------------------------
                // CLOSE ALL PIPES
                // -------------------------------------------------

                for (int j = 0;
                     j < number_of_processes - 1;
                     j++)
                {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }

                // -------------------------------------------------
                // EXEC
                // -------------------------------------------------

                const char** args =
                    fromVectorToArray(pipeline[i]);

                execv(
                    command_path.c_str(),
                    (char* const*)args
                );

                perror("execv");

                delete[] args;

                exit(EXIT_FAILURE);
            }

            else if (pid > 0)
            {
                // =================================================
                // PARENT
                // =================================================

                children.push_back(pid);
            }

            else
            {
                perror("fork");
            }
        }

        // ---------------------------------------------------------
        // PARENT CLOSES ALL PIPES
        // ---------------------------------------------------------

        for (int i = 0;
             i < number_of_processes - 1;
             i++)
        {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }

        // ---------------------------------------------------------
        // WAIT
        // ---------------------------------------------------------

        if (!background)
        {
            for (pid_t pid : children)
            {
                waitpid(pid, &status, 0);
            }
        }

        // ---------------------------------------------------------
        // BACKGROUND
        // ---------------------------------------------------------

        else
        {
            if (!children.empty())
            {
                Job job;

                job.command = cmd;

                job.PID = children[0];

                job.state = "RUNNING";

                jobsList.push_back(job);
            }
        }

    } while (true);

    return 0;
}