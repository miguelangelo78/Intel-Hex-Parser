# Intel-Hex-Parser
This is a small utility program for parsing Intel Hex files and to dump the data into a binary file.

# How to Compile

Simply run **`build.bat`** on Windows, or just run **`make -f makefile all`** on any other Platform.

# How to Use

- If you only want to see the dump of the program, run the compiled program: **`ihex yourhexfile.hex`**

- If you want a binary file with the actual data to be outputted, just put the -o flag: **`ihex yourhexfile.hex --ob`**

The complete list of options are:
> 1. **`-h`** > Show help on the screen. Example: **`ihex -h`**
> 2. **`--ob`** > Output hex file's data into binary file. Example: **`ihex myfile.hex --ob`**
> 3. **`--oh`** > Output original (or modified with the flags) hex file into a new file called 'a.hex'.  
Example: **`ihex myfile.hex --oh`**
>4. **`--ah`** > Append hex file with the current hex file. Example: **`ihex myfile.hex --ah newfile_to_append.hex`**
>5. **`--ab`** > Append binary file with the current hex file. New records will actually be created on the current hex file just so it can fit the new binary data. Example: **`ihex myfile.hex --ab mybinary_to_append.bin`**
>6. **`-c`** > Convert a binary file into a hex file. Example: **`ihex -c mybin.bin`** By using the -c flag, the program will use the binary as if it was the original hex file that was being passed originally without any flag.

* Remember that you can combine these flags. For example, if you wish to append one hex file and a binary into the original file, and want to get the result exported into a new file, just do:  
**`ihex myfile.hex --ab mybinary.bin --ah anotherhex.hex --oh`**

**Another example**:  
1. **`ihex -c mybinary.bin --oh`** -> Converts from binary to Intel Hex and generates Hex output file  
2. **`ihex a.hex --ob`** -> Converts the previous generated hex file back into a binary file that is equal to 'mybinary.bin'.  

**More examples**:  
1. **`ihex myhex.hex --ob`** -> Convert from hex to binary 'myhex.bin'  
2. **`ihex -c myhex.bin --oh`** -> Convert again the generated binary into the previous hex file.  

* This will create a new file called a.hex which you should rename to whatever you want. The result is a total of 3 files joined together.
* If you want to add more complexity, you can call the program multiple times, each call with different flags, which will allow you to make a complex and large hex file with a lot of information.
