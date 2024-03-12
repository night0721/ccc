# ccc

`ccc` is a rewrite of [fff](https://github.com/piotr-marendowski/fff) in C aiming for usefulness and speed.

The fact that it is written in C makes it more versatile and rapid, enabling us to add features that were previously ruled out due to time complexity. You may call it a `soft fork`.

## Features

- Vim-like key binding
- File Preview

Consider this project incomplete and WIP!

| Feature                        | Ported | Dropped | Added |
|--------------------------------|:------:|:-------:|:-----:|
| Standard movement              |   X    |         |       |
| Advanced movement (jumps)      |   X    |         |       |
| Searching                      |        |         |       |
| File preview                   |        |         |   X   |
| Sorting                        |        |         |       |
| Marking and marking operations |        |         |       |
| Other operations on files      |        |         |       |
| File details                   |   X    |         |       |
| Image previews                 |        |         |       |
| Help                           |        |         |       |
| History                        |        |         |       |
| Bookmarks                      |        |         |       |
| Bulk rename                    |        |         |       |
| Workspaces                     |        |         |       |

### Dependencies

- gcc
- make
- pkg-config
- ncurses

### Building

You will need to run these with elevated privilages.

```sh
$ git clone https://github.com/piotr-marendowski/ccc
$ make 
# make install
```
