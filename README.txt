The program works with any percentage of loss, the program also works with any percentage of corruption.
The command line input is "./p2.exe [loss 0-1.0] [corruption 0-1.0] [reording 0-1.0] [average time between packets] [tracing-level] [randomized?] [bidirectional]"
It is not recommended to do loss and corruption at the same time:
for loss ".\p2.exe 100 0 .2 0 200 0 0 0"
for corruption ".\p2.exe 100 .2 0 0 200 0 0 0"
The corruption command will take some time and print out a lot, that's is why there is no trace for it, but it will work