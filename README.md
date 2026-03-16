# Quantis

A **ultra fast**, **light**, and **beautiful** shell for Linux / *nix distributions.

### Whats's so special? 
- [ ] Interactive and beautifully styled shell, with no other deps (i.e unlike Oh-My-Posh, Starship).
- [ ] Dependencies are minimal; Just using standard C libraries and essential linux libs. Almost definitely you don't need anything extra to run this.
- [ ] The entire thing runs in one single directory, with 3 files `(Quantis (The compiled binary), .qnrs, .qnhistory)`
- [ ] There is no need of specific installation; Just give Quantis a folder to work in, and compile the C code. To remove entirely from the system, just delete the abovementioned 3 files.
- [ ] This shell was designed with portability in mind, for low resource systems or even custom operating systems, that provide the libc and needed syscalls.

> **Note :** If you added the shell to your _PATH_, upon uninstall you will need to remove it too from the respective shells.
> The other two files, the `.qnrc` and `.qnhistory` are automatically created in the same directory from where you run Quantis.

## > Step to compile :
 - `gcc quantis.c -o Quantis`
(Or use another C compiler you prefer.)
---

## Screenshot :
<div align="center">
<img width="1366" height="768" alt="image" src="https://github.com/user-attachments/assets/bd24d451-5532-4b90-baea-0e4fd70c0cf2"/>
