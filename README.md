# String Pooler
Given a file containing a newline-separated list of strings, this program will build a string that contains each of the individual strings as substrings. While assembling the final string, shared components of strings will attempt to be used as much as possible. As an example of an input and output:

```
this
is
a
list
of
strings
```
Would produce an output of `listringsthisofa`.