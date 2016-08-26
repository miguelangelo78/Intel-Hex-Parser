# Intel-Hex-Parser
This is a small utility program for parsing Intel Hex files and to dump the data into a binary file.

# How to Compile

Simply run **`build.bat`** on Windows, or just simply run **`make -f makefile all`**

# How to Use

- If you only want to see the dump of the program, run the compiled program: **`ihex yourhexfile.hex`**

- If you want a binary file with the actual data to be outputted, just put the -o flag: **`ihex yourhexfile.hex -o`**

# Screenshot
The following screenshot shows what kind of data is dumped with this program:
![Screen](http://image.prntscr.com/image/37468a89d8d94a548917e18a5aa5c0d3.png)
