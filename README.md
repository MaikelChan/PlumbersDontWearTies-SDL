# Plumbers Don't Wear Ties - Nintendo Switch

This project is my own implementation of the game `Plumbers Don't Wear Ties` written from scratch, that is, the original program has not been decompiled or reverse-engineered. However, the game still needs the assets from the original PC version to run. The only thing that has been reverse-engineered is the `GAME.BIN` file, done by [Daniel Marschall](https://misc.daniel-marschall.de/spiele/plumbers/?page=pc_gamebin), which contains information about the scenes, background images, sounds and sequences of events.

This game is infamous for being considered one of the worst games ever made. The 3DO version is much more known (relatively speaking), but there was also a very rare Windows version released in 1993. Due to being 16bit, it can't be run on modern PCs.

This is a port for the Nintendo Switch based on my SDL2 PC version of the game. This is the current state of the project:

- Rendering and audio are implemented with SDL2.
- It is possible to advance to the next picture to progress quickly through the game.
- It could still contain some other small bugs here and there that I haven't catched up yet.

## How to build

The main requirement is to have [devkitPro](https://devkitpro.org).

Follow the instructions to install devkitPro here: https://devkitpro.org/wiki/Getting_Started.
You will also need the Switch development package and the libraries `switch-sdl2` and `switch-sdl2_ttf`.

If you use Windows or Ubuntu, here are more detailed instructions.

### Windows

Even though devkitPro offers a Windows installer, I've had some issues setting it up. It's easier to use WSL. If you want to use the Windows installer anyway, check the link above for instructions.

1. Install [WSL](https://docs.microsoft.com/en-us/windows/wsl/install). By default it will install Ubuntu, which is fine.
2. Open a WSL terminal and just follow the Ubuntu instructions below. With the difference that, if you want to clone the project into, for example, the `C:\` folder, you will need move to that folder inside the terminal with the command `cd /mnt/c/`.

### Ubuntu and other Debian based linux distros

1. Open the terminal in the folder where you want to clone the project.
2. Clone it with the command `git clone --branch switch https://github.com/MaikelChan/PlumbersDontWearTies-SDL`. A subfolder called `PlumbersDontWearTies-SDL` will be created containing the project.
3. Move to that subfolder with `cd PlumbersDontWearTies-SDL`.
4. Download the latest version of the [custom devkitPro pacman](https://github.com/devkitPro/pacman/releases/tag/v1.0.2), that will be used to download the compilers and libraries to build the project. Once downloaded, put it in the `PlumbersDontWearTies-SDL` folder.
5. Install devkitPro pacman with this command: `sudo gdebi devkitpro-pacman.amd64.deb`. If gdebi is not found, install it with `sudo apt install gdebi-core`, and then try again installing pacman.
6. Use the following command to sync pacman databases: `sudo dkp-pacman -Sy`.
7. Now update packages with `sudo dkp-pacman -Syu`.
8. Install the Switch development tools with `sudo dkp-pacman -S switch-dev`.
9. Install SDL2 and SDL2_ttf with `sudo dkp-pacman -S switch-sdl2 switch-sdl2_ttf`.
10. Set the DEVKITPRO environment variables so the system knows where the compilers and libraries are installed with these commands:
    - `export DEVKITPRO=/opt/devkitpro`.
11. To generate Build the project with the command `make -j4`.

After a successful build, you will get a file called `PlumbersDontWearTies-SDL.nro`, which is the main executable.

## How to run

1. Go to the root of your SD card and create a folder named `PlumbersDontWearTies`. If you are using [Yuzu emulator](https://yuzu-emu.org/downloads/), open it, go to the menu `File/Open yuzu Folder`, and the root of the emulated SD card is in the `sdmc` folder.
2. For legal reasons, you will need to get the original PC game on your own to obtain the assets like graphics and sound effects. Those are not contained in this repository.
3. Copy all PC game's assets to the `PlumbersDontWearTies` folder that was created earlier.
4. Grab the `Font.ttf` located in this repository or pick a TTF font file of your liking and rename it `Font.ttf`. Put it inside the `PlumbersDontWearTies` folder. This will be used to render the score in-game.
5. If everything went fine, you should be able to run the game.

## How to play

| Button      | Action                                            |
|-------------|---------------------------------------------------|
| D-Pad Left  | Select first option in a choice selection screen  |
| D-Pad Up    | Select second option in a choice selection screen |
| D-Pad Right | Select third option in a choice selection screen  |
| A           | Skip to the next picture                          |
| -           | Exit the game                                     |

## Screenshots

<p align="center">
  <img title="Plumbers Don't Wear Ties screenshot" src="/screenshot00.png">
  <img title="Plumbers Don't Wear Ties screenshot" src="/screenshot01.png">
  <img title="Plumbers Don't Wear Ties screenshot" src="/screenshot02.png">
  <img title="Plumbers Don't Wear Ties screenshot" src="/screenshot03.png">
  <img title="Plumbers Don't Wear Ties screenshot" src="/screenshot04.png">
</p>