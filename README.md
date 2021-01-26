# File system control via HTTP [Partially implemented]

This repository is a project for compiling a program to manipulate file system and disk data via HTTP requests.

The program exposes a HTTP server in a specified port (default 8084) to allow for any program (e.g. a Web Browser) create new files, to query folders, save logs, etc, via the familiar HTTP.

It was developed to work on Windows and does not support other operational systems as of now.

## How to use

Download a release from the [releases page](https://github.com/GuilhermeRossato/fs-http-interface/releases) (an executable) and run it. It should open a server listening at port `8084` by default (check standard output), use a browser to access http://localhost:8084/ and use the tool

Check the standard output if anything went wrong, the program should be very verbose.

## Feature examples

### Check if a file exists

http://localhost:8084/file/exists/?path=C:/pagefile.sys

Returns a single digit: `0` if the path is not a file or does not exist and `1` if it exists and is a file.

### Read a file size

http://localhost:8084/file/size/?path=C:\pagefile.sys

Returns the size of a file (digits) in the body of the message, unless it doesnt exists in which case returns 404 error.

### Read a file contents

http://localhost:8084/file/contents/?path=C:\Program Files (x86)\Git\magic.py

Content-Length will be the exact file size. The result will be either:
    - a `404 Not Found when the` file was not found
    - a `500 Internal Server Error` when there's an error reading the file
    - a `200 OK` with the file contents as raw bytes in the HTTP body

Obs: As of now the maximum file size is roughly 1 MB, you can change the `OUTPUT_BUFFER_SIZE` to increase it.

### Check if a directory exists

http://localhost:8084/directory/exists/?path=C:\Program%20Files

Returns a single digit: `0` if the path is not a folder or does not exist and `1` if it exists and is a file.

You can also use [/folder/exists/](http://localhost:8084/folder/exists/?path=C:\Program%20Files).

### Read the folder contents (files and folders)

http://localhost:8084/directory/contents/?path=C:\Program%20Files

Returns files and directories inside the folder separated by a single newline with a trailing newline at the end.

Note that since your browser will probably interpret it as HTML, it will look like a single line in the browser.

You can also use [/folder/contents/](http://localhost:8084/folder/contents/?path=C:\Program%20Files).

If you want to know what is the **type** of each file you can add the `type` parameter:

http://localhost:8084/folder/contents/?type&path=.

The first character of each line is F or D, indicating File or Directory.

```
D .
D ..
D .git
F .gitignore
D build
F compile-windows-loop.bat
F main.obj
F readme.md
D src

```

If you want to know what is the **size** (or length) of each file (in bytes) you can add the `size` parameter:

http://localhost:8084/folder/contents/?size&path=.

The first space in the line separates the byte amount (digits) and the file name. Note that the file name might contain spaces (but not newlines).

```
0 .
0 ..
0 .git
32 .gitignore
0 build
907 compile-windows-loop.bat
34501 main.obj
3843 readme.md
0 src

```

You can also combine **size** and **type** parameters to get both information:

```
D 0 .
D 0 ..
D 0 .git
F 32 .gitignore
D 0 build
F 907 compile-windows-loop.bat
F 34501 main.obj
F 3843 readme.md
D 0 src

```

### Check if there's a file or a directory (or anything) in a path

http://localhost:8084/path/exists/?path=C:\Program%20Files\

Returns a single digit: `0` if the win32 file stat fails or `1` if it succeeds.

## Security

Do not leave this running on your computer unattended. A connected computer with access to your network can read and write all sorts of files.

## Disclaimer

The project is unfinished. I do not make any warranty about the working state of the software included in this repository. I shall not be responsible for any harm the use of the software might cause.
