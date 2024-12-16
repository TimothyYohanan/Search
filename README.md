This is a simple desktop application for testing and optimizing sqlite3 workflows.

I think I'll make 2 or 3 more commits to it over the next month or two, and then it will be 'done.'

This codebase is made to be simple and portable. The way that I'm using it is to work out sqlite3 strategies on my workstation for a mobile app that I'm working on.

Once I'm happy with how the database works, I can simply cut and paste the following files into the other codebase:
  src/DDL/**
  src/database
  src/search
  extern/sqlite3

Anyone is welcome to use this codebase for anything. I don't expect anything in return.
Look for #defines - there's conditional compilation statements for everything that prints to the console.
Read everything in the notes directory (there's not much there).

I've realized some interesting things while working on this simple project. I'm thinking about writing a short paper on it.
