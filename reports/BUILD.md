## Build instructions

For MacOS:
```
# if you need to install brew
/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

# installing sdl2 and its extensions
brew install sdl2
brew install sdl2_mixer
brew install sdl2_ttf

# building the game
git clone git@github.com:mlevatich/FormA.git
cd FormA
make FormA

# or if you want to build without SDL (this is the version which is model checked)
make NoSDL
```

For Arch:
```
sudo pacman -S sdl2 sdl2_mixer sdl2_ttf
git clone git@github.com:mlevatich/FormA.git
cd FormA
make FormA
make NoSDL
```

Playing the game:
```
# Opens a new window. Control the ship with arrow keys, shoot with spacebar.
./FormA
```

Running static analysis:
```
# clang-tidy
clang-tidy src/main.c

# cbmc
make NoSDL
cbmc NoSDL
```
