sntpclient, by 2012 Eric Belanger <bilange@hotmail.com>
Free to use, in beer, speech and ducks. Hack away! :-)

If you use this software or code, you acknowledge there are no warranties, no
hassle, nothing attached to this project.  YOU'RE ON YOUR OWN! I am not
responsible for explosions, burned toasts or unexpected punches in the face.
Your miles may vary. Objects in mirror are closer than they appear. 

On a slighty more serious note (not that it wasn't serious enough already),
this has been educational as far as i'm concerned (I am nowhere near a
professional C coder), and I hope it will be useful in some way to you too. If
it did, I would be glad to hear your stories. If you use parts of my source
code somewhere else, I would like (but not require) you to add my name in the
project's credits or thanks section.

-------

This is a *simple*, small sntp client written in C that I used to get replies
from a trustworthy server on my local network. I don't need scientific
precision, so I went for a single query-reply usage without double-checking
what the server sends back. Crazy, I know.  I used this project in a lab of
Windows 2000 workstations since its internal SNTP client isn't really reliable.
Yes, people still use Windows 2000.

USAGE: 
sntpclient.exe [-f] [-s] <hostname>
           -f      force a local time update
                   even if SNTP server isnt trustworthy
           -s      silent mode (no text on the console)

Where <hostname> can be an IP address or a hostname.

This has been compiled with mingw binaries and librairies, with MinGW's gcc
4.4.1.

