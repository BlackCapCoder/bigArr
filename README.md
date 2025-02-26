# bigArr

The vector type built into your favorite language will first allocate a small fixed-size array.
Whenever you outgrow the old array it allocates a new array that 1.5-2 times as large, then copy everything over.

This is a design for dynamically allocated array that avoids copying.
Like an array it has constant-time access, but the constant is larger than for a plain array.
Unlike a vector it has a fixed size, but the size is so large that you can pretend like it's infinite in practice (2^64).

----

Visual explaination of how indexing works: https://blackcapcoder.github.io/bigArr

The grid is 16x16. Click on any cell to allocate and jump to a new grid inside that cell.
When you get to the final layer clicking on a cell will write data to it instead.

