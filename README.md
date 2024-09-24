# THE MINECRAFT 3DS UNISTORE
The official "Minecraft 3DS Community" UniStore is here!

## Our unistore is now recommended! You can install it by going into the Recommended Unistores section in Universal Updater

## Information

**If you want to know how to use this store, read the manual on our discord: https://discord.gg/xSrN6k965F**
## Credits
### Support
Minecraft 3DS Community and Universal-Server Discord servers
### Testing
Nawrek, VanceForPresident, Pizzaleader, Cracko298 and various people from the Minecraft 3DS Community Discord server. Thanks Susbaconhairman for the idea of unistore.
## Item creators
* ArcModzzz
    * World in Jar 3DS
    * Find the Button
* Babylion122
    * Ender Dragon Elytra
    * Lapiz's Funland map
    * Stampy's Lovely World map port
* CleetusMcfarln
    * DanTDM's Lab
* Cracko298
    * MC3DS-MAC
    * MC3DS-Options
    * Nuclear Creeper
    * Bat Bombs
    * Mojangles Font
    * Festive World Override
    * Amplified World Generation
    * Zombie Apocalypse
    * DevWorld v2
    * Rare Bones
    * April Fools mod
    * Festive as Default Textures
    * TNT Mimic
    * Farlandia world
    * Xray Shader, Remove Clouds, Remove Skybox, and Creative Mode Block Break
    * Remove Mob Cap mod
    * Tricky Trials OST 320kHz
    * Skyblock Plus world
    * Mob Tower structure
    * Enhanced Particles
    * Block skin
    * SD Card Dropper Map
    * IronBrute
* CZX
    * The Dropper: 3DS Remaster
    * GenSpace Mod
* ENDERMANYK
    * Shaders
* Genspace
    * Minecraft 3DS Modernization Map
    * The Dropper: 3DS Remaster
    * GenSpace's Skyblock
    * SkyDen
* Glonk
    * Broken Stronghold Chest Fix
* KrftRV249
    * Random Loot mod
    * Guns mod
    * Instant Eating mod
* linuxwizard
    * Mipmapping patch
* Nawrek
    * JoJo's Bizarre Adventure skins
* PanguinBoi
    * PanGames
* PokéTube
    * PokéTube City map
* RetroRemade
    * Better Creepers mod
* Ruff64
    * The Dropper: 3DS Remaster
* STBUniverse
    * STB-MC3DS Unistore
    * Vanilla 1.20 textures port
* TheRustico36
    * Cleann'slick GUI
    * New Base Skins port
    * Legacy Console Default skin pack port
* UnknownLoser
    * DualFlow
* Vance
    * Technoblade Skin
    * Technoblade's Crown
    * Skyblock map port
    * Breaking Bad skins
    * Sheeptastic mod port
    * Giant Baby Zombie mod
    * 3DSpleef
    * Building Time map
    * Randoms map demo
    * Frequently Asked Questions
    * GenSpace Mod
    * Mega Dropper
* Vicrtl345
    * SkyDen map port
* Virtual Overtime
    * LavaCity PVP map
    * FNAF Hide and Seek map
* WaterMelon
    * The Dropper: 3DS Remaster
    * The Dropper Submission
    * Debug Mode 3D
    * Blind Mazes
    * PC Gamer Cow
    * GenSpace Textures
* wyndchyme
    * Modernization MegaPack (includes content from various other people)
    * MegaPack Standalone: Classic Textures
    * Pink Floyd Painting Pack
* Zexlo
    * 1.20 Lava textures
    * 1.20 Mob textures
    * 1.20 Seamless varied textures
    * Barebones textures


**All content has been approved by their creators to be used**

## Building Atlas *.t3x.
- Download [DevKitPro](https://github.com/devkitPro/installer/releases/latest)
- Add Directories: `.\devkitPro\tools\bin`, and `.\devkitPro\msys2\usr\bin` to your Account PATH.
- You may also need to add the Directories: `.\devkitPro\devkitARM\bin`, and `.\devkitPro\libctru\include` to your Account PATH.
- Start powershell, and run the command: `pacman -S tex3ds`.
- Create a new Directory, and get Textures you may need (48x48 size for Icons).
- Create a File inside of your 'new' Directory, called `.\file.t3s`, then open it in notepad, and write the following strings to it.
```
--atlas -f rgba -z auto

{yourIconFile0.png}
{yourIconFile1.png}
{yourIconFile2.png}
{yourIconFile3.png}
```
`{yourIconFileX.png} is your PNG Icons in the Current Directory with the 'file.t3s' file`.

- After getting the icon Images, you can now execute the Command in the Same Directory using PowerShell.
- Type: `tex3ds -i file.t3s -o mc3ds.t3x`
- It should generate a File with the Extension `*.t3x` not `*.t3s`.
- That is your Icon Atlas
