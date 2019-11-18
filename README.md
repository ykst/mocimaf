# mocimaf

Yet another NES emulator written by C.

## GOALS
* Ability to investigate lost technologies of NES era
* Pass the majority of existing test ROMs
* Easy to build

## NON-GOALS
* Providing good playing experience
* Prefer fast emulation over correctness
* Support every ROM

## Build

SDL2 is required.

```
$ make
```

## Run

* Run a `.nes` file

```
$ ./mocimaf $rom_file
```

* Run with console debugger

```
$ ./mocimaf -a $rom_file
```

Currently the debugger just works on good fast terminal (e.g. iTerm2) larger than 200 cols and 48 rows.

## Acknowledgements
[NES研究室](http://hp.vector.co.jp/authors/VA042397/nes/sample.html) provides good first helloworld ROM and bootstrap information for Japanese.
Other test ROMs are redistribution of those found at invaluable [Nesdev_Wiki](https://wiki.nesdev.com/w/index.php/Emulator_tests).
This project is an evidence that you can build a decent NES emulator from scrath by relying solely on the wiki.

## License
MIT

## Author
ykst \<ykstyhsk@gmail.com\>
