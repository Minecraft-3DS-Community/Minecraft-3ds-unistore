![mc](https://github.com/user-attachments/assets/3f99d4aa-1205-4e24-801c-ad95e92ccb0b)

<div align="center">
   
__THE OFFICIAL MINECRAFT 3DS COMMUNITY UNISTORE__

![GitHub last commit](https://img.shields.io/github/last-commit/Minecraft-3DS-Community/Minecraft-3ds-unistore?color=blue)

</div>

## About

This is a repository for a Universal-Updater UniStore that hosts mods for Minecraft: New Nintendo 3DS Edition. It can be installed on your console from the 'Recommended UniStores' menu in Universal-Updater.

**For more information, read the manual on our Discord server: https://discord.gg/xSrN6k965F** 

<br>

## Contents
| Creator | Mod(s) |
| --- | --- |
| ArcModzzz | <ul><li>World in a Jar 3DS</li><li>Find the Button</li></ul> | 
| Babylion122 | <ul><li>Ender Dragon Elytra</li><li>Lapiz's Funland Map</li><li>Stampy's Lovely World Port</li></ul> |
| CleetusMcfarln | <ul><li>DanTDM's Lab</li></ul>
| Cracko298 | <ul><li>MC3DS-MAC</li><li>MC3DS-Options</li><li>Nuclear Creeper</li><li>Bat Bombs</li><li>Mojangles Font</li><li>Festive World Override</li><li>Amplified World Generation</li><li>Zombie Apocalypse</li><li>DevWorld v2</li><li>Rare Bones</li><li>April Fools Mod</li><li>Festive as Default Textures</li><li>TNT Mimic</li><li>Farlandia World</li><li>X-Ray Shader, Remove Clouds, Remove Skybox and Creative Mode Block Break</li><li>Remove Mob Cap</li><li>Tricky Trials OST 320kHz</li><li>SkyBlock Plus World</li><li>Mob Tower</li><li>Enhanced Particles</li><li>Block Skin</li><li>SD Card Dropper Map</li><li>IronBrutes</li></ul> |
| CZX | <ul><li>The Dropper: 3DS Remaster</li><li>GenSpace Mod</li></ul> |
| ENDERMANYK | <ul><li>ENDERMANYK's Shaders</li></ul>
| GenSpace | <ul><li>Minecraft 3DS Modernization Map</li><li>The Dropper: 3DS Remaster</li><li>GenSpace's SkyBlock</li><li>SkyDen</li></ul> |
| Glonk | <ul><li>Broken Stronghold Chest Fix</li></ul> |
| KrftRV249 | <ul><li>Random Loot Mod</li><li>Guns Mod</li><li>Instant Eating Mod</li></ul> |
| linuxwizard | <ul><li>Mipmapping Patch (1.0 Only)</li></ul> |
| Nawrek | <ul><li>JoJo's Bizarre Adventure Skins</li></ul> | 
| PanguinBoi | <ul><li>PanGames</li></ul> |
| PokéTube | <ul><li>PokéTube City Map</li></ul> |
| RetroRemade | <ul><li>Better Creepers Mod</li></ul> |
| Ruff64 | <ul><li>The Dropper: 3DS Remaster</li></ul> |
| STBUniverse | <ul><li>STB-MC3DS UniStore</li><li<Vanilla 1.20 Textures Port</li></ul> |
| TheRustico36 | <ul><li>Cleann'slick GUI</li><li>New Base Skins Port</li><li>Legacy Console Default Skin Pack Port</li></ul> |
| UnknownLoser | <ul><li>DualFlow</li></ul> |
| VanceForPresident | <ul><li>Technoblade Skin</li><li>Technoblade's Crown</li><li>SkyBlock Map Port</li><li>Breaking Bad Skins</li><li>Sheeptastic Mod Port</li><li>Giant Baby Zombie Mod</li><li>3DSpleef</li><li>Building Time Map</li><li>Randoms Map Demo</li><li>Frequently Asked Questions</li><li>GenSpace Mod</li><li>Mega Dropper</li></ul> |
| Vicrtl345 | <ul><li>SkyDen</li></ul> |
| Virtual Overtime | <ul><li>LavaCity PVP Map</li><li>FNAF Hide and Seek Map</li></ul> |
| WaterMelon | <ul><li>The Dropper: 3DS Remaster</li><li>The Dropper Submission</li><li>Debug Mode 3D</li><li>Blind Mazes</li><li>PC Gamer Cow</li><li>GenSpace Textures</li></ul> |
| wyndchyme | <ul><li>Modernization MegaPack (inckudes content from various other people)</li><li>MegaPack Standalone: Classic Textures</li><li>MegaPack Standalone: Achievement Textures</li><li>Pink Floyd Painting Pack</li></ul> |
| Zexlo | <ul><li>1.20 Lava Textures</li><li>1.20 Mob Textures</li><li>1.20 Seamless Varied Textures</li><li>Bare Bones Textures</li></ul> | 

_All content has been approved by their creators to be hosted on this UniStore._

<br>

## Credits
### Development
- Cracko298
- Nawrek
- STBUniverse
- VanceForPresident
### Maintenence
- Cracko298
- Nawrek
- wyndchyme
### Testing
- Cracko298
- Nawrek
- Pizzaleader
- VanceForPresident
- _Various others from the Minecraft 3DS Community Discord server_
### Support
- Minecraft 3DS Community and Universal-Server Discord servers

<br>

_Thanks to Susbaconhairman for the idea of the UniStore._

<br>

## Building Atlas *.t3x.

<details><summary> Click to show/hide build instructions </summary>

<br>

- Download [DevKitPro](https://github.com/devkitPro/installer/releases/latest)
- Add directories `.\devkitPro\tools\bin`, and `.\devkitPro\msys2\usr\bin` to your Account PATH.
- You may also need to add the following directories to your Account PATH: `.\devkitPro\devkitARM\bin` and `.\devkitPro\libctru\include` 
- Start Powershell, and run the command `pacman -S tex3ds`.
- Create a new directory, and get textures you may need (48x48 size for icons).
- Create a file inside of your 'new' directory called `.\file.t3s`, then open it in a text editor and write the following strings to it:
```
--atlas -f rgba -z auto

{yourIconFile0.png}
{yourIconFile1.png}
{yourIconFile2.png}
{yourIconFile3.png}
```
`{yourIconFileX.png} is your PNG Icons in the Current Directory with the 'file.t3s' file`.

- After getting the icon images, you can now execute the command in the same directory using PowerShell.
- Type `tex3ds -i file.t3s -o mc3ds.t3x`
- It should generate a file with the Extension `*.t3x`, not `*.t3s`.
- That is your icon atlas.

</details>
