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
| File icons!                    |   X    |         |
| Searching for files            |        |         |
| Sorting                        |        |         |
| Marking and marking operations |        |         |
| Other operations on files      |        |         |
| Image previews                 |        |         |
| Help                           |   X    |         |
| History                        |        |         |
| Bookmarks                      |        |         |
| Bulk rename                    |        |         |

#### Features added that are not in [fff](https://github.com/piotr-marendowski/fff):

- File preview (without highlighting)

## Installation

### Dependencies

- gcc
- ncursesw
- make
- pkg-config
- Any [Nerd Font](https://www.nerdfonts.com/) for file icons (optional, but turned on by default)

## Building and Installing

You will need to run these with elevated privilages.

```
$ git clone https://github.com/piotr-marendowski/ccc
$ make 
# make install
```

### CD on Exit
#### POSIX Shell
```sh
# Add this to your .bashrc, .zshrc or equivalent.
# Run 'ccc' with 'c' or whatever you decide to name the function.
c() {
    ccc "$@"
    cd "$(cat "${XDG_CACHE_HOME:=${HOME}/.cache}/ccc/.ccc_d")"
}
```

## Usage
```
h: go to parent dir
j: scroll down
k: scroll up
l: go to child dir

left: go to parent dir
down: scroll down
up: scroll up
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
X: toggle executable

A: show directory disk usage/block size

f: new file

space: mark file
a: mark all files in directory
d: trash

?: show help
q: exit with last dir written to file
ctrl+c exit without writing last dir

TO BE DONE:
o: open file with
O: open file with a GUI program detached from file manager

:: go to a directory by typing.

/: search
!: open shell in current dir

u: sort files
x: view file/dir attributes
e: show history
y: copy filename to clipboard

n: new dir
r: rename

c: copy
m: move
s: symbolic link
b: bulk rename

p: execute paste/move/delete/bulk_rename

[1-9]: favourites/bookmarks (see customization)
```

### Using `ccc` in neovim as a file picker

See [ccc.nvim](https://github.com/night0721/ccc.nvim)

### License

This project has GNU GPL v.3 license.
