Web Server in C
===

## Goal and Summary
The purpose of this project was to gain experience working with sockets and binding in the C language, and how to properly use these to connect between to sockets to send messages. The project itself is web server capable of running and serving up webpages from a specified directory to a port on the local machine. I was even able to extend the project to execute PHP scripts as well as long as PHP is installed on the local machine. 

## Usage

To run the web server, simply clone the project and run the following two commands from within the directory in bash:

```bash
$ make all
$ ./server
```
Now, if you navigate to the following URL: http://localhost:10000/ . The server should display the root webpage found in this repository. 