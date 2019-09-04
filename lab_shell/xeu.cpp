#include "xeu_utils/StreamParser.h"

#include <iostream>
#include <vector>
#include <cstdio>
#include <sstream>

#include <unistd.h> // Used: fork, execvp, dup
#include  <fcntl.h> // Used: open, dup
#include <sys/wait.h> // Used: wait
#include  <sys/types.h> // Used: wait
#include  <list> // Used: wait
#include <thread>

using namespace xeu_utils;
using namespace std;

struct job {
  pid_t pid;
  string name;
};

list<job> jobs;

// This function is just to help you learn the useful methods from Command
void io_explanation(const Command& command) {
  // Let's use this input as example: (ps aux >out >o2 <in 2>er >ou)
  // we would print "$       io(): [0] >out [1] >o2 [2] <in [3] 2>er [4] >ou"
  cout << "$       io():";

  for (size_t i = 0; i < command.io().size(); i++) {
    IOFile io = command.io()[i];
    cout << " [" << i << "] " << io.repr();
    // Other methods - if we had 2>file, then:
    // io.fd() == 2
    // io.is_input() == false ('>' is output)
    // io.is_output() == true
    // io.path() == "file"
  }
}

// This function is just to help you learn the useful methods from Command
void command_explanation(const Command& command) {

  /* Methods that return strings (useful for debugging & maybe other stuff) */

  // This prints the command in a format that can be run by our xeu. If you
  // run the printed command, you will get the exact same Command
  {
    // Note: command.repr(false) doesn't show io redirection (i.e. <in >out)
    cout << "$     repr(): " << command.repr() << endl;
    cout << "$    repr(0): " << command.repr(false) << endl;
    // cout << "$   (string): " << string(command) << endl; // does the same
    // cout << "$ operator<<: " << command << endl; // does the same
  }

  // This is just args()[0] (e.g. in (ps aux), name() is "ps")
  {
    cout << "$     name(): " << command.name() << endl;
  }

  // Notice that args[0] is the command/filename
  {
    cout << "$     args():";
    for (int i = 0; i < command.args().size(); i++) {
      cout << " [" << i << "] " << command.args()[i];
    }
    cout << endl;
  }

  /* Methods that return C-string (useful in exec* syscalls) */

  // this is just the argv()[0] (same as name(), but in C-string)
  {
    printf("$ filename(): %s\n", command.filename());
  }

  // This is similar to args, but in the format required by exec* syscalls
  // After the last arg, there is always a NULL pointer (as required by exec*)
  {
    printf("$     argv():");
    for (int i = 0; command.argv()[i]; i++) {
      printf(" [%d] %s", i, command.argv()[i]);
    }
    printf("\n");
  }

  io_explanation(command);
}

// This function is just to help you learn the useful methods from Command
void commands_explanation(const vector<Command>& commands) {
  // Shows a representation (repr) of the command you input
  // cout << "$ Command::repr(0): " << Command::repr(commands, false) << endl;
  cout << "$ Command::repr(): " << Command::repr(commands) << endl << endl;

  // Shows details of each command
  for (int i = 0; i < commands.size(); i++) {
    cout << "# Command " << i << endl;
    command_explanation(commands[i]);
    cout << endl;
  }
}

bool is_pid_running(pid_t pid) {

    return 0;
}

void redirectIO(Command& command) {
  IOFile io = command.io()[0];
  const char* path = io.path().c_str();
  int fd_io;

  if(io.is_output()) {
    fd_io = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    dup2(fd_io, STDOUT_FILENO);
  } else if(io.is_input()) {
    fd_io = open(path, O_RDONLY);
    dup2(fd_io, STDIN_FILENO);
  }
}

void wait_bg_proc(pid_t pid) {
  waitpid(pid, NULL, 0);
  for(list<job>::iterator it=jobs.begin(); it != jobs.end(); ++it){
    if (it->pid == pid) {
      cout << "erasing " << pid << endl;
      jobs.erase(it);
      break;
    }
  }
}

pid_t exec(Command& command) {  
  if (command.name() == "bg") {
    Command cmd = Command();
    for (int i = 1; i < command.args().size(); i++) {
      cmd.add_arg(command.args()[i]);
    }
    command = cmd;
  }

  if(execvp(command.filename(), command.argv()) == -1){
    perror("Error in exec");
    exit(EXIT_FAILURE);
  }
}

void xjobs() {
  for(list<job>::iterator it=jobs.begin(); it != jobs.end(); ++it){
        cout << "pid: " << it->pid << " name: " << it->name << endl;
  }
}


int main() {
    while (true) {
      printf("xeu$ ");
      const vector<Command> commands = StreamParser().parse().commands();
      pid_t process_id;
      Command command = commands[0];

      if (command.name() == "xjobs") {
        xjobs();
      } else {
        process_id = fork();
        if (process_id == -1) {
          perror("Error forking");
          exit(EXIT_FAILURE);
        }
        bool isBg = command.name() == "bg";
        if (process_id == 0){
          if (command.io().size() > 0){
            redirectIO(command);
          }
          exec(command);
          
        } else {
          if (isBg) {
            job j;
            j.pid = process_id; 
            j.name = command.argv()[1];
            jobs.push_back(j);

            thread bg_thread(wait_bg_proc, process_id);
            bg_thread.detach();
          } else {
            wait(NULL);
          }
        }
      }
    } 
  
}
