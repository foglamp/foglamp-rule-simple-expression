===========================================
FogLAMP SimpleExpression notification rule plugin
===========================================

An expression based notification rule plugin:

The configuration items  are:

  - "asset" is the asset name for which notifications will be generated.
  - "expression" is the expression to evaluate in order to send notifications

Note this plugin is designed to work with a single asset name.

Example:

.. code-block:: console

    {
	"asset": {
		"description": "The asset name for which notifications will be generated.",
		"name": "modbus"
	},
	"expression": {
		"description": "The expression to evaluate",
		"name": "Expression",
		"type": "string",
		"value": "humidity > 50"
	}
    }
  
Expression is composed of datapoint values within given asset name.
There is no need to provide datapoint names because names and values
are dynamically added when "plugin_eval" is called.
If the value of expression is true, then the notification is sent.

Expression may contain any of the following...

- Mathematical operators (+, -, *, /, %, ^)

- Functions (min, max, avg, sum, abs, ceil, floor, round, roundn, exp, log, log10, logn, pow, root, sqrt, clamp, inrange, swap)

- Trigonometry (sin, cos, tan, acos, asin, atan, atan2, cosh, cot, csc, sec, sinh, tanh, d2r, r2d, d2g, g2d, hyp)

- Equalities & Inequalities (=, ==, <>, !=, <, <=, >, >=)

- Logical operators (and, nand, nor, not, or, xor, xnor, mand, mor)

The plugin uses the C++ Mathematical Expression Toolkit Library
by Arash Partow and is used under the MIT licence granted on that toolkit.

Build
-----
To build FogLAMP "SimpleExpression" notification rule C++ plugin,
in addition fo FogLAMP source code, the Notification server C++
header files are required (no .cpp files or libraries needed so far)

The path with Notification server C++ header files cab be specified only via
NOTIFICATION_SERVICE_INCLUDE_DIRS environment variable.

Example:

.. code-block:: console

  $ export NOTIFICATION_SERVICE_INCLUDE_DIRS=/home/ubuntu/source/foglamp-service-notification/C/services/common/include

.. code-block:: console

  $ mkdir build
  $ cd build
  $ cmake ..
  $ make

- By default the FogLAMP develop package header files and libraries
  are expected to be located in /usr/include/foglamp and /usr/lib/foglamp
- If **FOGLAMP_ROOT** env var is set and no -D options are set,
  the header files and libraries paths are pulled from the ones under the
  FOGLAMP_ROOT directory.
  Please note that you must first run 'make' in the FOGLAMP_ROOT directory.

You may also pass one or more of the following options to cmake to override 
this default behaviour:

- **FOGLAMP_SRC** sets the path of a FogLAMP source tree
- **FOGLAMP_INCLUDE** sets the path to FogLAMP header files
- **FOGLAMP_LIB sets** the path to FogLAMP libraries
- **FOGLAMP_INSTALL** sets the installation path of Random plugin

NOTE:
 - The **FOGLAMP_INCLUDE** option should point to a location where all the FogLAMP 
   header files have been installed in a single directory.
 - The **FOGLAMP_LIB** option should point to a location where all the FogLAMP
   libraries have been installed in a single directory.
 - 'make install' target is defined only when **FOGLAMP_INSTALL** is set

Examples:

- no options

  $ cmake ..

- no options and FOGLAMP_ROOT set

  $ export FOGLAMP_ROOT=/some_foglamp_setup

  $ cmake ..

- set FOGLAMP_SRC

  $ cmake -DFOGLAMP_SRC=/home/source/develop/FogLAMP  ..

- set FOGLAMP_INCLUDE

  $ cmake -DFOGLAMP_INCLUDE=/dev-package/include ..
- set FOGLAMP_LIB

  $ cmake -DFOGLAMP_LIB=/home/dev/package/lib ..
- set FOGLAMP_INSTALL

  $ cmake -DFOGLAMP_INSTALL=/home/source/develop/FogLAMP ..

  $ cmake -DFOGLAMP_INSTALL=/usr/local/foglamp ..

**********************************************
Packaging for 'SimpleExpression notification' plugin 
**********************************************

This repo contains the scripts used to create a foglamp-rule-simple-expression Debian package.

The make_deb script
===================

Run the make_deb command after compiling the plugin:

.. code-block:: console

  $ ./make_deb help
  make_deb {x86|arm} [help|clean|cleanall]
  This script is used to create the Debian package of FoglAMP C++ 'SimpleExpression notification' plugin
  Arguments:
   help     - Display this help text
   x86      - Build an x86_64 package
   arm      - Build an armv7l package
   clean    - Remove all the old versions saved in format .XXXX
   cleanall - Remove all the versions, including the last one
  $

Building a Package
==================

Finally, run the ``make_deb`` command:

.. code-block:: console

   $ ./make_deb
   The package root directory is                : /home/ubuntu/source/foglamp-rule-simple-expression
   The FogLAMP required version                 : >=1.5
   The Service notification required version    : >=1.5.2
   The package will be built in                 : /home/ubuntu/source/foglamp-rule-simple-expression/packages/build
   The architecture is set as                   : x86_64
   The package name is                          : foglamp-rule-simple-expression-1.5.2-x86_64

   ....

   Populating the package and updating version file...Done.
   Building the new package...
   dpkg-deb: building package 'foglamp-rule-simple-expression' in 'foglamp-rule-simple-expression-1.5.2-x86_64.deb'.
   Building Complete.
   $

Cleaning the Package Folder
===========================

Use the ``clean`` option to remove all the old packages and the files used to make the package.

Use the ``cleanall`` option to remove all the packages and the files used to make the package.
