#Advanced Operating Systems course 

Author: Ali Reza Barkhordari

---

###GENERAL INFORMATION

- Folder `SinghalClient`:
	Project root directory of Singhal clients, which is intended to run in virtual machines.
	It should be noted that, Singhal algorithm is completely implemented in this project.

- Folder `vmcc`:
	The name stands for, Virtual Machines Controlling Client. The server, by which clients can
    communicate with each other. This server is actually passive and has nothing to do with
	messages being passed throught it. This server always relays all messages regardless of
	what type they are and for whom they are going.

###RUN DIRECTION

- Project `SinghalClient`:
	This project has been designed very lightweight and use pure C++ without any additional framework.
	Actually this project is fully standalone and it is possible to build it by `g++`. It should be noted
	that this project is using POSIX Threads library. Thus in order to build this project you should
	consider using `-lpthread` flag for `g++` compiler.

- Project `vmcc`:
	This project is also written in C++, however it lies on the Qt framework to run. For instructons needed
	to getting this project ready to run, you may refer to our first written report.
