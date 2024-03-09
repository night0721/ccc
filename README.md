# ccc

`ccc` is a rewrite of [fff](https://github.com/piotr-marendowski/fff) in C aiming for [sucklessness](https://suckless.org/philosophy/) and speed.

The fact that it is written in C makes it more versatile and rapid, enabling us to add features that were previously ruled out due to time complexity. You may call it a `soft fork`.


## Features

Consider this project incomplete and WIP!

| Feature | Ported | Dropped | Added |
|---------|:------:|:-------:|:-----:|
| Standard movement | X | | |
| Advanced movement (jumps) | | | |
| Searching | | | |
| File preview | | | X |
| Sorting | | | |
| Marking and marking operations | | | |
| Other operations on files | | | |
| File details | | | |
| Image previews | | | |
| Help | | | |
| History | | | |
| Bookmarks | | | |
| Bulk rename | | | |
| Workspaces | | | |
| Workspaces | | | |

## Installation

### Dependencies

- gcc
- make
- pkg-config
- ncurses

### Building

You may need to run these with elevated privilages.

```
make install
```
