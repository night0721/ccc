# ccc
Soft fork of [fff](https://github.com/bogdan-the-great/fff) in C aiming for size and speed with no dependency, hackable with patches and configurable via `config.h`

The fact that it is written in C makes it more versatile and rapid, enabling us to add features that were previously ruled out due to time complexity.

# Features
Consider this project incomplete and WIP!

| Feature of fff                 | Ported | Dropped |
|--------------------------------|:------:|:-------:|
| Standard movement              |   X    |         |
| Advanced movement (jumps)      |   X    |         |
| File details                   |   X    |         |
| File icons!                    |   X    |         |
| Searching for files            |        |         |
| Sorting                        |   X    |         |
| Marking and marking operations |        |         |
| Image previews                 |   X    |         |
| Help                           |   X    |         |
| History                        |        |         |
| Bookmarks                      |        |         |
| Bulk rename                    |        |         |

## Features added that are not in fff:
- File preview

# Usage
```sh
ccc -p dir # File picker
ccc dir
```

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

g: go to top
G: go to bottom

ctrl+u: jump up
ctrl+d: jump down

t: go to trash dir
~: go to home dir
-: go to previous dir
z: refresh current dir
:: go to a directory by typing

.: toggle hidden files
A: show directory disk usage/block size
i: toggle file details
u: sort files
x: view file/dir attributes
e: show history
y: copy filename to clipboard
!: open shell in current dir

f: new file
n: new dir
r: rename
X: toggle executable

space: mark file
a: mark all files in directory
d: trash

?: show help
q: exit with last dir written to file
ctrl+c exit without writing last dir

TO BE DONE:
o: open file with
O: open file with a GUI program detached from file manager

/: search

c: copy
m: move
s: symbolic link
b: bulk rename

p: execute paste/move/delete/bulk_rename

[1-9]: favourites/bookmarks (see customization)
```

# Dependencies
- Any [Nerd Font](https://www.nerdfonts.com/) for file icons (optional, but turned on by default)
- libsixel (Optional)

# Building
You will need to run these with elevated privilages.
```
$ make 
# make install
```

# Customizing
## CD on Exit for POSIX Shell
```sh
# Add this to your .bashrc, .zshrc or equivalent.
# Run 'ccc' with 'c' or whatever you decide to name the function.
c() {
    ccc "$@"
    cd "$(cat "${XDG_CACHE_HOME:=${HOME}/.cache}/ccc/.ccc_d")"
}
```
## Using `ccc` in neovim as a file picker
See [ccc.nvim](https://github.com/night0721/ccc.nvim)

# Contributions
Contributions are welcomed, feel free to open a pull request.

# License
This project is licensed under the GNU Public License v3.0. See [LICENSE](https://github.com/night0721/ccc/blob/master/LICENSE) for more information.
