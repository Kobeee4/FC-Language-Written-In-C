===========================================================================
 FC + SDL2  (the DEPENDENCY build)
 native GUI windows: pixels, shapes, keyboard & mouse
===========================================================================

This folder is the variant of the FC interpreter that DOES use an external
dependency -- the SDL2 library -- to provide genuine, native, cross-platform
GUI windows. The interpreter language and every other module are identical
to the main (dependency-free) FC; the only addition here is the `win` module
and a Makefile that links SDL2.

The plain FC in the parent directory stays pure C with zero dependencies.
Use THIS build when you want a real on-screen window (games, visual apps).
The binary built here is called `fcg` (FC graphical) so it does not clash
with the dependency-free `fc`/`fcl`.


---------------------------------------------------------------------------
 1. INSTALL THE SDL2 DEPENDENCY
---------------------------------------------------------------------------
  Termux (Android):     pkg install sdl2
  Debian / Ubuntu:      sudo apt install libsdl2-dev
  Fedora:               sudo dnf install SDL2-devel
  Arch:                 sudo pacman -S sdl2
  macOS (Homebrew):     brew install sdl2
  Windows (MSYS2):      pacman -S mingw-w64-x86_64-SDL2

On Termux, an on-screen window also needs an X server:
  * Install the "Termux:X11" companion app, then:
        pkg install x11-repo
        pkg install termux-x11-nightly
        export DISPLAY=:0
    Start the X11 server app, then run ./fcg as usual.


---------------------------------------------------------------------------
 2. BUILD & RUN
---------------------------------------------------------------------------
        cd FCdependency
        make
        ./fcg win_demo.fc

  The Makefile auto-detects SDL2 with sdl2-config or pkg-config and falls
  back to -lSDL2. To put it on your PATH (Termux):

        make install            # copies fcg to $PREFIX/bin/fcg
        fcg win_demo.fc

  If SDL2 is NOT installed, the program still compiles, but calling any
  win.* function reports a friendly error telling you to install SDL2.


---------------------------------------------------------------------------
 3. THE `win` MODULE (native window API)
---------------------------------------------------------------------------
    win.open(title, w, h)          open a window with an RGB canvas
    win.clear(color)               clear the whole window to color (0xRRGGBB)
    win.pixel(x, y, color)         draw one pixel
    win.rect(x, y, w, h, color)    filled rectangle
    win.line(x0, y0, x1, y1, color) line
    win.present()                  show everything drawn since last present
    win.delay(ms)                  sleep milliseconds (frame pacing)
    win.poll()                     -> event dict, one of:
                                      {"type":"none"}
                                      {"type":"quit"}                 (window X)
                                      {"type":"key",   "key":"Left"}  (pressed)
                                      {"type":"keyup", "key":"Left"}  (released)
                                      {"type":"mouse", "x":, "y":, "button":}
    win.pressed(keyname)           true while a key is held down (e.g. "Up",
                                   "W", "Space", "Left", "Escape") -- best for
                                   smooth movement
    win.close()                    destroy the window and shut SDL down

  Colors are 0xRRGGBB integers, e.g. 16711680 = red, 65280 = green,
  255 = blue, 16777215 = white.

  Key names follow SDL: arrows are "Left"/"Right"/"Up"/"Down", letters are
  uppercase "A".."Z", plus "Space", "Return", "Escape", etc.


---------------------------------------------------------------------------
 4. MINIMAL GAME LOOP
---------------------------------------------------------------------------
        win.open("My Game", 640, 480)
        let running = true
        while running
            let ev = win.poll()
            if ev["type"] == "quit"
                running = false
            if win.pressed("Escape")
                running = false
            win.clear(0)                       + black background
            win.rect(100, 100, 50, 50, 16711680)
            win.present()
            win.delay(16)                      + ~60 FPS
        win.close()

  See win_demo.fc for a complete movable-box example.


---------------------------------------------------------------------------
 5. WHICH BUILD SHOULD I USE?
---------------------------------------------------------------------------
  Parent folder (fc / fcl)   zero dependencies, single binary, runs anywhere.
                             Websites (web.serve), terminal games (term.*),
                             image files (gui.save). No on-screen window.

  This folder (fcg)          adds SDL2 -> real native windows with live
                             keyboard/mouse. Needs SDL2 installed.

  Both share the exact same language and all other modules, so a script
  that does not use win.* runs identically under fc and fcg.
===========================================================================
