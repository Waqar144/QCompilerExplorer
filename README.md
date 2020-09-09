# QCompilerExplorer

Just made this as a fun project. It includes a local mini compiler explorer, which will show you the assembly generated for your C++ code using g++ / clang. It also includes a frontend for the [Compiler Explorer](https://godbolt.org). You can use either one of them.

You can open a folder(project folder) and browse your code and it's generated asm side by side (in development).

## Screenshot

![screenshot](screenshots/screenshot.png)

## Building QCompilerExplorer

```shell
git clone https://github.com/Waqar144/QCompilerExplorer.git
cd QCompilerExplorer
git submodule update --init
```

Then download [Qt Creator](http://www.qt.io/download-open-source), open the
project file `qcompilerexplorer.pro` and click on *Build / Run*.

Or you can build it directly in your terminal:

```shell
qmake qcompilerexplorer.pro
make
```

## Missing stuff

- Linking to libraries is not supported for the godbolt frontend. You can link to libraries if you use the local asm generator. (I may remove the godbolt frontend altogether since it doesn't seem very useful)
- Asm parsing / cleaning is not the best
- Highlighter struggles at times
- Has not been tested on windows at all
- No binary execution / output of program

## LICENSE

MIT
