# FastCGI

An attempt to build a light weight FCGI handler in C for use with Luajit.

## Build

Use cmake to generate a Makefile, then build using make.

# Limitations

 * Tested on Linux only.
 * Only supports the reponder role.
 * Doesn't handle or emit following record types
   * FCGI_ABORT_REQUEST
   * FCGI_DATA
   * FCGI_GET_VALUES
   * FCGI_GET_VALUES_RESULT

# License

Licensed under the MIT license. See LICENSE.

Copyright © 2014-2021 Erik Svensson <erik.public@gmail.com>
