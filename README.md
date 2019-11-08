# tsh shell

This is a simple Unix shell program.

About
-------
- This shell utilizes forking and signaling to do basic tasks of a unix shell
- This shell also has the capability of running some programs, which are attached

Usage
--------
Compile the program like so:
```bash
$ make
```
This will compile the tsh program and all of it's dependencies, as well as, the example programs that can be run in the shell

Here is an example of output with the tsh program

	tsh> ./bogus
	./bogus: Command not found.
	tsh> ./helloworld
	hello world
	tsh> ./myspin 10
	Job (9721) terminated by signal 2
	tsh> ./myspin 3 &
	[1] (9723) ./myspin 3 &
	tsh> ./myspin 4 &
	[2] (9725) ./myspin 4 &
	tsh> jobs
	[1] (9723) Running ./myspin 3 & 
	[2] (9725) Running ./myspin 4 & 
	tsh> fg %1
	Job [1] (9723) stopped by signal 20 
	tsh> jobs
	[1]  (9723) Stopped ./myspin 3 &
	[2]  (9725) Running ./myspin 4 &
	tsh> bg %3
	%3: No such job
	tsh> bg %1
	[1] (9723) ./myspin 3 &
	tsh> jobs
	[1] (9723) Running   ./myspin 3 &
	[2] (9725) Running   ./myspin 4 &
	tsh> fg %1
	tsh> quit


To exit out of the shell type quit or exit.

Finally to remove the excess files type:

	$ make clean