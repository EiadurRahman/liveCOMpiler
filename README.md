liveCOMpiler watches a .c file for changes using MD5 hash polling. The moment you save, it recompiles with gcc, clears the terminal, and runs the output, exactly as if you'd done it by hand. Compile errors print raw and immediately. 
Quit with Ctrl+C. No dependencies beyond Python 3 and gcc.

how to run? 

```bash
# run python3 version :
python3 liveCOMpiler.py <file name>


# run c version

# compile first
gcc liveCOMpiler.c -o liveCOMpiler 

# run the bin file
./liveCOMpiler <file name>

# exit with ctrl + c
```
