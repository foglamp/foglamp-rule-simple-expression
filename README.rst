===========================================
Fledge SimpleExpression notification rule plugin
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
To build Fledge "SimpleExpression" notification rule C++ plugin,
in addition fo Fledge source code, the Notification server C++
header files are required (no .cpp files or libraries needed so far)

The path with Notification server C++ header files cab be specified only via
NOTIFICATION_SERVICE_INCLUDE_DIRS environment variable.

Example:

.. code-block:: console

  $ export NOTIFICATION_SERVICE_INCLUDE_DIRS=/home/ubuntu/source/fledge-service-notification/C/services/common/include

.. code-block:: console

  $ mkdir build
  $ cd build
  $ cmake ..
  $ make

- By default the Fledge develop package header files and libraries
  are expected to be located in /usr/include/fledge and /usr/lib/fledge
- If **FLEDGE_ROOT** env var is set and no -D options are set,
  the header files and libraries paths are pulled from the ones under the
  FLEDGE_ROOT directory.
  Please note that you must first run 'make' in the FLEDGE_ROOT directory.

You may also pass one or more of the following options to cmake to override 
this default behaviour:

- **FLEDGE_SRC** sets the path of a Fledge source tree
- **FLEDGE_INCLUDE** sets the path to Fledge header files
- **FLEDGE_LIB sets** the path to Fledge libraries
- **FLEDGE_INSTALL** sets the installation path of Random plugin

NOTE:
 - The **FLEDGE_INCLUDE** option should point to a location where all the Fledge 
   header files have been installed in a single directory.
 - The **FLEDGE_LIB** option should point to a location where all the Fledge
   libraries have been installed in a single directory.
 - 'make install' target is defined only when **FLEDGE_INSTALL** is set

Examples:

- no options

  $ cmake ..

- no options and FLEDGE_ROOT set

  $ export FLEDGE_ROOT=/some_fledge_setup

  $ cmake ..

- set FLEDGE_SRC

  $ cmake -DFLEDGE_SRC=/home/source/develop/Fledge  ..

- set FLEDGE_INCLUDE

  $ cmake -DFLEDGE_INCLUDE=/dev-package/include ..
- set FLEDGE_LIB

  $ cmake -DFLEDGE_LIB=/home/dev/package/lib ..
- set FLEDGE_INSTALL

  $ cmake -DFLEDGE_INSTALL=/home/source/develop/Fledge ..

  $ cmake -DFLEDGE_INSTALL=/usr/local/fledge ..
