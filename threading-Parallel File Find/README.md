1 Introduction

The goal of this assignment was to gain experience with threads and filesystem system calls. In this
assignment, I created a program that searches a directory tree for files whose name matches
some search term. The program receives a directory D and a search term T, and finds every file
in D’s directory tree whose name contains T. The program parallelizes its work using threads.
Specifically, individual directories are searched by different threads.

2 Command line arguments:

• argv[1]: search root directory (search for files within this directory and its subdirectories).
• argv[2]: search term (search for file names that include the search term).
• argv[3]: number of searching threads to be used for the search (assume a valid integer greater than 0)
