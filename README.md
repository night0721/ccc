# ccc

`ccc` is a rewrite of [fff](https://github.com/piotr-marendowski/fff) in C aiming for usefulness and speed.

The fact that it is written in C makes it more versatile and rapid, enabling us to add features that were previously ruled out due to time complexity. You may call it a `soft fork`.

## Features

Consider this project incomplete and WIP!

| Feature of fff                 | Ported | Dropped |
|--------------------------------|:------:|:-------:|
| Standard movement              |   X    |         |
| Advanced movement (jumps)      |   X    |         |
| File details                   |   X    |         |
| Searching for files            |        |         |
| Sorting                        |        |         |
| Marking and marking operations |        |         |
| Other operations on files      |        |         |
| Image previews                 |        |         |
| Help                           |        |         |
| History                        |        |         |
| Bookmarks                      |        |         |
| Bulk rename                    |        |         |

#### Features added that are not in [fff](https://github.com/piotr-marendowski/fff):

- File preview (without highlighting)

## Installation

### Dependencies

- gcc
- ncurses
- make
- pkg-config

### Building

You will need to run these with elevated privilages.

```
$ git clone https://github.com/piotr-marendowski/ccc
$ make 
$ sudo make install
```

## Usage

```
h: go to parent dir
j: scroll down
k: scroll up
l: go to child dir

left:  go to parent dir
down:  scroll down
up:    scroll up
right: go to child dir

enter: go to child dir/open file
backspace: go to parent dir

gg: go to top
G: go to bottom

ctrl+u: jump up
ctrl+d: jump down

t: go to trash dir
~: go to home dir
-: go to previous dir
z: refresh current dir

i: toggle file details

A: show directory disk usage/block size
space: mark file
a: mark all files in directory

?: show help
q: exit with last dir written to file
ctrl+c exit without writing last dir
```

## License

This project has GNU GPL v.3 license.
