# --------------------------------------------------------------------------
#
#      4r  use 80486 timings and register argument passing
#       c  compile only
#      d1  include line number debugging information
#      d2  include full sybolic debugging information
#      ei  force enums to be of type int
#       j  change char default from unsigned to signed
#      oa  relax aliasing checking
#      od  do not optimize
#  oe[=#]  expand functions inline, # = quads (default 20)
#      oi  use the inline library functions
#      om  generate inline 80x87 code for math functions
#      ot  optimize for time
#      ox  maximum optimization
#       s  remove stack overflow checks
#     zp1  align structures on bytes
#      zq  use quiet mode
#  /i=dir  add include directories
#
# --------------------------------------------------------------------------


# Build options for 386DX/SX
#CCOPTS = $(EXTERNOPT) -2 -fpi87 -ml -s -omaxet -i=include -i=.\..\..\include -za99 -dHAVE_CONFIG_H -dFLAC__NO_ASM

# 80287 floating point
#CCOPTS = $(EXTERNOPT) -onatx -oh -oi -ei -zp8 -2 -fpi87 -fp2 -ml -s -omaxet -i=include -i=.\..\..\include -za99 -dHAVE_CONFIG_H -dFLAC__NO_ASM

# no fpu build
CCOPTS = $(EXTERNOPT) -ml -oneatx -oh -oi -ei -zp4 -0 -fpi -i=.\..\pub -dPUREC

#//-omaxet
#wcc fixed.obj -i=include -i=.\..\..\include -za99

PRG=mp3.exe

all:    $(PRG)


helix_sources = &
mp3dec.obj &
mp3tabs.obj &
bitstream.obj &
buffers.obj &
dct32.obj &
dequant.obj &
dqchan.obj &
huffman.obj &
hufftabs.obj &
imdct.obj &
polyphase.obj &
scalfact.obj &
stproc.obj &
subband.obj &
trigtabs.obj

HELIX_OBJS = $(helix_sources)

mp3_sources = &
main.obj

MP3_OBJS = $(mp3_sources)

.c.obj :
 wcc $(CCOPTS) -fo=$[*.obj $[*.c

#$(PRG) : $(OBJS)
#	$(CC) $(LDFLAGS) $(OBJS) -o $(PRG) $(LIBS)

$(PRG): $(HELIX_OBJS) $(MP3_OBJS)
  	%write mp3.lnk NAME   $@
  	%write mp3.lnk SYSTEM DOS
  	%write mp3.lnk OPTION MAP
  	%write mp3.lnk OPTION STACK=30k
  	%write mp3.lnk FILE   {$(HELIX_OBJS) $(MP3_OBJS)}
	wlink @mp3.lnk

!ifndef __LINUX__
DELCMD = del
!else
DELCMD = rm -f
!endif

clean : .SYMBOLIC
 $(DELCMD) *.lib
 $(DELCMD) *.err
 $(DELCMD) *.obj
 $(DELCMD) *.lst

