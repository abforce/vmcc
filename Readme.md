Singhal distributed systems mutual exclusion implementation
-----------------------------------------------------------

Theory: https://www.google.com/search?q=singhal+mutual+exclusion+algorithm

---


- Directory `SinghalClient`:
	Project root directory of Singhal clients, which is intended to run in virtual machines.
	It should be noted that, Singhal algorithm is completely implemented in this project.

- Directory `vmcc`:
	The name stands for, Virtual Machines Controlling Client. The server, by which clients can
    communicate with each other. This server is actually passive and has nothing to do with
	messages being passed throught. This server always relays all messages regardless of
	what type they are and for whom they are going.

Build instructions:

- Project `SinghalClient`:
	This project has been designed very lightweight and use pure C++ without any additional framework/library.
	Actually this project is fully standalone and it is possible to build it by `g++`. It should be noted
	that this project is using POSIX Threads library. Thus in order to build this project you should
	consider using `-lpthread` flag for `g++`.

- Project `vmcc`:
	This project is written in C++ using Qt framework. No matter what version of Qt you have. It will be linked against VMware VIX using `libvix`, so you will need a `-lvix` flag. See `.pro` file.
