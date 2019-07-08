#include "graph.h"
#include "db.capnp.h"

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>

#include <list>
#include <vector>
#include <set>
#include <queue>

#include <capnp/message.h>
#include <capnp/serialize.h>

#define UNCHANGED 0
#define CHANGED   1
#define UNKNOWN   2


struct db_file {
    std::string path;
    int status;

    db_file(std::string path, int status) : path(path), status(status) {}
};

//TODO setup arguments
struct db_command { 
    unsigned int id;
    unsigned int num_descendants;
    std::string executable;
    //std::list<std::string> args;
    std::set<db_file*> inputs;
    std::set<db_file*> outputs;

    db_command(unsigned int id, unsigned int num_descendants, std::string executable) : id(id), num_descendants(num_descendants), executable(executable) {}
};

int main(int argc, char* argv[]) {

    int db = open("db.dodo", O_RDONLY);
    if (db < 0) {
        perror("Failed to open database");
        exit(2);
    }

    ::capnp::StreamFdMessageReader message(db);
    auto db_graph = message.getRoot<db::Graph>();


    // reconstruct the graph

    // initialize array of files
    db_file* files[db_graph.getFiles().size()];
    unsigned int file_id = 0;
    for (auto file : db_graph.getFiles()) {
        int flag = UNCHANGED;
        std::string path = std::string((const char*) file.getPath().begin(), file.getPath().size()); 
        // if the path was passed as an argument to the dryrun, mark it as changed
        for (int i = 1; i < argc; i++) {
            if (path == std::string(argv[i])) {
                flag = CHANGED;
            }
        }
        files[file_id] = new db_file(path, flag);
        file_id++;
    }

    // initialize array of commands
    db_command* commands[db_graph.getCommands().size()];
    unsigned int cmd_id = 0;
    for (auto cmd : db_graph.getCommands()) { 
        auto executable = std::string((const char*) cmd.getExecutable().begin(), cmd.getExecutable().size());
        commands[cmd_id] = new db_command(cmd_id, cmd.getDescendants(), executable); 
        cmd_id++;
    } 

    // Add the dependencies
    for (auto dep : db_graph.getInputs()) {  
        commands[dep.getCommandID()]->inputs.insert(files[dep.getFileID()]);
    }
    for (auto dep : db_graph.getOutputs()) {
        db_file* file = files[dep.getFileID()];
        // if the file is an output of a command, mark it's status as unknown (until the command is run/simulated)
        if (!(file->status == CHANGED)) {
            files[dep.getFileID()]->status = UNKNOWN;
        }
        commands[dep.getCommandID()]->outputs.insert(files[dep.getFileID()]);
    }

    //TODO what does the run do with removals? 
    /*
    // Draw the removals
    for (auto dep : db_graph.getRemovals()) {
    if (display_file[dep.getFileID()]) {
    graph.add_edge("c" + std::to_string(dep.getCommandID()), "f" + std::to_string(dep.getFileID()), "color=red");
    }
    }
     */


    // initialize worklist to commands root 
    //TODO multiple roots 
    std::queue<db_command*> worklist;
    worklist.push(commands[0]);

    while (worklist.size() != 0) {
        // take command off list to run/check
        db_command* cur_command = worklist.front();
        worklist.pop();

        // check if command is ready to run
        bool ready = true;
        bool rerun = false;
        for (auto in : cur_command->inputs) {
            if (in->status == UNKNOWN) {
                ready = false;
            } else if (in->status == CHANGED) {
                rerun = true;
            }
        }
        // if any dependencies are "unknown," put the command back in the worklist
        if (!ready) {
            worklist.push(cur_command);
        } else {
            if (rerun) {
                // if the command is ready to run and one of it's dependencies has changed, 
                //" rerun" it (print for now)
                std::cout << cur_command->executable << "\n";
                // mark its outputs as changed
                for (auto out : cur_command->outputs) {
                    out->status = CHANGED;
                }
            } else {
                // if all inputs are unchanged, mark outputs as unchanged (unless they are explicitly marked as changed in the dryrun)
                for (auto out : cur_command->outputs) {
                    if (out->status == UNKNOWN) {
                        out->status = UNCHANGED;
                    }
                }
            }
            
            // add command's direct children to worklist
            unsigned int current_id = cur_command->id + 1;
            unsigned int end_id = current_id + cur_command->num_descendants; 
            while (current_id < end_id) { 
                worklist.push(commands[current_id]);
                current_id += commands[current_id]->num_descendants + 1;
            }
        }
    } 
}
