# Linkage
Linkage Designer and Simulator

This is an open source version of my Linkage program. Linkage is built in Visual Studio and requires WIX to create the installation file.

# Build

CMake 2.8.12 or higher is required.

    $ mkdir build
    $ cd build
    $ cmake -G "Visual Studio 12 2013 Win64" ..

Open the generated Linkage.sln.

# Generate the installer

    $ cd build
    $ cpack -C Release -G WIX

# Code Signing
The build proces will attempt to sign the code using a code signing certificate. If you happen to have a certificate with my name on it,
you will actually get signed code. But only my distribution should ever be signed by me so ignore any code signing errors during the 
build process - they should not keep you from building and running the Linkage program.
