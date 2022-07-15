import subprocess
import sys
import os
import platform
import configparser as cp

subprocess.call(["g++", "main.cpp"])
tmp = subprocess.call("a.out")
print(tmp)
# if len(sys.argv) != 3:
#     print("Incorrect number of arguments!")
#     exit(1)
    
#     try:
#         runsAmount = int(sys.argv[1])
#     except:
#         print("Incorrect number of runs!")
#         exit(1)

#     try:
#         config_file = sys.argv[2]
#     except:
#         if os.path.isfile("configuration.cfg"):
#             config_file = "configuration.cfg"
#         else:
#             print("Incorrect name of cfg file!")
#             exit(2)
            
# PROGRAM_PATH = os.path.join("cmake-build-debug", "startFile.exe")
# minTime = float("inf")
# runsAmount = 3
# config_file = "configuration.cfg"
# args = [PROGRAM_PATH, config_file]
# for i in range(runsAmount):
#     process = subprocess.Popen(args, shell = True, stdout=subprocess.PIPE)
#     process.wait()
#     (output, error) = process.communicate()
#     print(output)

#     # total_time = int(str(output)[2:-3].split('\\n')[0].split('=')[1])
#     exit_code = process.wait()