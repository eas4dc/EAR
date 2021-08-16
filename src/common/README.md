### string_enhanced

###### How to set a print table?

1) Initialize the function.

```
int tprintf_init(int fd, int mode, char *format);
```
- The output file descriptor (fd) can be yours or:
```
#define fdout	STDOUT_FILENO
#define fderr	STDERR_FILENO
```
- The mode:
```
#define STR_MODE_DEF		0 # Normal
#define STR_MODE_COL		1 # With colors
#define STR_MODE_CSV		2 # CSV style
```
- The format is the number of characters per column. Example:
Example:
```
tprintf_init(fderr, STR_MODE_DEF, "10 20");
```
2) Write your messages.
```
tprintf("%d||%s", number, string);
```
- The triple bar `|||` also print the character `|`
```
0         string zero
1         string one
2         string two
``` 