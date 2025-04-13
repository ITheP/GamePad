
ECHO OFF
ECHO rrefontgen [pbm file] [char w] [char h] ^<fontName^> ^<fontMode^> ^<overlap^> ^<optim^>
ECHO PBM file should be B^&W, 1-bit. The file should contain 16x6 or 32x3 characters
ECHO (96 characters mode), 16x8 (128 characters) or 1 line for digits and following characters.
ECHO Use irfanview to convert bitmaps in other formats
ECHO fileMode       = 1 (0-header,1-pbm,2-xml)
ECHO fontMode       = 0 (0-rect,1-vert,2-horiz)
ECHO overlap        = 0 (0-none,1-normal,2-less/optimal)
ECHO optim          = 0 (0-none,1-optimize wd/ht)
ECHO IMPORTANT
ECHO Photoshop doesnt create a valid .pbm file. Open and resave in Irfanview to fix

ECHO rrefontgen CustomIcons.16x16.pbm 16 16 CustomIcons16 0 2 0 > CustomIcons.16x16.h
rrefontgen CustomIcons.16x16.pbm 16 16 CustomIcons16 0 2 0 > CustomIcons.16x16.h