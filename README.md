## CSEE6863 Project: A game

Herein are documented the adventures of Max Levatich and Harry Choi as they
model check a game.

## Build

For MacOS:

```
# if you need to install brew
/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

# installing sdl2 and sdl2 mixer
brew install sdl2
brew install sdl2_mixer

# building the game
git clone git@github.com:mlevatich/csee6863-project.git
cd csee6863-project
make
```

Process for linux is probably similar, just need to find sdl2 and sdl2_mixer
somewhere such that they're visible to the linker.
