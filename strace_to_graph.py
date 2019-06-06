#!/usr/bin/env python3

import copy
import graphviz
import os
import sys
from typing import List, Dict, Set, Optional
import parse_strace as parser

TEMP_ID = 0

class DuplicateProcessError(Exception):
    def __init__(self, e: parser.Event) -> None:
        self.e = e
    
    def __str__(self) -> str:
        return 'Process {} already exists.\n  {}'.format(self.e.pid, e)

class ProcessNotFoundError(Exception):
    def __init__(self, e: parser.Event) -> None:
        self.e = e
    
    def __str__(self) -> str:
        return 'Process {} was not found.\n  {}'.format(self.e.pid, e)

class File:
    num_files = 0

    def __init__(self, filename: str) -> None:
        self.filename = filename
        self.users: Set[Command] = set()
        self.producers: Set[Command] = set()
        self.id = self.num_files
        self.num_files+= 1
        self.version = 0
        self.trunc = False
        self.in_use = False

    def is_local(self) -> bool:
        if len(sys.argv) == 4 and sys.argv[3] == "--show-sysfiles":
            return True
        if os.path.dirname(self.filename) == "":
            return True
        else:
            return False

    #TODO rework
    def is_intermediate(self) -> bool:
        if len(self.users)!=0 and len(self.producers)!=0:
            return True
        else:
            return False


    def print_file(self) -> None:
        print(self.filename + ": " + str(self.version))

class Context:
    def __init__(self, starting_dir: str) -> None:
        self.starting_dir = starting_dir
        self.commands: List[Command] = []
        self.processes: Dict[int, Process] = {}
        self.files: Set[Files] = set()
    
    def handle_event(self, e: parser.Event) -> None:
        # Handle the entry point by creating a process and command
        if len(self.processes) == 0 and e.name == 'execve' and e.retval == 0:
            cmd = Command(self, e.args)
            self.commands.append(cmd)
            self.processes[e.pid] = Process(self, self.starting_dir, cmd)
        elif e.pid not in self.processes:
            print(self.processes.keys())
            raise ProcessNotFoundError(e)
        else:
            self.processes[e.pid].handle_event(e)
    
    def to_graph(self) -> graphviz.Digraph:
        #for f in self.files: 
        #    f.print_file()
        g = graphviz.Digraph(engine='dot')
        g.attr('graph', [('rankdir', 'LR')])
        g.attr('node', [('fontname', 'Courier')])
        
        # Generate nodes for files
        #for f in self.files:
         #   if f.is_local():        
          #      g.node(f.filename, os.path.basename(f.filename), shape='rectangle')
        # Generate nodes for all base commands
        for c in self.commands:
            c.to_graph(g)
        return g

    # return latest version or create and put in files
    def find_file(self, filename: str) -> File:
        ret = None
        for f in self.files:
            #print(f.filename + ": " + str(f.version))
            if f.filename == filename:
                if ret is None:
                    ret = f
                elif f.version > ret.version:
                    ret = f
        if ret is None:
            f = File(filename)
            self.files.add(f)
            return f
        else:
            return ret

class Command:
    def __init__(self, context: Context, args) -> None:
        self.context = context
        self.args = args
        self.children: List[Command] = []
        self.inputs: Set[File] = set()     
        self.outputs: Set[File] = set()
    
    def make_child(self, e: parser.Event):
        cmd = Command(self.context, e.args)
        self.children.append(cmd)
        return cmd
    
    def add_input(self, filename: str):
        f = self.context.find_file(filename)
        if not f.trunc:
            f.users.add(self)
            self.inputs.add(f)
        #f.users.add(self)
        #print("adding input: " + filename + "version " + str(f.version) +": is trunced? " + str(f.trunc))

    def add_output(self, filename: str):
        f = self.context.find_file(filename)
        f.producers.add(self)
        self.outputs.add(f)
    
    def to_graph(self, g: graphviz.Digraph) -> str:
        id = ' '.join(self.args[1])
        g.node(id, os.path.basename(self.args[0]), shape='oval', style='filled', fillcolor='gray35', fontcolor='white')
        
        for c in self.children:
            child_id = c.to_graph(g)
            g.edge(id, child_id, style="dashed")
        
        for i in self.inputs:
            if i.is_local():
                g.node(i.filename, os.path.basename(i.filename), shape='rectangle')
                g.edge(i.filename, id, arrowhead='empty')

        for o in self.outputs: 
            global TEMP_ID
            if not o.is_intermediate():
                g.node(o.filename, os.path.basename(o.filename), shape='rectangle')
                g.edge(id, o.filename, arrowhead='empty')
            else:
                # For intermediate files, create a node in the graph but do not show a name
                node_id = 'temp_'+str(TEMP_ID)
                TEMP_ID += 1
                n = g.node(node_id, label='\\<temp\\>', shape='rectangle') 
                g.edge(id, node_id, arrowhead='empty')
                
                # Create edges from the intermediate file to its dependents, since those commands will not create edges from non-local files by default
                for u in o.users:
                        g.edge(node_id, ' '.join(u.args[1]), arrowhead='empty')
        return id

class Process:
    def __init__(self, context: Context, cwd: str, command: Command, fd={}) -> None:
        self.context = context
        self.cwd = cwd
        self.command = command
        self.fd = copy.deepcopy(fd)
    
    def normpath(self, path: str) -> str:
        if not os.path.isabs(path):
            path = os.path.join(self.cwd, path)
        return os.path.normpath(path)
    
    def handle_event(self, e: parser.Event) -> None:
        if e.name in ['fork', 'vfork'] and e.retval > 0:
            self.context.processes[e.retval] = Process(self.context, self.cwd, self.command)
        
        elif e.name == 'clone' and e.retval > 0:
            # Is this a fork?
            if 'SIGCHLD' in e.args[1] and e.retval and e.retval != -1:
                # Yes, create a new process
                self.context.processes[e.retval] = Process(self.context, self.cwd, self.command)
            else:
                print('Trace contains thread creation, which is not yet handled.')
                exit(2)
        
        elif e.name == 'execve' and e.retval == 0:
            if e.retval == -1:
                # TODO: Handle failed execve calls
                pass
            else:
                print('{} in process {}'.format(e.args[0], e.pid))
                self.command = self.command.make_child(e)
        
        elif e.name in ['access', 'stat', 'lstat', 'readlink'] and e.retval == 0:
            filename = self.normpath(e.args[0])
            #self.command.add_input(filename)
        
        elif e.name in ['unlink', 'chmod'] and e.retval == 0:
            # unlink is a bit complicated. If you rm a file without statting or
            # reading first, we'll say it's just an output
            filename = self.normpath(e.args[0])
            f = self.context.find_file(filename)
            version = f.version
            f = File(filename)
            f.trunc = True
            f.version = version +1
            self.context.files.add(f)

            #self.command.add_output(self.normpath(filename))
        
        elif e.name == 'openat' and e.retval > 0:
            filename = self.normpath(e.args[1])
            self.fd[e.retval] = filename
            f = self.context.find_file(filename)
            f.trunc = False
            #if f.closed:
            if "O_TRUNC" in e.args[2] or "O_EXCL" in e.args[2]:
                version = f.version
                f = File(filename)
                f.trunc = True
                f.version = version+1
                self.context.files.add(f)

        elif e.name == 'dup' and e.retval > 0:
            fdnum = int(e.args[0])
            self.fd[e.retval] = self.fd[fdnum]
        
        elif e.name == 'read' and e.retval >= 0:
            fdnum = int(e.args[0])
            self.command.add_input(self.fd[fdnum])
        
        elif e.name == 'write' and e.retval >= 0:
            fdnum = int(e.args[0])
            self.command.add_output(self.fd[fdnum])
            

        else:
            pass
            
        

def usage():
    print('Usage: {} <path to trace> <working directory>'.format(sys.argv[0]))
    print('    or {} <path to trace> <working directory> --show-sysfiles'.format(sys.argv[0]))
    sys.exit(1)

if __name__ == '__main__':
    if len(sys.argv) != 3 and len(sys.argv) != 4:
        usage()
    
    # Parse the event trace
    events = parser.parse_file(sys.argv[1])
    
    # Check for events
    if len(events) == 0:
        print('There were no events in the trace file provided.')
        exit(1)
    
    # Create a context to track processes
    cwd = sys.argv[2]
    c = Context(cwd)
    
    for e in events:
        try:
            c.handle_event(e)
        except Exception as ex:
            print('Error while processing {}'.format(e))
            raise(ex)
    
    c.to_graph().render('graph.gv')
