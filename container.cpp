#include <iostream>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <fstream>

using std::cout;
using std::fstream;
using std::ios;
using std::string;

const int SIZE = 100000;
const std::string CGROUP_PATH = "/sys/fs/cgroup/container/";
const int FLAGS = CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWIPC | CLONE_NEWNET;

/**
 * Return a stack that points to the end
 *
 * @param capacity Size to allocate to the stack - in bytes
 * @return the stack
 */
char* build_stack(int capacity){
    char* stack = new (std::nothrow) char[capacity];

    if(stack == nullptr){
        cout << "Unable to allocate memory\n";
        exit(EXIT_FAILURE);
    }
    return stack + capacity;
}

/**
 * Append to a file.
 *
 * @param path path of file
 * @param str the string to be appended 
 */
void append(string path, string str){
    fstream file;
    file.open(path, ios::app);
    file << str << "\n";
    file.close();
}

/**
 * Constructs a cgroup for the container 
 * and applies limitations to the container
 */
void construct_cgroup(){  
    mkdir(CGROUP_PATH.c_str(), S_IRUSR | S_IWUSR);

    string pid  = std::to_string(getpid());
    append(CGROUP_PATH + "cgroup.procs", pid);
    append(CGROUP_PATH + "notify_on_release", "1");
    append(CGROUP_PATH + "pids.max", "3");
    append(CGROUP_PATH + "memory.limit_in_bytes", std::to_string(40 * 1024 * 1024));
}

/**
 * Sets the new root of the container 
 * and sets the working directory of the container to the new root
 * 
 * @param folder the path of the new root
 */
void set_root(const char* folder){
    chroot(folder);
    chdir("/");
}

/**
 * Clears the enviromental variables that is passed by the parent process
 * and sets some basic enviromental varibles for the shell
 */
void set_env(){
    clearenv();
    setenv("TERM", "xterm-256color", 0);
    setenv("PATH", "/bin/:/sbin/:usr/bin:/usr/sbin", 0);
}

/**
 * Sets up the container
 * 
 * @param args the process to run 
 */
char** set_up(void* args){
    char** input = (char**)args;
    set_env();
    set_root(*input++);
    sethostname("container", 9);
    mount("proc", "/proc", "proc", 0, 0);
    return input;
}

/**
 * Prepares to shutdown the container
 */
void clean_up(){
    umount("/proc");
}

/**
 * Calling the exec family of functions, replaces the 
 * current process image with a new process image.
 * 
 * To continue execution (for clean-up), we need to have the container
 * spawn a child and run the shell inside there.
 * 
 * @param args [container root path, shell]
 */
int decouple(void* args){
    char** run = set_up(args);
    execvp(*run, run);
    return EXIT_SUCCESS;
}

/**
 * Calling the exec family of functions, replaces the 
 * current process image with a new process image.
 * 
 * To continue execution (for clean-up), we need to have the container
 * spawn a child and run the shell inside there.
 * 
 * @param args [container root path, shell]
 */
int container(void* args){
    construct_cgroup();
    clone(decouple, build_stack(SIZE), SIGCHLD, args);
    clean_up();
    wait(nullptr);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]){
    clone(container, build_stack(SIZE), FLAGS | SIGCHLD, ++argv);
    wait(nullptr);
    return EXIT_SUCCESS;
}