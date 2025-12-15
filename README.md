## Intro

dca3 is a port of GTA III/VC for the Dreamcast made by The Gang, using [re3](https://github.com/halpz/re3/tree/master/) as a base.

re3 a fully reversed source code for GTA III/VC.

This project was started by [Stefanos Kornilios Mitsis Poiitidis](https://x.com/poiitidis) and uses [KallistiOS](https://kos-docs.dreamcast.wiki/).

## Baking the CDI Using Google Notebook (recommended, thanks to jnmartin64!)
### Prerequisites
You need Grand Theft Auto III or Grand Theft Auto: Vice City installed. This version has been tested and works:  https://store.rockstargames.com/game/buy-grand-theft-auto-the-trilogy.

### For gta3
Follow the instructions in https://colab.research.google.com/drive/147aAqNQHhl-VJI2S5jkSUp7M1bVcy4UO

### For gta:vc
Follow the instructions in https://colab.research.google.com/drive/17XVOa4CQXtWRtbyIcs2xsduBhAo5jlc6

## Baking the CDI
### Prerequisites
You need Grand Theft Auto III or Grand Theft Auto: Vice City installed. This version has been tested and works:  https://store.rockstargames.com/game/buy-grand-theft-auto-the-trilogy.

Please note that *SOME VERSIONS* of the game may not work. It has been reported that `d4_gta.mp3` is corrupted sometimes.

Make sure you have a LEGIT copy with no corrupted files, as this process wont work otherwise.

You will also need the following tools installed
- git-scm http://git-scm.com/downloads/win
- dreamsdk r3 https://github.com/dreamsdk/dreamsdk/releases

### Cloning the dca3-game repo (this is required once)
- Open dreamsdk shell
- type `git clone --branch beta https://gitlab.com/skmp/dca3-game.git` (and press enter).
- It should take a moment and successfully clone the repo
- close the dreamsdk shell and proceed to the next step.

### Grand Theft Auto III build (liberty)
#### Preparing the liberty folder
- Open dreamsdk shell
- type `mkdir liberty` (and press enter)
- type `explorer .` (and press enter)
- This will open a folder named liberty. Copy your gta3 files in there.
  - If you use the 2cdrom version of the game, make sure to also copy the contents of the play disc to this folder.
- close the folder and the dreamsdk shell and proceed to the next step.

#### Downloading and extracting the prebuilt elf
- Open dreamsdk shell
- type `cd dca3-game/liberty` (and press enter)
- type `explorer .` (and press enter).
- A folder named dreamcast with some files should be open. Keep it on the side.
- Download the *liberty* beta prebuilt elf from https://gitlab.com/skmp/dca3-game/-/releases/beta
- Open artifacts.zip and extract dca-liberty.elf to the folder that was kept open before.
- Close the folder and dreamsdk shell

#### Repacking and making a prebuilt cdi FOR GD-EMU
- Open dreamsdk shell
- type `cd dca3-game/liberty` (and press enter)
- type `make cdi-prebuilt` (and press enter)
  - This should take a while (5-15 mins)
  - Due to an issue with dreamsdk, this won't fully complete the first time
- type `make cdi-prebuilt` (and press enter)
  - It will continue where it left off before
  - It should run to completion now and show "*** Repack Completed Successfully ***"
- type `explorer .` (and press enter)
- The dreamcast folder should open up, and it should contain dca-liberty.cdi for you (~ 900 megs)

#### Repacking and making a prebuilt cdi FOR burning CD-ROM
- Open dreamsdk shell
- type `cd dca3-game/liberty` (and press enter)
- type `make FOR_DISC=1 cdi-prebuilt` (and press enter)
  - This should take a while (5-15 mins)
  - Due to an issue with dreamsdk, this won't fully complete the first time
- type `make FOR_DISC=1 cdi-prebuilt` (and press enter)
  - It will continue where it left off before
  - It should run to completion now and show "*** CDI Baked Successfully ***"
- type `explorer .` (and press enter)
- The dreamcast folder should open up, and it should contain dca-liberty.cdi for you (~ 700 megs or ~260 megs)
- If the .cdi is not ~ 700 megs (linux/mkdcdisc) or ~260 megs (windows/cdi4dc), then you did something wrong.
  - You can type `rm -rf repack-data` (and press enter)
  - And then start this step from the beggining

### Grand Theft Auto Vice City build (miami)
#### Preparing the miami folder
- Open dreamsdk shell
- type `mkdir miami` (and press enter)
- type `explorer .` (and press enter)
- This will open a folder named miami. Copy your gtavc files in there.
- close the folder and the dreamsdk shell and proceed to the next step.

#### Downloading and extracting the prebuilt elf
- Open dreamsdk shell
- type `cd dca3-game/miami` (and press enter)
- type `explorer .` (and press enter).
- A folder named dreamcast with some files should be open. Keep it on the side.
- Download the *miami* beta prebuilt elf from https://gitlab.com/skmp/dca3-game/-/releases/beta
- Open artifacts.zip and extract dca-miami.elf to the folder that was kept open before.
- Close the folder and dreamsdk shell

#### Repacking and making a prebuilt cdi FOR GD-EMU
- Open dreamsdk shell
- type `cd dca3-game/miami` (and press enter)
- type `FOR_DISC=2 make cdi-prebuilt` (and press enter)
  - This should take a while (5-15 mins)
  - Due to an issue with dreamsdk, this won't fully complete the first time
- type `FOR_DISC=2 make cdi-prebuilt` (and press enter)
  - It will continue where it left off before
  - You will have to close the dreamshell window a few times and restart this procesure a few times
  - Eventually it should run to completion now and show "*** CDI Baked Successfully ***"
- type `explorer .` (and press enter)
- The dreamcast folder should open up, and it should contain dca-miami.cdi for you (~ 1.5 gigs)

#### Repacking and making a prebuilt cdi FOR burning CD-ROM
- Open dreamsdk shell
- type `cd dca3-game/miami` (and press enter)
- type `make FOR_DISC=1 cdi-prebuilt` (and press enter)
  - This should take a while (5-15 mins)
  - Due to an issue with dreamsdk, this won't fully complete the first time
- type `make FOR_DISC=1 cdi-prebuilt` (and press enter)
  - It will continue where it left off before
  - It should run to completion now and show "*** Repack Completed Successfully ***"
- type `explorer .` (and press enter)
- The dreamcast folder should open up, and it should contain dca-miami.cdi for you (~700 or ~550 megs)

## Running on emulators
You must have the 'trails' options turned off from the graphics settings, or a white overlay may appear over the 3d render

## Fine tuning settings
Two experimental modes, 24 bpp (640x480x24) and Anti Aliasing are provided in the Graphics settings.

- When using HDMI or VGA out, it is recommended to turn enable 24 bpp mode. Note that you also have to disable the 'trails' effect.
- Anti Aliasing will work with or without trails, however trails enabled will have a bigger performance hit in that mode.

Enabling any of those modes may result in some missing geometry under heavy scenes (more likely with AA mode).
They can also be combined.

## How to report issues
- Take a photo of your tv/monitor and vmu
- open a ticket via https://gitlab.com/skmp/dca3-game/-/issues/new
- write something descriptive of what is/went wrong

## License

The code should only be used for educational, documentation and modding purposes.\
We do not encourage piracy or commercial use.\
Please keep derivate work open source and give proper credit.