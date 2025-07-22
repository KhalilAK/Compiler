# Compiler

This is a simple compiler and virtual machine implementation for the PL/0 programming language. It supports basic arithmetic, variables, procedures, control flow (e.g. `if-else-fi`, `while-do-end`), and more.

---

## How to Compile

Use `gcc` to compile the compiler and virtual machine:

Compile
gcc -o compiler compiler.c

Execute
./pl0compiler input.txt

```Change "input.txt" to the correct name of the input file```


```Example Input
const a = 5;
var x, y;
begin
  x := a + 2;
  if x = 7 then
    y := x * 2
  else
    y := x + 3
  fi;
  write y;
end.


