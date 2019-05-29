===========================================
FogLAMP SimpleExpression notification rule plugin
===========================================

An expression based notification rule plugin:

The "rule_config" property is a JSON object with asset name, 
list of datapoints used in expression and the boolean expression itself:

Example:
    {
		"asset": {
			"description": "The asset name for which notifications will be generated.",
			"name": "modbus"
		},
		"datapoints": [{
			"type": "float",
			"name": "humidity"
		}, {
			"type": "float",
			"name": "temperature"
		}],
		"expression": {
			"description": "The expression to evaluate",
			"name": "Expression",
			"type": "string",
			"value": "if( humidity > 50, 1, 0)"
		}
	}
 
Expression is composed of datapoint values within given asset name.
And if the value of boolean expression toggles, then the notification is sent.


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
