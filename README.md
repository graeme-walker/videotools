VideoTools
==========

What is it?
-----------
VideoTools is software for video surveillance that can record, analyse and
replay images from webcams and network cameras. It runs on Linux and BSD
Unix, and it is licensed as Open Source under the GPLv3.

It can use direcly-attached webcams or network cameras that provide JPEG or
H.264 video streams over HTTP or RTP.

It is not suitable for entertainment or video conferencing applications
because there is no audio or strict time synchronisation.

VideoTools does some of the same things as "ZoneMinder" and other programs,
so check them out too.

How does it work?
-----------------
VideoTools is made up of several co-operating programs that communicate over
the network or through shared-memory channels.

The shared-memory publication channels are used for publishing video from
one program to another running on the same machine. Each channel has one
publisher but any number of subscribers, and once the publisher terminates
the channel is deleted.

The Linux kernel's "V4L" infrastructure is used to communicate with
directly-attached webcams, and standard HTTP or RTSP/RTP protocols are used
to communicate with network cameras. Network cameras must use JPEG or H.264
encoding, and videotools uses the "libav" library to handle the H.264
decoding.

Simple motion detection works by looking for pixels that change by more than
a set amount from one frame to the next, and then counting those pixels to
see if they are more than some threshold number. Motion detection events can
be used to control the recording process, or to raise alarms.

Video streams are recorded to disk with separate files for each video frame,
organised by date and time in a directory hierarchy.

A single "viewer" program is used for displaying video in a window. Web 
browsers can also be used for viewing video served up by the 
`vt-httpserver` web server.

Hardware support
----------------
Hardware support for webcams on Linux is dependent on the V4L framework, and
other websites should give you an idea whether your hardware will work with
V4L. Using the "libv4l" shim library (`./configure --with-libv4l`) might help 
with more obscure webcam data formats.

Network cameras must use either HTTP or RTP. When using HTTP the camera
must return one JPEG image for each HTTP GET, or generate a JPEG image stream 
using the `multipart/x-mixed-replace` content type. When using RTP the camera 
must return video images with JPEG or H.264 encoding.

Build and install
-----------------
The system can be built and installed with the usual rubric:

	./configure ; make ; sudo make install

Use your favourite package management tool (apt, pkg, or whatever) to install
the following dependencies before running the `configure` script:

+ libav
+ libjpeg
+ xlib
+ libncurses
+ libv4l
+ v4l_compat (BSD)
+ webcamd (BSD)

On some BSD systems you will have to add `CPPFLAGS` and `LDFLAGS` options to the
`configure` command to pick up the header files and libraries installed under
`/usr/local` or `/opt`. Refer to the `INSTALL` file for more information, or
just use the `configure.sh` script instead of `configure`.

If there are missing dependencies the build will still go ahead but some
features will not be available, so please look for warnings in the output
from the `configure` script.

The "start/stop" script, which is normally installed under `/etc/init.d` on
Linux or `/usr/local/etc/rc.d` on BSD, can be used to start up the server
processes, but you must edit its configuration file to tell it exactly what
to run.

On BSD systems enable the start/stop script by adding the following line to
`/etc/rc.conf`:

	videotools_enable="YES"

Instructions for the impatient
------------------------------
+ Use `vt-httpclient` and `vt-webcamplayer` to obtain
video from network cameras and webcams, and send each video stream to its own
publication channel.

+ Forward video streams across the network by running `vt-httpserver`
on one side and `vt-httpclient` on the other. Use
`--channel=<channel>` on the server's command-line and `vt://<address>`
on the client's.

+ Record video to disk by running `vt-recorder` for each channel.
Use a different base directory for each channel, and give each recorder a
control socket.

+ Do motion detection for each channel using `vt-watcher`, and let each
watcher control its channel's recorder via the recorder's control socket.

+ Run `vt-fileplayer` to replay each channel's recordings to a playback
channel. Give each fileplayer a control socket.

+ Run a central `vt-httpserver` web server with `--channel=*` and
`gateway=localhost` on the command-line.

+ Use a web browser with URL paths like `_0`, `_1` etc. to view each channel,
and add `?streaming=1` if your browser supports `multipart/x-mixed-replace`.
Check out the example HTML code for HTML5 streaming techniques and
user-interface ideas.

Getting started
---------------
Once the videotools system is installed you can try out some of the
components by running them from a terminal.

Start with the "fileplayer", and specify a directory that contains some
static picture files:

	vt-fileplayer --fade /usr/share

This should show all the images in succession and then stop. If running
without X Windows then the output will use text characters as pixels filling
the whole terminal, so stand back and squint, and use Control-C to quit.

Then try the "webcamplayer" in its test mode:

	vt-webcamplayer --device=test

This should show window with a test pattern of moving coloured bars.

Next try viewing video from a webcam, or install the Linux "vivi" module if
you do not have a webcam attached:

	sudo modprobe vivi
	vt-webcamplayer --retry=0 --device=/dev/video0

You might need to try different device names (`/dev/video1` etc.) to find the
webcam output. You will need root permissions to install the vivi module.

Then try viewing the webcam video using a web browser. For this you will need
to run the "webcamserver", listening on a free TCP port:

	vt-webcamserver -v --port=8080 --device=/dev/video0

Tell your web browser to connect to the server by putting
`http://127.0.0.1:8080` into the address bar. This should show a static image
from the webcam.

If your web browser supports the `multipart/x-mixed-replace` content type
you can get moving video by adding `/?streaming=1` to the URL in the address 
bar. Alternatively, add `--refresh=1` to the "webcamserver" command-line to 
get a one second refresh:

	vt-webcamserver -v --refresh=1 --port=8080 --device=/dev/video0

Next you can try viewing the output from a network camera, but you will need
to know the full URL for getting JPEG images over HTTP, including any
username and password. If you have this information then just run the
"httpclient" program with the URL on the command-line, eg:

	vt-httpclient -v --viewer http://admin:secret@192.168.0.3/streaming/channels/1/httppreview

Add the `--scale=3` option if the window is too big.

Now try sending video to a publication channel. Use the "httpclient", if
that worked, or use "webcamplayer". In either case add `--channel=camera`:

	vt-httpclient --channel=camera http://...

	vt-webcamplayer --channel=camera --device=demo

View the channel using the "viewer" program:

	vt-viewer camera

Try recording the video from the "camera" channel by running a "recorder",
using `/tmp` as the base directory:

	vt-recorder -v camera /tmp

Check for the recorded images under a sub-directory named after the current
calendar year, and replay the recorded video with the "fileplayer":

	find /tmp/20?? -type f
	vt-fileplayer --loop /tmp/20??

Make sure you kill the recorder before it has a chance to fill up your disk.

Now try doing motion detection on the "camera" channel by running the
"watcher" program:

	vt-watcher -v --viewer camera

You should see a dim, monochrome image with bright green highlights where the
image is changing.

Create a mask file to define which regions of the image are used for motion
detection. Run the "maskeditor" program using the "camera" channel to provide
the base image:

	vt-maskeditor --image-channel=camera /tmp/mask

Use the mouse to define the masked regions and close the window when you
are done. The new mask file can now be used by the "watcher":

	vt-watcher -v --viewer --mask=/tmp/mask camera

You should see the masked areas in dark red, and the motion detection
spots in bright green.

Finally, if you have a network camera that can send over RTP and you know its
URL scheme, try using the "rtspclient" program to encourage it to transmit:

	vt-rtscpclient -v rtsp://admin:secret@192.168.0.3:554/streaming/channels/1

While the "rtspclient" is still running check that you are getting RTP 
packets on UDP port 8000 by using "netcat":

	nc -l -u -4 0 8000 | base64

And then try viewing the RTP video stream using the "rtpserver":

	vt-rtpserver -v --viewer --scale=auto --port=8000

Configuration
-------------
The videotools system is normally configured through the system's `init`
system. For each videotools process that you want to run you should construct
a complete command-line containing all the relevant command-line options.
The options available for each videotools program are described in its `man`
page.

For traditional script-based `init` systems a videotools start/stop script is
installed as `/etc/init.d/videotools` on Linux, and this reads a configuration 
file `/etc/videotools` containing the list of "run" command-lines. Optional
overrides for directory paths etc. are taken from a file
`/etc/defaults/videotools`.

On BSD systems the relevant files are `/etc/rc.d/videotools`,
`/etc/videotools` and `/etc/rc.conf.d/videotools`, but possibly under
`/usr/local` or `/opt`.

As an example, consider a machine with just one webcam that needs to run a
"webcamserver" to capture the video from the webcam and serve it up over
HTTP. Its configuration file would have one "run" line, like this:

	# one webcam serving up jpeg video over http on port 80
	run vt-webcamserver --port 80 /dev/video0

A central file-server machine doing recording, motion detection and
web-serving might look more like this:

	# read from network camera "front" using h.264 encoding over rtp
	run vt-rtspclient --port=8000 --format-file=$RUNDIR/camera.cfg  rtsp://admin:secret@192.168.0.2/streaming/channels/1
	run vt-rtpserver --port=8000 --format-file=$RUNDIR/camera.cfg --scale=auto --channel=front
	
	# read from network camera "rear" using jpeg encoding over http
	run vt-httpclient --channel=rear http://192.168.0.3/streaming/channels/2

	# read from remote webcam "inside" by talking to its webcamserver
	run vt-httpclient --channel=inside vt://rpi.localnet:80

	# record the three video streams to disk
	run vt-recorder --command-socket=$RUNDIR/front.s front $SHAREDIR/front
	run vt-recorder --command-socket=$RUNDIR/rear.s rear $SHAREDIR/rear
	run vt-recorder --command-socket=$RUNDIR/inside.s inside $SHAREDIR/inside

	# motion detection, controlling the recorders
	run vt-watcher --recorder=$RUNDIR/front.s --event-channel=front-events front
	run vt-watcher --recorder=$RUNDIR/rear.s --event-channel=rear-events rear
	run vt-watcher --recorder=$RUNDIR/inside.s --event-channel=inside-events inside

	# alarms from motion detection events
	run vt-alarm front-events /usr/bin/aplay /usr/share/sounds/dog.wav
	run vt-alarm rear-events /usr/local/bin/my-alarm.sh
	run vt-alarm inside-events /usr/bin/wall

	# file players, for replaying recordings
	run vt-fileplayer --channel=front-replay --command-socket=udp://0:3001 $SHAREDIR/front
	run vt-fileplayer --channel=rear-replay --command-socket=udp://0:3002 $SHAREDIR/rear
	run vt-fileplayer --channel=inside-replay --command-socket=udp://0:3003 $SHAREDIR/inside

	# main web server, to allow viewing from a html5 web-browser
	run vt-httpserver --gateway=localhost --port=8080 --dir=$SHAREDIR --channel=*

Viewing
-------
The video passing through a channel can be viewed by simply running the
"viewer" program on the same machine as the channel.

	vt-viewer somechannel

Make sure you are using an account that has permission to access the
channel's shared memory.

If X-Windows is not available then the viewer program will try to display
the images in text mode, using characters as pixels.

To view the video images from a channel on another machine you should use
`ssh -X` (or `ssh -Y` from MacOS).

	ssh -X somebox.localnet vt-viewer somechannel

The "httpserver" program can also be used to serve up video from a channel,
so that it can be viewed using a web browser.

Snapshot images can be extracted from a channel by using the "channel"
utility and then viewed with a graphics program, perhaps after using
ImageMagick tools to change the image format.

	vt-channel --out=temp.dat read somechannel
	convert temp.dat temp.jpg
	gimp temp.jpg

Recording and playback
----------------------
The "recorder" program is used to record video from a publication channel.

For example, to record from the "camera1" channel to a directory tree under
`/data/video`, start up a recorder like this:

	vt-recorder camera1 /data/video/camera1

The "fileplayer" program can be used to play back the recordings for a
particular date:

	vt-fileplayer /data/video/camera1/2017/12/31

Motion detection
----------------
The "watcher" program can be used for motion detection. It monitors video
from one channel and emits motion detection events on another.

The "alarm" program can be used to react to the motion detection events,
perhaps by playing a sound or sending an email.

	vt-watcher --channel=camera1-events camera1
	vt-alarm camera1-events aplay alarm.wav

You will normally need a mask file to inhibit motion detection in certain
parts of the video image.

	vt-maskeditor --image-channel=camera1 camera1-mask.pbm
	vt-watcher --mask=camera1-mask.pbm --viewer --event-channel=camera1-events camera1

The watcher can control the recorder by using the recorder's
`--command-socket` option. Motion detection will then cause the recorder to
start recording in its "fast" mode.

	vt-recorder --command-socket=/var/run/recorder1.s --state=stopped --timeout=20 camera1 /data/video/camera1
	vt-watcher --recorder=/var/run/recorder1.s camera1

There are two main parameters for the motion detection algorithm: "squelch"
and "threshold". Refer to the "watcher" documentation for the details.

Security and privacy
--------------------
Always use firewalls and encrypted tunnels to protect your privacy when
exposed to the public internet. Also consider the following points.

+ Use the `--address` option to restrict the listening ports to a single
network interface.

+ Note that usernames and passwords in URLs on the command-line will be widely
visible on the local machine using `ps`.

+ Operating system resources such as files (excluding pid files), directories,
local-domain sockets, shared memory segments, semaphores etc. are all created
with permissions restricted by the inherited "umask", so make sure that the
umask in the videotools startup script (or equivalent) is set
appropriately.

+ Consider creating a special user group to control access to the videotools
resources more precisely: create a new group and make all the videotools
executables group-suid (for example), and then add user accounts to the group
in order to grant access to the videotools resources. Note that the `--user`
option does not affect resource ownership, although it can indirectly affect
the ownership of files created by an external program run by
`vt-alarm`.

+ Local-domain sockets are created whenever a program subscribes to a
publication channel and these sockets are briefly bound to a filesystem
path of `/tmp/vt-<channel>.<pid>`. However, they can be put in a
more private directory by prefixing the channel name on the subscriber's side
using an extended format of `<dir>/<name>/<channel>`, to give socket paths
of `<dir>/<name>.<pid>`.

+ The `vt-alarm` program uses the `system()` function to execute the program
you specify on the command-line. This provides a lot of flexibility for
output redirection, back-ticks, environment variable substitution etc, but
it also presents a large attack surface if the program's configuration or
run-time environment is compromised.

IPv6
----
IPv6 is used whenever an address uses an IPv6 format or whenever a hostname
resolves to an IPv6 address.

Programs that use the `--port` option for the port number also have an
`--address` option for the network address, so adding `--address=::` will
switch from IPv4 to IPv6 since "::" is the IPv6 wildcard address. Similarly
for programs with a `--command-socket` option the full IPv6 transport address
can be given, so for example using `--command-socket=:::9000` will switch
the socket to IPv6 port 9000.

IPv6 addresses can be used in URLs (eg. `vt-httpclient http://::1:80/x`),
but they will generally also need square brackets to avoid ambiguity with
respect to port numbers, eg.  `http://[fc00::1]/x`.

For hostnames that can resolve to IPv4 or IPv6 addresses the first returned
address is used. (Refer to `getaddrinfo()`'s `AI_ADDRCONFIG` option for more
details.)

RTP multicast features are only relevant to IPv4.

Connections
-----------
When videotools processes start up they usually wait for their primary inputs
to become available, whether that's a webcam device, publication channel, or
network peer; and if that input subsequently disappears then they will go
back to a waiting state. This means that a set of co-operating processes can
be started up without worrying about the order in which they connect to each
other, and it makes is easy to restart individual components without restarting
the whole videotools system.

However, this default behaviour can be modified by `--retry` and `--once`
command-line options: if `--retry=0` is used then the program will exit
immediately if the input resource is not available, and if `--once` is used 
then then any disappearance of a previously active input causes the program to 
exit. These options might be useful if the videotools processes are being 
managed by an "init" system that knows about their resource dependencies, or 
if they are operating in a stable, low-maintenance system where failing fast 
might be desirable.

The connectivity of an established set of local videotools processes can be
deduced from metadata held within publication channels. Each channel has a
small structure describing the publisher process and a set of "slot" 
structures where the channel subscribers register themselves.

Channel metadata can be seen by using the "channel" utility:

	vt-channel -q list | xargs -n1 vt-channel info

The "httpserver" program can also make the channel metadata available across 
the network via the special double-underscore url path ("__"):

	vt-httpserver --port=8080 --channel=\*
	wget -q http://localhost:8080/__ -O channels.json

Troubleshooting
---------------
If you encounter problems then stop all the videotools processes, and then
run only the relevant videotools program from the command-line. Use the the
`--verbose` option to get extra log messages, and do not use the `--daemon`
option. If the problems seem to be related to permissions then run your
tests as "root" by using `sudo` or equivalent.

Add the `--scale` option wherever possible to reduce the image sizes during
your tests. This is particularly important if you see dropped RTP packets or
corrupted H.264 video from the "rtpserver".

Work through the "Getting started" section (above) and vary the steps to get
closer to the problem you are seeing.

Program aborts can cause publication channels to become congested or invalid
in some way. Use the "channel" utility to purge or delete them. As a last
resort carefully remove files from the `/dev/shm` directory (Linux only and
at your own risk), or reboot.

	vt-channel list
	vt-channel purge somechannel
	vt-channel delete somechannel

Problems with USB webcams can sometimes be resolved by unplugging them and
the plugged them back in, particularly if you get weird errors on startup.

Wireshark can be useful for diagnosing network problems and for figuring out
the capabilities of network cameras; the `Analyze->FollowTcpStream` option
can be particularly productive for HTTP.

The "vlc" media player is useful for experimenting with RTSP/RTP.

If you have corruped webcam images then experiment with adding `;fmt=x` to
the device name, where x is a small integer, eg. `/dev/video0;fmt=10`. (Use
a backslash or single-quotes to protect the semi-colon.) Also try building
with and without "libv4l".

If JPEG video over RTP looks really bad then try using the "rtpserver"
program's `--jpeg-table` option.

Programs
--------
The following sections describe the videotools programs in more detail.

Program vt-alarm
----------------
Monitors a watcher's output channel and runs an external command on 
each motion-detection event.

For example, an "alarm" process can be used to generate an alarm sound 
when motion detection is triggered, by running an external command 
like "aplay".

If the `--run-as` option is given then the external command runs with
that account's user-id and group-id if possible.

If you need pipes, backticks, file redirection, etc. in your external 
command then add the "--shell" option. Awkward shell escapes can 
avoided by using "--url-decode".

### Usage

	vt-alarm [<options>] <event-channel> [<command> [<arg> ...]]

### Options

	--verbose             verbose logging
	--threshold=<pixels>  alarm threshold in pixels per image
	--shell               use '/bin/sh -c'
	--url-decode          treat the command as url-encoded

Program vt-channel
------------------
A tool for working with publication channels.

The following sub-commands are available: "list" lists all local
publication channels; "info" displays diagnostic information for 
the channel; "read" reads one message from the channel; "peek"
reads the last-available message from the channel without
waiting; "purge" clears subscriber slots if subscribers have 
failed without cleaning up; and "delete" deletes the channel,
in case a publisher fails without cleaning up.

### Usage

	vt-channel [<options>] {list|info|purge|delete|peek|read} <channel>

### Options

	--all             include info for unused slots
	--out=<filename>  output filename
	--quiet           fewer warnings

Program vt-maskeditor
---------------------
Creates and edits a mask file that can be used by the `vt-watcher` program.

The `vt-watcher` program analyses a video stream for moving objects, and it uses
a mask file to tell it what parts of the image to ignore.

The mask is edited over the top a background image, and that image can come 
from a static image file or from a publication channel (`--image-channel`).

Click and drag the mouse to create masked areas, and hold the shift key to
revert. The mask file is continuously updated in-place; there is no "save" 
operation.

### Usage

	vt-maskeditor [<options>] <mask-file>

### Options

	--verbose                   verbose logging
	--image-file=<path>         background image file
	--image-channel=<channel>   channel for capturing the initial backgound image
	--viewer-channel=<channel>  override the default viewer channel name

Program vt-fileplayer
---------------------
Plays back recorded video to a publication channel or into a viewer window.

The directory specified on the command-line is a base directory where the 
video images can be found. 

The base directory can also be specified using the `--root` option, in which 
case the directory on the end of the command-line is just the starting point
for the playback. If the `--rerootable` and `--command-socket` options are
used then the root directory can be changed at run-time by sending an
appropriate `move` command.

In interactive mode (`--interactive`) the ribbon displayed at the bottom of 
the viewer window can be used to move forwards and backwards through the 
day's recording. Click on the ribbon to move within the current day, or click
on the left of the main image to go back to the previous day, or on the right
to go forwards to the next.

The `--skip` option controls what proportion of images are skipped during
playback (speeding it up), and the `--sleep` option can be used to add a 
delay between each displayed image (slowing it down). A negative skip value
puts the player into reverse.

A socket interface (`--command-socket`) can be used to allow another program
to control the video playback. Sockets can be local-domain unix sockets or 
UDP sockets; for UDP the socket should be like `udp://<address>:<port>`, eg. 
`udp://0:20001`. 

Command messages are text datagrams in the form of a command-line, with 
commands including `play`, `move {first|last|next|previous|.|<full-path>}`, 
`ribbon <xpos>` and `stop`. The play command can have options of `--sleep=<ms>`, 
`--skip=<count>`, `--forwards` and `--backwards`. The `move` command can 
have options of `--match-name=<name>` and `--root=<root>`, although
the `--root` option requires `--rerootable` on the program command-line.
Multiple commands in one datagram should be separated by a semi-colon.

The timezone option (`--tz`) affects the ribbon at the bottom of the display 
window so that that the marks on the ribbon can span a local calendar day 
even with UTC recordings. It also changes the start time for each new day 
when navigating one day at a time. A positive timezone is typically used for
UTC recordings viewed at western longitudes.

If multiple recorders are using the same file store with different filename
prefixes then the `--match-name` option can be used to disentangle the
different recorder streams. Note that this is not the recommended because 
it does not scale well and it can lead to crazy-slow startup while looking 
for a matching file. The match name can be changed at run-time by using the
`--match-name` option on a `move` command sent to the command socket.

### Usage

	vt-fileplayer [<options>] <directory>

### Options

	--verbose                   verbose logging
	--viewer                    run a viewer
	--interactive               respond to viewer mouse events
	--viewer-channel=<channel>  override the default viewer channel name
	--channel=<channel>         publish the playback video to the named channel
	--sleep=<ms>                inter-frame delay time for slower playback
	--skip=<count>              file skip factor for faster playback
	--command-socket=<path>     socket for playback commands
	--root=<path>               base directory
	--rerootable                allow the root to be changed over the command-socket
	--stopped                   begin in the stopped state
	--loop                      loop back to the beginning when reaching the end
	--scale=<divisor>           reduce the image size
	--tz=<hours>                timezone offset for ribbon
	--match-name=<prefix>       prefix for matching image files
	--monochrome                convert to monochrome
	--width=<pixels>            output image width
	--height=<pixels>           output image height
	--fade                      fade image transitions

Program vt-httpclient
---------------------
Pulls video from a remote HTTP server or network camera and sends it to a 
publication channel or a viewer window.

Uses repeated GET requests on a persistent connection, with each GET request
separated by a configurable time delay (`--request-timeout`). A watchdog 
timer tears down the connection if no data is received for some time and 
then the connection is retried. Watchdog timeouts and peer disconnections 
result in connection retries; connection failures will terminate the 
program only if using `--retry=0`.

The server should normally return a mime type of `image/jpeg`, or 
preferably `multipart/x-mixed-replace` with `image/jpeg` sub-parts.
In the latter case the client only needs to make one GET request and
the server determines the frame rate.

When getting video from a network camera the exact form of the URL to use 
will depend on the camera, so please refer to its documentation or
search the internet for help.

To get video from a remote webcam use the `vt-webcamserver` on the remote
maching and use `vt-httpclient` to pull it across the network.

As a convenience, a URL of `vt://<address>` can be used instead of `http://<address>`
and this is equivalent to `http://<address>/_?streaming=1&type=any`.

### Usage

	vt-httpclient [<options>] <http-url>

### Options

	--verbose                       verbose logging
	--viewer                        run a viewer
	--scale=<divisor>               reduce the viewer image size
	--viewer-title=<title>          viewer window title
	--watchdog-timeout=<seconds>    watchdog timeout (default 20s)
	--request-timeout=<ms>          time between http get requests (default 1ms)
	--channel=<channel>             publish to the named channel
	--connection-timeout=<seconds>  connection timeout
	--retry=<seconds>               retry failed connections or zero to exit (default 1s)
	--once                          exit when the first connection closes

Program vt-httpserver
---------------------
Reads video from one or more publication channels and makes it available 
over the network using HTTP.

The HTTP client can either request individual images ("client-pull") or
receive a continuous JPEG video stream delivered as a 
`multipart/x-mixed-replace` content type ("server-push").

For server-push add a `streaming=1` parameter to the URL, eg. 
`http://localhost:80/?streaming=1`. This should always be used when the 
client is the `vt-httpclient` program, but be careful when using a browser 
because not all of them support `multipart/x-mixed-replace`, and some of 
them go beserk with "save-as" dialog boxes.

The client-pull model works best with a browser that uses JavaScript code
to generate a continuous stream of GET requests, typically using the 
"Fetch" API.

For browsers that do not have JavaScript or support for `multipart/x-mixed-replace`
a `refresh=1` URL parameter can be used so that the server sends
back static images with a HTTP "Refresh" header, causing the browser to 
repeat the GET requests at intervals. This can also be made the default
behaviour using the `--refresh` command-line option on the server.

A `scale` URL parameter can be used to reduce the size of the images, and
a `type` parameter can be used to request an image format (`jpeg`, `pnm`, or
`raw`, with `jpeg` as the default). The type can also be `any`, which means 
that no image conversion or validation is performed; whatever data is 
currently in the publication channel is served up.

Static files can be served up by using the `--dir` option. Normally any file
in the directory will be served up, but a restricted set of files can be 
specified by using the `--file` option. The `--file-type` option can be used
to give the files' content-type, by using the filename and the content-type 
separated by a colon, eg. `--file-type=data.json:application/json`. The default 
file that is served up for an empty path in the url can be modified by the 
`--default` option, eg. `--default=main.html`.

One or more channel names should normally be specified on the command-line
using the `--channel` option. Clients can select a channel by name by
using URL paths with an underscore prefix (eg. `_foo`, `_bar`); or by index 
by using URL paths like `_0`, `_1`, etc. 

If `--channel=*` is specified then any valid channel name can be used in 
the URL path. This also enables the special `/__` url that returns channel 
information as a JSON structure.

The `--gateway=<host>` option can be used to integrate static web pages
with UDP command sockets; HTTP requests with a `send` parameter of the
form `<port> <message>` are sent as UDP messages to the `--gateway` IP 
address and the specified port.

### Usage

	vt-httpserver [<options>]

### Options

	--port=<port>                 listening port
	--timeout=<seconds>           network idle timeout (default 60s)
	--data-timeout=<seconds>      initial image timeout (default 3s)
	--repeat-timeout=<ms>         image repeat timeout (default 1000ms)
	--address=<ip>                listening address
	--dir=<dir>                   serve up static files
	--file=<filename>             allow serving the named file 
	--file-type=<fname:type>      define the content-type of the the named file 
	--default=<fname-or-channel>  define the default resource
	--channel=<name>              serve up named video channel
	--gateway=<udp-host>          act as a http-post-to-udp-command-line gateway
	--refresh=<seconds>           add a refresh header to http responses by default

Program vt-recorder
-------------------
Reads a video stream from a publication channel and records it to disk.

Individual video frames are stored in a directory tree, organised by date
and time.

Images are recorded as fast as they are received, or once a second, or not 
at all. These are the fast, slow and stopped states.

The state can be changed by sending a `fast`, `slow`, or `stopped` command 
over the recorder's command socket (`--command-socket`).

The fast state is temporary unless the `--timeout` option is set to zero; 
after recording in the fast state for a while the state drops back to the 
previous slow or stopped state. 

The recorder enters the fast state on startup (`--fast`) or when it gets a 
`fast` command over its command socket, typically sent by the `vt-watcher` 
program. On entering the fast state any in-memory images from the last few
seconds are also committed to disk.

A sub-directory called `.cache` is used to hold the recent images that have 
not been commited to the main image store. These files are held open, and 
they should not be accessed externally. When the recorder is triggered
by a `fast` command on the command socket the cached files are moved
into the main image store so that they provide a lead-in to the event
that triggered the recording.

On machines with limited memory the cache size should be tuned so that
there is no disk activity when the recorder is in the "stopped" state.
The cache can be completely disabled by setting the cache size to zero, 
but then there will be no lead-in to recordings triggered by an event.

Use `--fast --timeout=0 --cache-size=0` for continuous recording of a 
channel.

The `--tz` option is the number of hours added to UTC when constructing
file system paths under the base directory, so to get local times at western
longitudes the value should be negative (eg. `--tz=-5`). However, if this 
option is used inconsistently then there is a risk of overwriting or 
corrupting old recordings, so it is safer to use UTC recordings and 
add `--tz` adjustments elsewhere.

Loopback filesystems are one way to put a hard limit on disk usage. On 
Linux do something like this as root 
`dd if=/dev/zero of=/usr/share/recordings.img count=20000`,
`mkfs -t ext2 -F /usr/share/recordings.img`, 
`mkdir /usr/share/recordings`,
`mount -o loop /usr/share/recordings.img /usr/share/recordings`,
add add the mount to `/etc/fstab`.

Individual video streams should normally be recorded into their own directory
tree, but the `--name` option allows multiple video streams to share the same
root directory by adding a fixed prefix to every image filename. However,
this approach does not scale well on playback, so only use it for a very
small number of video streams.

### Usage

	vt-recorder [<options>] <input-channel> <base-dir>

### Options

	--verbose                verbose logging
	--scale=<divisor>        reduce the image size
	--command-socket=<path>  socket for commands
	--file-type=<type>       file format: jpg, ppm, or pgm
	--timeout=<s>            fast-state timeout
	--fast                   start in fast state
	--state=<state>          state when not fast (slow or stopped, slow by default)
	--tz=<hours>             timezone offset
	--cache-size=<files>     cache size
	--name=<prefix>          prefix for all image files (defaults to the channel name)
	--retry=<timeout>        poll for the input channel to appear
	--once                   exit if the input channel disappears

Program vt-rtpserver
--------------------
Listens for an incoming RTP video stream and sends it to a local publication 
channel. 

Some network cameras use RTP rather than HTTP because it is more suited to 
high resolution video, especially when using H.264 encoding, and it also 
allows for IP multicasting.

Only the "H264/90000" and "JPEG/90000" RTP payload types are supported by this
server. 

The H.264 video format requires plenty of processing power; if the H.264 
decoding is too slow then packets will be missed and the packet stream
will have too many holes to decode anything. The processing load can 
be reduced by handling only RTP packets that match certain criteria by 
using the `--filter` option, or by using `--scale` and `--monochrome`
to reduce the image resolution after decoding.

Cameras that use RTP will normally be controlled by an RTSP client (such as
`vt-rtspclient`); the RTSP client asks the camera to start sending
its video stream to the address that the RTP server is listening on.

The camera responds to the RTSP request with a "fmtp" string, which describes
the video format for the H.264 video packets. If this information is not
available from within the RTP packet stream itself then it can be passed 
to the rtpserver using the `--format-file` option.

The RTCP protocol is not currently used so the rtpserver only opens one
UDP port for listening, not two. Also be aware that if two rtpservers
are listening on the same UDP port then they will steal packets from
one another and it is likely that both will stop working.

### Usage

	vt-rtpserver [<options>]

### Options

	--verbose              verbose logging
	--viewer               run a viewer
	--scale=<divisor>      reduce the image size (or 'auto')
	--monochrome           convert to monochrome
	--port=<port>          listening port
	--channel=<channel>    publish to the named channel
	--address=<ip>         listening address
	--multicast=<address>  multicast group address
	--format-file=<path>   file to read for the H264 format parameters
	--type=<type>          process packets of one rtp payload type (eg. 26 or 96)
	--jpeg-table=<ff>      tweak for jpeg quantisation tables (0,1 or 2)

Program vt-rtspclient
---------------------
Issues RTSP commands to control network cameras that send out their video 
over RTP with H.264 encoding (RTP/AVP).

The RTSP protocol acts as a "remote control" for video devices, so an RTSP
client is used to ask a camera to send its RTP video stream back to a 
particular pair of UDP ports where the RTP server is listening.

The RTSP client also reports on what video format the camera will send, 
encoded as a "fmtp" string. This information can help the RTP server 
decode the video stream. Use the `--format-file` option to make it
available to the RTP server.

The RTSP protocol is similar to HTTP, and it uses the same URL syntax, with
`rtsp://` instead of `http://` , eg. `rtsp://example.com/channel/1`. The 
format of right hand side of the URL, following the network address, will 
depend on the camera, so refer to its documentation.

The camera should send its video stream for as long as the `vt-rtspclient`
program keeps its session open; once it terminates then the video stream from
the camera will stop.

### Usage

	vt-rtspclient [<options>] <rtsp-url>

### Options

	--verbose                       verbose logging
	--port=<port-port>              listening port range eg. 8000-8001
	--multicast=<address>           multicast address eg. 224.1.1.1
	--format-file=<path>            a file to use for storing the video format string
	--connection-timeout=<seconds>  connection timeout
	--retry=<seconds>               retry failed connections or zero to exit (default 1s)
	--once                          exit when the first session closes

Program vt-socket
-----------------
Sends a command string to a local-domain or UDP socket. 

This can be used with the `vt-fileplayer` and `vt-recorder` programs when using 
their `--command-socket` option.

The socket is a local-domain socket by default, but a UDP network socket 
if the socket name looks like a transport address or if it starts 
with `udp://`.

The standard "netcat" utility (`nc`) can also be used to send messages to
UDP and local-domain sockets.

### Usage

	vt-socket <socket> <command-word> [<command-word> ...]

Program vt-viewer
-----------------
Reads images from a publication channel and displays them in a window. 

The viewer is also used when a parent process such as a the `vt-fileplayer`
needs a display window, and in that case the video is sent over a
private pipe rather than a publication channel and the two file descriptors
are passed on the command-line.

If there is no windowing system then the video images are displayed in the 
terminal using text characters for pixels. It looks better if you stand
back and squint.

Mouse events can be sent to a publication channel by using the `--channel` 
option. The `vt-maskeditor` and `vt-fileplayer` programs use this feature, and 
they have a `--viewer-channel` option to override the name of the publication 
channel used for these mouse events.

A pbm-format mask file can be used to dim fixed regions of the image.
The mask is stretched as necessary to fit the image.

### Usage

	vt-viewer [<options>] { <channel> | <shmem-fd> <pipe-fd> }

### Options

	--verbose            verbose logging
	--channel=<channel>  publish interaction events to the named channel
	--scale=<divisor>    reduce the image size
	--static             view only the first image
	--mask=<file>        use a mask file
	--title=<title>      sets the window title
	--quit               quit on q keypress

Program vt-watcher
------------------
Performs motion detection on a video stream received over a shared-memory 
publication channel.

If motion detection exceeds some threshold then an event is published on the 
`--event-channel` channel as a json formatted text message. These messages are 
normally read by a `vt-alarm` process that can run other external programs.

Motion detection events can also be used to control a `vt-recorder` process
by sending a `fast` command to the recorder's command socket.

The `--viewer` or `--image-channel` options can be used to visualise the 
motion detection. The viewer window shows in green the pixels that change in 
brightness by more than the "squelch" value, and it also shows in red the 
pixels that are ignored as the result of the mask.

The motion detection algorithm is extremely simple; one video frame is 
compared to the next, and pixels that change their brightness more that the 
squelch value (0..255) are counted up, and if the total count is more than 
the given threshold value then a motion detection event is emitted.

The `--scale` option also has an important effect because it increases the
size of the motion detection pixels (by decreasing the resolution of the 
image), and it affects the interpretation of the threshold value.

Histogram equalisation is enabled with the `--equalise` option and this can
be particularly useful for cameras that loose contrast in infra-red mode.

The command socket, enabled with the `--command-socket` option, accepts
`squelch`, `threshold` and `equalise` commands. Multiple commands can
be sent in one datagram by using semi-colon separators.

The squelch and threshold values have default values that will need
fine-tuning for your camera. This can be done interactively by temporarily
using the `--verbose`, `--viewer` and `--command-socket` options, and then 
sending `squelch`, `threshold` and `equalise` commands to the command socket
with the `vt-socket` tool (eg. `vt-watcher -C cmd --viewer -v <channel>`
and `vt-socket cmd squelch=20\;threshold=1\;equalise=off`).
Adjust the squelch value to eliminate the green highlights when there is 
no motion. Repeat in different lighting conditions.

### Usage

	vt-watcher [<options>] <input-channel>

### Options

	--log-threshold             threshold for logging motion detection events
	--verbose                   verbose logging
	--viewer                    run a viewer
	--viewer-title=<title>      viewer window title
	--mask=<file>               use a mask file
	--scale=<divisor>           reduce the image size
	--event-channel=<channel>   publish analysis events to the named channel
	--image-channel=<channel>   publish analysis images to the named channel
	--recorder=<path>           recorder socket path
	--squelch=<luma>            pixel value change threshold (0 to 255) (default 50)
	--threshold=<pixels>        pixel count threshold in changed pixels per image (default 100)
	--interval=<ms>             minimum time interval between comparisons (default 250)
	--once                      exit if the watched channel is unavailable
	--equalise                  histogram equalisation
	--command-socket=<path>     socket for update commands
	--plain                     do not show changed-pixel highlights in output images
	--repeat-timeout=<seconds>  send events repeatedly with the given period

Program vt-webcamserver
-----------------------
Serves up video images from a local webcam or video capture device over HTTP.

The HTTP client can either request individual images ("client-pull") or
receive a continuous JPEG video stream delivered as a 
`multipart/x-mixed-replace` content type ("server-push"). Refer to the
`vt-httpserver` program for more information.

This program is useful for running on satellite machines in the local network 
that have webcams attached; a central monitoring machine can run `vt-httpclient`
to pull the video stream across the network and onto a local publication 
channel. 

Use `--device=test` or `--device=demo` for testing without an attached 
camera, or on linux use the "vivi" kernel module (`sudo modprobe vivi`).
Device-specific configuration options can be added after the device
name, using a semi-colon separator.

The server will normally poll for the webcam device to come on-line, although
`--retry=0` can be used to disable this behaviour. When the device goes 
off-line again the program will go back to polling, or it will terminate if the
`--once` option is used.

The `--tz` option can be used with `--caption` to adjust the timezone for the
caption timestamp. It is typically a negative number for western longitudes 
in order to show local time.

### Usage

	vt-webcamserver [<options>]

### Options

	--verbose                 verbose logging
	--port=<port>             listening port
	--caption                 add a timestamp caption
	--caption-text=<text>     defines the caption text in ascii, with %t for a timestamp
	--timeout=<seconds>       network idle timeout (default 60s)
	--data-timeout=<seconds>  initial image timeout (default 3s)
	--repeat-timeout=<ms>     image repeat timeout (default 1000ms)
	--address=<ip>            listening address
	--tz=<hours>              timezone offset for the caption
	--device=<device>         webcam video device (eg. /dev/video0)
	--refresh=<seconds>       add a refresh header to http responses by default
	--retry=<timeout>         poll for the webcam device to appear (default 1s)
	--once                    exit if the webcam device disappears

Program vt-webcamplayer
-----------------------
Displays or publishes video from a local webcam or video capture device.

If no publication channel is specified then the video is just displayed in 
a window. The published video can be recorded by the `vt-recorder` program
and later played back by `vt-fileplayer`, and it can be analysed by a 
`vt-watcher`.

Use `--device=test` or `--device=demo` for testing without an attached 
camera, or on linux use the "vivi" kernel module (`sudo modprobe vivi`).
Device-specific configuration options can be added after the device
name, using a semi-colon separator.

The program will normally poll for the webcam device to come on-line, although
`--retry=0` can be used to disable this behaviour. When the device goes 
off-line again the program will go back to polling, or it will terminate if the
`--once` option is used.

The `--tz` option can be used with `--caption` to adjust the timezone for 
the caption timestamp. It is typically a negative number for western 
longitudes in order to show local time.

### Usage

	vt-webcamplayer [<options>]

### Options

	--verbose              verbose logging
	--device=<device>      video device (eg. /dev/video0)
	--caption              add a timestamp caption
	--caption-text=<text>  defines the caption text in ascii, with %t for a timestamp
	--viewer               run a viewer
	--channel=<channel>    publish to the named channel
	--scale=<divisor>      reduce the image size
	--tz=<hours>           timezone offset for the caption
	--monochrome           convert to monochrome
	--retry=<timeout>      poll for the device to appear
	--once                 exit if the webcam device disappears
