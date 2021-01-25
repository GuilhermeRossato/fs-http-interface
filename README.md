# File system control via HTTP [Unfinished]

This repository is a project for compiling a program to manipulate file system and disk data via HTTP requests.

The program exposes a HTTP server in a specified port (default 8084) to allow for any program (e.g. a Web Browser) create new files, to query folders, save logs, etc, via the familiar HTTP.

It was developed to work on Windows and does not support other operational systems as of now.

## How to use

Download a release from the [releases page](https://github.com/GuilhermeRossato/fs-http-interface/releases) (an executable) and run it. It should open a server listening at port `8084` by default (check standard output), use a browser to access http://localhost:8084/ and use the tool

Check the standard output if anything went wrong, the program should be very verbose.

## Feature examples

None as of now.

## Security

Do not leave this running on your computer unattended. A connected computer with access to your network can do all sorts of evils.

## Disclaimer

The project is unfinished, I do not make any warranty about the working state of the software included in this repository. I also shall not be responsible for any harm the use of the software might cause.
