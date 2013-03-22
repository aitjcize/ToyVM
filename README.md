# Toy Virtual Machine
Virtual machine for executing [Toy ISA](http://introcs.cs.princeton.edu/xtoy/) created by Introduction to CS, Princeton University.

## Features
* GDB-like debug interface
* Allow setting breakpoint

## Usage
    Usage: toyvm [-d] [-v] ToyFile [InputFile]
           toyvm [--version| -h| --help]
    
    NORMAL MODE
        -d            Enter Debug Mode
        -v            Verbose mode
        ToyFile       *.toy file you want to run.
        InputFile     This is optional, using InputFile instead of manually input.
        -h, --help    Show this help list.
        --version     Show version.
    
    DEBUG MODE
        run, r        Run program
        step, s       Run program in step mode, each line is shown before executing.
    
        next, n       Execute next program line (after stopping).
        continue, c   Continue  running your program (after stopping, e.g. at a
                      break point).
        break [LINE]  Set a breakpoint, program will pause at break point.
        info          Show breakpoint information.
        delete [NUM]  Delete a breakpoint, NUM can be found by the `info' command.
        reg           Show register data.
        list [FMT]    List memory file. FMT can be a line or a range. If a line is
                      entered, list will show 13 line around it. If a ragne like
                      A..B is entered, list will show lines from A to B.
        disasm [FMT]  Disassemble the lines specified by FMT. FMT is same as the
                      FMT in `list'.
        verbose, v    Verbose mode, every instruction is shown before executing.
        noverbose, nv Disable verbose mode.
        quit, q       Quit toyvm debuger.

## Bugs
Please report bugs to Aitjcize <aitjcize@gmail.com>

## Contributing
1. Fork it
2. Create your feature branch (git checkout -b my-new-feature)
3. Commit your changes (git commit -am 'Add some feature')
4. Push to the branch (git push origin my-new-feature)
5. Create new Pull Request
