# USE: Universal System Emulator
The USE is a platform created to ease the development of software-based emulators of archaic hardware.
The project aims to be easily understandable, extensible, portable and tries to follow good programming practices while
utilizing the newest C++ standard. For more information about the project, please consult the documentation.

## Quick start on Linux
This is a quick start guide on how to prepare the platform either for development of your own emulator or just testing.

### Get the repository
```shell
git clone https://github.com/andreondra/use.git
```
### Configure the CMake project
```shell
cd use
mkdir build
cd build
cmake ..
make
```

The CMake will download most of the dependecies, however, depending on your platform, there may be some other
libraries required. Please check the output of the `cmake ..` command. If there is any problem, do not hesitate
to open an Issue.

### Run the project
```shell
./use
```

## Contributing
You are always very welcome to file bug reports using Issues or opening PRs for enhancements.

## License
USE Copyright (C) 2023 Ondrej Golasowski

This project is licensed under the GNU GPLv3.
Full text of the license can be found in the `COPYING` file.
This license applies to all the files in this repository except:
- External projects downloaded during the CMake configuration step, these projects have their own license in their repository.
- Manually included projects. These projects contain their license in the directory.