## Building Munt components using the FreeBSD ports collection
For cross-platform components of the Munt project, such as _mt32emu_ library, _mt32emu-qt_
application and _mt32emu-smf2wav_ command-line tool, templates of FreeBSD port Makefiles are
provided in the related subdirectories, which can be used to generate the ports.

For creating the ports and building them, the FreeBSD ports collection must be installed. A simple
command `cmake -P make_ports.cmake` downloads the source tarballs from the GitHub repository,
configures the templates and create additional files as necessary to build valid ports.

In the process, all three components are actually built, and hence it makes sense to install
them in the system along the way. This can be achieved by running command
`cmake -DINSTALL_PORTS=ON -P make_ports.cmake` instead. Note however, the _mt32emu_ library has
to be installed in the system anyway, since it is required for building the other two components.

It's also easy to create the binary packages, if the ports got installed. Just running
`pkg create -x mt32emu` will build the packages in the current directory without necessity to build
the ports once again.

See  the script `make_ports.cmake` for additional variables that it supports.
