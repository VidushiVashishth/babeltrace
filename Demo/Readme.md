After installing babeltrace using the available instructions, cd into this directory and run the following command on the terminal:

"sudo babeltrace run --component=A:source.text.dmesg --params='no-extract-timestamp=no' --key=path --value ./rtemtrace --component=B:sink.ctf.fs --key=path --value ./ --connect=A:B"

This will create a ctf trace directory in the current directory. This new directory can be accessed using the following set of commands:

sudo chmod go+x rtemtrace-1
cd rtemtrace-1
sudo ls -alZ

These commands will show the generated metadata stream and a ctf trace stream.

The program.c file is a c program to call the above babeltrace conversion command which will take as input the path of the rtems trace file to be converted. This is currently 

rtemtrace is an instance of the rtems trace output generated when running the fileio sample testcase. I have saved it to a file.  



