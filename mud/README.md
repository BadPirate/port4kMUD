# Port 4k MUD

Some time in 1997 a MUD (Multi User Dungeon) was launched under the name “AAA Stockmud”. It was launched by then 15 year old Kevin Lohman using the source code from [Circlemud](http://circlemud.org) beta patch 11 by Jeremy Ellison. It grew a lot from there, eventually being put on a _VERY_ extended hiatus when Kevin joined the Navy back in 2002. The source was made public at that time. And it was lost for a while. In 2016 Jaxom found it again, got it working, and then promptly got busy with work, life and a couple of wonderful children. If it's 2025 now, then Port4k may get another renaissance thanks to AI, and Badpirate Garage (the new moniker and Indy software development that Jaxom is now using).

**Port 4000 MUD is** A highly modified derivative of Circlemud which is in itself a derivative of Dikumud. (See the [LICENSE.txt](LICENSE.txt) file).

## Build Instructions

1.  **Configure:** Run the configuration script from the root directory:

    ```bash
    ./configure
    ```

    This script will attempt to guess system settings, create the necessary Makefiles, and automatically apply flags needed for compatibility with modern compilers (like `-std=gnu89`).

2.  **Compile:** Change into the `src` directory and run make:

    ```bash
    cd src
    make
    ```

    This will compile the main MUD executable (`circle`) and place it in the `bin/` directory.

3.  **(Optional) Build Utilities:** If you need the utility programs, run `make utils` in the `src` directory:

    ```bash
    make utils
    ```

    This builds utilities like `autowiz`, `delobjs`, `listrent`, etc., and places them in the `bin/` directory.

4.  **Troubleshooting:** This is older C code. You may still encounter compilation issues on modern systems, especially related to missing includes.
    - **Missing Includes:** If `make` fails with errors about undeclared functions like `strlen` or `strcpy` (often in `color.c`), you'll need to add the missing header file. For example, edit `src/color.c` and add `#include <string.h>` near the top.
    - Address any other errors as they appear. The `src/Makefile.in` and `configure` script might need further adjustments depending on your compiler and system libraries.

## Running the MUD

1.  **Build:** Ensure you have successfully built the MUD (see Build Instructions above).

2.  **Direct Execution:** You can try running the MUD directly from the root directory:

    ```bash
    bin/circle
    ```

    By default, it will attempt to run on port 4000. You can specify a different port: `bin/circle <port_number>`. See `doc/running.doc` for other command-line flags (like `-q` for quiet mode used in the autorun script).

3.  **Using Autorun Script:** For continuous operation (automatic reboot on crash), use the provided autorun script. It's recommended to run it in the background using `nohup`:

    ```bash
    nohup ./autorun &
    ```

    This script handles logging and restarting the MUD process. Check the `autorun` script itself and the `syslog` file for details.

4.  **Connect:** Once the MUD is running, connect to it using a Telnet client:
    ```bash
    telnet localhost 4000
    ```
    (Replace `localhost` with the server's IP/hostname if running remotely, and `4000` if you used a different port).

## Documentation

Most of the documentation can be found in the `doc/` directory. Please note that some of it might be outdated relative to the current state of the code.
