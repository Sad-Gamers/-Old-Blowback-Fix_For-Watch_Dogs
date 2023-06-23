# Blowback Fix for Watch_Dogs
This small DLL fixes the issue WD has with not playing procedural animations, responsible for moving slides and ejection ports. 
## Version 1.1 Release Notes <br />
• Fixes a small issue with the idle animations for SMG pistols.  <br /> 
• Fixes a bug where blowback wouldn't work when too many NPCs are shooting.  <br /> 
### Apparently since the project does not seem to do this on it's own <br /> 
• Disable Precompiled Headers under Properties/C++/Precompiled Headers.  <br /> 
• Set the language standard to "ISO C++17 Standard" under Properties/C++/Language.  <br /> 
• Set your runtime library to "Multi-threaded (/MT)" under Properties/C++/Code Generation.  <br />
• Make sure you are targeting x64 platform.
# Acknowledgments
• This project uses a function coded by github.com/yubie-re in order to allow me to change read/write protections in certain memory regions protected by the game's DRM. <br /> 
• I use a fixed version of the M1404 shotgun model, provided to me by @vector4782 on discord.
