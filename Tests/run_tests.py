"""
This file is used for testing a C/C++ project programatically.

To begin create a file to contain the tests. by default './tests'
In this folder you can create your tests.


To create a test create a folder. This folder can be named anything.
Within this folder two files are needed. A test.json file and a main.c

The test.json file should contain a json file in the format:
{
    name: "test-name",

    "includedirs" : [list of inlcude directories],
    "linkdirs" : [list of library directories],
    "link" : [list of libraries to link to],

    "defines" : [list of preprocessor definitions]
}

The main.c file should contain a standard C entry point of
int main(int argc, char* argv[]) {}

The way the test runner evaluates the result of a test is by using it's return code
A success is marked as a program that exited with a ret code of 0
A faliure is when the program returns any other return code.


If a test requires some extra input like a file then it can be placed into a "resources"
folder. This is a folder, named resources, that is within the test directory. This
folder is excluded from being tested and can contain any files and even by used
as an include directory for test-global header files.


When a program is executed it is executed from the python script's working directory.
Because of this the python file should only be called from it's directory and all relative
paths should be created from that directory.  
"""

import argparse
import sys
from pathlib import Path

if __name__ != "__main__":
    sys.exit()

parser = argparse.ArgumentParser()
parser.add_argument("-t", "--threads", type=int, default=4, help="Number of threads to run tests on. Default is 4")
parser.add_argument("-i", "--input", type=Path, default="./tests", help="Directory containing tests. Default is ./tests")
parser.add_argument("-c", "--clean", action="store_true", default=False, help="Clean tests and exit. This will remove all obj folders and executables")
args = parser.parse_args()


#Imports
import json
import subprocess
import shutil
import time
import threading

#Defines
COMPILER_DIRECTORY = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build"
TEST_DIR = Path("./tests")
JSON_SCHEMA = {'name' : str, 'linkdirs' : list, 'links' : list, 'includedirs' : list, 'defines' : list}

INCLUDE_DIRECTORIES = []
LIBRARY_DIRECTORIES = []

DEFAULT_COMPILER_OPTIONS = ["/Zi", "/nologo"]
DEFAULT_LINKER_OPTIONS = ["/MACHINE:x64", "/SUBSYSTEM:CONSOLE", "/nologo"]

TEST_STOPPED = "STOPPED"
TEST_LOADING = "LOADING"
TEST_COMPILING = "COMPILING"
TEST_RUNNING = "RUNNING"

TEST_ERROR = "ERROR"
TEST_FAIL = "FAIL"
TEST_OK = "OK"

class Test:
    def __init__(self, directory : Path) -> None:
        self.dir = directory

        self.is_valid = False 

        self.json_data = None

        self.name = "Unknown"

        self.log_file = None
        self.status = TEST_STOPPED

        self.start_time = 0
        self.execution_time = 0

    def run(self, compileOptions, linkOptions, include_dirs, link_dirs):
        if self.is_valid == False:
            self.status = TEST_ERROR
            return
        
        self.log_file.write("COMPILING TEST...\n")
        objDir = self.dir / "obj"
        objDir.mkdir(exist_ok=True)

        self.log_file.write("COMPILING TEST...\n")

        self.status = TEST_COMPILING
        self.start_time = time.time()

        for d in include_dirs:
            compileOptions.append('/I \"' + d + "\"")
            
        for d in self.json_data['includedirs']:
            compileOptions.append('/I \"' + d + "\"")

        for v in self.json_data['defines']:
            compileOptions.append('/D \"' + v + "\"")

        for d in self.json_data['linkdirs']:
            linkOptions.append("/LIBPATH:\"" + d + "\"")

        for d in link_dirs:
            linkOptions.append("/LIBPATH:\"" + d + "\"")

        for d in self.json_data['links']:
            linkOptions.append(d)
        
        compileCommand = ' '.join(compileOptions)
        compileCommand += f" {self.dir}\\main.c"

        linkCommand = ' '.join(linkOptions)

        command = compileCommand + " /link " + linkCommand

        self.log_file.write("Executing CL command -> cl " + command + "\n")

        process = subprocess.Popen(f"{COMPILER_DIRECTORY}\\vcvarsall.bat x64 && cl {command}", stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = process.communicate()
        
        self.log_file.write("\n============ START OF CL OUTPUT ============\n")
        self.log_file.write(out.decode())
        self.log_file.write("============ END OF CL OUTPUT ============\n")

        #Check if we errored
        if process.returncode != 0:
            self.status = TEST_ERROR
            self.log_file.write("DETECTED ERROR WHILE COMPILING")
            self.log_file.write("\nTEST RESULT -> ERROR")
            self.log_file.close()
            return

        self.status = TEST_RUNNING
        self.log_file.write("\nRUNNING EXECUTABLE...\n")

        process = subprocess.Popen(f"{self.dir}\\main.exe", stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = process.communicate()

        if process.returncode != 0:
            self.status = TEST_FAIL
            self.log_file.write(f"TEST FAILED. RETURN CODE -> {process.returncode}\nTEST RESULT -> FAIL\n")
            self.log_file.close()
            return

        self.log_file.write("TEST RESULT -> OK\n")
        self.status = TEST_OK

        self.log_file.close()

    def load_test(self) -> str:
        self.log_file = open(self.dir / "log.log", 'w')
        self.log_file.write("========== BEGINNING OF LOG FILE ==========\n")
        self.log_file.write("CLEANING OLD TEST...")
        self.clean_test()
        self.log_file.write("\nLOADING TEST...")
        self.status = TEST_LOADING

        #Find JSON file
        files = list(self.dir.glob('test.json'))
        if len(files) != 1:
            self.log_file.write("MISSING 'test.json' FILE\n")
            self.log_file.close()
            return "Missing JSON file"

        #Load json
        try:
            with open(str(files[0]), 'r') as f:
                self.json_data = json.load(f)
        except:
            self.log_file.write("INVALID JSON FILE\n")
            self.log_file.close()
            return "Invalid JSON file"

        #Check JSON against schema
        out = self.__validate_json(self.json_data, JSON_SCHEMA)
        if out != True:
            self.log_file.write(f"JSON FILE MISSING ATTRIBUTES -> {out}\n")
            self.log_file.close()
            return "JSON missing attributes"
        
        self.name = self.json_data.get('name')

        self.is_valid = True
        self.log_file.write("\nLOADED TEST\n\n")
        return "OK"

    def clean_test(self):
        #Delete any executables
        for file in self.dir.glob("*.exe"):
            file.unlink()

        if self.log_file == None:
            for file in self.dir.glob("*.log"):
                file.unlink()

        obj_file = self.dir / "obj"
        if not obj_file.exists():
            return
        
        shutil.rmtree(obj_file)


    def __validate_json(self, json, types) -> str:
        for key in types.keys():
            
            if json.get(key) == None:
                return False
            
            if isinstance(types.get(key), dict):
                if isinstance(json.get(key), type(types.get(key))) == False:
                    return key
                
                ret = self.__validate_json(json.get(key), types.get(key))
                
                if not ret:
                    return key
            else:
                if isinstance(json.get(key), types.get(key)) == False:
                    return key

        return True
    

class TestRunner:
    def __init__(self, tests : list[Test]):
        self.tests = tests

        self.threads = []

    def run_all(self, threads = 1):
        #Place all tests into the queue
        taskQueue = list(self.tests)

        #Make cursor invisible
        sys.stdout.write("\033[?25l")
        self.show_header()

        while len(taskQueue) != 0 or len(self.threads) != 0:
            if len(self.threads) < threads and len(taskQueue) > 0:
                task = taskQueue.pop(0)
                t = threading.Thread(target=self.__task_thread, args=[task], daemon=True)
                t.start()
                self.threads.append(t)

            for thread in self.threads:
                if not thread.is_alive():
                    self.threads.remove(thread)

            self.show_tasks()

        self.show_tasks(True)
        sys.stdout.write("\033[?25h")

    def clean_all(self):
        for test in self.tests:
            test.clean_test()

    def show_header(self):
        sys.stdout.write("\033[0;36m========== Running Tests ==========\n")

    def show_tasks(self, leave = False):
        #Print out tasks
        for test in self.tests:
            #Get test color
            test_color = ""
            tD = 0
            if test.status == TEST_STOPPED:
                test_color = "\033[0;37m"
            elif test.status == TEST_LOADING or test.status == TEST_COMPILING or test.status == TEST_RUNNING:
                test_color = "\033[0;35m"
                tD = time.time() - test.start_time
                test.execution_time = tD
            elif test.status == TEST_ERROR:
                test_color = "\033[0;33m"
                tD = test.execution_time
            elif test.status == TEST_FAIL:
                test_color = "\033[0;31m"
                tD = test.execution_time
            elif test.status == TEST_OK:
                test_color = "\033[0;32m"
                tD = test.execution_time


            tD = str(round(tD, 1))

            if test.start_time == 0:
                tD = '0.0'

            sys.stdout.write(test_color + test.name + " [" + test.status + "] - " + tD + "s" + "                                           \n")

            sys.stdout.flush()

        if not leave:
            sys.stdout.write(f"\033[{len(self.tests)}F")

        else:
            sys.stdout.write("\033[0;39m")

    def __task_thread(self, test : Test):
        try:
            test.load_test()
            cOptions = DEFAULT_COMPILER_OPTIONS + [f"/Fo\".\\{test.dir}\\obj\\main.obj\"", f"/Fd\".\\{test.dir}\\obj\\main.pdb\""]
            lOptions = DEFAULT_LINKER_OPTIONS + [f"/OUT:\".\\{test.dir}\\main.exe\"", f"/PDB:\".\\{test.dir}\\obj\\main.pdb\"", f"/ILK:\".\\{test.dir}\\obj\\main.ilk\""]
            test.run(cOptions, lOptions, INCLUDE_DIRECTORIES, LIBRARY_DIRECTORIES)
        except Exception as e:
            test.status = TEST_ERROR
            test.log_file.write(str(e))
            test.log_file.close()
        


#Path functions
def get_tests(path):
    return [x for x in path.glob('*') if x.is_dir() and x.name != 'resources']

test_dirs = get_tests(args.input)
tests : list[Test] = [None] * len(test_dirs) 
for idx, d in enumerate(test_dirs):
    tests[idx] = Test(d)

runner = TestRunner(tests)

if args.clean:
    runner.clean_all()
    sys.exit()

runner.run_all(args.threads)
