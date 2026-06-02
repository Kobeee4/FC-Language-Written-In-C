===========================================================================
 FC PROGRAMMING LANGUAGE
 An ultra-lightweight, dependency-free, cross-platform interpreter in C
===========================================================================

FC is a small but capable interpreted language with a clean, Python-like
indented syntax. The interpreter is a tree-walking evaluator written in
portable C (compiles as C11 / gnu11) with zero external libraries beyond
the C standard library and the platform's standard socket headers.

Reference-counting manages all dynamically allocated values (strings,
lists, dicts, objects, functions). Runtime errors, type mismatches,
indentation errors and I/O failures are caught and reported as friendly
messages; the interpreter exits with status code 1 instead of crashing.


---------------------------------------------------------------------------
 1. FILE TREE
---------------------------------------------------------------------------
  main.c          CLI entry point: runs a .fc file or an interactive REPL
  lexer.h/.c      Tokenizer with an indentation stack (INDENT/DEDENT)
  parser.h/.c     Recursive-descent parser producing an AST
  interpreter.h   Shared value model, object structs and public API
  interpreter.c   Values, environments, evaluation, OOP, GC (refcounting),
                  control flow, and collection/string methods
  modules.c       Native modules and global builtins
  others.c        String builder, value formatting, file IO, SHA-256
  Makefile        Build automation for gcc / clang
  README.txt      This file


---------------------------------------------------------------------------
 2. BUILDING
---------------------------------------------------------------------------

LINUX / macOS (gcc or clang):

    make
    ./fc script.fc          # run a program
    ./fc                    # start the interactive REPL

Or compile directly without the Makefile:

    cc -O2 -std=gnu11 -o fc main.c lexer.c parser.c interpreter.c modules.c others.c -lm


--- TERMUX (Android) ------------------------------------------------------

Termux is a terminal emulator for Android that provides a full Linux-style
toolchain. FC builds and runs there with no changes.

  1. Install Termux from F-Droid (recommended) or the Play Store.

  2. Update packages and install a C compiler + make:

         pkg update && pkg upgrade
         pkg install clang make

  3. Copy the FC source files into a working directory. If the files are
     on shared storage, grant storage access first:

         termux-setup-storage
         cp /sdcard/Claude/*.c /sdcard/Claude/*.h ~/fc/
         cp /sdcard/Claude/Makefile ~/fc/
         cd ~/fc

     IMPORTANT: Build and run inside Termux's own home (~ or $HOME).
     Android mounts /sdcard with the "noexec" flag, so a binary placed
     there cannot be executed directly. Always copy the sources to
     $HOME and build there.

  4. Build and run:

         make
         ./fc script.fc
         ./fc                 # REPL

  5. Optional: put 'fc' on your PATH so you can run it from anywhere:

         cp fc $PREFIX/bin/fc
         fc script.fc

Notes for Termux:
  * 'clang' is the default compiler; the Makefile's "CC ?= cc" resolves to
    clang automatically. You can also run "make CC=clang".
  * The networking module (net.*) uses standard sockets and works in
    Termux over Wi-Fi / mobile data.
  * gui.refresh() writes fc_canvas.ppm to the current directory; view it
    with any image viewer that supports PPM (or convert it with the
    'imagemagick' package: pkg install imagemagick).


--- WINDOWS (MinGW-w64) ---------------------------------------------------

    gcc -O2 -std=gnu11 -o fc.exe main.c lexer.c parser.c interpreter.c modules.c others.c -lm
    fc.exe script.fc

--- WINDOWS (MSVC, Developer Command Prompt) ------------------------------

    cl /O2 main.c lexer.c parser.c interpreter.c modules.c others.c

Notes:
  * -lm links the C math library (needed on Linux/Termux; harmless else).
  * The net.* module uses BSD sockets on Linux/macOS/Termux. On the Windows
    build it compiles as a safe stub (returns a message rather than opening
    sockets) so the project always builds cleanly.
  * "make clean" removes object files and the binary.


---------------------------------------------------------------------------
 3. LANGUAGE OVERVIEW
---------------------------------------------------------------------------

COMMENTS
    Begin with a plus sign followed by a space. A line whose first
    non-space character is "+ " is a comment. A bare "+" elsewhere is the
    addition operator, so 5 + 2 is arithmetic.

        + this whole line is a comment
        let x = 5 + 2

INDENTATION
    Blocks are defined by indentation (4 spaces per level is conventional).
    The lexer maintains an indentation stack and emits INDENT / DEDENT
    tokens. Mismatched indentation is reported as a syntax error.

PRIMITIVE TYPES
    Numbers (stored as doubles), Strings, Booleans (true / false), Null.

VARIABLES
        let x = 10
        let name = "Ada"
        x = x + 1               + reassignment needs no 'let'

INPUT / OUTPUT
        input user_name
        out "Hello " + user_name

LISTS
        list nums = [1, 2, "three"]
        nums.append(4)
        let last = nums.pop()
        out nums.length()
        out nums[0]
        nums[1] = 20

DICTS
        dict m = {"key": "value", "n": 42}
        out m["key"]
        m["new"] = true
        out m.keys()
        out m.has("n")

FUNCTIONS
        func add(a, b)
            return a + b
        out add(2, 3)

CONDITIONALS
        if x > 5
            out "big"
        elif x == 5
            out "five"
        else
            out "small"

LOOPS
        loop 10 times
            out "Hi"

        let i = 0
        while i < 3
            out i
            i = i + 1

        for item in [10, 20, 30]
            out item

        for ch in "abc"            + iterates characters
            out ch

        for key in my_dict         + iterates keys
            out key + " = " + my_dict[key]

        for n in range(5)          + 0,1,2,3,4
            out n

BREAK / CONTINUE
        for n in range(100)
            if n == 3
                continue
            if n > 5
                break
            out n

LAMBDAS (anonymous single-expression functions)
        let sq = lambda x: x * x
        out sq(5)                          + 25
        out fn.map(lambda n: n * 2, [1, 2, 3])
        out fn.reduce(lambda a, b: a + b, range(1, 5), 0)

COMPOUND ASSIGNMENT
        let n = 10
        n += 5      n -= 2      n *= 3      n /= 2

MEMBERSHIP ('in' operator)
        out 3 in [1, 2, 3]                 + true
        out "key" in my_dict               + true if key present
        out "ell" in "hello"               + true (substring)

NEGATIVE INDEXING
        let xs = [10, 20, 30]
        out xs[-1]                         + 30 (last element)
        xs[-1] = 99                        + assign to last
        out "world"[-1]                    + d

IMPORT / CUSTOM PACKAGES
        A package is just an .fc file with functions, classes, or values.
        Import it to run it once and pull its top-level definitions into
        scope (re-imports are de-duplicated):

            import strutil          + finds strutil.fc in the search path
            import "lib/util.fc"    + or an explicit path
            use strutil             + 'use' is an alias for 'import'

        Search path for a bare name:  ./<name>.fc, ./packages/<name>.fc,
        then $HOME/.fc/packages/<name>.fc

        Install a package from a URL into ./packages/ with the pkg module:

            pkg.install("util", "http://example.com/util.fc")
            import util

        See the included packages/strutil.fc and app.fc for a full example.

CLASSES / OBJECTS
        class Person
            func init(name)
                let this.name = name
            func greet()
                out "Hi, I am " + this.name

        let p = Person("Bob")
        p.greet()

EXCEPTION HANDLING
        try
            let result = 10 / 0
        catch err
            out "Handled error: " + err

CONCURRENCY
        thread spawn my_func()
    Spawned work runs cooperatively (synchronously) so that shared
    reference-counted state can never be corrupted or segfault, preserving
    the "never crash" safety guarantee.

OPERATORS
    + - * / %        arithmetic (+ also concatenates strings and lists)
    == != < > <= >=  comparison (numbers, or strings lexicographically)
    and or not       logical (and / or short-circuit)


---------------------------------------------------------------------------
 4. GLOBAL BUILT-IN FUNCTIONS
---------------------------------------------------------------------------
    print(...)          print all arguments separated by spaces + newline
    len(x)              length of a string, list, or dict
    str(x)              convert any value to its string form
    num(x)              convert a string/bool to a number
    int(x)              truncate a value to an integer
    bool(x)             truthiness of a value as true/false
    type(x)             type name: "number","string","bool","null",
                        "list","dict","function","class","object"
    range(n)            list [0 .. n-1]
    range(a, b)         list [a .. b-1]
    range(a, b, step)   list with the given step (step may be negative)
    abs(x)              absolute value
    min(a, b, ...)      minimum of arguments, or of a single list argument
    max(a, b, ...)      maximum of arguments, or of a single list argument
    assert(cond, msg?)  raise a catchable error if cond is falsey
    chr(n)              one-character string from a character code
    ord(s)              character code of the first character of a string
    sum(list)           sum of a list of numbers
    sorted(list)        new sorted list (numbers or strings)
    reversed(x)         reversed copy of a list or string
    enumerate(list)     list of [index, item] pairs
    zip(a, b)           list of [a_i, b_i] pairs


---------------------------------------------------------------------------
 5. METHODS ON BUILT-IN TYPES
---------------------------------------------------------------------------

STRING methods (called as text.method(...))
    length()                    upper()                 lower()
    trim()                      contains(sub)           split(sep)
    replace(old, new)           find(sub)  -> index/-1  substr(start, len)
    repeat(n)                   starts_with(p)          ends_with(p)

LIST methods (called as list.method(...))
    length()    append(v)   pop()       get(i)      set(i, v)
    insert(i, v)            remove_at(i)             index_of(v) -> /-1
    contains(v)            reverse()    sort()       slice(start, end)
    join(sep)

DICT methods (called as dict.method(...))
    length()    keys()      values()    items()      has(key)
    get(key)    set(key, v) remove(key) merge(other) clear()


---------------------------------------------------------------------------
 6. BUILT-IN NATIVE MODULES
---------------------------------------------------------------------------

MATH
    math.sqrt(x)   math.pow(b,e)  math.abs(x)    math.floor(x)
    math.ceil(x)   math.round(x)  math.sin(x)    math.cos(x)
    math.tan(x)    math.atan(x)   math.atan2(y,x)
    math.log(x)    math.log10(x)  math.exp(x)    math.hypot(a,b)
    math.min(a,b)  math.max(a,b)  math.random()
    math.gcd(a,b)  math.lcm(a,b)  math.factorial(n)
    math.sign(x)   math.clamp(x,lo,hi)   math.deg(rad)   math.rad(deg)
    math.pi        math.e         math.tau

BIT (32-bit bitwise operations)
    bit.and(a,b)   bit.or(a,b)    bit.xor(a,b)
    bit.not(a)     bit.shl(a,n)   bit.shr(a,n)

CRYPTO
    crypto.hash("sha256", text)   -> 64-char hex digest
    crypto.base64_encode(text)    -> base64 string
    crypto.base64_decode(text)    -> decoded string

SEC (cybersecurity helpers)
    sec.xor(text, key)            repeating-key XOR (encrypt/decrypt)
    sec.caesar(text, shift)       Caesar cipher
    sec.rot13(text)               ROT13
    sec.hex_encode(text)          bytes -> hex string
    sec.hex_decode(hex)           hex -> bytes string
    sec.crc32(text)               CRC-32 checksum
    sec.rand_bytes(n)             n random bytes as hex
    sec.strength(password)        rough strength score 0-100

TIME
    time.now()              seconds since the epoch
    time.sleep(seconds)     pause execution
    time.format(timestamp)  "YYYY-MM-DD HH:MM:SS"
    time.clock()            high-resolution CPU seconds
    time.parts(timestamp)   dict {year,month,day,hour,minute,second,weekday}

TEXT (function-style string helpers; note: the global str() converts to
      string, while the text.* module holds string utilities)
    text.upper(s)           text.lower(s)           text.trim(s)
    text.split(s, sep)      text.join(list, sep)    text.replace(s,old,new)
    text.format(template, ...)  substitutes each {} with the next argument

AI / MATRIX CORE
    ai.matrix(rows, cols)               zero-filled matrix (list of lists)
    ai.randn(rows, cols)                random matrix in [-1, 1]
    ai.dot(m1, m2)                      matrix multiplication
    ai.add(m1, m2)  ai.sub(m1, m2)      element-wise add / subtract
    ai.scale(m, k)                      multiply every element by k
    ai.transpose(m)                     transpose
    ai.apply(func, m)                   apply func to every element
    ai.shape(m)                         [rows, cols]
    ai.train_linear(x_list, y_list)     least-squares fit -> {"m":, "b":}
    ai.predict(model, input)            input may be a number or a list

NN (neural-network building blocks; number or list element-wise)
    nn.sigmoid(x)   nn.relu(x)    nn.tanh(x)    nn.leaky_relu(x)
    nn.softmax(list)                    normalized probabilities
    nn.dsigmoid(x)  nn.drelu(x)   nn.dtanh(x)   (derivatives)

FN (higher-order functions; pair with lambda)
    fn.map(func, list)                  list of func(item)
    fn.filter(func, list)               items where func(item) is truthy
    fn.reduce(func, list, initial)      fold left: func(acc, item)
    fn.each(func, list)                 call func for each item (no result)

RANDOM
    random.seed(n)          seed the generator
    random.float()          float in [0, 1)
    random.int(a, b)        integer in [a, b] inclusive
    random.choice(list)     a random element

OS
    os.getenv(name)         environment variable, or null
    os.system(cmd)          run a shell command, return its exit code
    os.exit(code)           terminate the program with an exit code
    os.platform()           "posix" or "windows"
    os.cwd()                current working directory
    os.args()               list of command-line arguments (script first)

FILE SYSTEM & DB
    file.read("path.txt")               returns file contents as a string
    file.write("path.txt", "data")      returns bytes written
    file.append(path, data)             append text, returns bytes written
    file.exists(path)                   true / false
    file.delete(path)                   true if removed
    file.size(path)                     size in bytes (-1 if missing)
    file.lines(path)                    list of lines
    file.read_bytes(path)               list of byte values (0-255)
    file.write_bytes(path, list)        write a list of byte values
    db.query("SELECT ...")              mock layer; returns a list of rows

CSV
    csv.parse(text)                     text -> list of rows (lists of fields)
    csv.stringify(rows)                 list of rows -> CSV text

NETWORKING (POSIX builds)
    net.fetch_url("http://host/path")   simple HTTP GET, returns the body
    net.send(ip, port, data)            opens a TCP connection and sends
    net.listen(port)                    accepts one client, returns request

WEB (POSIX builds) -- build real websites and web apps
    web.get(url)                        HTTP GET, returns the body
    web.serve(port, handler)            run an HTTP server; handler(request)
                                        returns either an HTML string, or a
                                        dict {status, type, body} for full
                                        control (JSON APIs, redirects, etc.)
    web.serve_dir(port, dir)            static file server for a folder
    web.parse_request(req)              -> {method, path, query{}, headers{}, body}
    web.mime(path)                      content-type for a file extension
    web.url_encode(s)  web.url_decode(s)
    web.html_escape(s)

TERM (ANSI terminal control + keyboard, for real text games / TUIs)
    term.clear()            term.at(x, y)           term.write(text)
    term.color(code)        term.bg(code)           term.reset()
    term.hide_cursor()      term.show_cursor()
    term.raw(true/false)    enter/leave raw mode (no echo, no line buffering)
    term.key()              non-blocking: next key, or "" if none pressed
    term.getch()            blocking: wait for and return one key
    term.size()             [columns, rows]

GRAPHICS (offline image rendering to a PPM file)
    gui.window(title, w, h)             creates an in-memory RGB canvas
    gui.draw_pixel(x, y, color)         color is 0xRRGGBB
    gui.fill(color)                     fill the whole canvas
    gui.rect(x, y, w, h, color)         filled rectangle
    gui.line(x0, y0, x1, y1, color)     line (Bresenham)
    gui.save(path)                      write the canvas to a .ppm file
    gui.refresh()                       write the canvas to fc_canvas.ppm

DATA FORMATTING
    json.parse(text)                    JSON text -> FC value
    json.stringify(value)               FC value -> JSON text

COLOR (0xRRGGBB integers, for gui/term/win)
    color.rgb(r, g, b)                  pack 0-255 channels into an int
    color.hex("#ff8800")                parse a hex color string
    color.hsv(hue, sat, val)            HSV (hue 0-360, sat/val 0-1) -> rgb
    color.red color.green color.blue color.white color.black
    color.yellow color.cyan color.magenta color.gray color.orange

PATH (filename helpers)
    path.join(a, b, ...)                join parts with "/"
    path.basename(p)                    final component
    path.dirname(p)                     directory part
    path.ext(p)                         extension incl. dot, or ""

PACKAGE MANAGER
    pkg.install(name)                   report whether a local file is present
    pkg.install(name, url)              download a package into ./packages/
    pkg.installed(name)                 true if name resolves in the path
    pkg.list()                          list package names in ./packages/


---------------------------------------------------------------------------
 7. COMPLETE EXAMPLE PROGRAMS
---------------------------------------------------------------------------

--- Object-Oriented Programming -------------------------------------------

    class Counter
        func init(start)
            let this.value = start
        func bump(by)
            let this.value = this.value + by
        func show()
            out "count = " + this.value

    let c = Counter(10)
    c.bump(5)
    c.bump(7)
    c.show()                + prints: count = 22


--- Functional style with for / map / filter / reduce ---------------------

    func square(x)
        return x * x
    func is_odd(x)
        return x % 2 == 1
    func sum(a, b)
        return a + b

    let data = range(1, 6)             + [1,2,3,4,5]
    out "squares: " + fn.map(square, data)
    out "odds:    " + fn.filter(is_odd, data)
    out "total:   " + fn.reduce(sum, data, 0)

    for word in ["fast", "tiny", "safe"]
        out "feature: " + word


--- AI arrays / linear regression -----------------------------------------

    let xs = [1, 2, 3, 4, 5]
    let ys = [2, 4, 6, 8, 10]
    let model = ai.train_linear(xs, ys)
    out "slope = " + model["m"]
    out "predict(10) = " + ai.predict(model, 10)
    out "A . B = " + ai.dot([[1, 2], [3, 4]], [[5, 6], [7, 8]])


--- Threads (cooperative spawn) --------------------------------------------

    func work(label)
        loop 3 times
            out "task " + label
    thread spawn work("A")
    thread spawn work("B")
    out "all tasks dispatched"


--- File manipulation + JSON ----------------------------------------------

    dict config = {"app": "FC", "version": 1, "features": ["fast", "tiny"]}
    file.write("config.json", json.stringify(config))
    let parsed = json.parse(file.read("config.json"))
    out "app name: " + parsed["app"]
    out "first feature: " + parsed["features"][0]


--- Command-line tool with os + str ---------------------------------------

    let args = os.args()
    out "running on " + os.platform()
    out "from " + os.cwd()
    for a in args
        out "arg: " + text.upper(a)


--- Error recovery ---------------------------------------------------------

    func safe_divide(a, b)
        try
            return a / b
        catch err
            out "recovered: " + err
            return 0
    out safe_divide(10, 2)      + 5
    out safe_divide(10, 0)      + recovered: Division by zero, then 0


--- Hashing & base64 -------------------------------------------------------

    out crypto.hash("sha256", "hello")
    + 2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824
    out crypto.base64_encode("hello FC")     + aGVsbG8gRkM=
    out crypto.base64_decode("aGVsbG8gRkM=") + hello FC


---------------------------------------------------------------------------
 8. BUILDING WEBSITES, GAMES & APPS
---------------------------------------------------------------------------

WEBSITES & WEB APPS  (cross-platform: Linux, Windows, Android, anything)
    FC has a real built-in HTTP server. The handler you pass to web.serve
    receives each request, and you return the page (or a {status,type,body}
    dict). This is a genuine, dynamic website -- routing, query params,
    forms, and JSON APIs all work. See the included demo:

        fc website.fc       then open http://localhost:8080

    Because the front-end is plain HTML/CSS/JS in the browser, the SAME FC
    program gives you a GUI on every platform with a browser -- including
    Android. This is the realistic, recommended way to build a graphical,
    cross-platform "app" with FC (the same approach VS Code, Slack and
    Discord use). Serve a folder of static files with web.serve_dir.

TERMINAL GAMES & TUIs  (Linux, macOS, Android/Termux, Windows)
    term.raw, term.key/getch, term.clear and term.at give you real-time
    keyboard input and screen control -- enough for Snake, roguelikes,
    editors, dashboards, and menus. See the included demo:

        fc game.fc          (move with W A S D, quit with Q)

IMAGES & GENERATED GRAPHICS
    The gui.* drawing primitives render to an in-memory canvas you save as
    a .ppm image (convert with ImageMagick: 'pkg install imagemagick' then
    'convert art.ppm art.png'). Good for sprites, charts, procedural art,
    and game assets. See the included demo:

        fc art.fc           writes art.ppm

SUPPORTED PLATFORMS (be realistic about each)
    Linux / macOS / Termux(Android) / Windows:
        Fully supported. The interpreter is portable C; build it with the
        Makefile (or cc directly) and run .fc scripts and the web server.
    Android (as a real app):
        The interpreter core is plain C and compiles with the Android NDK
        as a native library you can embed in an app via JNI. Running scripts
        inside Termux works out of the box today.
    iOS / iPadOS:
        The C core compiles with the iOS toolchain (Xcode/clang) and can be
        embedded in an app as a static library. Shipping an actual App Store
        app needs Xcode on a Mac plus Apple code-signing, and Apple restricts
        apps that download-and-run code -- so bundle your .fc scripts inside
        the app rather than fetching them at runtime.
    "Runs everywhere" honestly means: the language + standard modules port to
    all of the above. For one graphical UI across every platform, use the web
    approach (web.serve + a browser front-end). For native on-screen windows,
    use the separate FCdependency/ build (links SDL2) -- Linux, Windows,
    macOS, and Android-via-Termux+X11.

NOTE ON NATIVE DESKTOP / ANDROID GUI TOOLKITS
    FC deliberately has NO native windowing toolkit (no X11/Win32/Cocoa
    windows, no Android Activities). Those require platform-specific
    libraries and SDKs, which would break FC's pure-C, zero-dependency,
    single-binary design, and no one code path spans Linux + Windows +
    Android. For a graphical app that truly runs everywhere, use the web
    approach above (web.serve + an HTML/CSS/JS front-end). For terminal
    apps, use the term.* module.


---------------------------------------------------------------------------
 9. SAFETY & MEMORY MODEL
---------------------------------------------------------------------------
  * All heap values are reference counted and freed automatically when the
    last reference is dropped; environments release their bindings on exit.
  * Errors use a handler stack (setjmp/longjmp). try/catch installs a
    handler; uncaught errors unwind to the top level, print
    "FC Runtime Error on Line X: <message>", and exit with code 1.
  * The interpreter is designed never to segfault on bad input: syntax
    errors, indentation errors, undefined names, type mismatches, bad
    indices, division by zero, and file/network failures are all reported
    gracefully.
  * Closures capture the global scope; reference cycles created through
    object fields are not collected (acceptable for short-lived scripts).

===========================================================================
