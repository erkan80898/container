#include <iostream>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <fstream>

#define CGROUP_FOLDER "/sys/fs/cgroup/container/"

using std::cout;
using std::fstream;
using std::ios;
using std::string;

const int SIZE = 100000;
const std::string cgroupPath = "/sys/fs/cgroup/container/";

char* build_stack(int capacity){
    char* stack = new (std::nothrow) char[capacity];

    if(stack == nullptr){
        cout << "Unable to allocate memory\n";
        exit(EXIT_FAILURE);
    }
    return stack + capacity;
}

void write(string path, string str){
    fstream file;
    file.open(path, ios::app);
    file << str << "\n";
    file.close();
}

void construct_cgroup(){  
    mkdir(CGROUP_FOLDER, S_IRUSR | S_IWUSR);

    const char* pid  = std::to_string(getpid()).c_str();
    write(cgroupPath + "cgroup.procs", pid);
    write(cgroupPath + "notify_on_release", "1");
    write(cgroupPath + "pids.max", "3");
    write(cgroupPath + "memory.limit_in_bytes", std::to_string(40 * 1024 * 1024));
}

void set_root(const char* folder){
    chroot(folder);
    chdir("/");
}

void set_env(){
    clearenv();
    setenv("TERM", "xterm-256color", 0);
    setenv("PATH", "/bin/:/sbin/:usr/bin:/usr/sbin", 0);
}

char** set_up(void* args){
    char** input = (char**)args;
    set_env();
    set_root(*input++);
    sethostname("container", 9);
    mount("proc", "/proc", "proc", 0, 0);
    return input;
}

void clean_up(void){
    umount("/proc");
}

int decouple(void* args){
    char** run = set_up(args);
    execvp(*run, run);
    return EXIT_SUCCESS;
}

int container(void* args){
    construct_cgroup();
    clone(decouple, build_stack(SIZE), SIGCHLD, args);
    clean_up();
    wait(nullptr);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]){
    int flags = CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWIPC | CLONE_NEWNET;
    clone(container, build_stack(SIZE), flags | SIGCHLD, ++argv);
    wait(nullptr);
    return EXIT_SUCCESS;
}